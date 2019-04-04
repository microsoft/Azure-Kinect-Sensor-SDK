using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    public class DeviceConfiguration
    {
        // Initialize values to the configuration sepcified by
        // K4A_DEVICE_CONFIG_INIT_DISABLE_ALL

        public ImageFormat color_format { get; set; } = ImageFormat.ColorMJPG;
        public ColorResolution color_resolution { get; set; } = ColorResolution.Off;
        public DepthMode depth_mode { get; set; } = DepthMode.Off;
        public FPS camera_fps { get; set; } = FPS.fps30;
        public bool synchronized_images_only { get; set; } = false;
        public TimeSpan depth_delay_off_color { get; set; } = TimeSpan.Zero;
        public WiredSyncMode wired_sync_mode { get; set; } = WiredSyncMode.Standalone;
        public TimeSpan subordinate_delay_off_master { get; set; } = TimeSpan.Zero;
        public bool disable_streaming_indicator { get; set; } = false;

        internal NativeMethods.k4a_device_configuration_t GetNativeConfiguration()
        {
            // Ticks are in 100ns units
            
            int depth_delay_off_color_usec = checked((int)(
                this.depth_delay_off_color.Ticks / 10
                ));

            uint subordinate_delay_off_master_usec = checked((uint)(
                this.subordinate_delay_off_master.Ticks / 10
                ));

            return new NativeMethods.k4a_device_configuration_t
            {
                color_format = this.color_format,
                color_resolution = this.color_resolution,
                depth_mode = this.depth_mode,
                camera_fps = this.camera_fps,
                synchronized_images_only = this.synchronized_images_only,
                depth_delay_off_color_usec = depth_delay_off_color_usec,
                wired_sync_mode = this.wired_sync_mode,
                subordinate_delay_off_master_usec = subordinate_delay_off_master_usec,
                disable_streaming_indicator = disable_streaming_indicator
            };
        }
    }
}
