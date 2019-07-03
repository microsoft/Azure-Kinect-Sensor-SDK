// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    [StructLayout(LayoutKind.Explicit)]
    public struct BGRA : IEquatable<BGRA>
    {
        public BGRA(byte B, byte G, byte R, byte A = 0)
        {
            this.value = 0;
            this.b = B;
            this.g = G;
            this.r = R;
            this.a = A;
        }

        [FieldOffset(0)]
        private byte b;
        [FieldOffset(1)]
        private byte g;
        [FieldOffset(2)]
        private byte r;
        [FieldOffset(3)]
        private byte a;

        [FieldOffset(0)]
        private int value;

        public byte A { get => a; set => a = value; }
        public byte R { get => r; set => r = value; }
        public byte G { get => g; set => g = value; }
        public byte B { get => b; set => b = value; }
        public int Value { get => value; set => this.value = value; }

        public override bool Equals(object obj)
        {
            return (obj is BGRA) ? this.Equals((BGRA)obj) : false;
        }

        public bool Equals(BGRA other)
        {
            return other.Value == Value;
        }


        public static bool operator==(BGRA a, BGRA b)
        {
            return a.Value == b.Value;
        }

        public static bool operator !=(BGRA a, BGRA b)
        {
            return a.Value != b.Value;
        }

        public override int GetHashCode()
        {
            return Value;
        }


    }
}
