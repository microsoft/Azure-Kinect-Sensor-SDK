using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Sequential)]
    public class Float2
    {
        public Float2()
        {
            System.Diagnostics.Debug.Assert(Marshal.SizeOf(this) == 8);
        }

        public Float2(float X, float Y)
        {
            v[0] = X;
            v[1] = Y;
        }

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
        private float[] v = new float[2];


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

        public float[] V
        {
            get
            {
                return v;
            }
        }
    }
}
