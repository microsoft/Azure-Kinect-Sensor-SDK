// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;

namespace Microsoft.Azure.Kinect.Sensor
{
    public class DeviceConfiguration
    {
        // Initialize values to the configuration sepcified by
        // K4A_DEVICE_CONFIG_INIT_DISABLE_ALL

        public ImageFormat ColorFormat { get; set; } = ImageFormat.ColorMJPG;
        public ColorResolution ColorResolution { get; set; } = ColorResolution.Off;
        public DepthMode DepthMode { get; set; } = DepthMode.Off;
        public FPS CameraFPS { get; set; } = FPS.fps30;
        public bool SynchronizedImagesOnly { get; set; } = false;
        public TimeSpan DepthDelayOffColor { get; set; } = TimeSpan.Zero;
        public WiredSyncMode WiredSyncMode { get; set; } = WiredSyncMode.Standalone;
        public TimeSpan SuboridinateDelayOffMaster { get; set; } = TimeSpan.Zero;
        public bool DisableStreamingIndicator { get; set; } = false;

        internal NativeMethods.k4a_device_configuration_t GetNativeConfiguration()
        {
            // Ticks are in 100ns units
            
            int depth_delay_off_color_usec = checked((int)(
                this.DepthDelayOffColor.Ticks / 10
                ));

            uint subordinate_delay_off_master_usec = checked((uint)(
                this.SuboridinateDelayOffMaster.Ticks / 10
                ));

            return new NativeMethods.k4a_device_configuration_t
            {
                color_format = this.ColorFormat,
                color_resolution = this.ColorResolution,
                depth_mode = this.DepthMode,
                camera_fps = this.CameraFPS,
                synchronized_images_only = this.SynchronizedImagesOnly,
                depth_delay_off_color_usec = depth_delay_off_color_usec,
                wired_sync_mode = this.WiredSyncMode,
                subordinate_delay_off_master_usec = subordinate_delay_off_master_usec,
                disable_streaming_indicator = this.DisableStreamingIndicator
            };
        }
    }
}
