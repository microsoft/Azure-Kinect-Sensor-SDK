using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Float3
    {
        public Float3(float X, float Y, float Z)
        {
            this.X = X;
            this.Y = Y;
            this.Z = Z;
        }

        public float X { get; set; }
        public float Y { get; set; }
        public float Z { get; set; }
    }
}
