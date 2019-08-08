//------------------------------------------------------------------------------
// <copyright file="ColorResolution.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Color sensor resolutions.
    /// </summary>
    [Native.NativeReference("k4a_color_resolution_t")]
    public enum ColorResolution
    {
        /// <summary>
        /// Color camera will be turned off with this setting.
        /// </summary>
        Off = 0,

        /// <summary>
        /// 1280 * 720 16:9
        /// </summary>
        R720p,

        /// <summary>
        /// 1920 * 1080 16:9
        /// </summary>
        R1080p,

        /// <summary>
        /// 2560 * 1440 16:9
        /// </summary>
        R1440p,

        /// <summary>
        /// 2048 * 1536 4:3
        /// </summary>
        R1536p,

        /// <summary>
        /// 3840 * 2160 16:9
        /// </summary>
        R2160p,

        /// <summary>
        /// 4096 * 3072 4:3
        /// </summary>
        R3072p,
    }
}
