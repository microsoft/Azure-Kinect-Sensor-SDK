// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Microsoft.Azure.Kinect.Sensor
{
    [StructLayout(LayoutKind.Sequential)]
    [Native.NativeReference("k4a_calibration_t")]
    public struct Calibration
    {
        [MarshalAs(UnmanagedType.Struct)]
        public CameraCalibration depth_camera_calibration;

        [MarshalAs(UnmanagedType.Struct)]
        public CameraCalibration color_camera_calibration;

        [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.Struct,
            SizeConst = ((int)(CalibrationDeviceType.Num) * ((int)(CalibrationDeviceType.Num))))]
        public Extrinsics[] device_extrinsics;

        public DepthMode depth_mode;

        public ColorResolution color_resolution;

        
        public Vector2? TransformTo2D(Vector2 sourcePoint2D,
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

        public Transformation CreateTransformation()
        {
            return new Transformation(this);
        }
    }
}
