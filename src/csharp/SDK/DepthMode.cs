//------------------------------------------------------------------------------
// <copyright file="DepthMode.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Depth sensor capture modes.
    /// </summary>
    /// <remarks>
    /// See the hardware specification for additional details on the field of view, and supported frame rates for each mode.
    /// NFOV and WFOV denote Narrow and Wide Field Of View configurations.
    /// Binned modes reduce the captured camera resolution by combining adjacent sensor pixels into a bin.
    /// </remarks>
    [Native.NativeReference("k4a_depth_mode_t")]
    public enum DepthMode
    {
#pragma warning disable CA1707 // Identifiers should not contain underscores
        /// <summary>
        /// Depth sensor will be turned off with this setting.
        /// </summary>
        Off = 0,

        /// <summary>
        /// Depth and Passive IR are captured at 320x288.
        /// </summary>
        NFOV_2x2Binned,

        /// <summary>
        /// Depth and Passive IR are captured at 640x576.
        /// </summary>
        NFOV_Unbinned,

        /// <summary>
        /// Depth and Passive IR are captured at 512x512.
        /// </summary>
        WFOV_2x2Binned,

        /// <summary>
        /// Depth and Passive IR are captured at 1024x1024.
        /// </summary>
        WFOV_Unbinned,

        /// <summary>
        /// Passive IR only is captured at 1024x1024.
        /// </summary>
        PassiveIR,
#pragma warning restore CA1707 // Identifiers should not contain underscores
    }
}
