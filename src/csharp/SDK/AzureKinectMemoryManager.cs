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
    class AzureKinectArrayMemoryOwner<T> : IMemoryOwner<T>
    {
        public AzureKinectArrayMemoryOwner(T[] array, int start, int length)
        {
            this.Memory = new Memory<T>(array, start, length);
        }

        public Memory<T> Memory { get; }

        public void Dispose()
        {
        }
    }

    internal class AzureKinectMemoryCast<TFrom, TTo> : MemoryManager<TTo>
        where TFrom : unmanaged
        where TTo : unmanaged
    {
        private Memory<TFrom> Source { get; }
        public AzureKinectMemoryCast(Memory<TFrom> memory)
        {
            this.Source = memory;
        }
        public override Span<TTo> GetSpan()
        {
            return MemoryMarshal.Cast<TFrom, TTo>(this.Source.Span);
        }

        public override MemoryHandle Pin(int elementIndex = 0)
        {
            throw new NotImplementedException();
        }

        public override void Unpin()
        {
            throw new NotImplementedException();
        }

        protected override void Dispose(bool disposing)
        {
        }
    }

    class AzureKinectMemorytOwnerCast<TFrom, TTo> : MemoryManager<TTo>
        where TFrom : unmanaged
        where TTo : unmanaged
    {
        
        public AzureKinectMemorytOwnerCast(IMemoryOwner<TFrom> memory, int from_start, int from_length)
        {
            this.Source = memory;
            this.FromStart = from_start;
            this.FromLength = from_length;
        }

        private IMemoryOwner<TFrom> Source { get; }

        private int FromStart { get; }
        private int FromLength { get; }

        public override Span<TTo> GetSpan()
        {
            return MemoryMarshal.Cast<TFrom, TTo>(this.Source.Memory.Span.Slice(FromStart, FromLength));
        }

        public override MemoryHandle Pin(int elementIndex = 0)
        {
            throw new NotImplementedException();
        }

        public override void Unpin()
        {
            throw new NotImplementedException();
        }

        protected override void Dispose(bool disposing)
        {
            this.Source.Dispose();
        }
    }


    class AzureKinectMemoryManager : MemoryManager<byte>
    {
        private Image image;

        internal AzureKinectMemoryManager(Image image)
        {
            this.image = image.Reference();

            memoryPressure = image.Size;
            //GC.AddMemoryPressure(memoryPressure);
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
                return new Span<byte>(image.Buffer, checked((int)image.Size));
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
                return new MemoryHandle(Unsafe.Add<byte>(image.Buffer, elementIndex), pinnable: this);
            }
        }

        public override void Unpin()
        {
            Interlocked.Decrement(ref pinCount);
        }

        ~AzureKinectMemoryManager()
        {
            Dispose(false);
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
                GC.SuppressFinalize(this);
            }

            //GC.RemoveMemoryPressure(memoryPressure);
        }
    }
}
