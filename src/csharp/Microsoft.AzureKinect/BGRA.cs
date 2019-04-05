using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    
    public struct BGRA
    {
        public BGRA(byte B, byte G, byte R, byte A = 0)
        {
            this.B = B;
            this.G = G;
            this.R = R;
            this.A = A;
        }

        public byte B;
        public byte G;
        public byte R;
        public byte A;
    }
}
