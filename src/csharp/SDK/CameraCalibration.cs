//------------------------------------------------------------------------------
// <copyright file="CameraCalibration.cs" company="Microsoft">
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
    /// Camera calibration contains intrinsic and extrinsic calibration information for a camera.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct CameraCalibration : IEquatable<CameraCalibration>
    {
        // Structure used for serialization to native SDK
#pragma warning disable CA1051 // Do not declare visible instance fields

        /// <summary>
        /// Extrinsic calibration data.
        /// </summary>
        [MarshalAs(UnmanagedType.Struct)]
        public Extrinsics Extrinsics;

        /// <summary>
        /// Intrinsic calibration data.
        /// </summary>
        [MarshalAs(UnmanagedType.Struct)]
        public Intrinsics Intrinsics;

        /// <summary>
        /// Resolution width of the camera sensor.
        /// </summary>
        public int ResolutionWidth;

        /// <summary>
        /// Resolution height of the camera sensor.
        /// </summary>
        public int ResolutionHeight;

        /// <summary>
        /// Maximum FOV of the camera.
        /// </summary>
        public float MetricRadius;

#pragma warning restore CA1051 // Do not declare visible instance fields

        /// <summary>
        /// Compare two CameraCalibrations for equality.
        /// </summary>
        /// <param name="left">First CameraCalibration to compare.</param>
        /// <param name="right">Second CameraCalibration to compare.</param>
        /// <returns>True if equal.</returns>
        public static bool operator ==(CameraCalibration left, CameraCalibration right)
        {
            return left.Equals(right);
        }

        /// <summary>
        /// Compare two CameraCalibrations for inequality.
        /// </summary>
        /// <param name="left">First CameraCalibration to compare.</param>
        /// <param name="right">Second CameraCalibration to compare.</param>
        /// <returns>True if not equal.</returns>
        public static bool operator !=(CameraCalibration left, CameraCalibration right)
        {
            return !(left == right);
        }

        /// <inheritdoc/>
        public override bool Equals(object obj)
        {
            return obj is CameraCalibration calibration && this.Equals(calibration);
        }

        /// <inheritdoc/>
        public bool Equals(CameraCalibration other)
        {
            return this.Extrinsics.Equals(other.Extrinsics) &&
                   this.Intrinsics.Equals(other.Intrinsics) &&
                   this.ResolutionWidth == other.ResolutionWidth &&
                   this.ResolutionHeight == other.ResolutionHeight &&
                   this.MetricRadius == other.MetricRadius;
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            int hashCode = 2125552744;
            hashCode = (hashCode * -1521134295) + EqualityComparer<Extrinsics>.Default.GetHashCode(this.Extrinsics);
            hashCode = (hashCode * -1521134295) + EqualityComparer<Intrinsics>.Default.GetHashCode(this.Intrinsics);
            hashCode = (hashCode * -1521134295) + this.ResolutionWidth.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.ResolutionHeight.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.MetricRadius.GetHashCode();
            return hashCode;
        }
    }
}
