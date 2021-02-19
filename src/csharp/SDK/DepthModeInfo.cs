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
    /// Represents the configuration to run a depth device in.
    /// </summary>
    /// <remarks>
    /// Default initialization is the same as K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.
    /// </remarks>
    public class DepthModeInfo
    {
        /// <summary>
        /// Gets or sets the Struct Size.
        /// </summary>
        public int StructSize { get; set; } = 52;

        /// <summary>
        /// Gets or sets the Struct Version.
        /// </summary>
        public int StructVersion { get; set; } = 1;

        /// <summary>
        /// Gets or sets the ModeId of the depth camera.
        /// </summary>
        public int ModeId { get; set; } = 0;

        /// <summary>
        /// Gets or sets a value indicating whether to use Passive IR Only.
        /// </summary>
        /// <remarks>
        /// If this is false, the Passive IR images are dropped.
        /// </remarks>
        public bool PassiveIROnly { get; set; } = false;

        /// <summary>
        /// Gets or sets the image width.
        /// </summary>
        public int Width { get; set; } = 0;

        /// <summary>
        /// Gets or sets the image height.
        /// </summary>
        public int Height { get; set; } = 0;

        /// <summary>
        /// Gets or sets the image format. Depth image type DEPTH16. Each pixel of DEPTH16 data is two bytes of little-endian unsigned depth data.
        /// </summary>
        public ImageFormat NativeFormat { get; set; } = ImageFormat.Depth16;

        /// <summary>
        /// Gets or sets the Horizontal FOV.
        /// </summary>
        public float HorizontalFOV { get; set; } = 0;

        /// <summary>
        /// Gets or sets the Vertical FOV.
        /// </summary>
        public float VerticalFOV { get; set; } = 0;

        /// <summary>
        /// Gets or sets the Min FPS.
        /// </summary>
        public int MinFPS { get; set; } = 0;

        /// <summary>
        /// Gets or sets the Max FPS.
        /// </summary>
        public int MaxFPS { get; set; } = 0;

        /// <summary>
        /// Gets or sets the Min Range.
        /// </summary>
        public int MinRange { get; set; } = 0;

        /// <summary>
        /// Gets or sets the Max Range.
        /// </summary>
        public int MaxRange { get; set; } = 0;

        /// <summary>
        /// Gets equivalent native configuration struct.
        /// </summary>
        /// <returns>k4a_depth_mode_info_t.</returns>
        internal k4a_depth_mode_info_t GetNativeConfiguration()
        {
            return new k4a_depth_mode_info_t
            {
                struct_size = (uint)this.StructSize,
                struct_version = (uint)this.StructVersion,
                mode_id = (uint)this.ModeId,
                passive_ir_only = Convert.ToByte(this.PassiveIROnly),
                width = (uint)this.Width,
                height = (uint)this.Height,
                native_format = (NativeMethods.k4a_image_format_t)this.NativeFormat,
                horizontal_fov = this.HorizontalFOV,
                vertical_fov = this.VerticalFOV,
                min_fps = (uint)this.MinFPS,
                max_fps = (uint)this.MaxFPS,
                min_range = (uint)this.MinRange,
                max_range = (uint)this.MaxRange,
            };
        }

        /// <summary>
        /// Set properties using native configuration struct.
        /// </summary>
        /// <param name="depthModeInfo">depthModeInfo.</param>
        internal void SetUsingNativeConfiguration(k4a_depth_mode_info_t depthModeInfo)
        {
            this.StructSize = (int)depthModeInfo.struct_size;
            this.StructVersion = (int)depthModeInfo.struct_version;
            this.ModeId = (int)depthModeInfo.mode_id;
            this.PassiveIROnly = Convert.ToBoolean(depthModeInfo.passive_ir_only);
            this.Width = (int)depthModeInfo.width;
            this.Height = (int)depthModeInfo.height;
            this.NativeFormat = (ImageFormat)depthModeInfo.native_format;
            this.HorizontalFOV = depthModeInfo.horizontal_fov;
            this.VerticalFOV = depthModeInfo.vertical_fov;
            this.MinFPS = (int)depthModeInfo.min_fps;
            this.MaxFPS = (int)depthModeInfo.max_fps;
            this.MinRange = (int)depthModeInfo.min_range;
            this.MaxRange = (int)depthModeInfo.max_range;
        }
    }
}
