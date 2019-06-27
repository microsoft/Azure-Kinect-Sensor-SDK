// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Short3
    {
        public Short3(Int16 X, Int16 Y, Int16 Z)
        {
            this.X = X;
            this.Y = Y;
            this.Z = Z;
        }
               
        public Int16 X { get; set; }
        public Int16 Y { get; set; }
        public Int16 Z { get; set; }

    }
}

