using System;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Record
{
    public class DataBlock : IDisposable
    {
        // The native handle for this data block.
        private readonly NativeMethods.k4a_playback_data_block_t handle;

        // To detect redundant calls to Dispose
        private bool disposedValue = false;

        private byte[] buffer = null;

        internal DataBlock(NativeMethods.k4a_playback_data_block_t handle)
        {
            this.handle = handle;
        }

        public byte[] Buffer
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(DataBlock));
                    }

                    if (this.buffer == null)
                    {
                        ulong bufferSize = NativeMethods.k4a_playback_data_block_get_buffer_size(this.handle);

                        this.buffer = new byte[bufferSize];
                        
                        IntPtr bufferPtr = NativeMethods.k4a_playback_data_block_get_buffer(this.handle);

                        if (bufferPtr != IntPtr.Zero)
                        {
                            Marshal.Copy(bufferPtr, this.buffer, 0, checked((int)bufferSize));
                        }
                        else
                        {
                            this.buffer = null;
                        }
                    }

                    return this.buffer;
                }
            }
        }

        public TimeSpan DeviceTimestamp
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(DataBlock));
                    }

                    ulong timeStamp = NativeMethods.k4a_playback_data_block_get_device_timestamp_usec(this.handle);

                    return TimeSpan.FromTicks(checked((long)timeStamp) * 10);
                }
            }
        }


        /// <inheritdoc/>
        public void Dispose()
        {
            this.Dispose(true);

            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Handle the disposing of the object.
        /// </summary>
        /// <param name="disposing">true when called by Dispose(), false when called by the finalizer.</param>
        protected virtual void Dispose(bool disposing)
        {
            lock (this)
            {
                this.handle.Close();

                this.disposedValue = true;
            }
        }
    }
}
