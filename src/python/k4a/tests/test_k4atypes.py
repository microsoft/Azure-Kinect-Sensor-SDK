'''
test_k4atypes.py

Tests for the k4a types and enums.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import unittest
import ctypes
    
import k4a


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

    def test_Status(self):
        enum_values = get_enum_values(len(k4a.EStatus))
        self.assertEqual(k4a.EStatus.SUCCEEDED, next(enum_values))
        self.assertEqual(k4a.EStatus.FAILED, next(enum_values))

    def test_k4a_buffer_result_t(self):
        enum_values = get_enum_values(len(k4a.EBufferStatus))
        self.assertEqual(k4a.EBufferStatus.SUCCEEDED, next(enum_values))
        self.assertEqual(k4a.EBufferStatus.FAILED, next(enum_values))
        self.assertEqual(k4a.EBufferStatus.BUFFER_TOO_SMALL, next(enum_values))

    def test_k4a_wait_result_t(self):
        enum_values = get_enum_values(len(k4a.EWaitStatus))
        self.assertEqual(k4a.EWaitStatus.SUCCEEDED, next(enum_values))
        self.assertEqual(k4a.EWaitStatus.FAILED, next(enum_values))
        self.assertEqual(k4a.EWaitStatus.TIMEOUT, next(enum_values))

    def test_k4a_log_level_t(self):
        enum_values = get_enum_values(len(k4a.ELogLevel))
        self.assertEqual(k4a.ELogLevel.CRITICAL, next(enum_values))
        self.assertEqual(k4a.ELogLevel.ERROR, next(enum_values))
        self.assertEqual(k4a.ELogLevel.WARNING, next(enum_values))
        self.assertEqual(k4a.ELogLevel.INFO, next(enum_values))
        self.assertEqual(k4a.ELogLevel.TRACE, next(enum_values))
        self.assertEqual(k4a.ELogLevel.OFF, next(enum_values))

    def test_k4a_depth_mode_t(self):
        enum_values = get_enum_values(len(k4a.EDepthMode))
        self.assertEqual(k4a.EDepthMode.OFF, next(enum_values))
        self.assertEqual(k4a.EDepthMode.NFOV_2X2BINNED, next(enum_values))
        self.assertEqual(k4a.EDepthMode.NFOV_UNBINNED, next(enum_values))
        self.assertEqual(k4a.EDepthMode.WFOV_2X2BINNED, next(enum_values))
        self.assertEqual(k4a.EDepthMode.WFOV_UNBINNED, next(enum_values))
        self.assertEqual(k4a.EDepthMode.PASSIVE_IR, next(enum_values))

    def test_k4a_color_resolution_t(self):
        enum_values = get_enum_values(len(k4a.EColorResolution))
        self.assertEqual(k4a.EColorResolution.OFF, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_720P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_1080P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_1440P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_1536P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_2160P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_3072P, next(enum_values))

    def test_k4a_image_format_t(self):
        enum_values = get_enum_values(len(k4a.EImageFormat))
        self.assertEqual(k4a.EImageFormat.COLOR_MJPG, next(enum_values))
        self.assertEqual(k4a.EImageFormat.COLOR_NV12, next(enum_values))
        self.assertEqual(k4a.EImageFormat.COLOR_YUY2, next(enum_values))
        self.assertEqual(k4a.EImageFormat.COLOR_BGRA32, next(enum_values))
        self.assertEqual(k4a.EImageFormat.DEPTH16, next(enum_values))
        self.assertEqual(k4a.EImageFormat.IR16, next(enum_values))
        self.assertEqual(k4a.EImageFormat.CUSTOM8, next(enum_values))
        self.assertEqual(k4a.EImageFormat.CUSTOM16, next(enum_values))
        self.assertEqual(k4a.EImageFormat.CUSTOM, next(enum_values))

    def test_k4a_transformation_interpolation_type_t(self):
        enum_values = get_enum_values(len(k4a.ETransformInterpolationType))
        self.assertEqual(k4a.ETransformInterpolationType.NEAREST, next(enum_values))
        self.assertEqual(k4a.ETransformInterpolationType.LINEAR, next(enum_values))

    def test_k4a_fps_t(self):
        enum_values = get_enum_values(len(k4a.EFramePerSecond))
        self.assertEqual(k4a.EFramePerSecond.FPS_5, next(enum_values))
        self.assertEqual(k4a.EFramePerSecond.FPS_15, next(enum_values))
        self.assertEqual(k4a.EFramePerSecond.FPS_30, next(enum_values))

    def test_k4a_color_control_command_t(self):
        enum_values = get_enum_values(len(k4a.EColorControlCommand))
        self.assertEqual(k4a.EColorControlCommand.EXPOSURE_TIME_ABSOLUTE, next(enum_values))
        self.assertEqual(k4a.EColorControlCommand.AUTO_EXPOSURE_PRIORITY, next(enum_values))
        self.assertEqual(k4a.EColorControlCommand.BRIGHTNESS, next(enum_values))
        self.assertEqual(k4a.EColorControlCommand.CONTRAST, next(enum_values))
        self.assertEqual(k4a.EColorControlCommand.SATURATION, next(enum_values))
        self.assertEqual(k4a.EColorControlCommand.SHARPNESS, next(enum_values))
        self.assertEqual(k4a.EColorControlCommand.WHITEBALANCE, next(enum_values))
        self.assertEqual(k4a.EColorControlCommand.BACKLIGHT_COMPENSATION, next(enum_values))
        self.assertEqual(k4a.EColorControlCommand.GAIN, next(enum_values))
        self.assertEqual(k4a.EColorControlCommand.POWERLINE_FREQUENCY, next(enum_values))

    def test_k4a_color_control_mode_t(self):
        enum_values = get_enum_values(len(k4a.EColorControlMode))
        self.assertEqual(k4a.EColorControlMode.AUTO, next(enum_values))
        self.assertEqual(k4a.EColorControlMode.MANUAL, next(enum_values))

    def test_k4a_wired_sync_mode_t(self):
        enum_values = get_enum_values(len(k4a.EWiredSyncMode))
        self.assertEqual(k4a.EWiredSyncMode.STANDALONE, next(enum_values))
        self.assertEqual(k4a.EWiredSyncMode.MASTER, next(enum_values))
        self.assertEqual(k4a.EWiredSyncMode.SUBORDINATE, next(enum_values))

    def test_k4a_calibration_type_t(self):
        enum_values = get_enum_values(len(k4a.ECalibrationType))
        self.assertEqual(k4a.ECalibrationType.UNKNOWN, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.DEPTH, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.COLOR, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.GYRO, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.ACCEL, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.NUM_TYPES, next(enum_values))

    def test_k4a_calibration_model_type_t(self):
        enum_values = get_enum_values(len(k4a.ECalibrationModelType))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_UNKNOWN, next(enum_values))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_THETA, next(enum_values))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_POLYNOMIAL_3K, next(enum_values))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_RATIONAL_6KT, next(enum_values))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_BROWN_CONRADY, next(enum_values))

    def test_k4a_firmware_build_t(self):
        enum_values = get_enum_values(len(k4a.EFirmwareBuild))
        self.assertEqual(k4a.EFirmwareBuild.RELEASE, next(enum_values))
        self.assertEqual(k4a.EFirmwareBuild.DEBUG, next(enum_values))

    def test_k4a_firmware_signature_t(self):
        enum_values = get_enum_values(len(k4a.EFirmwareSignature))
        self.assertEqual(k4a.EFirmwareSignature.MSFT, next(enum_values))
        self.assertEqual(k4a.EFirmwareSignature.TEST, next(enum_values))
        self.assertEqual(k4a.EFirmwareSignature.UNSIGNED, next(enum_values))

    def test_K4A_SUCCEEDED_True(self):
        self.assertTrue(k4a.K4A_SUCCEEDED(k4a.EStatus.SUCCEEDED))

    def test_K4A_SUCCEEDED_False(self):
        self.assertFalse(k4a.K4A_SUCCEEDED(k4a.EStatus.FAILED))


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
        device_handle = k4a._bindings._k4a._DeviceHandle()
        self.assertIsInstance(device_handle, k4a._bindings._k4a._DeviceHandle)

    def test_k4a_capture_t(self):
        capture_handle = k4a._bindings._k4a._CaptureHandle()
        self.assertIsInstance(capture_handle, k4a._bindings._k4a._CaptureHandle)

    def test_k4a_image_t(self):
        image_handle = k4a._bindings._k4a._ImageHandle()
        self.assertIsInstance(image_handle, k4a._bindings._k4a._ImageHandle)

    def test_k4a_transformation_t(self):
        transformation_handle = k4a._bindings._k4a._TransformationHandle()
        self.assertIsInstance(transformation_handle, k4a._bindings._k4a._TransformationHandle)

    def test_k4a_device_configuration_t(self):
        device_config = k4a.DeviceConfiguration()
        self.assertIsNotNone(device_config)
        self.assertEqual(len(device_config._fields_), 9)

    def test_k4a_calibration_extrinsics_t(self):
        calibration_extrinsics = k4a.CalibrationExtrinsics()
        self.assertIsNotNone(calibration_extrinsics)
        self.assertEqual(len(calibration_extrinsics._fields_), 2)

    def test__k4a_calibration_intrinsic_param(self):
        calib_intrinsic = k4a.CalibrationIntrinsicParam()
        self.assertIsNotNone(calib_intrinsic)
        self.assertEqual(len(calib_intrinsic._fields_), 15)

    def test_k4a_calibration_intrinsics_t(self):
        calib_intrinsic = k4a.CalibrationIntrinsics()
        self.assertIsNotNone(calib_intrinsic)
        self.assertEqual(len(calib_intrinsic._fields_), 3)

    def test_k4a_calibration_camera_t(self):
        camera_calibration = k4a.CalibrationCamera()
        self.assertIsNotNone(camera_calibration)
        self.assertEqual(len(camera_calibration._fields_), 5)

    def test_k4a_calibration_t(self):
        calibration = k4a.Calibration()
        self.assertIsNotNone(calibration)
        self.assertEqual(len(calibration._fields_), 5)

    def test_k4a_version_t(self):
        version = k4a.Version()
        self.assertIsNotNone(version)
        self.assertEqual(len(version._fields_), 3)

    def test_k4a_hardware_version_t(self):
        version = k4a.HardwareVersion()
        self.assertIsNotNone(version)
        self.assertEqual(len(version._fields_), 6)

    def test__k4a_xy(self):
        xy = k4a._bindings._k4atypes._XY()
        self.assertIsNotNone(xy)
        self.assertEqual(len(xy._fields_), 2)

    def test_k4a_float2_t(self):
        xy = k4a.Float2()
        self.assertIsNotNone(xy)
        self.assertEqual(len(xy._fields_), 2)

    def test__k4a_xyz(self):
        xyz = k4a._bindings._k4atypes._XYZ()
        self.assertIsNotNone(xyz)
        self.assertEqual(len(xyz._fields_), 3)

    def test_k4a_float3_t(self):
        xyz = k4a.Float3()
        self.assertIsNotNone(xyz)
        self.assertEqual(len(xyz._fields_), 2)

    def test_k4a_imu_sample_t(self):
        imu = k4a.ImuSample()
        self.assertIsNotNone(imu)
        self.assertEqual(len(imu._fields_), 5)


if __name__ == '__main__':
    unittest.main()