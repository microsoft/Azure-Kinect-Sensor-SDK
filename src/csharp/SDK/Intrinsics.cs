//------------------------------------------------------------------------------
// <copyright file="Intrinsics.cs" company="Microsoft">
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
    /// Camera sensor intrinsic calibration data.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct Intrinsics : IEquatable<Intrinsics>
    {
        // Structure used for serialization to native SDK
#pragma warning disable CA1051 // Do not declare visible instance fields

        /// <summary>
        /// Type of calibration model used.
        /// </summary>
        public CalibrationModelType Type;

        /// <summary>
        /// Number of valid entries in parameters.
        /// </summary>
        public int ParameterCount;

        /// <summary>
        /// Calibration parameters.
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 15)]
        public float[] Parameters;

#pragma warning restore CA1051 // Do not declare visible instance fields

        /// <summary>
        /// Compare two <see cref="Intrinsics"/> for equality.
        /// </summary>
        /// <param name="left">First intrinsic to compare.</param>
        /// <param name="right">Second intrinsic to compare.</param>
        /// <returns>True if equal.</returns>
        public static bool operator ==(Intrinsics left, Intrinsics right)
        {
            return left.Equals(right);
        }

        /// <summary>
        /// Compare two <see cref="Intrinsics"/> for inequality.
        /// </summary>
        /// <param name="left">First intrinsic to compare.</param>
        /// <param name="right">Second intrinsic to compare.</param>
        /// <returns>True if not equal.</returns>
        public static bool operator !=(Intrinsics left, Intrinsics right)
        {
            return !(left == right);
        }

        /// <inheritdoc/>
        public override bool Equals(object obj)
        {
            return obj is Intrinsics intrinsics && this.Equals(intrinsics);
        }

        /// <inheritdoc/>
        public bool Equals(Intrinsics other)
        {
            return this.Type == other.Type &&
                   this.ParameterCount == other.ParameterCount &&
                   EqualityComparer<float[]>.Default.Equals(this.Parameters, other.Parameters);
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            int hashCode = -426656627;
            hashCode = (hashCode * -1521134295) + this.Type.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.ParameterCount.GetHashCode();
            hashCode = (hashCode * -1521134295) + EqualityComparer<float[]>.Default.GetHashCode(this.Parameters);
            return hashCode;
        }
    }
}
