//------------------------------------------------------------------------------
// <copyright file="FirmwareSignature.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Firmware signature type.
    /// </summary>
    [Native.NativeReference("k4a_firmware_signature_t")]
    public enum FirmwareSignature
    {
        /// <summary>
        /// Microsoft signed firmware.
        /// </summary>
        Msft,

        /// <summary>
        /// Test signed firmware.
        /// </summary>
        Test,

        /// <summary>
        /// Unsigned firmware.
        /// </summary>
        NotSigned,
    }
}
