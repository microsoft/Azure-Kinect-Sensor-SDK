//------------------------------------------------------------------------------
// <copyright file="Calibration.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Device Calibration.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    [Native.NativeReference("k4a_calibration_t")]
    public struct Calibration : IEquatable<Calibration>
    {
        // Struct used for serialization to native SDK
#pragma warning disable CA1051 // Do not declare visible instance fields

        /// <summary>
        /// Depth camera calibration.
        /// </summary>
        [MarshalAs(UnmanagedType.Struct)]
        public CameraCalibration DepthCameraCalibration;

        /// <summary>
        /// Color camera calibration.
        /// </summary>
        [MarshalAs(UnmanagedType.Struct)]
        public CameraCalibration ColorCameraCalibration;

        /// <summary>
        /// Extrinsic transformation parameters.
        /// </summary>
        /// <remarks>
        /// The extrinsic parameters allow 3D coordinate conversions between depth camera, color camera, the IMU's gyroscope and accelerometer.
        ///
        /// To transform from a source to a target 3D coordinate system, use the parameters stored under DeviceExtrinsics[source][target].
        /// </remarks>
        [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.Struct, SizeConst = (int)CalibrationDeviceType.Num * ((int)CalibrationDeviceType.Num))]
        public Extrinsics[] DeviceExtrinsics;

        /// <summary>
        /// Depth camera mode for which calibration was obtained.
        /// </summary>
        public DepthMode DepthMode;

        /// <summary>
        /// Color camera resolution for which calibration was obtained.
        /// </summary>
        public ColorResolution ColorResolution;

