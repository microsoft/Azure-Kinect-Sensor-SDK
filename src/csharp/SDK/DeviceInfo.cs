//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    [StructLayout(LayoutKind.Sequential)]
    [Native.NativeReference("k4a_device_info_t")]
    public struct DeviceInfo : IEquatable<DeviceInfo>
    {
        public int StructSize { get; set; } 

        public int StructVersion { get; set; }

        public int VendorId { get; set; }

        public int DeviceId { get; set; }

        public int Capabilities { get; set; }

        public static bool operator ==(DeviceInfo left, DeviceInfo right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(DeviceInfo left, DeviceInfo right)
        {
            return !(left == right);
        }

        public override bool Equals(object obj)
        {
            return obj is DeviceInfo deviceInfo && this.Equals(deviceInfo);
        }

        public bool Equals(DeviceInfo other)
        {
            return this.StructSize == other.StructSize &&
                this.StructVersion == other.StructVersion &&
                this.VendorId == other.VendorId &&
                this.DeviceId == other.DeviceId &&
                this.Capabilities == other.Capabilities;
        }

        public override int GetHashCode()
        {
            int hashCode = -454809512;
            hashCode = (hashCode * -1521134295) + this.StructSize.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.StructVersion.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.VendorId.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.DeviceId.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.Capabilities.GetHashCode();
            return hashCode;
        }
    }
}
