//------------------------------------------------------------------------------
// <copyright file="CalibrationDeviceType.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Specifies a type of calibration.
    /// </summary>
    [Native.NativeReference("k4a_calibration_type_t")]
    public enum CalibrationDeviceType
    {
        /// <summary>
        /// Calibration type is unknown.
        /// </summary>
        [Native.NativeReference("K4A_CALIBRATION_TYPE_UNKNOWN")]
        Unknown = -1,

        /// <summary>
        /// Depth sensor.
        /// </summary>
        [Native.NativeReference("K4A_CALIBRATION_TYPE_DEPTH")]
        Depth,

        /// <summary>
        /// Color sensor.
        /// </summary>
        [Native.NativeReference("K4A_CALIBRATION_TYPE_COLOR")]
        Color,

        /// <summary>
        /// Gyroscope sensor.
        /// </summary>
        [Native.NativeReference("K4A_CALIBRATION_TYPE_GYRO")]
        Gyro,

        /// <summary>
        /// Accelerometer sensor.
        /// </summary>
        [Native.NativeReference("K4A_CALIBRATION_TYPE_ACCEL")]
        Accel,

        /// <summary>
        /// Number of types excluding unknown type.
        /// </summary>
        [Native.NativeReference("K4A_CALIBRATION_TYPE_NUM")]
        Num,
    }
}
