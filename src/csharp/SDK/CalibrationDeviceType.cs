// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_calibration_type_t")]
    public enum CalibrationDeviceType
    {
        [Native.NativeReference("K4A_CALIBRATION_TYPE_UNKNOWN")]
        Unknown = -1,
        [Native.NativeReference("K4A_CALIBRATION_TYPE_DEPTH")]
        Depth,
        [Native.NativeReference("K4A_CALIBRATION_TYPE_COLOR")]
        Color,
        [Native.NativeReference("K4A_CALIBRATION_TYPE_GYRO")]
        Gyro,
        [Native.NativeReference("K4A_CALIBRATION_TYPE_ACCEL")]
        Accel,
        [Native.NativeReference("K4A_CALIBRATION_TYPE_NUM")]
        Num
    }
}
