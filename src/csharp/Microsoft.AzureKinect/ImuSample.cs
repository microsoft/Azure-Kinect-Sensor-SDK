using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Sequential)]
    [Native.NativeReference("k4a_imu_sample_t")]
    public struct ImuSample
    {
        public float temperature;
        public Float3 acc_sample;
        public UInt64 acc_timestamp_usec;
        public Float3 gyro_sample;
        public UInt64 gyro_timestamp_usec;

    }
}
