//------------------------------------------------------------------------------
// <copyright file="Extrinsics.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Extrinsic calibration data.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct Extrinsics : IEquatable<Extrinsics>
    {
        // Structure used for serialization to native SDK
#pragma warning disable CA1051 // Do not declare visible instance fields

        /// <summary>
        /// Gets 3x3 Rotation matrix stored in row major order.
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 9)]
        public float[] Rotation;

        /// <summary>
        /// Gets translation vector, x, y, z (in millimeters).
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        public float[] Translation;

#pragma warning restore CA1051 // Do not declare visible instance fields

        /// <summary>
        /// Compares two <see cref="Extrinsics"/> for equality.
        /// </summary>
        /// <param name="left">First extrinsic to compare.</param>
        /// <param name="right">Second extrinsic to compare.</param>
        /// <returns>True if equal.</returns>
        public static bool operator ==(Extrinsics left, Extrinsics right)
        {
            return left.Equals(right);
        }

        /// <summary>
        /// Compares two <see cref="Extrinsics"/> for inequality.
        /// </summary>
        /// <param name="left">First extrinsic to compare.</param>
        /// <param name="right">Second extrinsic to compare.</param>
        /// <returns>True if not equal.</returns>
        public static bool operator !=(Extrinsics left, Extrinsics right)
        {
            return !(left == right);
        }

        /// <inheritdoc/>
        public override bool Equals(object obj)
        {
            return obj is Extrinsics extrinsics && this.Equals(extrinsics);
        }

        /// <inheritdoc/>
        public bool Equals(Extrinsics other)
        {
            return EqualityComparer<float[]>.Default.Equals(this.Rotation, other.Rotation) &&
                   EqualityComparer<float[]>.Default.Equals(this.Translation, other.Translation);
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            int hashCode = 1370020195;
            hashCode = (hashCode * -1521134295) + EqualityComparer<float[]>.Default.GetHashCode(this.Rotation);
            hashCode = (hashCode * -1521134295) + EqualityComparer<float[]>.Default.GetHashCode(this.Translation);
            return hashCode;
        }
    }
}
