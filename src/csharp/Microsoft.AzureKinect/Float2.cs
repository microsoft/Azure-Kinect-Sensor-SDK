using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Float2 : IEquatable<Float2>
    {
        public Float2(float X, float Y)
        {
            this.X = X;
            this.Y = Y;
        }

        public float X { get; set; }
        public float Y { get; set; }

        public override bool Equals(object obj)
        {
            return obj is Float2 ? this.Equals((Float2)obj) : false;
        }

        public bool Equals(Float2 obj)
        {
            return this.X == obj.X && this.Y == obj.Y;
        }

        public override int GetHashCode()
        {
            return X.GetHashCode() * 7 ^ Y.GetHashCode();
        }

        public static bool operator==(Float2 a, Float2 b)
        {
            return a.Equals(b);
        }
        public static bool operator != (Float2 a, Float2 b)
        {
            return !a.Equals(b);
        }
    }
}
