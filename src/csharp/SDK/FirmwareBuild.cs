// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
        /// Pre-production firmeare.
        /// </summary>
        Debug,
    }
}
