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
        public Camera depth_camera_calibration;

        [MarshalAs(UnmanagedType.Struct)]
        public Camera color_camera_calibration;

        [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.Struct,
            SizeConst = ((int)(Calibration.DeviceType.Num) * ((int)(Calibration.DeviceType.Num))))]
        public Extrinsics[] device_extrinsics;

        public DepthMode depth_mode;

        public ColorResolution color_resolution;
        
        [StructLayout(LayoutKind.Sequential)]
        public struct Camera
        {
            [MarshalAs(UnmanagedType.Struct)]
            public Extrinsics extrinsics;

            [MarshalAs(UnmanagedType.Struct)]
            public Intrinsics intrinsics;

            public int resolution_width;
            public int resolution_height;
            public float metric_radius;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Extrinsics
        {
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 9)]
            public float[] rotation;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] translation;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct Intrinsics
        {
            public ModelType type;

            public int parameter_count;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 15)]
            public float[] parameters;
        }

        [Native.NativeReference("k4a_calibration_model_type_t")]
        public enum ModelType
        {
            [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_UNKNOWN")]
            Unknown = 0,
            [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_THETA")]
            Theta,
            [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_POLYNOMIAL_3K")]
            Polynomial3K,
            [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT")]
            Rational6KT,
            [Native.NativeReference("K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY")]
            BrownConrady
        }

        [Native.NativeReference("k4a_calibration_type_t")]
        public enum DeviceType
        {
            [Native.NativeReference("K4A_CALIBRATION_TYPE_UNKNOWN")]
            Unknown = -1,
            [Native.NativeReference("K4A_CALIBRATION_TYPE_DEPTH")]
            Depth,
            [Native.NativeReference("K4A_CALIBRATION_TYPE_COLOR")]
            Color,
            [Native.NativeReference("K4A_CALIBRATION_TYPE_GYRO")]
            Gyro,
            [Native.NativeReference("K4A_CALIBRATION_TYPE_ACCEL")]
            Accel,
            [Native.NativeReference("K4A_CALIBRATION_TYPE_NUM")]
            Num
        }
        
        public Vector2? TransformTo2D(Vector2 sourcePoint2D,
                                     float sourceDepth,
                                     DeviceType sourceCamera,
                                     DeviceType targetCamera)
        {
            AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_calibration_2d_to_2d(
                ref this,
                ref sourcePoint2D,
                sourceDepth,
                sourceCamera,
                targetCamera,
                out Vector2 target_point2d,
                out bool valid));
            if (valid)
                return target_point2d;
            return null;
        }

        public Vector3? TransformTo3D(Vector2 sourcePoint2D, float sourceDepth, DeviceType sourceCamera, DeviceType targetCamera)
        {
            AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_calibration_2d_to_3d(
                ref this,
                ref sourcePoint2D,
                sourceDepth,
                sourceCamera,
                targetCamera,
                out Vector3 target_point3d,
                out bool valid));

            if (valid)
                return target_point3d;
            return null;
        }

        public Vector2? TransformTo2D(Vector3 sourcePoint3D, DeviceType sourceCamera, DeviceType targetCamera)
        {
            AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_calibration_3d_to_2d(
                ref this,
                ref sourcePoint3D,
                sourceCamera,
                targetCamera,
                out Vector2 target_point2d,
                out bool valid));

            if (valid)
                return target_point2d;
            return null;
        }

        public Vector3? TransformTo3D(Vector3 sourcePoint3D, DeviceType sourceCamera, DeviceType targetCamera)
        {
            AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_calibration_3d_to_3d(
                ref this,
                ref sourcePoint3D,
                sourceCamera,
                targetCamera,
                out Vector3 target_point3d));

            return target_point3d;
        }

        public static Calibration GetFromRaw(byte[] raw, DepthMode depthMode, ColorResolution colorResolution)
        {
            AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_calibration_get_from_raw(
                raw,
                (UIntPtr)raw.Length,
                depthMode,
                colorResolution,
                out Calibration calibration));

            return calibration;
        }

        public Transformation CreateTransformation()
        {
            return new Transformation(this);
        }
    }
}
