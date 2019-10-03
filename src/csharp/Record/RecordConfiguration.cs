using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Record
{
    public class RecordConfiguration
    {
        ImageFormat ColorFormat { get; set; }
        ColorResolution ColorResolution { get; set; }
        DepthMode DepthMode { get; set; }
        FPS CameraFPS { get; set; }
        bool ColorTrackEnabled { get; set; }
        bool DepthTrackEnabled { get; set; }
        bool IMUTrackEnabled { get; set; }
        TimeSpan DepthDelayOffColor { get; set; }
        WiredSyncMode WiredSyncMode { get; set; }
        TimeSpan SubordinateDelayOffMaster { get; set; }
        TimeSpan StartTimestampOffset { get; set; }

    }
}
