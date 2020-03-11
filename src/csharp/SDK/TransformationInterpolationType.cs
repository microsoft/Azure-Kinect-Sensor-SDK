//------------------------------------------------------------------------------
// <copyright file="TransformationInterpolationType.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Specifies a type of transformation interpolation.
    /// </summary>
    [Native.NativeReference("k4a_transformation_interpolation_type_t")]
    public enum TransformationInterpolationType
    {
        /// <summary>
        /// Nearest neighbor interpolation.
        /// </summary>
        [Native.NativeReference("K4A_TRANSFORMATION_INTERPOLATION_TYPE_NEAREST")]
        Nearest = 0,

        /// <summary>
        /// Linear interpolation.
        /// </summary>
        [Native.NativeReference("K4A_TRANSFORMATION_INTERPOLATION_TYPE_LINEAR")]
        Linear,
    }
}
