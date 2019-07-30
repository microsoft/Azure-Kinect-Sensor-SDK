using System;
using System.Buffers;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor
{
    internal class LargeArrayPool : ArrayPool<byte>
    {
        private readonly List<WeakReference> pool = new List<WeakReference>();

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
                        if (buffer.Length >= minimumLength)
                        {
                            _ = this.pool.Remove(x);
                            return buffer;
                        }
                    }
                }

                return new byte[minimumLength];
            }
        }

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
