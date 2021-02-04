//------------------------------------------------------------------------------
// <copyright file="DeviceConfiguration.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Represents the configuration to run an Azure Kinect device in.
    /// </summary>
    /// <remarks>
    /// Default initialization is the same as K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.
    /// </remarks>
    public class DeviceConfiguration
    {
        /// <summary>
        /// Gets or sets the image format to capture with the color camera.
        /// </summary>
        public ImageFormat ColorFormat { get; set; } = ImageFormat.ColorMJPG;

        /// <summary>
        /// Gets or sets the color mode id to capture with the color camera.
        /// </summary>
        public int ColorModeId { get; set; } = 0; // 0 = Off

        /// <summary>
        /// Gets or sets the capture mode id for the depth camera.
        /// </summary>
        public int DepthModeId { get; set; } = 0; // 0 = Off

        /// <summary>
        /// Gets or sets the desired frame rate id for the color and depth cameras.
        /// </summary>
        public int FPSModeId { get; set; } = 0;

        /// <summary>
        /// Gets or sets a value indicating whether to only return synchronized depth and color images.
        /// </summary>
        /// <remarks>
        /// If this is false, when color or depth images are dropped, the other corresponding image will be dropped too.
        /// </remarks>
        public bool SynchronizedImagesOnly { get; set; } = false;

        /// <summary>
        /// Gets or sets the desired delay between the capture of the color image and the capture of the depth image.
        /// </summary>
        public TimeSpan DepthDelayOffColor { get; set; } = TimeSpan.Zero;

        /// <summary>
        /// Gets or sets the external synchronization mode.
        /// </summary>
        public WiredSyncMode WiredSyncMode { get; set; } = WiredSyncMode.Standalone;

        /// <summary>
        /// Gets or sets the external synchronization timing.
        /// </summary>
        public TimeSpan SuboridinateDelayOffMaster { get; set; } = TimeSpan.Zero;

        /// <summary>
        /// Gets or sets a value indicating whether the automatic streaming indicator light is disabled.
        /// </summary>
        public bool DisableStreamingIndicator { get; set; } = false;

        /// <summary>
        /// Get the equivalent native configuration structure.
        /// </summary>
        /// <returns>A k4a_device_configuration_t.</returns>
        internal NativeMethods.k4a_device_configuration_t GetNativeConfiguration()
        {
            // Ticks are in 100ns units
            int depth_delay_off_color_usec = checked((int)(
                this.DepthDelayOffColor.Ticks / 10));

            uint subordinate_delay_off_master_usec = checked((uint)(
                this.SuboridinateDelayOffMaster.Ticks / 10));

            return new NativeMethods.k4a_device_configuration_t
            {
                color_format = this.ColorFormat,
                color_mode_id = (uint) this.ColorModeId,
                depth_mode_id = (uint) this.DepthModeId,
                fps_mode_id = (uint) this.FPSModeId,
                synchronized_images_only = this.SynchronizedImagesOnly,
                depth_delay_off_color_usec = depth_delay_off_color_usec,
                wired_sync_mode = this.WiredSyncMode,
                subordinate_delay_off_master_usec = subordinate_delay_off_master_usec,
                disable_streaming_indicator = this.DisableStreamingIndicator,
            };
        }
    }
}
