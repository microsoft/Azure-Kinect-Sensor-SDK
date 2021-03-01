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
        /// Gets the Struct Size. Default set in the NativeMethods class and must be kept in sync with k4a_fps_mode_info_t k4atypes.h.
        /// </summary>
        public int StructSize { get; private set; } = fpsModeInfoStructSize;

        /// <summary>
        /// Gets the Struct Version.
        /// </summary>
        public int StructVersion { get; private set; } = 1;

        /// <summary>
        /// Gets the ModeId.
        /// </summary>
        public int ModeId { get; private set; } = 0;

        /// <summary>
        /// Gets the FPS.
        /// </summary>
        public int FPS { get; private set; } = 0;

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
                fps = (uint)this.FPS,
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
            this.FPS = (int)fpsModeInfo.fps;
        }
    }
}
