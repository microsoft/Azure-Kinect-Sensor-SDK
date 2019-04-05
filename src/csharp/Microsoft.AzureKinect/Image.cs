using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    public abstract class Image : IDisposable
    {
        internal Image(NativeMethods.k4a_image_t handle)
        {
            this.handle = handle;
        }

        public static Image Create(ImageFormat format, int width_pixels, int height_pixels, int stride_bytes)
        {
            throw new NotImplementedException();
        }



        protected IntPtr UnmanagedBufferPointer
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    return NativeMethods.k4a_image_get_buffer(handle);
                }
            }
        }

        public byte[] GetBufferCopy()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                byte[] copy = new byte[this.Size];
                System.Runtime.InteropServices.Marshal.Copy(UnmanagedBufferPointer, copy, 0, checked((int)this.Size));
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

                System.Runtime.InteropServices.Marshal.Copy(UnmanagedBufferPointer, destination, destinationOffset, count);
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
                        ((UnsafeImage)this).CopyBytesTo((IntPtr)destinationPointer, 
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
                using (UnsafeImage unsafeDestination = (UnsafeImage)destination.Reference())
                {
                    IntPtr destinationPointer = unsafeDestination.UnsafeBufferPointer;

                    ((UnsafeImage)this).CopyBytesTo(
                        destinationPointer,
                        checked((int)unsafeDestination.Size),
                        destinationOffset,
                        sourceOffset,
                        count);
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

                System.Runtime.InteropServices.Marshal.Copy(source, sourceOffset, UnmanagedBufferPointer, count);
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
                using (UnsafeImage unsafeSource = (UnsafeImage)source.Reference())
                {
                    IntPtr sourcePointer = unsafeSource.UnsafeBufferPointer;

                    ((UnsafeImage)this).CopyBytesFrom(
                        sourcePointer,
                        checked((int)unsafeSource.Size),
                        sourceOffset,
                        destinationOffset,
                        count);
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

        public ImageFormat Format
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    return NativeMethods.k4a_image_get_format(handle);
                }
            }
        }

        public int HeightPixels
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    return NativeMethods.k4a_image_get_height_pixels(handle);
                }
            }
        }

        public int WidthPixels
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    return NativeMethods.k4a_image_get_width_pixels(handle);
                }
            }
        }

        public int StrideBytes
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    return NativeMethods.k4a_image_get_stride_bytes(handle);
                }
            }
        }

        public long Size
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Image));

                    return checked((long)NativeMethods.k4a_image_get_size(handle).ToUInt64());
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

        internal IntPtr DangerousGetHandle()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                return handle.DangerousGetHandle();
            }
        }

        public Image Reference()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Image));

                return new UnsafeImage(handle.DuplicateReference());
            }
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
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
