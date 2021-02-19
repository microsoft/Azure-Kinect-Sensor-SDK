//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Runtime.InteropServices;
using static Microsoft.Azure.Kinect.Sensor.NativeMethods;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Represents the configuration to run an Azure Kinect device in.
    /// </summary>
    /// <remarks>
    /// Gets or sets the Device Info.
    /// </remarks>
    public class DeviceInfo
    {
        /// <summary>
        /// Gets or sets Struct Size.
        /// </summary>
        public int StructSize { get; set; } = 20;

        /// <summary>
        /// Gets or sets the Struct Version.
        /// </summary>
        public int StructVersion { get; set; } = 1;

        /// <summary>
        /// Gets or sets the Vendor Id.
        /// </summary>
        public int VendorId { get; set; } = 0;

        /// <summary>
        /// Gets or sets the Device Id.
        /// </summary>
        public int DeviceId { get; set; } = 0;

        /// <summary>
        /// Gets or sets the Capabilities.
        /// </summary>
        public int Capabilities { get; set; } = 0;

        /// <summary>
        /// Get the equivalent native configuration structure.
        /// </summary>
        /// <returns>k4a_device_info_t.</returns>
        internal k4a_device_info_t GetNativeConfiguration()
        {
            return new k4a_device_info_t
            {
                struct_size = (uint)this.StructSize,
                struct_version = (uint)this.StructVersion,
                vendor_id = (uint)this.VendorId,
                device_id = (uint)this.DeviceId,
                capabilities = (uint)this.Capabilities,
            };
        }

        /// <summary>
        /// Set properties using native configuration struct.
        /// </summary>
        /// <param name="deviceInfo">deviceInfo.</param>
        internal void SetUsingNativeConfiguration(k4a_device_info_t deviceInfo)
        {
            this.StructSize = (int)deviceInfo.struct_size;
            this.StructVersion = (int)deviceInfo.struct_version;
            this.VendorId = (int)deviceInfo.vendor_id;
            this.DeviceId = (int)deviceInfo.device_id;
            this.Capabilities = (int)deviceInfo.capabilities;
        }
    }
}
