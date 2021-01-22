'''
test_unit_k4atypes.py

Tests for the k4a types and enums.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import unittest
import ctypes
    
import k4a


def get_enum_values(n, start_value = 0):
    value = start_value
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

    def test_unit_EStatus(self):
        enum_values = get_enum_values(len(k4a.EStatus))
        self.assertEqual(k4a.EStatus.SUCCEEDED, next(enum_values))
        self.assertEqual(k4a.EStatus.FAILED, next(enum_values))

    def test_unit_EBufferStatus(self):
        enum_values = get_enum_values(len(k4a.EBufferStatus))
        self.assertEqual(k4a.EBufferStatus.SUCCEEDED, next(enum_values))
        self.assertEqual(k4a.EBufferStatus.FAILED, next(enum_values))
        self.assertEqual(k4a.EBufferStatus.BUFFER_TOO_SMALL, next(enum_values))

    def test_unit_EWaitStatus(self):
        enum_values = get_enum_values(len(k4a.EWaitStatus))
        self.assertEqual(k4a.EWaitStatus.SUCCEEDED, next(enum_values))
        self.assertEqual(k4a.EWaitStatus.FAILED, next(enum_values))
        self.assertEqual(k4a.EWaitStatus.TIMEOUT, next(enum_values))

    def test_unit_ELogLevel(self):
        enum_values = get_enum_values(len(k4a.ELogLevel))
        self.assertEqual(k4a.ELogLevel.CRITICAL, next(enum_values))
        self.assertEqual(k4a.ELogLevel.ERROR, next(enum_values))
        self.assertEqual(k4a.ELogLevel.WARNING, next(enum_values))
        self.assertEqual(k4a.ELogLevel.INFO, next(enum_values))
        self.assertEqual(k4a.ELogLevel.TRACE, next(enum_values))
        self.assertEqual(k4a.ELogLevel.OFF, next(enum_values))

    def test_unit_EDepthMode(self):
        enum_values = get_enum_values(len(k4a.EDepthMode))
        self.assertEqual(k4a.EDepthMode.OFF, next(enum_values))
        self.assertEqual(k4a.EDepthMode.NFOV_2X2BINNED, next(enum_values))
        self.assertEqual(k4a.EDepthMode.NFOV_UNBINNED, next(enum_values))
        self.assertEqual(k4a.EDepthMode.WFOV_2X2BINNED, next(enum_values))
        self.assertEqual(k4a.EDepthMode.WFOV_UNBINNED, next(enum_values))
        self.assertEqual(k4a.EDepthMode.PASSIVE_IR, next(enum_values))

    def test_unit_EColorResolution(self):
        enum_values = get_enum_values(len(k4a.EColorResolution))
        self.assertEqual(k4a.EColorResolution.OFF, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_720P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_1080P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_1440P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_1536P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_2160P, next(enum_values))
        self.assertEqual(k4a.EColorResolution.RES_3072P, next(enum_values))

    def test_unit_EImageFormat(self):
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

    def test_unit_ETransformInterpolationType(self):
        enum_values = get_enum_values(len(k4a.ETransformInterpolationType))
        self.assertEqual(k4a.ETransformInterpolationType.NEAREST, next(enum_values))
        self.assertEqual(k4a.ETransformInterpolationType.LINEAR, next(enum_values))

    def test_unit_EFramesPerSecond(self):
        enum_values = get_enum_values(len(k4a.EFramesPerSecond))
        self.assertEqual(k4a.EFramesPerSecond.FPS_5, next(enum_values))
        self.assertEqual(k4a.EFramesPerSecond.FPS_15, next(enum_values))
        self.assertEqual(k4a.EFramesPerSecond.FPS_30, next(enum_values))

    def test_unit_EColorControlCommand(self):
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

    def test_unit_EColorControlMode(self):
        enum_values = get_enum_values(len(k4a.EColorControlMode))
        self.assertEqual(k4a.EColorControlMode.AUTO, next(enum_values))
        self.assertEqual(k4a.EColorControlMode.MANUAL, next(enum_values))

    def test_unit_EWiredSyncMode(self):
        enum_values = get_enum_values(len(k4a.EWiredSyncMode))
        self.assertEqual(k4a.EWiredSyncMode.STANDALONE, next(enum_values))
        self.assertEqual(k4a.EWiredSyncMode.MASTER, next(enum_values))
        self.assertEqual(k4a.EWiredSyncMode.SUBORDINATE, next(enum_values))

    def test_unit_ECalibrationType(self):
        enum_values = get_enum_values(len(k4a.ECalibrationType), start_value = -1)
        self.assertEqual(k4a.ECalibrationType.UNKNOWN, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.DEPTH, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.COLOR, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.GYRO, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.ACCEL, next(enum_values))
        self.assertEqual(k4a.ECalibrationType.NUM_TYPES, next(enum_values))

    def test_unit_ECalibrationModelType(self):
        enum_values = get_enum_values(len(k4a.ECalibrationModelType))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_UNKNOWN, next(enum_values))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_THETA, next(enum_values))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_POLYNOMIAL_3K, next(enum_values))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_RATIONAL_6KT, next(enum_values))
        self.assertEqual(k4a.ECalibrationModelType.LENS_DISTORTION_MODEL_BROWN_CONRADY, next(enum_values))

    def test_unit_EFirmwareBuild(self):
        enum_values = get_enum_values(len(k4a.EFirmwareBuild))
        self.assertEqual(k4a.EFirmwareBuild.RELEASE, next(enum_values))
        self.assertEqual(k4a.EFirmwareBuild.DEBUG, next(enum_values))

    def test_unit_EFirmwareSignature(self):
        enum_values = get_enum_values(len(k4a.EFirmwareSignature))
        self.assertEqual(k4a.EFirmwareSignature.MSFT, next(enum_values))
        self.assertEqual(k4a.EFirmwareSignature.TEST, next(enum_values))
        self.assertEqual(k4a.EFirmwareSignature.UNSIGNED, next(enum_values))

    def test_unit_K4A_SUCCEEDED_True(self):
        self.assertTrue(k4a.K4A_SUCCEEDED(k4a.EStatus.SUCCEEDED))

    def test_unit_K4A_SUCCEEDED_False(self):
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

    def test_unit__DeviceHandle(self):
        device_handle = k4a._bindings.k4a._DeviceHandle()
        self.assertIsInstance(device_handle, k4a._bindings.k4a._DeviceHandle)

    def test_unit__CaptureHandle(self):
        capture_handle = k4a._bindings.k4a._CaptureHandle()
        self.assertIsInstance(capture_handle, k4a._bindings.k4a._CaptureHandle)

    def test_unit__ImageHandle(self):
        image_handle = k4a._bindings.k4a._ImageHandle()
        self.assertIsInstance(image_handle, k4a._bindings.k4a._ImageHandle)

    def test_unit__TransformationHandle(self):
        transformation_handle = k4a._bindings.k4a._TransformationHandle()
        self.assertIsInstance(transformation_handle, k4a._bindings.k4a._TransformationHandle)

    def test_unit_DeviceConfiguration(self):
        device_config = k4a.DeviceConfiguration()
        self.assertIsNotNone(device_config)
        self.assertEqual(len(device_config._fields_), 9)

    def test_unit_CalibrationExtrinsics(self):
        calibration_extrinsics = k4a.CalibrationExtrinsics()
        self.assertIsNotNone(calibration_extrinsics)
        self.assertEqual(len(calibration_extrinsics._fields_), 2)

    def test_unit_CalibrationIntrinsicParam(self):
        calib_intrinsic = k4a.CalibrationIntrinsicParam()
        self.assertIsNotNone(calib_intrinsic)
        self.assertEqual(len(calib_intrinsic._fields_), 15)

    def test_unit_CalibrationIntrinsics(self):
        calib_intrinsic = k4a.CalibrationIntrinsics()
        self.assertIsNotNone(calib_intrinsic)
        self.assertEqual(len(calib_intrinsic._fields_), 3)

    def test_unit_CalibrationCamera(self):
        camera_calibration = k4a.CalibrationCamera()
        self.assertIsNotNone(camera_calibration)
        self.assertEqual(len(camera_calibration._fields_), 5)

    def test_unit__Calibration(self):
        calibration = k4a._bindings.k4a._Calibration()
        self.assertIsNotNone(calibration)
        self.assertEqual(len(calibration._fields_), 5)

    def test_unit_Version(self):
        version = k4a.Version()
        self.assertIsNotNone(version)
        self.assertEqual(len(version._fields_), 3)

    def test_unit_HardwareVersion(self):
        version = k4a.HardwareVersion()
        self.assertIsNotNone(version)
        self.assertEqual(len(version._fields_), 6)

    def test_unit__XY(self):
        xy = k4a._bindings.k4atypes._XY()
        self.assertIsNotNone(xy)
        self.assertEqual(len(xy._fields_), 2)

    def test_unit__Float2(self):
        xy = k4a._bindings.k4a._Float2()
        self.assertIsNotNone(xy)
        self.assertEqual(len(xy._fields_), 2)

    def test_unit__XYZ(self):
        xyz = k4a._bindings.k4atypes._XYZ()
        self.assertIsNotNone(xyz)
        self.assertEqual(len(xyz._fields_), 3)

    def test_unit__Float3(self):
        xyz = k4a._bindings.k4a._Float3()
        self.assertIsNotNone(xyz)
        self.assertEqual(len(xyz._fields_), 2)

    def test_unit_ImuSample(self):
        imu = k4a.ImuSample()
        self.assertIsNotNone(imu)
        self.assertEqual(len(imu._fields_), 5)


if __name__ == '__main__':
    unittest.main()