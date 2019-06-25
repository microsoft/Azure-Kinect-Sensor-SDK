using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Record
{
    [Native.NativeReference("k4a_playback_seek_origin_t")]
    public enum PlaybackSeekOrigin
    {
        Begin,
        End
    }
}
