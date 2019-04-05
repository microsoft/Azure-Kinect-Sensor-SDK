using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Float2
    {
        public Float2(float X, float Y)
        {
            this.X = X;
            this.Y = Y;
        }

        public float X { get; set; }
        public float Y { get; set; }
    }
}
