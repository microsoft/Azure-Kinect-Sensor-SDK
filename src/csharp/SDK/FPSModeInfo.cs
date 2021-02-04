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
    [Native.NativeReference("k4a_fps_mode_info_t")]
    public struct FPSModeInfo : IEquatable<FPSModeInfo>
    {
        public int StructSize { get; set; }

        public int StructVersion { get; set; }

        public int ModeId { get; set; }

        public int FPS { get; set; }

        public static bool operator ==(FPSModeInfo left, FPSModeInfo right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(FPSModeInfo left, FPSModeInfo right)
        {
            return !(left == right);
        }

        public override bool Equals(object obj)
        {
            return obj is FPSModeInfo fpsModeInfo && this.Equals(fpsModeInfo);
        }

        public bool Equals(FPSModeInfo other)
        {
            return this.StructSize == other.StructSize &&
                this.StructVersion == other.StructVersion &&
                this.ModeId == other.ModeId &&
                this.FPS == other.FPS;
        }

        public override int GetHashCode()
        {
            int hashCode = -454809512;
            hashCode = (hashCode * -1521134295) + this.StructSize.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.StructVersion.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.ModeId.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.FPS.GetHashCode();
            return hashCode;
        }
    }
}
