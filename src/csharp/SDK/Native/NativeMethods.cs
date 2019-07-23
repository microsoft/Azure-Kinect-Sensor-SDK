//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Azure.Kinect.Sensor.Native;

namespace Microsoft.Azure.Kinect.Sensor
{
#pragma warning disable IDE1006 // Naming Styles

    internal class NativeMethods
    {
        // These types are used internally by the interop dll for marshaling purposes and are not exposed
        // over the public surface of the managed dll.

        #region Handle Types

        public class k4a_device_t : Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid
        {
            private k4a_device_t() : base(true)
            {
            }

            protected override bool ReleaseHandle()
            {
                NativeMethods.k4a_device_close(handle);
                return true;
            }
        }

        public class k4a_capture_t : Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid
        {
            private k4a_capture_t() : base(true)
            {
            }

            public k4a_capture_t DuplicateReference()
            {
                k4a_capture_t duplicate = new k4a_capture_t();

                NativeMethods.k4a_capture_reference(handle);

                duplicate.handle = this.handle;
                return duplicate;

            }

            protected override bool ReleaseHandle()
            {
                NativeMethods.k4a_capture_release(handle);
                return true;
            }
        }

        public class k4a_image_t : Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid
        {
            private k4a_image_t() : base(true)
            {
            }

            public k4a_image_t DuplicateReference()
            {
                k4a_image_t duplicate = new k4a_image_t();

                NativeMethods.k4a_image_reference(handle);
                duplicate.handle = this.handle;
                return duplicate;
            }
            protected override bool ReleaseHandle()
            {
                NativeMethods.k4a_image_release(handle);
                return true;
            }
        }

        public class k4a_transformation_t : Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid
        {
            private k4a_transformation_t() : base(true)
            {
            }

            protected override bool ReleaseHandle()
            {
                NativeMethods.k4a_transformation_destroy(handle);
                return true;
            }
        }

        #endregion

        #region Enumerations
        [NativeReference]
        public enum k4a_buffer_result_t
        {
            K4A_BUFFER_RESULT_SUCCEEDED = 0,
            K4A_BUFFER_RESULT_FAILED,
            K4A_BUFFER_RESULT_TOO_SMALL
        }

        [NativeReference]
        public enum k4a_wait_result_t
        {
            K4A_WAIT_RESULT_SUCCEEDED = 0,
            K4A_WAIT_RESULT_FAILED,
            K4A_WAIT_RESULT_TIMEOUT
        }

        [NativeReference]
        public enum k4a_result_t
        {
            K4A_RESULT_SUCCEEDED = 0,
            K4A_RESULT_FAILED,
        }

        [NativeReference]
        public enum k4a_stream_result_t
        {
            K4A_STREAM_RESULT_SUCCEEDED = 0,
            K4A_STREAM_RESULT_FAILED,
            K4A_STREAM_RESULT_EOF
        }

        #endregion

        #region Structures

        [NativeReference]
        [StructLayout(LayoutKind.Sequential)]
        public struct k4a_version_t
        {
            public int major;
            public int minor;
            public int revision;

            public Version ToVersion()
            {
                return new Version(major, minor, revision);
            }
        }

        [NativeReference]
        [StructLayout(LayoutKind.Sequential)]
        public struct k4a_hardware_version_t
        {
            public k4a_version_t rgb;
            public k4a_version_t depth;
            public k4a_version_t audio;
            public k4a_version_t depth_sensor;
            public FirmwareBuild firmware_build;
            public FirmwareSignature firmware_signature;

            public HardwareVersion ToHardwareVersion()
            {
                return new HardwareVersion
                {
                    RGB = this.rgb.ToVersion(),
                    Depth = this.depth.ToVersion(),
                    Audio = this.audio.ToVersion(),
                    DepthSensor = this.depth_sensor.ToVersion(),
                    FirmwareBuild = this.firmware_build,
                    FirmwareSignature = this.firmware_signature
                };
            }
        }

        [NativeReference]
        [StructLayout(LayoutKind.Sequential)]
        public struct k4a_device_configuration_t
        {
            public ImageFormat color_format;
            public ColorResolution color_resolution;
            public DepthMode depth_mode;
            public FPS camera_fps;
            public bool synchronized_images_only;
            public int depth_delay_off_color_usec;
            public WiredSyncMode wired_sync_mode;
            public uint subordinate_delay_off_master_usec;
            public bool disable_streaming_indicator;
        }


        #endregion

