using System;
using System.Buffers;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Microsoft.Azure.Kinect.Sensor
{
    
    
    class AzureKinectMemoryManager : MemoryManager<byte>
    {
        private Image image;

        internal AzureKinectMemoryManager(Image image)
        {
            this.image = image.Reference();

            memoryPressure = image.Size;
        }

        long memoryPressure;

        public unsafe override Span<byte> GetSpan()
        {
            lock (this)
            {
                if (image == null)
                {
                    throw new ObjectDisposedException(nameof(AzureKinectMemoryManager));
                }
                return new Span<byte>(image.GetUnsafeBuffer(), checked((int)image.Size));
            }
        }

        private int pinCount = 0;

        public unsafe override MemoryHandle Pin(int elementIndex = 0)
        {
            lock (this)
            {
                if (image == null)
                {
                    throw new ObjectDisposedException(nameof(AzureKinectMemoryManager));
                }
                Interlocked.Increment(ref pinCount);
                return new MemoryHandle(Unsafe.Add<byte>(image.GetUnsafeBuffer(), elementIndex), pinnable: this);
            }
        }

        public override void Unpin()
        {
            Interlocked.Decrement(ref pinCount);
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                lock (this)
                {
                    if (image != null)
                    {
                        image.Dispose();
                        image = null;
                    }

                    if (pinCount != 0)
                    {
                        throw new Exception("Buffer disposed while pinned");
                    }
                }
            }
        }
    }
}
