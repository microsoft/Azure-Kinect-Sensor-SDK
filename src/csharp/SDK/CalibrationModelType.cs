// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_calibration_model_type_t")]
    public enum CalibrationModelType
    {
        [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_UNKNOWN")]
        Unknown = 0,
        [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_THETA")]
        Theta,
        [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_POLYNOMIAL_3K")]
        Polynomial3K,
        [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT")]
        Rational6KT,
        [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY")]
        BrownConrady
    }
}
