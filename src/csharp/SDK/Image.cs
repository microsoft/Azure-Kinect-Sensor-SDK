// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Buffers;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    public class Image : IDisposable
    {
        Allocator Allocator = Allocator.Singleton;

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

                byte[] copy = new byte[this.Size];
                using (MemoryHandle pin = this.Pin())
                {
                    System.Runtime.InteropServices.Marshal.Copy((IntPtr)pin.Pointer, copy, 0, checked((int)this.Size));
                }
                return copy;
            }
        }


        private byte[] bufferCopy = null;
        private IntPtr _Buffer = IntPtr.Zero;

        private unsafe void* NativeBuffer
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

        public MemoryHandle Pin()
        {
            return this.Memory.Pin();
        }

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

                    IntPtr bufferAddress = (IntPtr)this.NativeBuffer;

                    Memory<byte> memory = this.Allocator.GetMemory(bufferAddress, this.Size);
                    if (!memory.IsEmpty)
                    {
                        return memory;
                    }

                    // The underlying buffer is not in managed memory
                    // Create a copy
                    this.bufferCopy = new byte[this.Size];
                    
                    System.Runtime.InteropServices.Marshal.Copy((IntPtr)this.NativeBuffer, this.bufferCopy, 0, checked((int)this.Size));

                    return new Memory<byte>(bufferCopy);
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
                        System.Runtime.InteropServices.Marshal.Copy(this.bufferCopy, 0, (IntPtr)this.NativeBuffer, (int)this.Size);
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
                        System.Runtime.InteropServices.Marshal.Copy((IntPtr)this.NativeBuffer, this.bufferCopy, 0, checked((int)this.Size));
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
                        // TODO: dispose managed state (managed objects).
                        Allocator.Singleton.Unhook(this);
                    }

                    handle.Close();
                    handle = null;

                    disposedValue = true;
                }
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        // ~Image()
        // {
        //   // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
        //   Dispose(false);
        // }

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
