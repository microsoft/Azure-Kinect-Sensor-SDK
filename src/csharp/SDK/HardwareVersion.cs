//------------------------------------------------------------------------------
// <copyright file="HardwareVersion.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// The hardware version information.
    /// </summary>
    public class HardwareVersion
    {
        /// <summary>
        /// Gets or sets the color camera firmware version.
        /// </summary>
        public Version RGB { get; set; }

        /// <summary>
        /// Gets or sets the depth camera firmware version.
        /// </summary>
        public Version Depth { get; set; }

        /// <summary>
        /// Gets or sets the audio device firmware version.
        /// </summary>
        public Version Audio { get; set; }

        /// <summary>
        /// Gets or sets the depth sensor firmware version.
        /// </summary>
        public Version DepthSensor { get; set; }

        /// <summary>
        /// Gets or sets the build type.
        /// </summary>
        public FirmwareBuild FirmwareBuild { get; set; }

        /// <summary>
        /// Gets or sets the firmware signature.
        /// </summary>
        public FirmwareSignature FirmwareSignature { get; set; }
    }
}
