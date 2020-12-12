'''Tests for the k4a types and enums.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import unittest
import ctypes
    
import k4a
from k4a import k4atypes


def get_enum_values(n):
    value = 0
    while(value < n):
        yield value
        value = value + 1


class TestEnums(unittest.TestCase):
    '''Test enum instantiation and values to ensure they are not broken.
    '''

    @classmethod
    def setUpClass(cls):
        pass

    @classmethod
    def tearDownClass(cls):
        pass

    def test_k4a_result_t(self):
        enum_values = get_enum_values(len(k4a.k4a_result_t))
        self.assertEqual(k4a.k4a_result_t.K4A_RESULT_SUCCEEDED, next(enum_values))
        self.assertEqual(k4a.k4a_result_t.K4A_RESULT_FAILED, next(enum_values))

    def test_k4a_buffer_result_t(self):
        enum_values = get_enum_values(len(k4a.k4a_buffer_result_t))
        self.assertEqual(k4a.k4a_buffer_result_t.K4A_BUFFER_RESULT_SUCCEEDED, next(enum_values))
        self.assertEqual(k4a.k4a_buffer_result_t.K4A_BUFFER_RESULT_FAILED, next(enum_values))
        self.assertEqual(k4a.k4a_buffer_result_t.K4A_BUFFER_RESULT_TOO_SMALL, next(enum_values))

    def test_k4a_wait_result_t(self):
        enum_values = get_enum_values(len(k4a.k4a_wait_result_t))
        self.assertEqual(k4a.k4a_wait_result_t.K4A_WAIT_RESULT_SUCCEEDED, next(enum_values))
        self.assertEqual(k4a.k4a_wait_result_t.K4A_WAIT_RESULT_FAILED, next(enum_values))
        self.assertEqual(k4a.k4a_wait_result_t.K4A_WAIT_RESULT_TIMEOUT, next(enum_values))

    def test_k4a_log_level_t(self):
        enum_values = get_enum_values(len(k4a.k4a_log_level_t))
        self.assertEqual(k4a.k4a_log_level_t.K4A_LOG_LEVEL_CRITICAL, next(enum_values))
        self.assertEqual(k4a.k4a_log_level_t.K4A_LOG_LEVEL_ERROR, next(enum_values))
        self.assertEqual(k4a.k4a_log_level_t.K4A_LOG_LEVEL_WARNING, next(enum_values))
        self.assertEqual(k4a.k4a_log_level_t.K4A_LOG_LEVEL_INFO, next(enum_values))
        self.assertEqual(k4a.k4a_log_level_t.K4A_LOG_LEVEL_TRACE, next(enum_values))
        self.assertEqual(k4a.k4a_log_level_t.K4A_LOG_LEVEL_OFF, next(enum_values))

    def test_k4a_depth_mode_t(self):
        enum_values = get_enum_values(len(k4a.k4a_depth_mode_t))
        self.assertEqual(k4a.k4a_depth_mode_t.K4A_DEPTH_MODE_OFF, next(enum_values))
        self.assertEqual(k4a.k4a_depth_mode_t.K4A_DEPTH_MODE_NFOV_2X2BINNED, next(enum_values))
        self.assertEqual(k4a.k4a_depth_mode_t.K4A_DEPTH_MODE_NFOV_UNBINNED, next(enum_values))
        self.assertEqual(k4a.k4a_depth_mode_t.K4A_DEPTH_MODE_WFOV_2X2BINNED, next(enum_values))
        self.assertEqual(k4a.k4a_depth_mode_t.K4A_DEPTH_MODE_WFOV_UNBINNED, next(enum_values))
        self.assertEqual(k4a.k4a_depth_mode_t.K4A_DEPTH_MODE_PASSIVE_IR, next(enum_values))

    def test_k4a_color_resolution_t(self):
        enum_values = get_enum_values(len(k4a.k4a_color_resolution_t))
        self.assertEqual(k4a.k4a_color_resolution_t.K4A_COLOR_RESOLUTION_OFF, next(enum_values))
        self.assertEqual(k4a.k4a_color_resolution_t.K4A_COLOR_RESOLUTION_720P, next(enum_values))
        self.assertEqual(k4a.k4a_color_resolution_t.K4A_COLOR_RESOLUTION_1080P, next(enum_values))
        self.assertEqual(k4a.k4a_color_resolution_t.K4A_COLOR_RESOLUTION_1440P, next(enum_values))
        self.assertEqual(k4a.k4a_color_resolution_t.K4A_COLOR_RESOLUTION_1536P, next(enum_values))
        self.assertEqual(k4a.k4a_color_resolution_t.K4A_COLOR_RESOLUTION_2160P, next(enum_values))
        self.assertEqual(k4a.k4a_color_resolution_t.K4A_COLOR_RESOLUTION_3072P, next(enum_values))

    def test_k4a_image_format_t(self):
        enum_values = get_enum_values(len(k4a.k4a_image_format_t))
        self.assertEqual(k4a.k4a_image_format_t.K4A_IMAGE_FORMAT_COLOR_MJPG, next(enum_values))
        self.assertEqual(k4a.k4a_image_format_t.K4A_IMAGE_FORMAT_COLOR_NV12, next(enum_values))
        self.assertEqual(k4a.k4a_image_format_t.K4A_IMAGE_FORMAT_COLOR_YUY2, next(enum_values))
        self.assertEqual(k4a.k4a_image_format_t.K4A_IMAGE_FORMAT_COLOR_BGRA32, next(enum_values))
        self.assertEqual(k4a.k4a_image_format_t.K4A_IMAGE_FORMAT_DEPTH16, next(enum_values))
        self.assertEqual(k4a.k4a_image_format_t.K4A_IMAGE_FORMAT_IR16, next(enum_values))
        self.assertEqual(k4a.k4a_image_format_t.K4A_IMAGE_FORMAT_CUSTOM8, next(enum_values))
        self.assertEqual(k4a.k4a_image_format_t.K4A_IMAGE_FORMAT_CUSTOM16, next(enum_values))
        self.assertEqual(k4a.k4a_image_format_t.K4A_IMAGE_FORMAT_CUSTOM, next(enum_values))

    def test_k4a_transformation_interpolation_type_t(self):
        enum_values = get_enum_values(len(k4a.k4a_transformation_interpolation_type_t))
        self.assertEqual(k4a.k4a_transformation_interpolation_type_t.K4A_TRANSFORMATION_INTERPOLATION_TYPE_NEAREST, next(enum_values))
        self.assertEqual(k4a.k4a_transformation_interpolation_type_t.K4A_TRANSFORMATION_INTERPOLATION_TYPE_LINEAR, next(enum_values))

    def test_k4a_fps_t(self):
        enum_values = get_enum_values(len(k4a.k4a_fps_t))
        self.assertEqual(k4a.k4a_fps_t.K4A_FRAMES_PER_SECOND_5, next(enum_values))
        self.assertEqual(k4a.k4a_fps_t.K4A_FRAMES_PER_SECOND_15, next(enum_values))
        self.assertEqual(k4a.k4a_fps_t.K4A_FRAMES_PER_SECOND_30, next(enum_values))

    def test_k4a_color_control_command_t(self):
        enum_values = get_enum_values(len(k4a.k4a_color_control_command_t))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_BRIGHTNESS, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_CONTRAST, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_SATURATION, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_SHARPNESS, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_WHITEBALANCE, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_GAIN, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_command_t.K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, next(enum_values))

    def test_k4a_color_control_mode_t(self):
        enum_values = get_enum_values(len(k4a.k4a_color_control_mode_t))
        self.assertEqual(k4a.k4a_color_control_mode_t.K4A_COLOR_CONTROL_MODE_AUTO, next(enum_values))
        self.assertEqual(k4a.k4a_color_control_mode_t.K4A_COLOR_CONTROL_MODE_MANUAL, next(enum_values))

    def test_k4a_wired_sync_mode_t(self):
        enum_values = get_enum_values(len(k4a.k4a_wired_sync_mode_t))
        self.assertEqual(k4a.k4a_wired_sync_mode_t.K4A_WIRED_SYNC_MODE_STANDALONE, next(enum_values))
        self.assertEqual(k4a.k4a_wired_sync_mode_t.K4A_WIRED_SYNC_MODE_MASTER, next(enum_values))
        self.assertEqual(k4a.k4a_wired_sync_mode_t.K4A_WIRED_SYNC_MODE_SUBORDINATE, next(enum_values))

    def test_k4a_calibration_type_t(self):
        enum_values = get_enum_values(len(k4a.k4a_calibration_type_t))
        self.assertEqual(k4a.k4a_calibration_type_t.K4A_CALIBRATION_TYPE_UNKNOWN, next(enum_values))
        self.assertEqual(k4a.k4a_calibration_type_t.K4A_CALIBRATION_TYPE_DEPTH, next(enum_values))
        self.assertEqual(k4a.k4a_calibration_type_t.K4A_CALIBRATION_TYPE_COLOR, next(enum_values))
        self.assertEqual(k4a.k4a_calibration_type_t.K4A_CALIBRATION_TYPE_GYRO, next(enum_values))
        self.assertEqual(k4a.k4a_calibration_type_t.K4A_CALIBRATION_TYPE_ACCEL, next(enum_values))
        self.assertEqual(k4a.k4a_calibration_type_t.K4A_CALIBRATION_TYPE_NUM, next(enum_values))

    def test_k4a_calibration_model_type_t(self):
        enum_values = get_enum_values(len(k4a.k4a_calibration_model_type_t))
        self.assertEqual(k4a.k4a_calibration_model_type_t.K4A_CALIBRATION_LENS_DISTORTION_MODEL_UNKNOWN, next(enum_values))
        self.assertEqual(k4a.k4a_calibration_model_type_t.K4A_CALIBRATION_LENS_DISTORTION_MODEL_THETA, next(enum_values))
        self.assertEqual(k4a.k4a_calibration_model_type_t.K4A_CALIBRATION_LENS_DISTORTION_MODEL_POLYNOMIAL_3K, next(enum_values))
        self.assertEqual(k4a.k4a_calibration_model_type_t.K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT, next(enum_values))
        self.assertEqual(k4a.k4a_calibration_model_type_t.K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY, next(enum_values))

    def test_k4a_firmware_build_t(self):
        enum_values = get_enum_values(len(k4a.k4a_firmware_build_t))
        self.assertEqual(k4a.k4a_firmware_build_t.K4A_FIRMWARE_BUILD_RELEASE, next(enum_values))
        self.assertEqual(k4a.k4a_firmware_build_t.K4A_FIRMWARE_BUILD_DEBUG, next(enum_values))

    def test_k4a_firmware_signature_t(self):
        enum_values = get_enum_values(len(k4a.k4a_firmware_signature_t))
        self.assertEqual(k4a.k4a_firmware_signature_t.K4A_FIRMWARE_SIGNATURE_MSFT, next(enum_values))
        self.assertEqual(k4a.k4a_firmware_signature_t.K4A_FIRMWARE_SIGNATURE_TEST, next(enum_values))
        self.assertEqual(k4a.k4a_firmware_signature_t.K4A_FIRMWARE_SIGNATURE_UNSIGNED, next(enum_values))

    def test_K4A_SUCCEEDED_True(self):
        self.assertTrue(k4a.K4A_SUCCEEDED(k4a.k4a_result_t.K4A_RESULT_SUCCEEDED))

    def test_K4A_SUCCEEDED_False(self):
        self.assertFalse(k4a.K4A_SUCCEEDED(k4a.k4a_result_t.K4A_RESULT_FAILED))


class TestStructs(unittest.TestCase):
    '''Test struct instantiation and values to ensure they are not broken.
    '''

    @classmethod
    def setUpClass(cls):
        pass

    @classmethod
    def tearDownClass(cls):
        pass

    def test_k4a_device_t(self):
        device_handle = k4a.k4a_device_t()
        self.assertIsInstance(device_handle, ctypes.POINTER(k4atypes._handle_k4a_device_t))

    def test_k4a_capture_t(self):
        capture_handle = k4a.k4a_capture_t()
        self.assertIsInstance(capture_handle, ctypes.POINTER(k4atypes._handle_k4a_capture_t))

    def test_k4a_image_t(self):
        image_handle = k4a.k4a_image_t()
        self.assertIsInstance(image_handle, ctypes.POINTER(k4atypes._handle_k4a_image_t))

    def test_k4a_transformation_t(self):
        transformation_handle = k4a.k4a_transformation_t()
        self.assertIsInstance(transformation_handle, ctypes.POINTER(k4atypes._handle_k4a_transformation_t))

    def test_k4a_device_configuration_t(self):
        device_config = k4a.k4a_device_configuration_t()
        self.assertIsNotNone(device_config)
        self.assertEqual(len(device_config._fields_), 9)

    def test_k4a_calibration_extrinsics_t(self):
        calibration_extrinsics = k4a.k4a_calibration_extrinsics_t()
        self.assertIsNotNone(calibration_extrinsics)
        self.assertEqual(len(calibration_extrinsics._fields_), 2)

    def test__k4a_calibration_intrinsic_param(self):
        calib_intrinsic = k4atypes._k4a_calibration_intrinsic_param()
        self.assertIsNotNone(calib_intrinsic)
        self.assertEqual(len(calib_intrinsic._fields_), 15)

    def test_k4a_calibration_intrinsics_t(self):
        calib_intrinsic = k4a.k4a_calibration_intrinsics_t()
        self.assertIsNotNone(calib_intrinsic)
        self.assertEqual(len(calib_intrinsic._fields_), 3)

    def test_k4a_calibration_camera_t(self):
        camera_calibration = k4a.k4a_calibration_camera_t()
        self.assertIsNotNone(camera_calibration)
        self.assertEqual(len(camera_calibration._fields_), 5)

    def test_k4a_calibration_t(self):
        calibration = k4a.k4a_calibration_t()
        self.assertIsNotNone(calibration)
        self.assertEqual(len(calibration._fields_), 5)

    def test_k4a_version_t(self):
        version = k4a.k4a_version_t()
        self.assertIsNotNone(version)
        self.assertEqual(len(version._fields_), 3)

    def test_k4a_hardware_version_t(self):
        version = k4a.k4a_hardware_version_t()
        self.assertIsNotNone(version)
        self.assertEqual(len(version._fields_), 6)

    def test__k4a_xy(self):
        xy = k4atypes._k4a_xy()
        self.assertIsNotNone(xy)
        self.assertEqual(len(xy._fields_), 2)

    def test_k4a_float2_t(self):
        xy = k4atypes.k4a_float2_t()
        self.assertIsNotNone(xy)
        self.assertEqual(len(xy._fields_), 2)

    def test__k4a_xyz(self):
        xyz = k4atypes._k4a_xyz()
        self.assertIsNotNone(xyz)
        self.assertEqual(len(xyz._fields_), 3)

    def test_k4a_float3_t(self):
        xyz = k4atypes.k4a_float3_t()
        self.assertIsNotNone(xyz)
        self.assertEqual(len(xyz._fields_), 2)

    def test_k4a_imu_sample_t(self):
        imu = k4a.k4a_imu_sample_t()
        self.assertIsNotNone(imu)
        self.assertEqual(len(imu._fields_), 5)


if __name__ == '__main__':
    unittest.main()