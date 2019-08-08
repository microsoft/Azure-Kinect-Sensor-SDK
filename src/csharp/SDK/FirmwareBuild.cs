//------------------------------------------------------------------------------
// <copyright file="FirmwareBuild.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Firmware build type.
    /// </summary>
    [Native.NativeReference("k4a_firmware_build_t")]
    public enum FirmwareBuild
    {
        /// <summary>
        /// Production firmware.
        /// </summary>
        Release = 0,

        /// <summary>
        /// Preproduction firmware.
        /// </summary>
        Debug,
    }
}
