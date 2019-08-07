//------------------------------------------------------------------------------
// <copyright file="AzureKinectMemoryManager.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Buffers;
using System.Runtime.CompilerServices;
using System.Threading;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Manages the native memory allocated by the Azure Kinect SDK.
    /// </summary>
    internal class AzureKinectMemoryManager : MemoryManager<byte>
    {
        private Image image;
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
        }

        /// <inheritdoc/>
        public unsafe override Span<byte> GetSpan()
        {
            lock (this)
            {
                if (this.image == null)
                {
                    throw new ObjectDisposedException(nameof(AzureKinectMemoryManager));
                }

                return new Span<byte>(this.image.GetUnsafeBuffer(), checked((int)this.image.Size));
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

                _ = Interlocked.Increment(ref this.pinCount);
                return new MemoryHandle(Unsafe.Add<byte>(this.image.GetUnsafeBuffer(), elementIndex), pinnable: this);
            }
        }

        /// <inheritdoc/>
        public override void Unpin()
        {
            _ = Interlocked.Decrement(ref this.pinCount);
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
                        throw new AzureKinectException("Buffer disposed while pinned");
                    }
                }
            }
        }
    }
}
