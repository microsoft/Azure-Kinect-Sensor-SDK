using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_color_control_mode_t")]
    public enum ColorControlMode
    {
        Auto = 0,
        Manual
    }
}
