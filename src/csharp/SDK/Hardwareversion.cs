// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;

namespace Microsoft.Azure.Kinect.Sensor
{
    public class HardwareVersion : IEquatable<HardwareVersion>
    {
        public Version RGB { get; set; }
        public Version Depth { get; set; }
        public Version Audio { get; set; }
        public Version DepthSensor { get; set; }
        public FirmwareBuild FirmwareBuild { get; set; }
        public FirmwareSignature FirmwareSignature { get; set; }

        public override bool Equals(object obj)
        {
            return obj is HardwareVersion ? this.Equals((HardwareVersion)obj) : false;
        }

        public bool Equals(HardwareVersion other)
        {
            return other != null && 
                this.RGB.Equals(other.RGB) &&
                this.Depth.Equals(other.Depth) &&
                this.Audio.Equals(other.Audio) &&
                this.DepthSensor.Equals(other.DepthSensor) &&
                this.FirmwareBuild.Equals(other.FirmwareBuild) &&
                this.FirmwareSignature.Equals(other.FirmwareSignature);
        }

        public override int GetHashCode()
        {
            int hash = 7;
            hash = (hash * 7) ^ RGB.GetHashCode();
            hash = (hash * 7) ^ Depth.GetHashCode();
            hash = (hash * 7) ^ Audio.GetHashCode();
            hash = (hash * 7) ^ DepthSensor.GetHashCode();
            hash = (hash * 7) ^ FirmwareBuild.GetHashCode();
            hash = (hash * 7) ^ FirmwareSignature.GetHashCode();
            return hash;
        }
        
        public static bool operator==(HardwareVersion a, HardwareVersion b)
        {
            if (a is null) return b is null;

            return a.Equals(b);
        }

        public static bool operator!=(HardwareVersion a, HardwareVersion b)
        {
            if (a is null) return !(b is null);

            if (object.ReferenceEquals(a, null))
                return !object.ReferenceEquals(b, null);

            return !a.Equals(b);
        }


    }
}
