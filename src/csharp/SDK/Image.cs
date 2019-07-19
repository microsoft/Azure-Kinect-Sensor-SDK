// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Buffers;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    public class Image : IMemoryOwner<byte>, IDisposable
    {

        Allocator Allocator = Allocator.Singleton;

        /// <summary>
        /// Gets a Span of all the pixels in the image.
        /// </summary>
        /// <typeparam name="PixelT">The type of each pixel.</typeparam>
        /// <remarks>If the image stride does not evenly align with the number of pixels in each row, this function will throw an exception.</remarks>
        /// <returns>Span representing the set of pixels in the image.</returns>
        public Span<PixelT> GetPixels<PixelT>()
            where PixelT : unmanaged
        {
            if (this.StrideBytes != Marshal.SizeOf(typeof(PixelT)) * this.WidthPixels)
            {
                throw new AzureKinectException("Pixels not aligned to stride of each line");
            }

            return MemoryMarshal.Cast<byte, PixelT>(this.Memory.Span.Slice(0, this.WidthPixels * this.HeightPixels * Marshal.SizeOf(typeof(PixelT))));
        }

        /// <summary>
        /// Gets a Span of all the pixels on a row of an image.
        /// </summary>
        /// <typeparam name="PixelT">The type of each pixel.</typeparam>
        /// <param name="row">Row of the image to get the pixels for</param>
        /// <returns>Span representing the set of pixels in the image.</returns>
        public Span<PixelT> GetPixels<PixelT>(int row)
            where PixelT : unmanaged
        {
            if (row > this.HeightPixels || row < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(row));
            }

            if (this.StrideBytes < Marshal.SizeOf(typeof(PixelT)) * this.WidthPixels)
            {
                throw new AzureKinectException("The image stride is not large enough for pixels of this size");
            }

            return MemoryMarshal.Cast<byte, PixelT>(this.Memory.Span.Slice(row * this.StrideBytes, Marshal.SizeOf(typeof(PixelT)) * this.WidthPixels));
        }

        /// <summary>
        /// Gets a pixel value in the image.
        /// </summary>
        /// <typeparam name="PixelT">The type of the pixel.</typeparam>
        /// <param name="row">The image row.</param>
        /// <param name="col">The image column.</param>
        /// <returns>A reference to the pixel at the row and column.</returns>
        public ref PixelT GetPixel<PixelT>(int row, int col)
            where PixelT : unmanaged
        {
            if (col < 0 || col > this.WidthPixels)
            {
                throw new ArgumentOutOfRangeException(nameof(col));
            }

            Span<PixelT> rowPixels = GetPixels<PixelT>(row);

            return ref rowPixels[col];
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

        internal Image(NativeMethods.k4a_image_t handle, Image clone = null)
        {
            Allocator.Singleton.Hook(this);

            this.handle = handle;
            if (clone != null)
            {
                this.bufferCopy = clone.bufferCopy;
            }
        }

        public unsafe byte[] GetBufferCopy()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                return this.Memory.ToArray();
            }
        }


        private byte[] bufferCopy = null;
        private AzureKinectMemoryManager memoryManager = null;

        private IntPtr _Buffer = IntPtr.Zero;

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
        

        /// <summary>
        /// Returns a reference to the the underlying memory.
        /// </summary>
        /// <remarks>
        /// This returns an accessor to the Image's memory without making
        /// a copy.
        /// 
        /// Once managed code has accessed the image's memory, the memory
        /// will be retained until it is garbage collected.
        /// </remarks>
        public unsafe Memory<byte> Memory
        {
            get
            {
                lock (this)
                {
                
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    if (bufferCopy != null)
                    {
                        return new Memory<byte>(bufferCopy);
                    }
                    if (memoryManager != null)
                    {
                        return memoryManager.Memory;
                    }

                    IntPtr bufferAddress = (IntPtr)this.Buffer;

                    Memory<byte> memory = this.Allocator.GetMemory(bufferAddress, this.Size);
                    if (!memory.IsEmpty)
                    {
                        return memory;
                    }

                    // The underlying buffer is not in managed memory

                    if (Allocator.CopyNativeBuffers)
                    {
                        // Create a copy
                        this.bufferCopy = new byte[this.Size];

                        System.Runtime.InteropServices.Marshal.Copy((IntPtr)this.Buffer, this.bufferCopy, 0, checked((int)this.Size));

                        return new Memory<byte>(bufferCopy);
                    }
                    else
                    {
                        memoryManager = new AzureKinectMemoryManager(this);

                        Allocator.Singleton.Hook(memoryManager);

                        return memoryManager.Memory;
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

                if (this.bufferCopy != null)
                {
                    unsafe
                    {
                        System.Runtime.InteropServices.Marshal.Copy(this.bufferCopy, 0, (IntPtr)this.Buffer, (int)this.Size);
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

                if (this.bufferCopy != null)
                {
                    unsafe
                    {
                        System.Runtime.InteropServices.Marshal.Copy((IntPtr)this.Buffer, this.bufferCopy, 0, checked((int)this.Size));
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

                return new Image(handle.DuplicateReference(), this);
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
                        this.FlushMemory();

                        //((IDisposable)this.memoryManager)?.Dispose();

                        // TODO: dispose managed state (managed objects).
                        Allocator.Singleton.Unhook(this);

                        handle.Close();
                        handle = null;
                    }

                    disposedValue = true;
                }
            }
        }


        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            // GC.SuppressFinalize(this);
        }
        #endregion
    }
}
