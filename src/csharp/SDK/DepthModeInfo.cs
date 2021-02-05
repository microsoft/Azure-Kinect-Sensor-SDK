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
    public class DepthModeInfo
    {
        public int StructSize { get; set; } = 52;

        public int StructVersion { get; set; } = 1;

        public int ModeId { get; set; } = 0;

        public bool PassiveIROnly { get; set; } = false;

        public int Width { get; set; } = 0;

        public int Height { get; set; } = 0;

        public ImageFormat NativeFormat { get; set; } = ImageFormat.Depth16;

        public float HorizontalFOV { get; set; } = 0;

        public float VerticalFOV { get; set; } = 0;

        public int MinFPS { get; set; } = 0;

        public int MaxFPS { get; set; } = 0;

        public int MinRange { get; set; } = 0;

        public int MaxRange { get; set; } = 0;

        internal k4a_depth_mode_info_t GetNativeConfiguration()
        {
            return new k4a_depth_mode_info_t
            {
                struct_size = (uint)this.StructSize,
                struct_version = (uint)this.StructVersion,
                mode_id = (uint)this.ModeId,
                passive_ir_only = this.PassiveIROnly,
                width = (uint)this.Width,
                height = (uint)this.Height,
                native_format = (NativeMethods.k4a_image_format_t)this.NativeFormat,
                horizontal_fov = this.HorizontalFOV,
                vertical_fov = this.VerticalFOV,
                min_fps = this.MinFPS,
                max_fps = this.MaxFPS,
                min_range = this.MinRange,
                max_range = this.MaxRange,
            };
        }
    }
}
