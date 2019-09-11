//------------------------------------------------------------------------------
// <copyright file="CalibrationModelType.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// The model used to interpret the calibration parameters.
    /// </summary>
    [Native.NativeReference("k4a_calibration_model_type_t")]
    public enum CalibrationModelType
    {
        /// <summary>
        /// Calibration model is unknown.
        /// </summary>
        [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_UNKNOWN")]
        Unknown = 0,

        /// <summary>
        /// Calibration model is Rational 6KT.
        /// </summary>
        [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT")]
        Rational6KT,

        /// <summary>
        /// Calibration model is Brown Conrady.
        /// </summary>
        [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY")]
        BrownConrady,
    }
}
