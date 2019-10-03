//------------------------------------------------------------------------------
// <copyright file="Record.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Record
{
    /// <summary>
    /// Represents a writable sensor recording.
    /// </summary>
    public class Record : IDisposable
    {
        // The native handle for this recording.
        private NativeMethods.k4a_record_t handle;

        // To detect redundant calls to Dispose
        private bool disposedValue = false;

        private Record(NativeMethods.k4a_record_t handle)
        {
            this.handle = handle;
        }

        /// <summary>
        /// Create a recording.
        /// </summary>
        /// <param name="path">Path to the recording.</param>
        /// <param name="device">Device to get properties from. May be null for user-generated recordings.</param>
        /// <param name="deviceConfiguration">Parameters used to open the device.</param>
        /// <returns>A new recording object.</returns>
        public static Record Create(string path, Device device, DeviceConfiguration deviceConfiguration)
        {
            NativeMethods.k4a_record_t handle = null;
            if (device != null)
            {
                // If a device was specified, lock that device to avoid disposal while in use.
                // Device.Dispose will take the same lock.
                lock (device)
                {
                    AzureKinectCreateRecordingException.ThrowIfNotSuccess(() => NativeMethods.k4a_record_create(path, device.Handle, NativeMethods.k4a_device_configuration_t.FromDeviceConfiguration(deviceConfiguration), out handle));
                }
            }
            else
            {
                AzureKinectCreateRecordingException.ThrowIfNotSuccess(() => NativeMethods.k4a_record_create(path, IntPtr.Zero, NativeMethods.k4a_device_configuration_t.FromDeviceConfiguration(deviceConfiguration), out handle));
            }

            return new Record(handle);
        }

        /// <summary>
        /// Adds a tag to the recroding.
        /// </summary>
        /// <param name="name">Name of the tag.</param>
        /// <param name="value">Value of the tag.</param>
        public void AddTag(string name, string value)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Record));
                }

                AzureKinectAddTagException.ThrowIfNotSuccess(() => NativeMethods.k4a_record_add_tag(this.handle, name, value));
            }
        }

        /// <summary>
        /// Writes the recording header to disk.
        /// </summary>
        public void WriteHeader()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Record));
                }

                // TODO: Change exception type
                AzureKinectAddTagException.ThrowIfNotSuccess(() => NativeMethods.k4a_record_write_header(this.handle));
            }
        }

        /// <summary>
        /// Writes a capture to the recording file.
        /// </summary>
        /// <param name="capture">Capture containing data to write.</param>
        public void WriteCapture(Capture capture)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Record));
                }

                if (capture == null)
                {
                    throw new ArgumentNullException(nameof(capture));
                }

                using (Capture reference = capture.Reference())
                {
                    // TODO: Change exception type
                    AzureKinectAddTagException.ThrowIfNotSuccess(() => NativeMethods.k4a_record_write_capture(this.handle, reference.Handle));
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
