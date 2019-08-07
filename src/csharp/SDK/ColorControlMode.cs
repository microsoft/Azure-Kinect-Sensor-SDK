//------------------------------------------------------------------------------
// <copyright file="ColorControlMode.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Color sensor control mode.
    /// </summary>
    [Native.NativeReference("k4a_color_control_mode_t")]
    public enum ColorControlMode
    {
        /// <summary>
        /// Set the associated <see cref="ColorControlCommand"/> to Auto.
        /// </summary>
        Auto = 0,

        /// <summary>
        /// Set the associated <see cref="ColorControlCommand"/> to Manual.
        /// </summary>
        Manual,
    }
}
