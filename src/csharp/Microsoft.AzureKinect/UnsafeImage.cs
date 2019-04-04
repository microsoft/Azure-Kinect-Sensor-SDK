using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    public class UnsafeImage : Image
    {
        internal UnsafeImage(NativeMethods.k4a_image_t handle) : base(handle)
        {
        }

        public IntPtr UnsafeBufferPointer
        {
            get
            {
                // We don't need to check to see if we are disposed since the call to UnmanagedBufferPointer will 
                // perform that check

                return this.UnmanagedBufferPointer;
            }
        }

        public void CopyBytesTo(IntPtr destination, int destinationLength, int destinationOffset, int sourceOffset, int count)
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

                unsafe
                {
                    Buffer.MemoryCopy((void*)UnmanagedBufferPointer, (void*)destination, destinationLength, count);
                }
            }
        }

        public void CopyBytesFrom(IntPtr source, int sourceLength, int sourceOffset, int destinationOffset, int count)
        {
            lock (this)
            {
                // We don't need to check to see if we are disposed since the call to UnmanagedBufferPointer will 
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
                    Buffer.MemoryCopy((void*)source, (void*)UnmanagedBufferPointer, this.Size, (long)count);
                }
            }
        }


    }
}
