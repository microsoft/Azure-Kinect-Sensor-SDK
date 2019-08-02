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
    /// <summary>
    /// Manages the native memory allocated by the Azure Kinect SDK.
    /// </summary>
    internal class AzureKinectMemoryManager : MemoryManager<byte>
    {
        private Image image;
        readonly long memoryPressure;
        private int pinCount = 0;

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectMemoryManager"/> class.
        /// </summary>
        /// <param name="image">Image with native memory.</param>
        /// <remarks>
        /// Constructs a new MemoryManager representing the native memory backing an Image.
        /// </remarks>
        internal AzureKinectMemoryManager(Image image)
        {
            this.image = image.Reference();

            this.memoryPressure = image.Size;
        }

        /// <inheritdoc/>
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

        /// <inheritdoc/>
        public unsafe override MemoryHandle Pin(int elementIndex = 0)
        {
            lock (this)
            {
                if (this.image == null)
                {
                    throw new ObjectDisposedException(nameof(AzureKinectMemoryManager));
                }

                Interlocked.Increment(ref this.pinCount);
                return new MemoryHandle(Unsafe.Add<byte>(this.image.GetUnsafeBuffer(), elementIndex), pinnable: this);
            }
        }

        /// <inheritdoc/>
        public override void Unpin()
        {
            Interlocked.Decrement(ref this.pinCount);
        }

        /// <inheritdoc/>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                lock (this)
                {
                    if (this.image != null)
                    {
                        this.image.Dispose();
                        this.image = null;
                    }

                    if (this.pinCount != 0)
                    {
                        throw new AzureKinectAllocatorException("Buffer disposed while pinned");
                    }
                }
            }
        }
    }
}
