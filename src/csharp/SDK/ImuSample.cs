//------------------------------------------------------------------------------
// <copyright file="ImuSample.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
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
        /// Gets or sets temperature reading of this sample (Celsius).
        /// </summary>
        public float Temperature { get; set; }

        /// <summary>
        /// Gets or sets accelerometer reading of this sample (meters per second squared).
        /// </summary>
        public Vector3 AccelerometerSample { get; set; }

        /// <summary>
        /// Gets or sets time-stamp of the accelerometer.
        /// </summary>
        public TimeSpan AccelerometerTimestamp { get; set; }

        /// <summary>
        /// Gets or sets gyroscope sample in radians per second.
        /// </summary>
        public Vector3 GyroSample { get; set; }

        /// <summary>
        /// Gets or sets time-stamp of the gyroscope.
        /// </summary>
        public TimeSpan GyroTimestamp { get; set; }
    }
}
