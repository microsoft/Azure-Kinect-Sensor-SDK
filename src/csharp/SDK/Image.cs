// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Buffers;
using System.ComponentModel;
using System.Diagnostics.Tracing;
using System.Dynamic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace Microsoft.Azure.Kinect.Sensor
{
    public class Image : IMemoryOwner<byte>, IDisposable
    {

        Allocator Allocator = Allocator.Singleton;


        public Memory<PixelT> GetPixels<PixelT>()
            where PixelT : unmanaged
        {
            if (this.StrideBytes != Marshal.SizeOf(typeof(PixelT)) * this.WidthPixels)
            {
                throw new AzureKinectException("Pixels not aligned to stride of each line");
            }

            return new AzureKinectMemoryCast<byte, PixelT>(this.Memory).Memory;
        }

        public Memory<PixelT> GetPixels<PixelT>(int row)
            where PixelT : unmanaged
        {
            if (row > this.HeightPixels || row < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(row));
            }

            int activeLineBytes = Marshal.SizeOf(typeof(PixelT)) * this.WidthPixels;
            if (this.StrideBytes < activeLineBytes)
            {
                throw new AzureKinectException("The image stride is not large enough for pixels of this size");
            }

            return new AzureKinectMemoryCast<byte, PixelT>(this.Memory.Slice(row * this.StrideBytes, activeLineBytes)).Memory;
        }

        /// <summary>
        /// Gets a pixel value in the image.
        /// </summary>
        /// <typeparam name="PixelT">The type of the pixel.</typeparam>
        /// <param name="row">The image row.</param>
        /// <param name="col">The image column.</param>
        /// <returns>The pixel value at the row and column.</returns>
        public unsafe PixelT GetPixel<PixelT>(int row, int col)
            where PixelT : unmanaged
        {
            if (row < 0 || row > this.HeightPixels)
            {
                throw new ArgumentOutOfRangeException(nameof(row));
            }

            if (col < 0 || col > this.WidthPixels)
            {
                throw new ArgumentOutOfRangeException(nameof(col));
            }

            return *(PixelT*)((byte*)this.Buffer + (this.StrideBytes * row) + (col * sizeof(PixelT)));
        }

        public unsafe void SetPixel<PixelT>(int row, int col, PixelT pixel)
            where PixelT : unmanaged
        {
            if (row < 0 || row > this.HeightPixels)
            {
                throw new ArgumentOutOfRangeException(nameof(row));
            }

            if (col < 0 || col > this.WidthPixels)
            {
                throw new ArgumentOutOfRangeException(nameof(col));
            }

            *(PixelT*)((byte*)this.Buffer + (this.StrideBytes * row) + (col * sizeof(PixelT))) = pixel;
        }

        public Image(ImageFormat format, int width_pixels, int height_pixels, int stride_bytes)
        {
            Allocator.Singleton.Hook(this);

            AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_image_create(format,
                width_pixels,
                height_pixels,
                stride_bytes,
                out this.handle));
        }

        public Image(ImageFormat format, int width_pixels, int height_pixels)
        {
            int pixelSize;
            switch (format)
            {
                case ImageFormat.ColorBGRA32:
                    pixelSize = 4;
                    break;
                case ImageFormat.Depth16:
                case ImageFormat.IR16:
                    pixelSize = 2;
                    break;
                default:
                    throw new AzureKinectException($"Unable to allocate array for format {format}");
            }

            int stride_bytes = width_pixels * pixelSize;

            Allocator.Singleton.Hook(this);

            AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_image_create(format,
                width_pixels,
                height_pixels,
                stride_bytes,
                out this.handle));
        }

        internal Image(NativeMethods.k4a_image_t handle)
        {
            Allocator.Singleton.Hook(this);

            this.handle = handle;
        }

        public unsafe byte[] GetBufferCopy()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                int bufferSize = checked((int)this.Size);
                byte[] copy = new byte[bufferSize];

                // If we are using a managed buffer copy, ensure the native memory is up to date
                this.FlushMemory();

                System.Runtime.InteropServices.Marshal.Copy((IntPtr)this.Buffer, copy, 0, bufferSize);

                return copy;
            }
        }

        
        
        // The pointer to the underlying native memory
        private IntPtr _Buffer = IntPtr.Zero;

        // If we have made a managed copy of the native memory, this is the managed array
        private byte[] managedBufferCache = null;

        // If we have wrapped the native memory in a IMemoryOwner<T>, this is the memory owner\
        private IMemoryOwner<byte> nativeBufferWrapper = null;


        /// <summary>
        /// Returns a native pointer to the underlying memory.
        /// </summary>
        /// <remarks>
        /// This property may only be accessed by unsafe code.
        ///
        /// This returns an unsafe pointer to the image's memory. It is important that the
        /// caller ensures the Image is not Disposed or garbage collected while this pointer is
        /// in use, since it may become invalid when the Image is disposed or finalized.
        /// 
        /// If this method needs to be used in a context where the caller cannot garantee that the
        /// Image will be disposed by another thread, the caller can call <see cref="Reference"/>
        /// to create a duplicate reference to the Image which can be disposed separately.
        /// 
        /// For safe buffer access <see cref="Memory"/>. 
        /// </remarks>
        public unsafe void* Buffer
        {
            get
            {
                if (_Buffer != IntPtr.Zero)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    return (void*)_Buffer;
                }

                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    _Buffer = NativeMethods.k4a_image_get_buffer(handle);
                    if (_Buffer == IntPtr.Zero)
                    {
                        throw new AzureKinectException("Image has NULL buffer");
                    }

                    return (void*)_Buffer;
                }
            }
        }
        

        public unsafe Memory<byte> Memory
        {
            get
            {
                lock (this)
                {

                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    // If we previously copied the native memory to a managed array, return that array's memory
                    if (managedBufferCache != null)
                    {
                        return new Memory<byte>(managedBufferCache, 0, (int)this.Size);
                    }

                    // If we previously wrapped the native memory in a IMemoryOwner<T>, return that memory object
                    if (nativeBufferWrapper != null)
                    {
                        return nativeBufferWrapper.Memory;
                    }

                    IntPtr bufferAddress = (IntPtr)this.Buffer;

                    // If the native buffer is within a memory block that the managed allocator provided,
                    // return that memory.
                    Memory<byte> memory = this.Allocator.GetMemory(bufferAddress, this.Size);
                    if (!memory.IsEmpty)
                    {
                        return memory;
                    }

                    // The underlying buffer is not in managed memory.
                    // We can use one of two strategies to return a Memory<T>
                    
                    // If we use the CopyNativeBuffers method, we will allocate and then copy the native memory
                    // to a managed array, ensuring memory-safe access to the contents of memory, at the expense of
                    // a memcpy each time we transition the buffer from native to managed, or from managed to native.

                    // If we don't copy the native buffers, we can construct a MemoryManager<T> that wraps that native
                    // buffer. This has no memcpy cost, but exposes the possibilty of use after free bugs to consumers
                    // of the library. This is therefore not enabled by default.
                    if (this.Allocator.SafeCopyNativeBuffers)
                    {
                        // Create a copy
                        int bufferSize = checked((int)this.Size);
                        this.managedBufferCache = Allocator.GetBufferCache((IntPtr)this.Buffer, bufferSize);


                        return new Memory<byte>(managedBufferCache, 0, (int)this.Size);
                    }
                    else
                    {
                        // Provide a Memory<T> object that wraps a native pointer
                        // This is UNSAFE. Callers in safe code may access unowned native
                        // memory if they use the Memory<T> or Span<T> after the Image has
                        // been disposed.

                        nativeBufferWrapper = new AzureKinectMemoryManager(this);

                        return nativeBufferWrapper.Memory;
                    }
                }
            }
        }

        internal void FlushMemory()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                if (this.managedBufferCache != null)
                {
                    unsafe
                    {
                        System.Runtime.InteropServices.Marshal.Copy(this.managedBufferCache, 0, (IntPtr)this.Buffer, (int)this.Size);
                    }
                }
            }
        }

        internal void InvalidateMemory()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                if (this.managedBufferCache != null)
                {
                    unsafe
                    {
                        System.Runtime.InteropServices.Marshal.Copy((IntPtr)this.Buffer, this.managedBufferCache, 0, checked((int)this.Size));
                    }
                }
            }
        }

        public TimeSpan Exposure
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    ulong exposure = NativeMethods.k4a_image_get_exposure_usec(handle);
                    return TimeSpan.FromTicks(checked((long)exposure) * 10);
                }
            }
            set
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    NativeMethods.k4a_image_set_exposure_time_usec(handle, checked((ulong)value.Ticks / 10));
                }
            }
        }

        private ImageFormat? _Format = null;

        public ImageFormat Format
        {
            get
            {
                if (_Format.HasValue) return _Format.Value;

                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    _Format = NativeMethods.k4a_image_get_format(handle);
                    return _Format.Value;
                }
            }
        }


        private int _HeightPixels = -1;

        public int HeightPixels
        {
            get
            {
                if (_HeightPixels >= 0) return _HeightPixels;

                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    _HeightPixels = NativeMethods.k4a_image_get_height_pixels(handle);
                    return _HeightPixels;
                }
            }
        }

        private int _WidthPixels = -1;
        public int WidthPixels
        {
            get
            {
                if (_WidthPixels >= 0) return _WidthPixels;

                lock (this)
                {
                    

                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    _WidthPixels = NativeMethods.k4a_image_get_width_pixels(handle);
                    return _WidthPixels;
                }
            }
        }

        private int _StrideBytes = -1;

        public int StrideBytes
        {
            get
            {

                if (_StrideBytes >= 0) return _StrideBytes;

                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    _StrideBytes = NativeMethods.k4a_image_get_stride_bytes(handle);

                    return _StrideBytes;
                }
            }
        }

        private long _Size = -1;

        public long Size
        {
            get
            {

                if (_Size >= 0) return _Size;

                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    _Size = checked((long)NativeMethods.k4a_image_get_size(handle).ToUInt64());

                    return _Size;
                }
            }
        }

        public TimeSpan Timestamp
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    ulong timestamp = NativeMethods.k4a_image_get_timestamp_usec(handle);
                    return TimeSpan.FromTicks(checked((long)timestamp) * 10);
                }
            }
            set
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    NativeMethods.k4a_image_set_timestamp_usec(handle, checked((ulong)value.Ticks / 10));
                }
            }
        }

        public int ISOSpeed
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    return checked((int)NativeMethods.k4a_image_get_iso_speed(handle));
                }
            }
            set
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    NativeMethods.k4a_image_set_iso_speed(handle, checked((uint)value));
                }
            }
        }


        public int WhiteBalance
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    return checked((int)NativeMethods.k4a_image_get_white_balance(handle));
                }
            }
            set
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    NativeMethods.k4a_image_set_white_balance(handle, checked((uint)value));
                }
            }
        }

        private NativeMethods.k4a_image_t handle;

        internal NativeMethods.k4a_image_t DangerousGetHandle()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                return handle;
            }
        }

        public Image Reference()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                return new Image(handle.DuplicateReference());
            }
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                lock (this)
                {
                    if (disposing)
                    {
                        //this.FlushMemory();

                        // If we have a native buffer wrapper, dispose it since we
                        // are a wrapper for it's IMemoryOwner<T> interface
                        this.nativeBufferWrapper?.Dispose();

                        // TODO: dispose managed state (managed objects).
                        Allocator.Singleton.Unhook(this);

                        handle.Close();
                        handle = null;
                    }

                    if (this.managedBufferCache != null)
                    {
                        unsafe
                        {
                            Allocator.ReturnBufferCache((IntPtr)this.Buffer);
                        }
                        this.managedBufferCache = null;
                    }

                    disposedValue = true;
                }
            }
        }

        ~Image()
        {
            Dispose(false);
        }


        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            GC.SuppressFinalize(this);
        }
        #endregion
    }
}
