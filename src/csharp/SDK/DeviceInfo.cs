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
    public class DeviceInfo
    {
        public int StructSize { get; set; } = 20;

        public int StructVersion { get; set; } = 1;

        public int VendorId { get; set; } = 0;

        public int DeviceId { get; set; } = 0;

        public int Capabilities { get; set; } = 0;

        internal k4a_device_info_t GetNativeConfiguration()
        {
            return new k4a_device_info_t
            {
                struct_size = (uint) this.StructSize,
                struct_version = (uint) this.StructVersion,
                vendor_id = (uint) this.VendorId,
                device_id = (uint) this.DeviceId,
                capabilities = (uint) this.Capabilities,
            };
        }
    }
}
