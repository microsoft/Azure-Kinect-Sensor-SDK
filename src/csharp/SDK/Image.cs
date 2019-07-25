// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{

    public class ArrayImage<T> : Image where T : unmanaged
    {

        private class CallbackContext
        {
            public GCHandle BufferPin { get; set; }
            public NativeMethods.k4a_memory_destroy_cb_t CallbackDelegate { get; set; }
        }

        private static NativeMethods.k4a_image_t CreateHandle(ImageFormat format, int width_pixels, int height_pixels, out T[] data)
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
                    throw new AzureKinectException($"Unable to allocate {typeof(T).Name} array for format {format}");
            }

            int stride_bytes = pixelSize * width_pixels;

            if (stride_bytes % Marshal.SizeOf(typeof(T)) != 0)
            {
                throw new AzureKinectException($"{typeof(T).Name} does not fit evenly on a line of {width_pixels} pixels of type {format}");
            }

            // Allocate the buffer
            data = new T[height_pixels * stride_bytes / Marshal.SizeOf(typeof(T))];

            CallbackContext context = new CallbackContext()
            {
                BufferPin = GCHandle.Alloc(data, GCHandleType.Pinned),
                CallbackDelegate = new NativeMethods.k4a_memory_destroy_cb_t(MemoryDestroyCallback)
            };

            GCHandle ContextPin = GCHandle.Alloc(context);
            try
            {
                int size = height_pixels * stride_bytes;
                AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_image_create_from_buffer(format,
                    width_pixels,
                    height_pixels,
                    stride_bytes,
                    context.BufferPin.AddrOfPinnedObject(),
                    (UIntPtr)size,
                    context.CallbackDelegate,
                    (IntPtr)ContextPin,
                    out NativeMethods.k4a_image_t handle
                    ));

                return handle;
            }
            catch
            {
                context?.BufferPin.Free();
                ContextPin.Free();
                
                throw;
            }
        }


        public ArrayImage(ImageFormat format, int width_pixels, int height_pixels) :
            base(CreateHandle(format, width_pixels, height_pixels, out T[] data))
        {
            Buffer = data;
        }

        public T[] Buffer { get; private set; }

        private static void MemoryDestroyCallback(IntPtr buffer, IntPtr context)
        {
            GCHandle contextPin = (GCHandle)context;
            CallbackContext ctx = (CallbackContext)contextPin.Target;
            ctx.BufferPin.Free();
            contextPin.Free();
        }
    }

    public class Image : IDisposable
    {
        public Image(ImageFormat format, int width_pixels, int height_pixels, int stride_bytes)
        {
            AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_image_create(format,
                width_pixels,
                height_pixels,
                stride_bytes,
                out this.handle));
        }


        internal Image(NativeMethods.k4a_image_t handle)
        {
            this.handle = handle;
        }

        

        public unsafe byte[] GetBufferCopy()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                byte[] copy = new byte[this.Size];
                System.Runtime.InteropServices.Marshal.Copy((IntPtr)this.Buffer, copy, 0, checked((int)this.Size));
                return copy;
            }
        }

        public void CopyBytesTo(byte[] destination, int destinationOffset, int sourceOffset, int count)
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                if (destination == null)
                    throw new ArgumentNullException(nameof(destination));
                if (destinationOffset < 0)
                    throw new ArgumentOutOfRangeException(nameof(destinationOffset));
                if (sourceOffset < 0)
                    throw new ArgumentOutOfRangeException(nameof(sourceOffset));
                if (count < 0)
                    throw new ArgumentOutOfRangeException(nameof(count));
                if (destination.Length < checked(destinationOffset + count))
                    throw new ArgumentException("Destination buffer not long enough", nameof(destination));
                if (this.Size < checked((long)(sourceOffset + count)))
                    throw new ArgumentException("Source buffer not long enough");

                unsafe
                {
                    System.Runtime.InteropServices.Marshal.Copy((IntPtr)this.Buffer, destination, destinationOffset, count);
                }
            }
        }

        public void CopyTo<T>(T[] destination, int destinationOffset, int sourceOffsetElements, int countElements) where T : unmanaged
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                unsafe
                {
                    int elementSize = sizeof(T);

                    if (destination == null)
                        throw new ArgumentNullException(nameof(destination));
                    if (destinationOffset < 0)
                        throw new ArgumentOutOfRangeException(nameof(destinationOffset));
                    if (sourceOffsetElements < 0)
                        throw new ArgumentOutOfRangeException(nameof(sourceOffsetElements));
                    if (countElements < 0)
                        throw new ArgumentOutOfRangeException(nameof(countElements));
                    if (destination.Length < checked(destinationOffset + countElements))
                        throw new ArgumentException("Destination buffer not long enough", nameof(destination));
                    if (this.Size < checked((long)((sourceOffsetElements + countElements) * elementSize))) 
                        throw new ArgumentException("Source buffer not long enough");

                    fixed(T* destinationPointer = &destination[destinationOffset])
                    {
                        this.CopyBytesTo(destinationPointer, 
                            (destination.Length - destinationOffset) * elementSize, 
                            0, sourceOffsetElements * elementSize, 
                            countElements * elementSize);
                    }

                    //System.Runtime.InteropServices.Marshal.Copy(UnmanagedBufferPointer, destination, destinationOffset, count);
                }
            }
        }

        public void CopyBytesTo(Image destination, int destinationOffset, int sourceOffset, int count)
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                // Take a new reference on the destinaion image to ensure that if the destinaion object
                // is disposed by another thread, the underlying native memory cannot be freed
                using (Image referenceDestination= destination.Reference())
                {
                    unsafe
                    {
                        void* destinationPointer = referenceDestination.Buffer;

                        this.CopyBytesTo(
                            destinationPointer,
                            checked((int)referenceDestination.Size),
                            destinationOffset,
                            sourceOffset,
                            count);
                    }
                }
            }
        }

        public void CopyBytesFrom(byte[] source, int sourceOffset, int destinationOffset, int count)
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                if (source == null)
                    throw new ArgumentNullException(nameof(source));
                if (sourceOffset < 0)
                    throw new ArgumentOutOfRangeException(nameof(sourceOffset));
                if (destinationOffset < 0)
                    throw new ArgumentOutOfRangeException(nameof(destinationOffset));
                if (count < 0)
                    throw new ArgumentOutOfRangeException(nameof(count));
                if (source.Length < checked(sourceOffset + count))
                    throw new ArgumentException("Source buffer not long enough", nameof(source));
                if (this.Size < checked((long)(destinationOffset + count)))
                    throw new ArgumentException("Destination buffer not long enough");

                unsafe
                {
                    System.Runtime.InteropServices.Marshal.Copy(source, sourceOffset, (IntPtr)this.Buffer, count);
                }
            }
        }

        public void CopyBytesFrom(Image source, int sourceOffset, int destinationOffset, int count)
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                // Take a new reference on the source Image to ensure that if the source object
                // is disposed by another thread, the underlying native memory cannot be freed
                using (Image sourceReference = source.Reference())
                {
                    unsafe
                    {
                        void* sourcePointer = sourceReference.Buffer;

                        this.CopyBytesFrom(
                            sourcePointer,
                            checked((int)sourceReference.Size),
                            sourceOffset,
                            destinationOffset,
                            count);
                    }
                }
            }
        }

        private IntPtr _Buffer = IntPtr.Zero;

        
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

        protected unsafe void CopyBytesTo(void* destination, int destinationLength, int destinationOffset, int sourceOffset, int count)
        {
            lock (this)
            {
                // We don't need to check to see if we are disposed since the call to UnmanagedBufferPointer will 
                // perform that check

                if (destination == null)
                    throw new ArgumentNullException(nameof(destination));
                if (destinationOffset < 0)
                    throw new ArgumentOutOfRangeException(nameof(destination));
                if (sourceOffset < 0)
                    throw new ArgumentOutOfRangeException(nameof(sourceOffset));
                if (count < 0)
                    throw new ArgumentOutOfRangeException(nameof(count));
                if (destinationLength < checked(destinationOffset + count))
                    throw new ArgumentException("Destination buffer not long enough", nameof(destination));
                if (this.Size < checked((long)(sourceOffset + count)))
                    throw new ArgumentException("Source buffer not long enough");

                System.Buffer.MemoryCopy(this.Buffer, destination, destinationLength, count);
            }
        }

        protected unsafe void CopyBytesFrom(void* source, int sourceLength, int sourceOffset, int destinationOffset, int count)
        {
            lock (this)
            {
                // We don't need to check to see if we are disposed since the call to this.UnsafeBufferPointer will 
                // perform that check

                if (source == null)
                    throw new ArgumentNullException(nameof(source));
                if (sourceOffset < 0)
                    throw new ArgumentOutOfRangeException(nameof(sourceOffset));
                if (destinationOffset < 0)
                    throw new ArgumentOutOfRangeException(nameof(destinationOffset));
                if (count < 0)
                    throw new ArgumentOutOfRangeException(nameof(count));
                if (sourceLength < checked(sourceOffset + count))
                    throw new ArgumentException("Source buffer not long enough", nameof(source));
                if (this.Size < checked((long)(destinationOffset + count)))
                    throw new ArgumentException("Destination buffer not long enough");

                unsafe
                {
                    System.Buffer.MemoryCopy((void*)source, (void*)this.Buffer, this.Size, (long)count);
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
                        // TODO: dispose managed state (managed objects).
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
