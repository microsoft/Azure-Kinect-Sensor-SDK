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
    [Native.NativeReference("k4a_color_mode_info_t")]
    public class ColorModeInfo : IEquatable<ColorModeInfo>
    {
        public int StructSize { get; set; }

        public int StructVersion { get; set; }

        public int ModeId { get; set; }

        public int Width { get; set; }

        public int Height { get; set; }

        public ImageFormat NativeFormat { get; set; }

        public float HorizontalFOV { get; set; }

        public float VerticalFOV { get; set; }

        public int MinFPS { get; set; }

        public int MaxFPS { get; set; }

        public static bool operator ==(ColorModeInfo left, ColorModeInfo right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(ColorModeInfo left, ColorModeInfo right)
        {
            return !(left == right);
        }

        public override bool Equals(object obj)
        {
            return obj is ColorModeInfo colorModeInfo && this.Equals(colorModeInfo);
        }

        public bool Equals(ColorModeInfo other)
        {
            return this.StructSize == other.StructSize &&
                this.StructVersion == other.StructVersion &&
                this.ModeId == other.ModeId &&
                this.Width == other.Width &&
                this.Height == other.Height &&
                this.NativeFormat == other.NativeFormat &&
                this.HorizontalFOV == other.HorizontalFOV &&
                this.VerticalFOV == other.VerticalFOV &&
                this.MinFPS == other.MinFPS &&
                this.MaxFPS == other.MaxFPS;
        }

        public override int GetHashCode()
        {
            int hashCode = -454809512;
            hashCode = (hashCode * -1521134295) + this.StructSize.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.StructVersion.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.ModeId.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.Width.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.Height.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.NativeFormat.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.HorizontalFOV.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.VerticalFOV.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.MinFPS.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.MaxFPS.GetHashCode();
            return hashCode;
        }
    }
}
