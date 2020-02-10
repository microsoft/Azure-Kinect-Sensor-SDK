//------------------------------------------------------------------------------
// <copyright file="LargeArrayPool.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Buffers;
using System.Collections.Generic;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// An array pool implementation for large arrays.
    /// </summary>
    /// <remarks>
    /// This ArrayPool allocates and re-uses large arrays to reduce the overhead of
    /// zeroing out the buffers and allocating from the managed heap.
    ///
    /// Unused arrays are held by weak references and may be garbage collected.
    /// </remarks>
    internal class LargeArrayPool : ArrayPool<byte>
    {
        private readonly List<WeakReference> pool = new List<WeakReference>();

        /// <inheritdoc/>
        public override byte[] Rent(int minimumLength)
        {
            lock (this)
            {
                byte[] buffer;
                foreach (WeakReference x in this.pool)
                {
                    if (x.IsAlive)
                    {
                        buffer = (byte[])x.Target;
                        if (buffer != null && buffer.Length >= minimumLength)
                        {
                            _ = this.pool.Remove(x);
                            return buffer;
                        }
                    }
                }

                return new byte[minimumLength];
            }
        }

        /// <inheritdoc/>
        public override void Return(byte[] array, bool clearArray = false)
        {
            lock (this)
            {
                if (clearArray)
                {
                    throw new NotSupportedException();
                }

                this.pool.Add(new WeakReference(array));

                int count = this.pool.RemoveAll((x) => !x.IsAlive);
            }
        }
    }
}
