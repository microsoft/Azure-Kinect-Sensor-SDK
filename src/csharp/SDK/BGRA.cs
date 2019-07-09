// <copyright file="BGRA.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
using System;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Describes a pixel of color in terms of blue, green, red, and alpha channels.
    /// </summary>
    [StructLayout(LayoutKind.Explicit)]
    public struct BGRA : IEquatable<BGRA>
    {
        [FieldOffset(0)]
        private byte blue;
        [FieldOffset(1)]
        private byte green;
        [FieldOffset(2)]
        private byte red;
        [FieldOffset(3)]
        private byte alpha;

        [FieldOffset(0)]
        private int value;

        /// <summary>
        /// Initializes a new instance of the <see cref="BGRA"/> structure.
        /// </summary>
        /// <param name="blue">The blue channel value of the color.</param>
        /// <param name="green">The green channel value of the color.</param>
        /// <param name="red">The red channel value of the color.</param>
        /// <param name="alpha">The alpha channel value of the color.</param>
        public BGRA(byte blue, byte green, byte red, byte alpha = 0)
        {
            this.value = 0;
            this.blue = blue;
            this.green = green;
            this.red = red;
            this.alpha = alpha;
        }

        /// <summary>
        /// Gets or sets the BGRA alpha channel value of the color.
        /// </summary>
        public byte A { get => this.alpha; set => this.alpha = value; }

        /// <summary>
        /// Gets or sets the BGRA red channel value of the color.
        /// </summary>
        public byte R { get => this.red; set => this.red = value; }

        /// <summary>
        /// Gets or sets the BGRA green channel value of the color.
        /// </summary>
        public byte G { get => this.green; set => this.green = value; }

        /// <summary>
        /// Gets or sets the BGRA blue channel value of the color.
        /// </summary>
        public byte B { get => this.blue; set => this.blue = value; }

        /// <summary>
        /// Gets or sets the combined BGRA value of the color.
        /// </summary>
        public int Value { get => this.value; set => this.value = value; }

        /// <summary>
        /// Tests whether two <see cref="BGRA"/> structures are identical.
        /// </summary>
        /// <param name="bgra1">The first <see cref="BGRA"/> structure to compare.</param>
        /// <param name="bgra2">The second <see cref="BGRA"/> structure to compare.</param>
        /// <returns><c>true</c> if <paramref name="bgra1"/> and <paramref name="bgra2"/> are exactly identical; otherwise, <c>flase</c>.</returns>
        public static bool operator ==(BGRA bgra1, BGRA bgra2)
        {
            return bgra1.Value == bgra2.Value;
        }

        /// <summary>
        /// Tests whether two <see cref="BGRA"/> structures are not identical.
        /// </summary>
        /// <param name="bgra1">The first <see cref="BGRA"/> structure to compare.</param>
        /// <param name="bgra2">The second <see cref="BGRA"/> structure to compare.</param>
        /// <returns><c>true</c> if <paramref name="bgra1"/> and <paramref name="bgra2"/> are note equal; otherwise, <c>flase</c>.</returns>
        public static bool operator !=(BGRA bgra1, BGRA bgra2)
        {
            return bgra1.Value != bgra2.Value;
        }

        /// <summary>
        /// Tests whether the specified object is a <see cref="BGRA"/> structure and is equivalent to this <see cref="BGRA"/>.
        /// </summary>
        /// <param name="obj">The object to compare to this <see cref="BGRA"/> structure.</param>
        /// <returns><c>true</c> if the specified object is a <see cref="BGRA"/> structure and is identical to the current <see cref="BGRA"/> structure; otherwise, <c>flase</c>.</returns>
        public override bool Equals(object obj)
        {
            return (obj is BGRA) ? this.Equals((BGRA)obj) : false;
        }

        /// <summary>
        /// Tests whether the specified <see cref="BGRA"/> structure is equivalent to this <see cref="BGRA"/>.
        /// </summary>
        /// <param name="other">The <see cref="BGRA"/> structure to compare to the current <see cref="BGRA"/> structure.</param>
        /// <returns><c>true</c> if the specified <see cref="BGRA"/> structure is identical to the current <see cref="BGRA"/> structure; otherwise, <c>flase</c>.</returns>
        public bool Equals(BGRA other)
        {
            return other.Value == this.Value;
        }

        /// <summary>
        /// Gets a hash code for this <see cref="BGRA"/> structure.
        /// </summary>
        /// <returns>A hash code for this <see cref="BGRA"/> structure.</returns>
        public override int GetHashCode()
        {
            return this.Value;
        }
    }
}
