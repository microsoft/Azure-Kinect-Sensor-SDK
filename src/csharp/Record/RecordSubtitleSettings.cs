using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Record
{
    [StructLayout(LayoutKind.Sequential)]
    public class RecordSubtitleSettings
    {
        public bool HighFrequencyData { get; set; }
    }
}