        #region Functions

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_calibration_2d_to_2d(
            [In] ref Calibration calibration,
            ref Vector2 source_point2d,
            float source_depth,
            Calibration.DeviceType source_camera,
            Calibration.DeviceType target_camera,
            out Vector2 target_point2d,
            out bool valid);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_calibration_2d_to_3d(
            [In] ref Calibration calibration,
            ref Vector2 source_point2d,
            float source_depth,
            Calibration.DeviceType source_camera,
            Calibration.DeviceType target_camera,
            out Vector3 target_point3d,
            out bool valid);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_calibration_3d_to_2d(
            [In] ref Calibration calibration,
            ref Vector3 source_point3d,
            Calibration.DeviceType source_camera,
            Calibration.DeviceType target_camera,
            out Vector2 target_point2d,
            out bool valid);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_calibration_3d_to_3d(
            [In] ref Calibration calibration,
            ref Vector3 source_point3d,
            Calibration.DeviceType source_camera,
            Calibration.DeviceType target_camera,
            out Vector3 target_point3d);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_calibration_get_from_raw(
            byte[] raw_calibration,
            UIntPtr raw_calibration_size,
            DepthMode depth_mode,
            ColorResolution color_resolution,
            out Calibration calibration);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_transformation_t k4a_transformation_create([In] ref Calibration calibration);


        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_transformation_destroy(IntPtr transformation_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_transformation_depth_image_to_color_camera(
            k4a_transformation_t transformation_handle,
            k4a_image_t depth_image,
            k4a_image_t transformed_depth_image);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_transformation_color_image_to_depth_camera(
            k4a_transformation_t transformation_handle,
            k4a_image_t depth_image,
            k4a_image_t color_image,
            k4a_image_t transformed_color_image);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_transformation_depth_image_to_point_cloud(
                k4a_transformation_t transformation_handle,
                k4a_image_t depth_image,
                Calibration.DeviceType camera,
                k4a_image_t xyz_image);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_device_close(IntPtr device_handle);


        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_capture_create(out k4a_capture_t capture_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_image_t k4a_capture_get_color_image(k4a_capture_t capture_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_image_t k4a_capture_get_depth_image(k4a_capture_t capture_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_image_t k4a_capture_get_ir_image(k4a_capture_t capture_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern float k4a_capture_get_temperature_c(k4a_capture_t capture_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_capture_set_color_image(k4a_capture_t capture_handle, k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_capture_set_depth_image(k4a_capture_t capture_handle, k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_capture_set_ir_image(k4a_capture_t capture_handle, k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_capture_set_temperature_c(k4a_capture_t capture_handle, float temperature_c);



        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_capture_reference(IntPtr capture_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_capture_release(IntPtr capture_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_image_create(ImageFormat format,
            int width_pixels,
            int height_pixels,
            int stride_bytes,
            out k4a_image_t image_handle);

        public delegate void k4a_memory_destroy_cb_t(IntPtr buffer, IntPtr context);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_image_create_from_buffer(
            ImageFormat format,
            int width_pixels,
            int height_pixels,
            int stride_bytes,
            IntPtr buffer,
            UIntPtr buffer_size,
            k4a_memory_destroy_cb_t buffer_release_cb,
            IntPtr buffer_release_cb_context,
            out k4a_image_t image_handle
        );

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_image_reference(IntPtr image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_image_release(IntPtr image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern UInt32 k4a_device_get_installed_count();

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_device_get_calibration(
            k4a_device_t device_handle,
            DepthMode depth_mode,
            ColorResolution color_resolution,
            out Calibration calibration);


        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_wait_result_t k4a_device_get_capture(
            k4a_device_t device_handle,
            out k4a_capture_t capture_handle,
            Int32 timeout_in_ms);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_wait_result_t k4a_device_get_imu_sample(
            k4a_device_t device_handle,
            ImuSample imu_sample,
            Int32 timeout_in_ms);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_device_get_sync_jack(
            k4a_device_t device_handle,
            out bool sync_in_jack_connected,
            out bool sync_out_jack_connected);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_device_get_version(
            k4a_device_t device_handle,
            out k4a_hardware_version_t version);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_buffer_result_t k4a_device_get_raw_calibration(k4a_device_t device_handle, [Out] byte[] data, ref UIntPtr data_size);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_device_set_color_control(k4a_device_t device_handle, ColorControlCommand command, ColorControlMode mode, Int32 value);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_device_get_color_control(k4a_device_t device_handle, ColorControlCommand command, out ColorControlMode mode, out Int32 value);


        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_device_start_cameras(k4a_device_t device_handle, [In] ref k4a_device_configuration_t config);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_device_stop_cameras(k4a_device_t device_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_device_start_imu(k4a_device_t device_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_device_stop_imu(k4a_device_t device_handle);


        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_device_open(UInt32 index, out k4a_device_t device_handle);


        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        [NativeReference]
        public static extern k4a_buffer_result_t k4a_device_get_serialnum(k4a_device_t device_handle, StringBuilder serial_number, ref UIntPtr data_size);


        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern UInt64 k4a_image_get_exposure_usec(k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_image_set_exposure_time_usec(k4a_image_t image_handle, UInt64 value);


        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern ImageFormat k4a_image_get_format(k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern int k4a_image_get_height_pixels(k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern int k4a_image_get_width_pixels(k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern int k4a_image_get_stride_bytes(k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern UIntPtr k4a_image_get_size(k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern UInt32 k4a_image_get_iso_speed(k4a_image_t image_handle);


        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_image_set_iso_speed(k4a_image_t image_handle, UInt32 value);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern UInt32 k4a_image_get_white_balance(k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_image_set_white_balance(k4a_image_t image_handle, UInt32 value);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern UInt64 k4a_image_get_timestamp_usec(k4a_image_t image_handle);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern void k4a_image_set_timestamp_usec(k4a_image_t image_handle, UInt64 value);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern IntPtr k4a_image_get_buffer(k4a_image_t image_handle);

        public delegate void k4a_logging_message_cb_t(IntPtr context, LogLevel level, [MarshalAs(UnmanagedType.LPStr)] string file, int line, [MarshalAs(UnmanagedType.LPStr)] string message);

        [DllImport("k4a", CallingConvention = CallingConvention.Cdecl)]
        [NativeReference]
        public static extern k4a_result_t k4a_set_debug_message_handler(
            k4a_logging_message_cb_t message_cb,
            IntPtr message_cb_context,
            LogLevel min_level);

        #endregion

    }
#pragma warning restore IDE1006 // Naming Styles
}
