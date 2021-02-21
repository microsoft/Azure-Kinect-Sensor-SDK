'''
test_k4a_azurekinect.py

Tests for the k4a functions for Azure Kinect device.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import unittest
from unittest import mock
import ctypes
    
import k4a

from mock_k4a_dll_azurekinect import *


class Test_Unit_Device_AzureKinect(unittest.TestCase):
    '''Test k4a ctypes functions without requiring a device.
    '''

    @classmethod
    @mock.patch('k4a._bindings.device.k4a_device_get_info', side_effect=mock_k4a_get_device_info)
    @mock.patch('k4a._bindings.device.k4a_device_get_version', side_effect=mock_k4a_get_version)
    @mock.patch('k4a._bindings.device.k4a_device_get_serialnum', side_effect=mock_k4a_get_serialnum)
    @mock.patch('k4a._bindings.device.k4a_device_open', side_effect=mock_k4a_device_open)
    def setUpClass(cls, mock_func_open, mock_func_serialnum, mock_func_version, mock_func_info):

        cls.device = k4a.Device.open()

        mock_func_open.assert_called_once()
        mock_func_serialnum.assert_called_once()
        mock_func_version.assert_called_once()
        mock_func_info.assert_called_once()
        
        cls.assertIsNotNone(cls, cls.device)
        cls.assertTrue(cls, cls.device.serial_number == "b'0123-4567890'")
        cls.assertTrue(cls, str(cls.device.hardware_version.rgb)=="0.1.0")
        cls.assertTrue(cls, str(cls.device.hardware_version.depth)=="0.1.0")
        cls.assertTrue(cls, str(cls.device.hardware_version.audio)=="0.1.0")
        cls.assertTrue(cls, str(cls.device.hardware_version.depth_sensor)=="0.1.0")
        cls.assertTrue(cls, cls.device.hardware_version.firmware_build==1)
        cls.assertTrue(cls, cls.device.hardware_version.firmware_signature==1)

    @classmethod
    @mock.patch('k4a._bindings.device.k4a_device_close', side_effect=mock_k4a_device_close)
    @mock.patch('k4a._bindings.device.k4a_device_stop_imu')
    @mock.patch('k4a._bindings.device.k4a_device_stop_cameras')
    def tearDownClass(cls, mock_func_stop_cameras, mock_func_stop_imu, mock_func_close):

        cls.device.close()

        mock_func_stop_cameras.assert_called_once()
        mock_func_stop_imu.assert_called_once()
        mock_func_close.assert_called_once()

    @mock.patch('k4a._bindings.device.k4a_device_get_info', side_effect=mock_k4a_get_device_info)
    @mock.patch('k4a._bindings.device.k4a_device_get_version', side_effect=mock_k4a_get_version)
    @mock.patch('k4a._bindings.device.k4a_device_get_serialnum', side_effect=mock_k4a_get_serialnum)
    @mock.patch('k4a._bindings.device.k4a_device_open', side_effect=mock_k4a_device_open)
    @mock.patch('k4a._bindings.device.k4a_device_close', side_effect=mock_k4a_device_close)
    @mock.patch('k4a._bindings.device.k4a_device_stop_imu')
    @mock.patch('k4a._bindings.device.k4a_device_stop_cameras')
    def test_unit_device_open_and_close_same_device_many_times(self,
        mock_stop_cameras, mock_stop_imu, mock_close, mock_open, 
        mock_get_serial, mock_get_version, mock_get_info):

        devices_list = list()
        num_iter = 10

        self.assertEqual(mock_stop_cameras.call_count, 0)
        self.assertEqual(mock_stop_imu.call_count, 0)
        self.assertEqual(mock_open.call_count, 0)
        self.assertEqual(mock_close.call_count, 0)
        self.assertEqual(mock_get_serial.call_count, 0)
        self.assertEqual(mock_get_version.call_count, 0)
        self.assertEqual(mock_get_info.call_count, 0)

        # Open device index 1 and close it. The device at index 0 is already opened.
        for n in range(1, num_iter + 1):
            device = k4a.Device.open(1)
            devices_list.append(device)
            device.close()

            self.assertIsNotNone(device)
            self.assertEqual(mock_stop_cameras.call_count, n)
            self.assertEqual(mock_stop_imu.call_count, n)
            self.assertEqual(mock_open.call_count, n)
            self.assertEqual(mock_close.call_count, n)
            self.assertEqual(mock_get_serial.call_count, n)
            self.assertEqual(mock_get_version.call_count, n)
            self.assertEqual(mock_get_info.call_count, n)
        
        self.assertEqual(mock_stop_cameras.call_count, num_iter)
        self.assertEqual(mock_stop_imu.call_count, num_iter)
        self.assertEqual(mock_open.call_count, num_iter)
        self.assertEqual(mock_close.call_count, num_iter)
        self.assertEqual(mock_get_serial.call_count, num_iter)
        self.assertEqual(mock_get_version.call_count, num_iter)
        self.assertEqual(mock_get_info.call_count, num_iter)

    @mock.patch('k4a._bindings.device.k4a_device_open', return_value=k4a.EStatus.FAILED)
    def test_unit_device_open_fail(self, mock_func):

        device = k4a.Device.open()
        self.assertIsNone(device)
        self.assertEqual(mock_func.call_count, 1)

    @mock.patch('k4a._bindings.device.k4a_device_get_info', side_effect=mock_k4a_get_device_info)
    @mock.patch('k4a._bindings.device.k4a_device_get_version', side_effect=mock_k4a_get_version)
    @mock.patch('k4a._bindings.device.k4a_device_get_serialnum', return_value=k4a.EStatus.FAILED)
    @mock.patch('k4a._bindings.device.k4a_device_open', side_effect=mock_k4a_device_open)
    def test_unit_device_open_get_serial_num_fail(self, mock_open, mock_serialnum, mock_version, mock_info):

        device = k4a.Device.open()
        self.assertIsNotNone(device)

        # Check that serial number is empty.
        self.assertEqual(device.serial_number, '')

        self.assertIsInstance(device.device_info, k4a.DeviceInfo)
        self.assertEqual(device.device_info.struct_size, 20)
        self.assertEqual(device.device_info.struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(device.device_info.vendor_id, 0x045E)
        self.assertEqual(device.device_info.device_id, 0x097C)
        self.assertEqual(device.device_info.capabilities.value, 0x000F)

        self.assertIsInstance(device.hardware_version, k4a.HardwareVersion)
        self.assertTrue(str(device.hardware_version.rgb)=="0.1.0")
        self.assertTrue(str(device.hardware_version.depth)=="0.1.0")
        self.assertTrue(str(device.hardware_version.audio)=="0.1.0")
        self.assertTrue(str(device.hardware_version.depth_sensor)=="0.1.0")
        self.assertTrue(device.hardware_version.firmware_build==1)
        self.assertTrue(device.hardware_version.firmware_signature==1)

        self.assertEqual(mock_open.call_count, 1)
        self.assertEqual(mock_serialnum.call_count, 1)
        self.assertEqual(mock_version.call_count, 1)
        self.assertEqual(mock_info.call_count, 1)

    @mock.patch('k4a._bindings.device.k4a_device_get_info', side_effect=mock_k4a_get_device_info)
    @mock.patch('k4a._bindings.device.k4a_device_get_version', return_value=k4a.EStatus.FAILED)
    @mock.patch('k4a._bindings.device.k4a_device_get_serialnum', side_effect=mock_k4a_get_serialnum)
    @mock.patch('k4a._bindings.device.k4a_device_open', side_effect=mock_k4a_device_open)
    def test_unit_device_open_get_version_fail(self, mock_open, mock_serialnum, mock_version, mock_info):
        
        device = k4a.Device.open()
        self.assertIsNotNone(device)

        # Check that serial number is empty.
        self.assertEqual(device.serial_number, "b'0123-4567890'")

        self.assertIsInstance(device.device_info, k4a.DeviceInfo)
        self.assertEqual(device.device_info.struct_size, 20)
        self.assertEqual(device.device_info.struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(device.device_info.vendor_id, 0x045E)
        self.assertEqual(device.device_info.device_id, 0x097C)
        self.assertEqual(device.device_info.capabilities.value, 0x000F)

        self.assertIsInstance(device.hardware_version, k4a.HardwareVersion)
        self.assertTrue(str(device.hardware_version.rgb)=="0.0.0")
        self.assertTrue(str(device.hardware_version.depth)=="0.0.0")
        self.assertTrue(str(device.hardware_version.audio)=="0.0.0")
        self.assertTrue(str(device.hardware_version.depth_sensor)=="0.0.0")
        self.assertTrue(device.hardware_version.firmware_build==0)
        self.assertTrue(device.hardware_version.firmware_signature==0)

        self.assertEqual(mock_open.call_count, 1)
        self.assertEqual(mock_serialnum.call_count, 1)
        self.assertEqual(mock_version.call_count, 1)
        self.assertEqual(mock_info.call_count, 1)

    @mock.patch('k4a._bindings.device.k4a_device_get_info', return_value=k4a.EStatus.FAILED)
    @mock.patch('k4a._bindings.device.k4a_device_get_version', side_effect=mock_k4a_get_version)
    @mock.patch('k4a._bindings.device.k4a_device_get_serialnum', side_effect=mock_k4a_get_serialnum)
    @mock.patch('k4a._bindings.device.k4a_device_open', side_effect=mock_k4a_device_open)
    def test_unit_device_open_get_info_fail(self, mock_open, mock_serialnum, mock_version, mock_info):

        device = k4a.Device.open()
        self.assertIsNotNone(device)

        # Check that serial number is empty.
        self.assertEqual(device.serial_number, "b'0123-4567890'")

        self.assertIsInstance(device.device_info, k4a.DeviceInfo)
        self.assertEqual(device.device_info.struct_size, 20)
        self.assertEqual(device.device_info.struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(device.device_info.vendor_id, 0)
        self.assertEqual(device.device_info.device_id, 0)
        self.assertEqual(device.device_info.capabilities.value, 0)

        self.assertIsInstance(device.hardware_version, k4a.HardwareVersion)
        self.assertTrue(str(device.hardware_version.rgb)=="0.1.0")
        self.assertTrue(str(device.hardware_version.depth)=="0.1.0")
        self.assertTrue(str(device.hardware_version.audio)=="0.1.0")
        self.assertTrue(str(device.hardware_version.depth_sensor)=="0.1.0")
        self.assertTrue(device.hardware_version.firmware_build==1)
        self.assertTrue(device.hardware_version.firmware_signature==1)

        self.assertEqual(mock_open.call_count, 1)
        self.assertEqual(mock_serialnum.call_count, 1)
        self.assertEqual(mock_version.call_count, 1)
        self.assertEqual(mock_info.call_count, 1)

    @mock.patch('k4a._bindings.device.k4a_device_get_installed_count', side_effect=mock_k4a_device_get_installed_count)
    def test_unit_device_get_installed_count(self, mock_func):

        device_count = k4a.Device.get_device_count()

        self.assertEqual(device_count, 4) # Allow for up to 4 connected non-existent devices.
        mock_func.assert_called_once()

    @mock.patch('k4a._bindings.device.k4a_device_stop_cameras')
    @mock.patch('k4a._bindings.device.k4a_device_start_cameras', side_effect=mock_k4a_start_cameras)
    def test_unit_device_start_stop_cameras(self, mock_func_start_cameras, mock_func_stop_cameras):

        # Start cameras.
        config = k4a.DeviceConfiguration(
            color_format=k4a.EImageFormat.COLOR_BGRA32,
            color_mode_id=1,  # 720p
            depth_mode_id=1, # NFOV_2X2BINNED
            fps_mode_id=15    # 15 frames per second
        )

        status = self.device.start_cameras(config)
        self.assertTrue(k4a.K4A_SUCCEEDED(status))
        mock_func_start_cameras.assert_called_once()
        mock_func_stop_cameras.assert_not_called()

        # Stop cameras.
        self.device.stop_cameras()
        mock_func_stop_cameras.assert_called_once()
        mock_func_start_cameras.assert_called_once()

    @mock.patch('k4a._bindings.device.k4a_device_stop_cameras')
    @mock.patch('k4a._bindings.device.k4a_device_start_cameras', return_value=k4a.EStatus.FAILED)
    def test_unit_device_start_stop_cameras_fail(self, mock_func_start_cameras, mock_func_stop_cameras):

        # Start cameras.
        config = k4a.DeviceConfiguration(
            color_format=k4a.EImageFormat.COLOR_BGRA32,
            color_mode_id=1,  # 720p
            depth_mode_id=1, # NFOV_2X2BINNED
            fps_mode_id=15    # 15 frames per second
        )

        status = self.device.start_cameras(config)
        self.assertTrue(k4a.K4A_FAILED(status))
        mock_func_start_cameras.assert_called_once()
        mock_func_stop_cameras.assert_not_called()

        # Stop cameras.
        self.device.stop_cameras()
        mock_func_stop_cameras.assert_called_once()
        mock_func_start_cameras.assert_called_once()

    @mock.patch('k4a._bindings.device.k4a_device_stop_imu')
    @mock.patch('k4a._bindings.device.k4a_device_start_imu', return_value=k4a.EStatus.SUCCEEDED)
    def test_unit_device_start_stop_imu(self, mock_func_start_imu, mock_func_stop_imu):

        status = self.device.start_imu()
        self.assertTrue(k4a.K4A_SUCCEEDED(status))
        mock_func_start_imu.assert_called_once()
        mock_func_stop_imu.assert_not_called()

        # Stop cameras.
        self.device.stop_imu()
        mock_func_stop_imu.assert_called_once()
        mock_func_start_imu.assert_called_once()

    @mock.patch('k4a._bindings.device.k4a_device_get_serialnum', side_effect=mock_k4a_get_serialnum)
    def test_unit_device_serial_number(self, mock_func_serialnum):

        serial_number = self.device.serial_number

        self.assertIsInstance(serial_number, str)
        self.assertEqual(serial_number, "b'0123-4567890'")
        mock_func_serialnum.assert_not_called() # Not called because it is a device property.

    @mock.patch('k4a._bindings.device.k4a_device_get_version', side_effect=mock_k4a_get_version)
    def test_unit_device_hardware_version(self, mock_func_version):

        hardware_version = self.device.hardware_version

        self.assertIsInstance(hardware_version, k4a.HardwareVersion)
        self.assertTrue(str(hardware_version.rgb)=="0.1.0")
        self.assertTrue(str(hardware_version.depth)=="0.1.0")
        self.assertTrue(str(hardware_version.audio)=="0.1.0")
        self.assertTrue(str(hardware_version.depth_sensor)=="0.1.0")
        self.assertTrue(hardware_version.firmware_build==1)
        self.assertTrue(hardware_version.firmware_signature==1)
        mock_func_version.assert_not_called()

    @mock.patch('k4a._bindings.device.k4a_device_get_info', side_effect=mock_k4a_get_device_info)
    def test_unit_device_device_info(self, mock_func_info):

        device_info = self.device.device_info

        capabilities = (k4a.EDeviceCapabilities.DEPTH | k4a.EDeviceCapabilities.COLOR |
            k4a.EDeviceCapabilities.IMU | k4a.EDeviceCapabilities.MICROPHONE)

        self.assertIsInstance(device_info, k4a.DeviceInfo)
        self.assertEqual(device_info.struct_size, 20)
        self.assertEqual(device_info.struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(device_info.vendor_id, 0x045E)
        self.assertEqual(device_info.device_id, 0x097C)
        self.assertEqual(device_info.capabilities.value, capabilities)
        mock_func_info.assert_not_called()

    @mock.patch('k4a._bindings.device.k4a_device_get_color_mode_count', side_effect=mock_k4a_get_mode_count)
    @mock.patch('k4a._bindings.device.k4a_device_get_color_mode', side_effect=mock_k4a_get_color_mode)
    def test_unit_device_get_color_modes(self, mock_func_mode, mock_func_count):

        color_modes = self.device.get_color_modes()

        self.assertIsInstance(color_modes, list)
        self.assertEqual(len(color_modes), 2)

        self.assertIsInstance(color_modes[0], k4a.ColorModeInfo)
        self.assertEqual(color_modes[0].struct_size, 40)
        self.assertEqual(color_modes[0].struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(color_modes[0].mode_id, 0)
        self.assertEqual(color_modes[0].width, 0)
        self.assertEqual(color_modes[0].height, 0)
        self.assertEqual(color_modes[0].native_format, k4a.EImageFormat.COLOR_MJPG)
        self.assertEqual(color_modes[0].horizontal_fov, 0)
        self.assertEqual(color_modes[0].vertical_fov, 0)
        self.assertEqual(color_modes[0].min_fps, 0)
        self.assertEqual(color_modes[0].max_fps, 0)

        self.assertIsInstance(color_modes[1], k4a.ColorModeInfo)
        self.assertEqual(color_modes[1].struct_size, 40)
        self.assertEqual(color_modes[1].struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(color_modes[1].mode_id, 1)
        self.assertEqual(color_modes[1].width, 512)
        self.assertEqual(color_modes[1].height, 256)
        self.assertEqual(color_modes[1].native_format, k4a.EImageFormat.COLOR_MJPG)
        self.assertEqual(color_modes[1].horizontal_fov, 120)
        self.assertEqual(color_modes[1].vertical_fov, 120)
        self.assertEqual(color_modes[1].min_fps, 5)
        self.assertEqual(color_modes[1].max_fps, 30)

        mock_func_count.assert_called_once()
        self.assertEqual(mock_func_mode.call_count, 2) # Check that k4a_get_color_mode is called twice, once for each mode.

    @mock.patch('k4a._bindings.device.k4a_device_get_color_mode_count', return_value=k4a.EStatus.FAILED)
    @mock.patch('k4a._bindings.device.k4a_device_get_color_mode', side_effect=mock_k4a_get_color_mode)
    def test_unit_device_get_color_modes_count_fail(self, mock_func_mode, mock_func_count):

        color_modes = self.device.get_color_modes()
        self.assertIsNone(color_modes)

        mock_func_count.assert_called_once()
        mock_func_mode.assert_not_called() # If mode count fails, then get mode does not get called.

    @mock.patch('k4a._bindings.device.k4a_device_get_color_mode_count', side_effect=mock_k4a_get_mode_count)
    @mock.patch('k4a._bindings.device.k4a_device_get_color_mode', return_value=k4a.EStatus.FAILED)
    def test_unit_device_get_color_modes_fail(self, mock_func_mode, mock_func_count):

        color_modes = self.device.get_color_modes()
        self.assertIsNone(color_modes)

        mock_func_count.assert_called_once()
        self.assertEqual(mock_func_mode.call_count, 2)

    @mock.patch('k4a._bindings.device.k4a_device_get_depth_mode_count', side_effect=mock_k4a_get_mode_count)
    @mock.patch('k4a._bindings.device.k4a_device_get_depth_mode', side_effect=mock_k4a_get_depth_mode)
    def test_unit_device_get_depth_modes(self, mock_func_mode, mock_func_count):

        depth_modes = self.device.get_depth_modes()

        self.assertIsInstance(depth_modes, list)
        self.assertEqual(len(depth_modes), 2)

        self.assertIsInstance(depth_modes[0], k4a.DepthModeInfo)
        self.assertEqual(depth_modes[0].struct_size, 52)
        self.assertEqual(depth_modes[0].struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(depth_modes[0].mode_id, 0)
        self.assertEqual(depth_modes[0].width, 0)
        self.assertEqual(depth_modes[0].height, 0)
        self.assertEqual(depth_modes[0].native_format, k4a.EImageFormat.DEPTH16)
        self.assertEqual(depth_modes[0].horizontal_fov, 0)
        self.assertEqual(depth_modes[0].vertical_fov, 0)
        self.assertEqual(depth_modes[0].min_fps, 0)
        self.assertEqual(depth_modes[0].max_fps, 0)

        self.assertIsInstance(depth_modes[1], k4a.DepthModeInfo)
        self.assertEqual(depth_modes[1].struct_size, 52)
        self.assertEqual(depth_modes[1].struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(depth_modes[1].mode_id, 1)
        self.assertEqual(depth_modes[1].width, 512)
        self.assertEqual(depth_modes[1].height, 256)
        self.assertEqual(depth_modes[1].native_format, k4a.EImageFormat.DEPTH16)
        self.assertEqual(depth_modes[1].horizontal_fov, 120)
        self.assertEqual(depth_modes[1].vertical_fov, 120)
        self.assertEqual(depth_modes[1].min_fps, 5)
        self.assertEqual(depth_modes[1].max_fps, 30)

        mock_func_count.assert_called_once()
        self.assertEqual(mock_func_mode.call_count, 2) # Check that k4a_get_depth_mode is called twice, once for each mode.

    @mock.patch('k4a._bindings.device.k4a_device_get_depth_mode_count', return_value=k4a.EStatus.FAILED)
    @mock.patch('k4a._bindings.device.k4a_device_get_depth_mode', side_effect=mock_k4a_get_depth_mode)
    def test_unit_device_get_depth_modes_count_fail(self, mock_func_mode, mock_func_count):

        depth_modes = self.device.get_depth_modes()
        self.assertIsNone(depth_modes)

        mock_func_count.assert_called_once()
        mock_func_mode.assert_not_called() # If mode count fails, then get mode does not get called.

    @mock.patch('k4a._bindings.device.k4a_device_get_depth_mode_count', side_effect=mock_k4a_get_mode_count)
    @mock.patch('k4a._bindings.device.k4a_device_get_depth_mode', return_value=k4a.EStatus.FAILED)
    def test_unit_device_get_depth_modes_fail(self, mock_func_mode, mock_func_count):

        depth_modes = self.device.get_depth_modes()
        self.assertIsNone(depth_modes)

        mock_func_count.assert_called_once()
        self.assertEqual(mock_func_mode.call_count, 2)

    @mock.patch('k4a._bindings.device.k4a_device_get_fps_mode_count', side_effect=mock_k4a_get_mode_count)
    @mock.patch('k4a._bindings.device.k4a_device_get_fps_mode', side_effect=mock_k4a_get_fps_mode)
    def test_unit_device_get_fps_modes(self, mock_func_mode, mock_func_count):

        fps_modes = self.device.get_fps_modes()

        self.assertIsInstance(fps_modes, list)
        self.assertEqual(len(fps_modes), 2)

        self.assertIsInstance(fps_modes[0], k4a.FPSModeInfo)
        self.assertEqual(fps_modes[0].struct_size, 16)
        self.assertEqual(fps_modes[0].struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(fps_modes[0].mode_id, 0)
        self.assertEqual(fps_modes[0].fps, 0)

        self.assertIsInstance(fps_modes[1], k4a.FPSModeInfo)
        self.assertEqual(fps_modes[1].struct_size, 16)
        self.assertEqual(fps_modes[1].struct_version, k4a.K4A_ABI_VERSION)
        self.assertEqual(fps_modes[1].mode_id, 15)
        self.assertEqual(fps_modes[1].fps, 15)

        mock_func_count.assert_called_once()
        self.assertEqual(mock_func_mode.call_count, 2) # Check that k4a_get_depth_mode is called twice, once for each mode.

    @mock.patch('k4a._bindings.device.k4a_device_get_fps_mode_count', return_value=k4a.EStatus.FAILED)
    @mock.patch('k4a._bindings.device.k4a_device_get_fps_mode', side_effect=mock_k4a_get_fps_mode)
    def test_unit_device_get_fps_modes_count_fail(self, mock_func_mode, mock_func_count):

        fps_modes = self.device.get_fps_modes()
        self.assertIsNone(fps_modes)

        mock_func_count.assert_called_once()
        mock_func_mode.assert_not_called() # If mode count fails, then get mode does not get called.

    @mock.patch('k4a._bindings.device.k4a_device_get_fps_mode_count', side_effect=mock_k4a_get_mode_count)
    @mock.patch('k4a._bindings.device.k4a_device_get_fps_mode', return_value=k4a.EStatus.FAILED)
    def test_unit_device_get_fps_modes_fail(self, mock_func_mode, mock_func_count):

        fps_modes = self.device.get_fps_modes()
        self.assertIsNone(fps_modes)

        mock_func_count.assert_called_once()
        self.assertEqual(mock_func_mode.call_count, 2)

    @mock.patch('k4a._bindings.device.k4a_device_get_capture', side_effect=mock_k4a_device_get_capture)
    def test_unit_device_get_capture(self, mock_func_capture):
        
        capture = self.device.get_capture(0)
        self.assertIsInstance(capture, k4a.Capture)
        mock_func_capture.assert_called_once()

    @mock.patch('k4a._bindings.device.k4a_device_get_capture', return_value=k4a.EStatus.FAILED)
    def test_unit_device_get_capture_fail(self, mock_func_capture):
        
        capture = self.device.get_capture(0)
        self.assertIsNone(capture)
        mock_func_capture.assert_called_once()

    @mock.patch('k4a._bindings.device.k4a_device_get_imu_sample', side_effect=mock_k4a_device_get_imu_sample)
    def test_unit_device_get_imu(self, mock_func_imu):
        
        imu_sample = self.device.get_imu_sample(0)
        self.assertIsInstance(imu_sample, k4a.ImuSample)
        self.assertAlmostEqual(imu_sample.temperature, 1.2, 6)
        self.assertEqual(imu_sample.acc_sample.xyz.x, 1)
        self.assertEqual(imu_sample.acc_sample.xyz.y, 2)
        self.assertEqual(imu_sample.acc_sample.xyz.z, 3)
        self.assertEqual(imu_sample.acc_timestamp_usec, 1)
        self.assertEqual(imu_sample.gyro_sample.xyz.x, 4)
        self.assertEqual(imu_sample.gyro_sample.xyz.y, 5)
        self.assertEqual(imu_sample.gyro_sample.xyz.z, 6)

        mock_func_imu.assert_called_once()

    @mock.patch('k4a._bindings.device.k4a_device_get_imu_sample', return_value=k4a.EStatus.FAILED)
    def test_unit_device_get_imu_fail(self, mock_func_imu):
        
        imu_sample = self.device.get_imu_sample(0)
        self.assertIsNone(imu_sample)
        mock_func_imu.assert_called_once()

    @mock.patch('k4a._bindings.device.k4a_device_get_color_control_capabilities', side_effect=mock_k4a_device_get_color_control_capabilities)
    def test_unit_device_get_color_control_capabilities(self, mock_func):
        
        cap_list, cap_dict = self.device.get_color_control_capabilities()
        self.assertIsInstance(cap_list, list)
        self.assertIsInstance(cap_dict, dict)

        self.assertEqual(mock_func.call_count, 9) # Check that get_color_control_capabilities is called 9 times, once for each capability.

        for cap in cap_list:
            self.assertIsInstance(cap, k4a.ColorControlCapability)
            self.assertIsInstance(cap.command, k4a.EColorControlCommand)
            self.assertEqual(cap.supports_auto, True)
            self.assertEqual(cap.min_value, 100)
            self.assertEqual(cap.max_value, 9999)
            self.assertEqual(cap.step_value, 1)
            self.assertEqual(cap.default_value, 700)
            self.assertEqual(cap.default_mode, k4a.EColorControlMode.MANUAL)

        for command, cap in cap_dict.items():
            self.assertIsInstance(command, k4a.EColorControlCommand)
            self.assertIsInstance(cap, k4a.ColorControlCapability)
            self.assertIsInstance(cap.command, k4a.EColorControlCommand)
            self.assertEqual(cap.supports_auto, True)
            self.assertEqual(cap.min_value, 100)
            self.assertEqual(cap.max_value, 9999)
            self.assertEqual(cap.step_value, 1)
            self.assertEqual(cap.default_value, 700)
            self.assertEqual(cap.default_mode, k4a.EColorControlMode.MANUAL)

    @mock.patch('k4a._bindings.device.k4a_device_get_color_control_capabilities', return_value=k4a.EStatus.FAILED)
    def test_unit_device_get_color_control_capabilities_fail(self, mock_func):
        
        cap_list, cap_dict = self.device.get_color_control_capabilities()
        self.assertIsNone(cap_list)
        self.assertIsNone(cap_dict)
        self.assertEqual(mock_func.call_count, 9) # Check that get_color_control_capabilities is called 9 times, once for each capability.

    @mock.patch('k4a._bindings.device.k4a_device_get_color_control', side_effect=mock_k4a_device_get_color_control)
    def test_unit_device_get_color_control(self, mock_func):
        
        for command in k4a.EColorControlCommand:

            value, mode = self.device.get_color_control(command)

            self.assertIsNotNone(value)
            self.assertIsNotNone(mode)
            self.assertIsInstance(mode, k4a.EColorControlMode)
            self.assertEqual(value, 1000)
            self.assertEqual(mode, k4a.EColorControlMode.MANUAL)

        self.assertEqual(mock_func.call_count, len(k4a.EColorControlCommand))

    @mock.patch('k4a._bindings.device.k4a_device_get_color_control', return_value=k4a.EStatus.FAILED)
    def test_unit_device_get_color_control_fail(self, mock_func):
        
        for command in k4a.EColorControlCommand:

            value, mode = self.device.get_color_control(command)

            self.assertIsNone(value)
            self.assertIsNone(mode)

        self.assertEqual(mock_func.call_count, len(k4a.EColorControlCommand))

    @mock.patch('k4a._bindings.device.k4a_device_set_color_control', side_effect=mock_k4a_device_set_color_control)
    def test_unit_device_set_color_control(self, mock_func):
        
        for command in k4a.EColorControlCommand:
            status = self.device.set_color_control(command, k4a.EColorControlMode.MANUAL, 1000)
            self.assertEqual(status, k4a.EStatus.SUCCEEDED)

    @mock.patch('k4a._bindings.device.k4a_device_set_color_control', return_value=k4a.EStatus.FAILED)
    def test_unit_device_set_color_control_fail(self, mock_func):
        
        for command in k4a.EColorControlCommand:
            status = self.device.set_color_control(command, k4a.EColorControlMode.MANUAL, 1000)
            self.assertEqual(status, k4a.EStatus.FAILED)

    @mock.patch('k4a._bindings.device.k4a_device_get_sync_jack', side_effect=mock_k4a_device_get_sync_jack)
    def test_unit_device_get_sync_jack(self, mock_func):
        
        sync_in, sync_out = self.device.get_sync_jack()

        mock_func.assert_called_once()
        self.assertIsNotNone(sync_in)
        self.assertIsNotNone(sync_out)
        self.assertEqual(sync_in, False)
        self.assertEqual(sync_out, True)

    @mock.patch('k4a._bindings.device.k4a_device_get_sync_jack', return_value=k4a.EStatus.FAILED)
    def test_unit_device_get_sync_jack_fail(self, mock_func):
        
        sync_in, sync_out = self.device.get_sync_jack()

        mock_func.assert_called_once()
        self.assertIsNotNone(sync_in)
        self.assertIsNotNone(sync_out)
        self.assertEqual(sync_in, False)
        self.assertEqual(sync_out, False)

    @mock.patch('k4a._bindings.device.k4a_device_get_raw_calibration', side_effect=mock_k4a_device_get_raw_calibration)
    def test_unit_device_get_raw_calibration(self, mock_func):
        
        buffer = self.device.get_raw_calibration()
        self.assertEqual(mock_func.call_count, 2)
        self.assertIsNotNone(buffer)

    @mock.patch('k4a._bindings.device.k4a_device_get_raw_calibration', return_value=k4a.EBufferStatus.FAILED)
    def test_unit_device_get_raw_calibration_fail(self, mock_func):
        
        buffer = self.device.get_raw_calibration()
        self.assertEqual(mock_func.call_count, 1)
        self.assertIsNone(buffer)

    @mock.patch('k4a._bindings.device.k4a_device_get_calibration', side_effect=mock_k4a_device_get_calibration)
    def test_unit_device_get_calibration(self, mock_func):
        
        calibration = self.device.get_calibration(1, 1)
        self.assertEqual(mock_func.call_count, 1)
        self.assertIsNotNone(calibration)
        self.assertIsInstance(calibration, k4a.Calibration)

    @mock.patch('k4a._bindings.device.k4a_device_get_calibration', return_value=k4a.EStatus.FAILED)
    def test_unit_device_get_calibration_fail(self, mock_func):
        
        calibration = self.device.get_calibration(1, 1)
        self.assertEqual(mock_func.call_count, 1)
        self.assertIsNone(calibration)
        
