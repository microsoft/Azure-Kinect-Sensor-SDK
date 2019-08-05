// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Intrinsics
    {
        public CalibrationModelType type;

        public int parameter_count;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 15)]
        public float[] parameters;
    }
}
