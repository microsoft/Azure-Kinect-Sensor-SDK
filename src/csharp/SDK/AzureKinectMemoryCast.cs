using System;
using System.Buffers;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Helper to cast from one Memory type to another.
    /// </summary>
    /// <typeparam name="TFrom">Element type of the original Memory.</typeparam>
    /// <typeparam name="TTo">Element type of the new Memory.</typeparam>
    /// <remarks>
    /// This type does not take ownership of the Memory, so Dispose does nothing.
    /// The resultant Memory object derived from this type has the same useful lifetime as
    /// the source Memory object.
    /// </remarks>
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
}
