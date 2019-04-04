using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    [StructLayout(LayoutKind.Sequential)]
    [Native.NativeReference("k4a_calibration_t")]
    public struct Calibration
    {
        [MarshalAs(UnmanagedType.Struct)]
        public Calibration.Camera depth_camera_calibration;

        [MarshalAs(UnmanagedType.Struct)]
        public Calibration.Camera color_camera_calibration;

        [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.Struct,
            SizeConst = ((int)(Calibration.DeviceType.Num) * ((int)(Calibration.DeviceType.Num))))]
        public Calibration.Extrinsics[] extrinsics;

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
        
        public Float2 Transform_2d_to_2d(Float2 source_point2d, float source_depth, DeviceType source_camera, DeviceType target_camera)
        {
            Float2 target_point2d = null;
            bool valid = false;

            Exception.ThrowIfNotSuccess(NativeMethods.k4a_calibration_2d_to_2d(
                this,
                source_point2d,
                source_depth,
                source_camera,
                target_camera,
                out target_point2d,
                out valid));

            return valid ? target_point2d : null;
        }

        public Float3 Transform_2d_to_3d(Float2 source_point2d, float source_depth, DeviceType source_camera, DeviceType target_camera)
        {
            Float3 target_point3d = null;
            bool valid = false;

            Exception.ThrowIfNotSuccess(NativeMethods.k4a_calibration_2d_to_3d(
                this,
                source_point2d,
                source_depth,
                source_camera,
                target_camera,
                out target_point3d,
                out valid));

            return valid ? target_point3d : null;
        }

        public static Calibration GetFromRaw(byte[] raw, DepthMode depth_mode, ColorResolution color_resolution)
        {
            Calibration calibration;

            Exception.ThrowIfNotSuccess(NativeMethods.k4a_calibration_get_from_raw(
                raw,
                (UIntPtr)raw.Length,
                depth_mode,
                color_resolution,
                out calibration));
            
            return calibration;
        }
    }
}