#pragma warning restore CA1051 // Do not declare visible instance fields

        /// <summary>
        /// Compares two Calibrations.
        /// </summary>
        /// <param name="left">First Calibration to compare.</param>
        /// <param name="right">Second Calibration to compare.</param>
        /// <returns>True if equal.</returns>
        public static bool operator ==(Calibration left, Calibration right)
        {
            return left.Equals(right);
        }

        /// <summary>
        /// Compares two Calibrations.
        /// </summary>
        /// <param name="left">First Calibration to compare.</param>
        /// <param name="right">Second Calibration to compare.</param>
        /// <returns>True if not equal.</returns>
        public static bool operator !=(Calibration left, Calibration right)
        {
            return !(left == right);
        }

        /// <summary>
        /// Get the camera calibration for a device from a raw calibration blob.
        /// </summary>
        /// <param name="raw">Raw calibration blob obtained from a device or recording.</param>
        /// <param name="depthMode">Mode in which depth camera is operated.</param>
        /// <param name="colorResolution">Resolution in which the color camera is operated.</param>
        /// <returns>Calibration object.</returns>
        public static Calibration GetFromRaw(byte[] raw, DepthMode depthMode, ColorResolution colorResolution)
        {
            Calibration calibration = default;
            AzureKinectException.ThrowIfNotSuccess(() => NativeMethods.k4a_calibration_get_from_raw(
                raw,
                (UIntPtr)raw.Length,
                depthMode,
                colorResolution,
                out calibration));

            return calibration;
        }

        /// <summary>
        /// Transform a 2D pixel coordinate with an associated depth value of the source camera into a 2D pixel coordinate of the target camera.
        /// </summary>
        /// <param name="sourcePoint2D">The 2D pixel <paramref name="sourceCamera"/> coordinates.</param>
        /// <param name="sourceDepth">The depth of <paramref name="sourcePoint2D"/> in millimeters.</param>
        /// <param name="sourceCamera">The current camera.</param>
        /// <param name="targetCamera">The target camera.</param>
        /// <returns>The 2D pixel in <paramref name="targetCamera"/> coordinates, or null if the source point is not valid in the target coordinate system.</returns>
        public Vector2? TransformTo2D(
            Vector2 sourcePoint2D,
            float sourceDepth,
            CalibrationDeviceType sourceCamera,
            CalibrationDeviceType targetCamera)
        {
            using (LoggingTracer tracer = new LoggingTracer())
            {
                AzureKinectException.ThrowIfNotSuccess(tracer, NativeMethods.k4a_calibration_2d_to_2d(
                    ref this,
                    ref sourcePoint2D,
                    sourceDepth,
                    sourceCamera,
                    targetCamera,
                    out Vector2 target_point2d,
                    out bool valid));

                return valid ? (Vector2?)target_point2d : null;
            }
        }

        /// <summary>
        /// Transform a 2D pixel coordinate with an associated depth value of the source camera into a 3D point of the target coordinate system.
        /// </summary>
        /// <param name="sourcePoint2D">The 2D pixel in <paramref name="sourceCamera"/> coordinates.</param>
        /// <param name="sourceDepth">The depth of <paramref name="sourceCamera"/> in millimeters.</param>
        /// <param name="sourceCamera">The current camera.</param>
        /// <param name="targetCamera">The target camera.</param>
        /// <returns>The 3D coordinate of the input pixel in the coordinate system of <paramref name="targetCamera"/> in millimeters or null if the input point is not valid in that coordinate system.</returns>
        public Vector3? TransformTo3D(Vector2 sourcePoint2D, float sourceDepth, CalibrationDeviceType sourceCamera, CalibrationDeviceType targetCamera)
        {
            using (LoggingTracer tracer = new LoggingTracer())
            {
                AzureKinectException.ThrowIfNotSuccess(tracer, NativeMethods.k4a_calibration_2d_to_3d(
                ref this,
                ref sourcePoint2D,
                sourceDepth,
                sourceCamera,
                targetCamera,
                out Vector3 target_point3d,
                out bool valid));

                return valid ? (Vector3?)target_point3d : null;
            }
        }

        /// <summary>
        /// Transform a 3D point of a source coordinate system into a 2D pixel coordinate of the target camera.
        /// </summary>
        /// <param name="sourcePoint3D">The 3D coordinate in millimeters representing a point in <paramref name="sourceCamera"/> coordinate system.</param>
        /// <param name="sourceCamera">The current camera.</param>
        /// <param name="targetCamera">The target camera.</param>
        /// <returns>The 2D pixel coordinate in <paramref name="targetCamera"/> coordinates or null if the point is not valid in that coordinate system.</returns>
        public Vector2? TransformTo2D(Vector3 sourcePoint3D, CalibrationDeviceType sourceCamera, CalibrationDeviceType targetCamera)
        {
            using (LoggingTracer tracer = new LoggingTracer())
            {
                AzureKinectException.ThrowIfNotSuccess(tracer, NativeMethods.k4a_calibration_3d_to_2d(
                ref this,
                ref sourcePoint3D,
                sourceCamera,
                targetCamera,
                out Vector2 target_point2d,
                out bool valid));

                return valid ? (Vector2?)target_point2d : null;
            }
        }

        /// <summary>
        /// Transform a 3D point of a source coordinate system into a 3D point of the target coordinate system.
        /// </summary>
        /// <param name="sourcePoint3D">The 3D coordinates in millimeters representing a point in <paramref name="sourceCamera"/>.</param>
        /// <param name="sourceCamera">The current camera.</param>
        /// <param name="targetCamera">The target camera.</param>
        /// <returns>A point in 3D coordiantes of <paramref name="targetCamera"/> stored in millimeters.</returns>
        public Vector3? TransformTo3D(Vector3 sourcePoint3D, CalibrationDeviceType sourceCamera, CalibrationDeviceType targetCamera)
        {
            using (LoggingTracer tracer = new LoggingTracer())
            {
                AzureKinectException.ThrowIfNotSuccess(tracer, NativeMethods.k4a_calibration_3d_to_3d(
                ref this,
                ref sourcePoint3D,
                sourceCamera,
                targetCamera,
                out Vector3 target_point3d));

                return target_point3d;
            }
        }

        /// <summary>
        /// Transform a 2D pixel coordinate from color camera into a 2D pixel coordinate of the depth camera.
        /// </summary>
        /// <param name="sourcePoint2D">The 2D pixel color camera coordinates.</param>
        /// <param name="depth">The depth image.</param>
        /// <returns>The 2D pixel in depth camera coordinates, or null if the source point is not valid in the depth camera coordinate system.</returns>
        public Vector2? TransformColor2DToDepth2D(Vector2 sourcePoint2D, Image depth)
        {
            if (depth == null)
            {
                throw new ArgumentNullException(nameof(depth));
            }

            using (LoggingTracer tracer = new LoggingTracer())
            using (Image depthReference = depth.Reference())
            {
                AzureKinectException.ThrowIfNotSuccess(tracer, NativeMethods.k4a_calibration_color_2d_to_depth_2d(
                ref this,
                ref sourcePoint2D,
                depthReference.DangerousGetHandle(),
                out Vector2 target_point2d,
                out bool valid));

                return valid ? (Vector2?)target_point2d : null;
            }
        }

        /// <summary>
        /// Creates a Transformation object from this calibration.
        /// </summary>
        /// <returns>A new Transformation object.</returns>
        public Transformation CreateTransformation()
        {
            return new Transformation(this);
        }

        /// <inheritdoc/>
        public override bool Equals(object obj)
        {
            return obj is Calibration calibration && this.Equals(calibration);
        }

        /// <inheritdoc/>
        public bool Equals(Calibration other)
        {
            return this.DepthCameraCalibration.Equals(other.DepthCameraCalibration) &&
                   this.ColorCameraCalibration.Equals(other.ColorCameraCalibration) &&
                   EqualityComparer<Extrinsics[]>.Default.Equals(this.DeviceExtrinsics, other.DeviceExtrinsics) &&
                   this.DepthMode == other.DepthMode &&
                   this.ColorResolution == other.ColorResolution;
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            int hashCode = -454809512;
            hashCode = (hashCode * -1521134295) + EqualityComparer<CameraCalibration>.Default.GetHashCode(this.DepthCameraCalibration);
            hashCode = (hashCode * -1521134295) + EqualityComparer<CameraCalibration>.Default.GetHashCode(this.ColorCameraCalibration);
            hashCode = (hashCode * -1521134295) + EqualityComparer<Extrinsics[]>.Default.GetHashCode(this.DeviceExtrinsics);
            hashCode = (hashCode * -1521134295) + this.DepthMode.GetHashCode();
            hashCode = (hashCode * -1521134295) + this.ColorResolution.GetHashCode();
            return hashCode;
        }
    }
}
