using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_wired_sync_mode_t")]
    public enum WiredSyncMode
    {
        Standalone,
        Master,
        Subordinate
    }
}
