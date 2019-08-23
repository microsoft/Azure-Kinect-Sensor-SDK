//------------------------------------------------------------------------------
// <copyright file="Short3.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// A value representing a vector of 3 shorts.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct Short3 : IEquatable<Short3>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Short3"/> structure.
        /// </summary>
        /// <param name="x">X value of the vector.</param>
        /// <param name="y">Y value of the vector.</param>
        /// <param name="z">Z value of the vector.</param>
        public Short3(short x, short y, short z)
        {
            this.X = x;
            this.Y = y;
            this.Z = z;
        }

        /// <summary>
        /// Gets or sets the X component of the vector.
        /// </summary>
        public short X { get; set; }

        /// <summary>
        /// Gets or sets the Y component of the vector.
        /// </summary>
        public short Y { get; set; }

        /// <summary>
        /// Gets or sets the Z component of the vector.
        /// </summary>
        public short Z { get; set; }

        /// <summary>
        /// Compares two Short3 values for equality.
        /// </summary>
        /// <param name="left">The first Short3 to compare.</param>
        /// <param name="right">The second Shrot3 to compare.</param>
        /// <returns>True if the values are equal.</returns>
        public static bool operator ==(Short3 left, Short3 right) => left.Equals(right);

        /// <summary>
        /// Compares two Short3 values for inequality.
        /// </summary>
        /// <param name="left">The first Short3 to compare.</param>
        /// <param name="right">The second Shrot3 to compare.</param>
        /// <returns>True if the values are not equal.</returns>
        public static bool operator !=(Short3 left, Short3 right) => !(left == right);

        /// <inheritdoc/>
        public override bool Equals(object obj)
        {
            if (!(obj is Short3))
            {
                return false;
            }

            Short3 other = (Short3)obj;

            return this.Equals(other);
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            return (this.X << 1) + (this.Y << 3) + (this.Z << 5);
        }

        /// <inheritdoc/>
        public bool Equals(Short3 other)
        {
            return this.X == other.X &&
                this.Y == other.Y &&
                this.Z == other.Z;
        }
    }
}
