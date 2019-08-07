//------------------------------------------------------------------------------
// <copyright file="AzureKinectMemoryCast.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Buffers;
using System.Runtime.InteropServices;

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
        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectMemoryCast{TFrom, TTo}"/> class.
        /// </summary>
        /// <param name="memory">Memory object to cast.</param>
        public AzureKinectMemoryCast(Memory<TFrom> memory)
        {
            this.Source = memory;
        }

        private Memory<TFrom> Source { get; }

        /// <inheritdoc/>
        public override Span<TTo> GetSpan()
        {
            return MemoryMarshal.Cast<TFrom, TTo>(this.Source.Span);
        }

        /// <inheritdoc/>
        public override MemoryHandle Pin(int elementIndex = 0)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc/>
        public override void Unpin()
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc/>
        protected override void Dispose(bool disposing)
        {
        }
    }
}
