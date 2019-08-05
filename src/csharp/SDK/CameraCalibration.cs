// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    [StructLayout(LayoutKind.Sequential)]
    public struct CameraCalibration
    {
        [MarshalAs(UnmanagedType.Struct)]
        public Extrinsics extrinsics;

        [MarshalAs(UnmanagedType.Struct)]
        public Intrinsics intrinsics;

        public int resolution_width;
        public int resolution_height;
        public float metric_radius;
    }
}
