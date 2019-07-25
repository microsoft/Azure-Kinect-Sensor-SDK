using System;
using System.Buffers;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Manages buffer allocation.
    /// </summary>
    internal class Allocator
    {
        // Objects we are tracking for disposal prior to CLR shutdown
        private readonly HashSet<WeakReference<IDisposable>> disposables = new HashSet<WeakReference<IDisposable>>();

        // Allocations made by the managed code for the native library
        private readonly SortedDictionary<long, AllocationContext> allocations = new SortedDictionary<long, AllocationContext>();

        // A recycleable large array pool to prevent unnecissary managed allocationsns and clearing of memory
        private readonly ArrayPool<byte> pool = new LargeArrayPool();

        // Managed buffers used as caches for the native memory.
        private readonly Dictionary<IntPtr, BufferCacheEntry> bufferCache = new Dictionary<IntPtr, BufferCacheEntry>();

        // Native delegates
        private NativeMethods.k4a_memory_allocate_cb_t allocateDelegate;
        private NativeMethods.k4a_memory_destroy_cb_t freeDelegate;

        // Native allocator hook state
        private bool hooked;

        /// <summary>
        /// Gets the Allocator.
        /// </summary>
        public static Allocator Singleton { get; } = new Allocator();

        /// <summary>
        /// Gets or sets a value indicating whether to have the native library use the managed allocator.
        /// </summary>
        public bool UseManagedAllocator { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether to make a safe copy of native buffers.
        /// </summary>
        public bool SafeCopyNativeBuffers { get; set; } = true;


        /// <summary>
        /// Hook the native allocator and register an object for dispose during shutdown.
        /// </summary>
        /// <param name="disposable">Object to dispose before native hooks are disconnected.</param>
        /// <remarks>
        /// When the CLR shuts down, native callbacks in to the CLR result in an application crash. The allocator free method
        /// is a native callback to the managed layer that is called whenever the hooked native API needs to free memory.
        ///
        /// To avoid this callback after the CLR shuts down, the native library must be completly cleaned up prior CLR shutdown.
        ///
        /// Any object that may hold references to the native library (and will therefore generate native to manged callbacks when it
        /// gets cleaned up) should register with the Allocator.Hook method to ensure it is cleaned up in the correct order.
        /// during shutdown.
        /// </remarks>
        public void Hook(IDisposable disposable)
        {
            lock (this)
            {
                // Track the object as one we may need to dispose during shutdown.
                // Use a weak reference to allow the object to be garbage collected earliler if possible.
                _ = this.disposables.Add(new WeakReference<IDisposable>(disposable));

                // Only hook the native layer once
                if (this.hooked)
                {
                    return;
                }

                this.allocateDelegate = new NativeMethods.k4a_memory_allocate_cb_t(this.AllocateFunction);
                this.freeDelegate = new NativeMethods.k4a_memory_destroy_cb_t(this.FreeFunction);

                if (this.UseManagedAllocator)
                {
                    AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_set_allocator(this.allocateDelegate, this.freeDelegate));
                }

                // Register for ProcessExit and DomainUnload to allow us to unhook the native layer before
                // native to managed callbacks are no longer allowed.
                AppDomain.CurrentDomain.DomainUnload += this.CurrentDomain_DomainUnload;
                AppDomain.CurrentDomain.ProcessExit += this.CurrentDomain_DomainUnload;

                this.hooked = true;
            }
        }

        /// <summary>
        /// Unregister the object for disposal.
        /// </summary>
        /// <param name="disposable">Object to unregister.</param>
        /// <remarks>
        /// This does not unhook the native allocator, but only unregisters the object for
        /// disposal.
        /// </remarks>
        public void Unhook(IDisposable disposable)
        {
            lock (this)
            {
                // Remove the object and clean up any dead weak references.
                _ = this.disposables.RemoveWhere((r) =>
                      {
                          bool alive = r.TryGetTarget(out IDisposable target);

                          return !alive || target == disposable;
                      });
            }
        }

        /// <summary>
        /// Get a Memory reference to the managed memory that was used by the hooked native
        /// allocator.
        /// </summary>
        /// <param name="address">Native address of the memory.</param>
        /// <param name="size">Size of the memory region.</param>
        /// <returns>Reference to the memory, or an empty memory reference.</returns>
        /// <remarks>
        /// If the address originally came from a managed array that was provided to the native
        /// API through the allocator hook, this function will return a Memory reference to the managed
        /// memory. Since this is a reference to the managed memory and not the native pointer, it
        /// is safe and not subject to use after free bugs.
        ///
        /// The address and size do not need to reference the exact pointer provided to the native layer
        /// by the allocator, but can refer to any region in the allocated memory.
        /// </remarks>
        public Memory<byte> GetManagedAllocatedMemory(IntPtr address, long size)
        {
            lock (this)
            {
                AllocationContext allocation = this.FindNearestContext(address);
                if (allocation == null)
                {
                    return null;
                }

                long offset = (long)address - (long)allocation.BufferAddress;

                // Check that the beginning of the memory is in this allocation
                if (offset > allocation.Buffer.LongLength)
                {
                    return null;
                }

                // Check that the end of the memory is in this allocation
                if (checked(offset + size) > allocation.Buffer.LongLength)
                {
                    return null;
                }

                // Return a reference to this memory
                return new Memory<byte>(allocation.Buffer, checked((int)offset), checked((int)size));
            }
        }

        // Find the allocation context who's address is closest but not
        // greater than the search address
        private AllocationContext FindNearestContext(IntPtr address)
        {
            lock (this)
            {
                long[] keys = new long[this.allocations.Count];
                this.allocations.Keys.CopyTo(keys, 0);

                int searchIndex = Array.BinarySearch(keys, (long)address);
                if (searchIndex >= 0)
                {
                    return this.allocations[keys[searchIndex]];
                }
                else
                {
                    int nextLowestIndex = ~searchIndex - 1;
                    if (nextLowestIndex < 0)
                    {
                        return null;
                    }

                    AllocationContext allocation = this.allocations[keys[nextLowestIndex]];

                    return allocation;
                }
            }
        }

        // This function is called by the native layer to allocate memory
        private IntPtr AllocateFunction(int size, out IntPtr context)
        {
            byte[] buffer = this.pool.Rent(size);
            AllocationContext allocationContext = new AllocationContext()
            {
                Buffer = buffer,
                BufferPin = GCHandle.Alloc(buffer, GCHandleType.Pinned),
                CallbackDelegate = this.freeDelegate,
            };

            allocationContext.BufferAddress = allocationContext.BufferPin.AddrOfPinnedObject();

            context = (IntPtr)GCHandle.Alloc(allocationContext);

            lock (this)
            {
                this.allocations.Add((long)allocationContext.BufferAddress, allocationContext);
            }

            return allocationContext.BufferAddress;
        }

        // This function is called by the native layer to free memory
        private void FreeFunction(IntPtr buffer, IntPtr context)
        {
            GCHandle contextPin = (GCHandle)context;
            AllocationContext allocationContext = (AllocationContext)contextPin.Target;

            lock (this)
            {
                System.Diagnostics.Debug.Assert(object.ReferenceEquals(this.allocations[(long)buffer], allocationContext), "Allocation context does not match expected value");
                _ = this.allocations.Remove((long)buffer);
            }

            allocationContext.BufferPin.Free();
            this.pool.Return(allocationContext.Buffer);
            contextPin.Free();
        }

        // Called when the AppDomain is unloaded or the application exits
        private void CurrentDomain_DomainUnload(object sender, EventArgs e)
        {
            lock (this)
            {
                System.Diagnostics.Debug.WriteLine($"Disposable count {this.disposables.Count} (Allocation Count {this.allocations.Count})");

                // First dispose of all the registered objects
                List<IDisposable> disposeList = new List<IDisposable>();
                foreach (WeakReference<IDisposable> r in this.disposables)
                {
                    if (r.TryGetTarget(out IDisposable disposable))
                    {
                        disposeList.Add(disposable);
                    }
                }

                this.disposables.Clear();

                foreach (IDisposable disposable in disposeList)
                {
                    System.Diagnostics.Debug.WriteLine($"Disposed {disposable} ({disposable.GetType().FullName}) (Allocation Count {this.allocations.Count})");
                    disposable.Dispose();
                }

                // If the allocation count is not zero, we will be called again with a free function
                // if this happens after the CLR has entered shutdown, the CLR will generate an exception.
                if (this.allocations.Count > 0)
                {
                    throw new Exception("Not all native allocations have been freed before managed shutdown");
                }

                if (this.UseManagedAllocator)
                {
                    AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_set_allocator(null, null));
                }
            }
        }

        private class AllocationContext
        {
            public byte[] Buffer { get; set; }

            public IntPtr BufferAddress { get; set; }

            public GCHandle BufferPin { get; set; }

            public NativeMethods.k4a_memory_destroy_cb_t CallbackDelegate { get; set; }
        }

        private class BufferCacheEntry
        {
            public byte[] ManagedBufferCache { get; set; }

            public int UsedSize { get; set; }

            public int ReferenceCount { get; set; }

            public bool Initialized { get; set; }
        }

        private void PopulateEntry(IntPtr nativeAddress, BufferCacheEntry entry)
        {
            Marshal.Copy(nativeAddress, entry.ManagedBufferCache, 0, entry.UsedSize);
        }

        /// <summary>
        /// Get a managed array to cache the contents of a native buffer.
        /// </summary>
        /// <param name="nativeAddress">Native buffer to mirror.</param>
        /// <param name="size">Size of the native memory.</param>
        /// <returns>A managed array populated with the content of the native buffer.</returns>
        /// <remarks>Multiple callers asking for the same address will get the same buffer.
        /// When done with the buffer the caller must call <seealso cref="ReturnBufferCache(IntPtr)"/>.
        /// </remarks>
        public byte[] GetBufferCache(IntPtr nativeAddress, int size)
        {
            BufferCacheEntry entry;

            lock (this)
            {
                if (this.bufferCache.ContainsKey(nativeAddress))
                {
                    entry = this.bufferCache[nativeAddress];
                    entry.ReferenceCount++;

                    if (entry.UsedSize != size)
                    {
                        throw new Exception("Multiple image buffers sharing the same address cannot have the same size");
                    }

                }
                else
                {
                    entry = new BufferCacheEntry
                    {
                        ManagedBufferCache = this.pool.Rent(size),
                        UsedSize = size,
                        ReferenceCount = 1,
                        Initialized = false,
                    };

                    this.bufferCache.Add(nativeAddress, entry);
                }
            }

            lock (entry)
            {
                if (!entry.Initialized)
                {
                    this.PopulateEntry(nativeAddress, entry);
                    entry.Initialized = true;
                }
            }

            return entry.ManagedBufferCache;
        }

        /// <summary>
        /// Return the buffer cache.
        /// </summary>
        /// <param name="nativeAddress">Address of the native buffer.</param>
        /// <remarks>Must be called exactly once for each buffer provided by <see cref="GetBufferCache(IntPtr, int)"/>.</remarks>
        public void ReturnBufferCache(IntPtr nativeAddress)
        {
            lock (this)
            {
                BufferCacheEntry entry = this.bufferCache[nativeAddress];
                entry.ReferenceCount--;
                if (entry.ReferenceCount == 0)
                {
                    this.pool.Return(entry.ManagedBufferCache);
                    _ = this.bufferCache.Remove(nativeAddress);
                }
            }
        }


    }


}
