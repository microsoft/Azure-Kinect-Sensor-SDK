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
    public class FPSModeInfo
    {
        public int StructSize { get; set; } = 16;

        public int StructVersion { get; set; } = 1;

        public int ModeId { get; set; } = 0;

        public int FPS { get; set; } = 0;

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
    }
}
