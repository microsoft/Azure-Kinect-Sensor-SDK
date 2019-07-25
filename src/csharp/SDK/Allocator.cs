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

        public void Hook(IDisposable disposable)
        {
            lock (this)
            {
                _ = this.disposables.Add(new WeakReference<IDisposable>(disposable));

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

                AppDomain.CurrentDomain.DomainUnload += this.CurrentDomain_DomainUnload;
                AppDomain.CurrentDomain.ProcessExit += this.CurrentDomain_DomainUnload;

                this.hooked = true;
            }
        }

        public void Unhook(IDisposable disposable)
        {
            lock (this)
            {
                _ = this.disposables.RemoveWhere((r) =>
                      {
                          bool alive = r.TryGetTarget(out IDisposable target);

                          return !alive || target == disposable;
                      });
            }
        }

        public Memory<byte> GetMemory(IntPtr address, long size)
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

        private void FreeFunction(IntPtr buffer, IntPtr context)
        {
            GCHandle contextPin = (GCHandle)context;
            AllocationContext allocationContext = (AllocationContext)contextPin.Target;

            lock (this)
            {
                System.Diagnostics.Debug.Assert(Object.ReferenceEquals(this.allocations[(long)buffer], allocationContext), "Allocation context does not match expected value");
                _ = this.allocations.Remove((long)buffer);
            }

            allocationContext.BufferPin.Free();
            this.pool.Return(allocationContext.Buffer);
            contextPin.Free();
        }

        private void CurrentDomain_DomainUnload(object sender, EventArgs e)
        {
            lock (this)
            {
                System.Diagnostics.Debug.WriteLine($"Disposable count {this.disposables.Count} (Allocation Count {this.allocations.Count})");

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

                if (this.allocations.Count > 0)
                {
                    throw new Exception("Not all native allocations have been freed before managed shutdown");
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
                        Initialized = false
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

        public void ReturnBufferCache(IntPtr nativeAddress)
        {
            lock (this)
            {
                BufferCacheEntry entry = this.bufferCache[nativeAddress];
                entry.ReferenceCount--;
                if (entry.ReferenceCount == 0)
                {
                    this.pool.Return(entry.ManagedBufferCache);
                    this.bufferCache.Remove(nativeAddress);
                }
            }
        }


    }


}
