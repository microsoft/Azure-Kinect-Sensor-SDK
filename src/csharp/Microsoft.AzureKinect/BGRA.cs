using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Explicit)]
    public struct BGRA
    {
        public BGRA(byte B, byte G, byte R, byte A = 0)
        {
            this.Value = 0;
            this.B = B;
            this.G = G;
            this.R = R;
            this.A = A;
        }

        [FieldOffset(0)]
        public byte B;
        [FieldOffset(1)]
        public byte G;
        [FieldOffset(2)]
        public byte R;
        [FieldOffset(3)]
        public byte A;

        [FieldOffset(0)]
        public int Value;
    }
}
