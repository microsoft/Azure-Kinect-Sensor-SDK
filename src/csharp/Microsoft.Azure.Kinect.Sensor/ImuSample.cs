// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    [StructLayout(LayoutKind.Sequential)]
    [Native.NativeReference("k4a_imu_sample_t")]
    public class ImuSample
    {
        public float Temperature { get; set; }
        public Vector3 AccelerometerSample { get; set; }
        public Int64 AccelerometerTimestampInUsec { get; set; }
        public Vector3 GyroSample { get; set; }
        public Int64 GyroTimestampInUsec { get; set; }

    }
}
