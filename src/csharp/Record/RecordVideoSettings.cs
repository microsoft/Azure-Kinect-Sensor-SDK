using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Record
{
    [StructLayout(LayoutKind.Sequential)]
    public class RecordVideoSettings
    {
        public ulong Width { get; set; }
        public ulong Height { get; set; }
        public ulong FrameRate { get; set; }
    }
}
