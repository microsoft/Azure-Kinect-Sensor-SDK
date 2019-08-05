// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// IMU sample.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    [Native.NativeReference("k4a_imu_sample_t")]
    public class ImuSample
    {
        /// <summary>
        /// Gets or sets temperature reading of this sample (Celcius).
        /// </summary>
        public float Temperature { get; set; }

        /// <summary>
        /// Gets or sets accelerometer reading of this sample (meters per second squared).
        /// </summary>
        public Vector3 AccelerometerSample { get; set; }

        /// <summary>
        /// Gets or sets timestamp of the accerometer in microseconds.
        /// </summary>
        public long AccelerometerTimestampInUsec { get; set; }

        /// <summary>
        /// Gets or sets gyroscope sample in radians per second.
        /// </summary>
        public Vector3 GyroSample { get; set; }

        /// <summary>
        /// Gets or sets timestamp of the gyroscope in microseconds.
        /// </summary>
        public long GyroTimestampInUsec { get; set; }

    }
}
