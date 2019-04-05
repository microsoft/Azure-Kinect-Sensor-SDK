using System;
using System.Collections.Generic;
using System.Text;

using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Sequential)]
    public class Float3
    {
        public Float3()
        {
            System.Diagnostics.Debug.Assert(Marshal.SizeOf(this) == 12);
        }

        public Float3(float X, float Y, float Z)
        {
            v[0] = X;
            v[1] = Y;
            v[2] = Z;
        }

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        private readonly float[] v = new float[3];


        public float X
        {
            get
            {
                return v[0];
            }
            set
            {
                v[0] = value;
            }
        }

        public float Y
        {
            get
            {
                return v[1];
            }
            set
            {
                v[1] = value;
            }
        }

        public float Z
        {
            get
            {
                return v[2];
            }
            set
            {
                v[2] = value;
            }
        }

        public float[] V
        {
            get
            {
                return v;
            }
        }
    }
}
