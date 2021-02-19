//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Runtime.InteropServices;
using static Microsoft.Azure.Kinect.Sensor.NativeMethods;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Represents the configuration to run a color and/or depth device in.
    /// </summary>
    /// <remarks>
    /// Gets or sets the FPS Mode Info.
    /// </remarks>
    public class FPSModeInfo
    {
        /// <summary>
        /// Gets or sets the Struct Size.
        /// </summary>
        public int StructSize { get; set; } = 16;

        /// <summary>
        /// Gets or sets the Struct Version.
        /// </summary>
        public int StructVersion { get; set; } = 1;

        /// <summary>
        /// Gets or sets the ModeId.
        /// </summary>
        public int ModeId { get; set; } = 0;

        /// <summary>
        /// Gets or sets the FPS.
        /// </summary>
        public int FPS { get; set; } = 0;

        /// <summary>
        ///  Get the equivalent native configuration struct.
        /// </summary>
        /// <returns>k4a_fps_mode_info_t.</returns>
        internal k4a_fps_mode_info_t GetNativeConfiguration()
        {
            return new k4a_fps_mode_info_t
            {
                struct_size = (uint)this.StructSize,
                struct_version = (uint)this.StructVersion,
                mode_id = (uint)this.ModeId,
                fps = this.FPS,
            };
        }

        /// <summary>
        /// Set properties using native configuration struct.
        /// </summary>
        /// <param name="fpsModeInfo">fpsModeInfo.</param>
        internal void SetUsingNativeConfiguration(k4a_fps_mode_info_t fpsModeInfo)
        {
            this.StructSize = (int)fpsModeInfo.struct_size;
            this.StructVersion = (int)fpsModeInfo.struct_version;
            this.ModeId = (int)fpsModeInfo.mode_id;
            this.FPS = fpsModeInfo.fps;
        }
    }
}
