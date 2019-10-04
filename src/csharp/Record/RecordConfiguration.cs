using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Record
{
    public class RecordConfiguration
    {
        public ImageFormat ColorFormat { get; set; }
        public ColorResolution ColorResolution { get; set; }
        public DepthMode DepthMode { get; set; }
        public FPS CameraFPS { get; set; }
        public bool ColorTrackEnabled { get; set; }
        public bool DepthTrackEnabled { get; set; }
        public bool IMUTrackEnabled { get; set; }
        public TimeSpan DepthDelayOffColor { get; set; }
        public WiredSyncMode WiredSyncMode { get; set; }
        public TimeSpan SubordinateDelayOffMaster { get; set; }
        public TimeSpan StartTimestampOffset { get; set; }

        internal static RecordConfiguration FromNative(NativeMethods.k4a_record_configuration_t config)
        {
            return new RecordConfiguration()
            {
                ColorFormat = config.color_format,
                ColorResolution = config.color_resolution,
                DepthMode = config.depth_mode,
                CameraFPS = config.camera_fps,
                ColorTrackEnabled = config.color_track_enabled,
                DepthTrackEnabled = config.depth_track_enabled,
                IMUTrackEnabled = config.imu_track_enabled,
                DepthDelayOffColor = TimeSpan.FromTicks(config.subordinate_delay_off_master_usec * 10),
                WiredSyncMode = config.wired_sync_mode,
                SubordinateDelayOffMaster = TimeSpan.FromTicks(config.subordinate_delay_off_master_usec * 10),
                StartTimestampOffset = TimeSpan.FromTicks(config.start_timestamp_offset_usec)
            };
        }
    }
}
