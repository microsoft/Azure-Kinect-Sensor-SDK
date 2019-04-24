using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Sequential)]
    [Native.NativeReference("k4a_imu_sample_t")]
    public class ImuSample
    {
        public float Temperature { get; set; }
        public Float3 AccelerometerSample { get; set; }
        public Int64 AccelerometerTimestampInUsec { get; set; }
        public Float3 GyroSample { get; set; }
        public Int64 GyroTimestampInUsec { get; set; }

    }
}
