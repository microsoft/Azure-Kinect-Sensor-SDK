using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Float3 : IEquatable<Float3>
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

        public override bool Equals(object obj)
        {
            return obj is Float3 ? this.Equals((Float3)obj) : false;
        }

        public bool Equals(Float3 obj)
        {
            return this.X == obj.X && this.Y == obj.Y && this.Z == obj.Z;
        }

        public override int GetHashCode()
        {
            return X.GetHashCode() * 7 ^ (Y.GetHashCode() * 7*7) ^ Z.GetHashCode();
        }

        public static bool operator ==(Float3 a, Float3 b)
        {
            return a.Equals(b);
        }
        public static bool operator !=(Float3 a, Float3 b)
        {
            return !a.Equals(b);
        }
    }
}
