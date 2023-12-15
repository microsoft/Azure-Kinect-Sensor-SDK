'''
test_device_azurekinect.py

Tests for the Device class for Azure Kinect device.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import unittest
import copy
from time import sleep

import numpy as np

import k4a
import test_config


def k4a_device_set_and_get_color_control(
    device:k4a.Device,
    color_control_command:k4a.EColorControlCommand):

    saved_value = 0

    # Get the step size.
    supports_auto = device.color_ctrl_cap.__dict__[color_control_command].supports_auto
    min_value = device.color_ctrl_cap.__dict__[color_control_command].min_value
    max_value = device.color_ctrl_cap.__dict__[color_control_command].max_value
    step_value = device.color_ctrl_cap.__dict__[color_control_command].step_value
    default_value = device.color_ctrl_cap.__dict__[color_control_command].default_value
    mode = k4a.EColorControlMode.MANUAL

    # Read the original value.
    (saved_value, mode) = device.get_color_control(color_control_command)
    
    # Write a new value.
    new_value = 0
    if (saved_value + step_value <= max_value):
        new_value = saved_value + step_value
    else:
        new_value = saved_value - step_value

    status1 = device.set_color_control(
        color_control_command,
        mode,
        new_value)

    # Read back the value to check that it was written.
    (new_value_readback, mode) = device.get_color_control(color_control_command)

    # Write the original saved value.
    status2 = device.set_color_control(
        color_control_command,
        mode,
        saved_value)

    # Read back the value to check that it was written.
    (saved_value_readback, mode) = device.get_color_control(color_control_command)

    return (status1, status2, saved_value, saved_value_readback, new_value, new_value_readback)


class Test_Functional_API_Device_AzureKinect(unittest.TestCase):
    '''Test Device class for Azure Kinect device.
    '''

    @classmethod
    def setUpClass(cls):
        cls.device = k4a.Device.open()
        assert(cls.device is not None)

        cls.lock = test_config.glb_lock

    @classmethod
    def tearDownClass(cls):

        # Stop the cameras and imus before closing device.
        cls.device.stop_cameras()
        cls.device.stop_imu()
        cls.device.close()
        del cls.device

    def test_functional_fast_api_open_twice_expected_fail(self):
        device2 = k4a.Device.open()
        self.assertIsNone(device2)

        device2 = k4a.Device.open(1000000)
        self.assertIsNone(device2)

    def test_functional_fast_api_device_shallow_copy(self):
        device2 = copy.copy(self.device)
        self.assertIsNone(device2)

    def test_functional_fast_api_device_deep_copy(self):
        device2 = copy.deepcopy(self.device)
        self.assertIsNone(device2)

    def test_functional_fast_api_get_serial_number(self):
        serial_number = self.device.serial_number
        self.assertIsInstance(serial_number, str)
        self.assertGreater(len(serial_number), 0)

    # Helper method for test_set_serial_number().
    @staticmethod
    def set_serial_number(device:k4a.Device, serial_number:str):
        device.serial_number = serial_number

    def test_functional_fast_api_set_serial_number(self):
        self.assertRaises(AttributeError, 
            Test_Functional_API_Device_AzureKinect.set_serial_number, 
            self.device, "not settable")

    def test_functional_fast_api_get_capture(self):

        # Start the cameras.
        status = self.device.start_cameras(
            k4a.DEVICE_CONFIG_BGRA32_2160P_WFOV_UNBINNED_FPS15)
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        # Get a capture, waiting indefinitely.
        capture = self.device.get_capture(-1)
        self.assertIsNotNone(capture)

        # Stop the cameras.
        self.device.stop_cameras()

    def test_functional_fast_api_get_imu_sample(self):

        # Start the cameras.
        status = self.device.start_cameras(
            k4a.DEVICE_CONFIG_BGRA32_2160P_WFOV_UNBINNED_FPS15)
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        # Start the imu.
        status = self.device.start_imu()
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        # Get an imu sample, waiting indefinitely.
        imu_sample = self.device.get_imu_sample(-1)
        self.assertIsNotNone(imu_sample)

        # Stop the cameras and imu.
        self.device.stop_cameras()
        self.device.stop_imu()

    def test_functional_fast_api_get_serial_number(self):
        serial_number = self.device.serial_number
        self.assertIsInstance(serial_number, str)
        self.assertNotEqual(len(serial_number), 0)

    def test_functional_fast_api_get_hardware_version(self):
        hardware_version = self.device.hardware_version
        self.assertIsInstance(hardware_version, k4a.HardwareVersion)
        self.assertNotEqual(str(hardware_version), 0)

    def test_functional_fast_api_get_color_ctrl_cap(self):
        color_ctrl_cap = self.device.color_ctrl_cap
        self.assertNotEqual(str(color_ctrl_cap), 0)

    def test_functional_fast_api_get_sync_out_connected(self):
        sync_out_connected = self.device.sync_out_connected
        self.assertIsInstance(sync_out_connected, bool)

    def test_functional_fast_api_get_sync_in_connected(self):
        sync_in_connected = self.device.sync_in_connected
        self.assertIsInstance(sync_in_connected, bool)

    def test_functional_fast_api_start_stop_cameras(self):
        status = self.device.start_cameras(k4a.DEVICE_CONFIG_BGRA32_2160P_WFOV_UNBINNED_FPS15)
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        self.device.stop_cameras()

    def test_functional_fast_api_start_stop_imu(self):
        status = self.device.start_cameras(k4a.DEVICE_CONFIG_BGRA32_2160P_WFOV_UNBINNED_FPS15)
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        status = self.device.start_imu()
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        self.device.stop_imu()
        self.device.stop_cameras()

    def test_functional_fast_api_get_color_control(self):

        color_control_commands = [
            k4a.EColorControlCommand.BACKLIGHT_COMPENSATION,
            k4a.EColorControlCommand.BRIGHTNESS,
            k4a.EColorControlCommand.CONTRAST,
            k4a.EColorControlCommand.EXPOSURE_TIME_ABSOLUTE,
            k4a.EColorControlCommand.GAIN,
            k4a.EColorControlCommand.POWERLINE_FREQUENCY,
            k4a.EColorControlCommand.SATURATION,
            k4a.EColorControlCommand.SHARPNESS,
            k4a.EColorControlCommand.WHITEBALANCE
        ]

        for command in color_control_commands:
            with self.subTest(command = command):
                (value, mode) = self.device.get_color_control(command)
                self.assertIsNotNone(value)
                self.assertIsNotNone(mode)

    def test_functional_fast_api_set_color_control(self):

        color_control_commands = [
            k4a.EColorControlCommand.BACKLIGHT_COMPENSATION,
            k4a.EColorControlCommand.BRIGHTNESS,
            k4a.EColorControlCommand.CONTRAST,
            #k4a.EColorControlCommand.EXPOSURE_TIME_ABSOLUTE,
            k4a.EColorControlCommand.GAIN,
            k4a.EColorControlCommand.POWERLINE_FREQUENCY,
            k4a.EColorControlCommand.SATURATION,
            k4a.EColorControlCommand.SHARPNESS,
            #k4a.EColorControlCommand.WHITEBALANCE
        ]

        for command in color_control_commands:
            with self.subTest(command = command):
                (status1, status2, saved_value, saved_value_readback, 
                    new_value, new_value_readback) = \
                    k4a_device_set_and_get_color_control(self.device, command)

                self.assertTrue(k4a.K4A_SUCCEEDED(status1))
                self.assertTrue(k4a.K4A_SUCCEEDED(status2))
                self.assertEqual(saved_value, saved_value_readback)
                self.assertEqual(new_value, new_value_readback)
                self.assertNotEqual(saved_value, new_value)

    def test_functional_fast_api_get_raw_calibration(self):
        raw_calibration = self.device.get_raw_calibration()
        self.assertIsNotNone(raw_calibration)

    def test_functional_fast_api_get_calibration(self):
    
        depth_modes = [
            k4a.EDepthMode.NFOV_2X2BINNED,
            k4a.EDepthMode.NFOV_UNBINNED,
            k4a.EDepthMode.WFOV_2X2BINNED,
            k4a.EDepthMode.WFOV_UNBINNED,
            k4a.EDepthMode.PASSIVE_IR,
        ]

        color_resolutions = [
            k4a.EColorResolution.RES_3072P,
            k4a.EColorResolution.RES_2160P,
            k4a.EColorResolution.RES_1536P,
            k4a.EColorResolution.RES_1440P,
            k4a.EColorResolution.RES_1080P,
            k4a.EColorResolution.RES_720P,
        ]
        
        for depth_mode in depth_modes:
            for color_resolution in color_resolutions:
                with self.subTest(
                    depth_mode = depth_mode, 
                    color_resolution = color_resolution):
                    
                    calibration = self.device.get_calibration(
                        depth_mode,
                        color_resolution)
                        
                    self.assertIsNotNone(calibration)
                    self.assertIsInstance(calibration, k4a.Calibration)


class Test_Functional_API_Capture_AzureKinect(unittest.TestCase):
    '''Test Capture class for Azure Kinect device.
    '''

    @classmethod
    def setUpClass(cls):
        cls.device = k4a.Device.open()
        assert(cls.device is not None)

        cls.lock = test_config.glb_lock

    @classmethod
    def tearDownClass(cls):

        # Stop the cameras and imus before closing device.
        cls.device.stop_cameras()
        cls.device.stop_imu()
        cls.device.close()
        del cls.device

    def setUp(self):
        status = self.device.start_cameras(
            k4a.DEVICE_CONFIG_BGRA32_2160P_WFOV_UNBINNED_FPS15)
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        self.capture = self.device.get_capture(-1)
        self.assertIsNotNone(self.capture)

    def tearDown(self):
        self.device.stop_cameras()
        self.device.stop_imu()
        del self.capture

    def check_copy(self, image1:k4a.Image, image2:k4a.Image):
        
        # Check that the images are not the same instance.
        self.assertIsNot(image1, image2)

        # Check that the image contents are equal.
        if (image1.data.ndim == 3):
            self.assertEqual(image1.data[0, 0, 0], image2.data[0, 0, 0])
            self.assertEqual(image1.data[100, 100, 1], image2.data[100, 100, 1])
            self.assertEqual(image1.data[100, 50, 2], image2.data[100, 50, 2])
        elif (image1.data.ndim == 2):
            self.assertEqual(image1.data[0, 0], image2.data[0, 0])
            self.assertEqual(image1.data[100, 100], image2.data[100, 100])
            self.assertEqual(image1.data[100, 50], image2.data[100, 50])

        self.assertEqual(image1.image_format, image2.image_format)
        self.assertEqual(image1.size_bytes, image2.size_bytes)
        self.assertEqual(image1.width_pixels, image2.width_pixels)
        self.assertEqual(image1.height_pixels, image2.height_pixels)
        self.assertEqual(image1.stride_bytes, image2.stride_bytes)
        self.assertEqual(image1.device_timestamp_usec, image2.device_timestamp_usec)
        self.assertEqual(image1.system_timestamp_nsec, image2.system_timestamp_nsec)
        self.assertEqual(image1.exposure_usec, image2.exposure_usec)
        self.assertEqual(image1.white_balance, image2.white_balance)
        self.assertEqual(image1.iso_speed, image2.iso_speed)

    def test_functional_fast_api_capture_shallow_copy(self):
        capture2 = copy.copy(self.capture)

        # Check that the copy of the capture is not the same as the original.
        self.assertIsNotNone(capture2)
        self.assertIsNot(capture2, self.capture)

        # Check that the images are not the same as in the original.
        self.check_copy(capture2.color, self.capture.color)
        self.check_copy(capture2.depth, self.capture.depth)
        self.check_copy(capture2.ir, self.capture.ir)
        self.assertAlmostEqual(capture2.temperature, self.capture.temperature, 4)

        # Check that the image handles are the same.
        self.assertIs(capture2.color._image_handle, self.capture.color._image_handle)
        self.assertIs(capture2.depth._image_handle, self.capture.depth._image_handle)
        self.assertIs(capture2.ir._image_handle, self.capture.ir._image_handle)

        # Check that modifying one also modifies the other.
        self.capture.temperature = self.capture.temperature + 1
        self.assertEqual(capture2.temperature, self.capture.temperature)

        self.capture.color.white_balance = self.capture.color.white_balance + 1
        self.assertEqual(capture2.color.white_balance, self.capture.color.white_balance)

        self.capture.color.data[0, 0, 0] = self.capture.color.data[0, 0, 0] + 1
        self.assertEqual(capture2.color.data[0, 0, 0], self.capture.color.data[0, 0, 0])

        # Check that the copy of capture is still valid even if the original
        # capture is deleted. This is because the original capture's reference
        # count is increased when the copy is made.
        del capture2
        self.assertIsNotNone(self.capture)
        self.assertIsNotNone(self.capture.color)
        self.assertIsNotNone(self.capture.depth)
        self.assertIsNotNone(self.capture.ir)
        self.assertIsNotNone(self.capture.temperature)

    def test_functional_fast_api_capture_deep_copy(self):
        capture2 = copy.deepcopy(self.capture)

        # Check that the copy of the capture is not the same as the original.
        self.assertIsNotNone(capture2)
        self.assertIsNot(capture2, self.capture)

        # Check that the images are not the same as in the original.
        self.check_copy(capture2.color, self.capture.color)
        self.check_copy(capture2.depth, self.capture.depth)
        self.check_copy(capture2.ir, self.capture.ir)
        self.assertAlmostEqual(capture2.temperature, self.capture.temperature, 4)

        # Check that the image handles are not the same.
        self.assertIsNot(capture2.color._image_handle, self.capture.color._image_handle)
        self.assertIsNot(capture2.depth._image_handle, self.capture.depth._image_handle)
        self.assertIsNot(capture2.ir._image_handle, self.capture.ir._image_handle)

        # Check that modifying one does not modify the other.
        self.capture.temperature = self.capture.temperature + 1
        self.assertNotAlmostEqual(capture2.temperature, self.capture.temperature, 4)

        self.capture.color.white_balance = self.capture.color.white_balance + 1
        self.assertNotAlmostEqual(capture2.color.white_balance, self.capture.color.white_balance, 4)

        self.capture.color.data[0, 0, 0] = self.capture.color.data[0, 0, 0] + 1
        self.assertNotEqual(capture2.color.data[0, 0, 0], self.capture.color.data[0, 0, 0])

        # Check that the copy of capture is still valid even if the original
        # capture is deleted. This is because the original capture's reference
        # count is increased when the copy is made.
        del capture2
        self.assertIsNotNone(self.capture)
        self.assertIsNotNone(self.capture.color)
        self.assertIsNotNone(self.capture.depth)
        self.assertIsNotNone(self.capture.ir)
        self.assertIsNotNone(self.capture.temperature)

    def test_functional_fast_api_get_color(self):
        color = self.capture.color
        self.assertIsNotNone(color)
        self.assertEqual(color.width_pixels, 3840)
        self.assertEqual(color.height_pixels, 2160)
        self.assertEqual(color.image_format, k4a.EImageFormat.COLOR_BGRA32)

    def test_functional_fast_api_get_depth(self):
        depth = self.capture.depth
        self.assertIsNotNone(depth)
        self.assertEqual(depth.width_pixels, 1024)
        self.assertEqual(depth.height_pixels, 1024)
        self.assertEqual(depth.image_format, k4a.EImageFormat.DEPTH16)

    def test_functional_fast_api_get_ir(self):
        ir = self.capture.ir
        self.assertIsNotNone(ir)
        self.assertEqual(ir.width_pixels, 1024)
        self.assertEqual(ir.height_pixels, 1024)
        self.assertEqual(ir.image_format, k4a.EImageFormat.IR16)

    def test_functional_fast_api_set_color(self):

        color1 = self.capture.color
        self.assertIsNotNone(color1)
        self.assertEqual(color1.width_pixels, 3840)
        self.assertEqual(color1.height_pixels, 2160)
        self.assertEqual(color1.image_format, k4a.EImageFormat.COLOR_BGRA32)

        color2 = copy.deepcopy(color1)
        self.capture.color = color2

        color3 = self.capture.color
        self.assertIsNotNone(color3)
        self.assertEqual(color3.width_pixels, 3840)
        self.assertEqual(color3.height_pixels, 2160)
        self.assertEqual(color3.image_format, k4a.EImageFormat.COLOR_BGRA32)

        self.assertIsNot(color3, color1)
        self.assertIsNot(color2, color1)
        self.assertIs(color3, color2)


    def test_functional_fast_api_set_depth(self):

        depth1 = self.capture.depth
        self.assertIsNotNone(depth1)
        self.assertEqual(depth1.width_pixels, 1024)
        self.assertEqual(depth1.height_pixels, 1024)
        self.assertEqual(depth1.image_format, k4a.EImageFormat.DEPTH16)

        depth2 = copy.deepcopy(depth1)
        self.capture.depth = depth2

        depth3 = self.capture.depth
        self.assertIsNotNone(depth3)
        self.assertEqual(depth3.width_pixels, 1024)
        self.assertEqual(depth3.height_pixels, 1024)
        self.assertEqual(depth3.image_format, k4a.EImageFormat.DEPTH16)
        
        self.assertIsNot(depth3, depth1)
        self.assertIsNot(depth2, depth1)
        self.assertIs(depth3, depth2)

    def test_functional_fast_api_set_ir(self):
        ir1 = self.capture.ir
        self.assertIsNotNone(ir1)
        self.assertEqual(ir1.width_pixels, 1024)
        self.assertEqual(ir1.height_pixels, 1024)
        self.assertEqual(ir1.image_format, k4a.EImageFormat.IR16)

        ir2 = copy.deepcopy(ir1)
        self.capture.ir = ir2

        ir3 = self.capture.ir
        self.assertIsNotNone(ir3)
        self.assertEqual(ir3.width_pixels, 1024)
        self.assertEqual(ir3.height_pixels, 1024)
        self.assertEqual(ir3.image_format, k4a.EImageFormat.IR16)
        
        self.assertIsNot(ir3, ir1)
        self.assertIsNot(ir2, ir1)
        self.assertIs(ir3, ir2)


class Test_Functional_API_Image_AzureKinect(unittest.TestCase):
    '''Test Image class for Azure Kinect device.
    '''

    @classmethod
    def setUpClass(cls):
        cls.device = k4a.Device.open()
        assert(cls.device is not None)

        cls.lock = test_config.glb_lock

    @classmethod
    def tearDownClass(cls):

        # Stop the cameras and imus before closing device.
        cls.device.stop_cameras()
        cls.device.stop_imu()
        cls.device.close()
        del cls.device

    def setUp(self):
        status = self.device.start_cameras(
            k4a.DEVICE_CONFIG_BGRA32_2160P_WFOV_UNBINNED_FPS15)
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        self.capture = self.device.get_capture(-1)
        self.assertIsNotNone(self.capture)

        self.color = self.capture.color
        self.depth = self.capture.depth
        self.ir = self.capture.ir

    def tearDown(self):
        self.device.stop_cameras()
        self.device.stop_imu()

        del self.ir
        del self.depth
        del self.color
        del self.capture

    def check_copy(self, image1:k4a.Image, image2:k4a.Image):
        
        # Check that the images are not the same instance.
        self.assertIsNot(image1, image2)

        # Check that the image contents are equal.
        if (image1.data.ndim == 3):
            self.assertEqual(image1.data[0, 0, 0], image2.data[0, 0, 0])
            self.assertEqual(image1.data[100, 100, 1], image2.data[100, 100, 1])
            self.assertEqual(image1.data[100, 50, 2], image2.data[100, 50, 2])
        elif (image1.data.ndim == 2):
            self.assertEqual(image1.data[0, 0], image2.data[0, 0])
            self.assertEqual(image1.data[100, 100], image2.data[100, 100])
            self.assertEqual(image1.data[100, 50], image2.data[100, 50])

        self.assertEqual(image1.image_format, image2.image_format)
        self.assertEqual(image1.size_bytes, image2.size_bytes)
        self.assertEqual(image1.width_pixels, image2.width_pixels)
        self.assertEqual(image1.height_pixels, image2.height_pixels)
        self.assertEqual(image1.stride_bytes, image2.stride_bytes)
        self.assertEqual(image1.device_timestamp_usec, image2.device_timestamp_usec)
        self.assertEqual(image1.system_timestamp_nsec, image2.system_timestamp_nsec)
        self.assertEqual(image1.exposure_usec, image2.exposure_usec)
        self.assertEqual(image1.white_balance, image2.white_balance)
        self.assertEqual(image1.iso_speed, image2.iso_speed)

    def test_functional_fast_api_image_shallow_copy(self):
        color2 = copy.copy(self.color)
        depth2 = copy.copy(self.depth)
        ir2 = copy.copy(self.ir)

        # Check that the images are not the same as in the original.
        self.check_copy(self.color, color2)
        self.check_copy(self.depth, depth2)
        self.check_copy(self.ir, ir2)

        # Check that the image handles are the same.
        self.assertIs(color2._image_handle, self.color._image_handle)
        self.assertIs(depth2._image_handle, self.depth._image_handle)
        self.assertIs(ir2._image_handle, self.ir._image_handle)

        # Check that modifying one also modifies the other.
        self.color.white_balance = self.color.white_balance + 1
        self.assertEqual(color2.white_balance, self.color.white_balance)

        self.color.data[0, 0, 0] = self.color.data[0, 0, 0] + 1
        self.assertEqual(color2.data[0, 0, 0], self.color.data[0, 0, 0])

        # Check that the copy of capture is still valid even if the original
        # capture is deleted. This is because the original capture's reference
        # count is increased when the copy is made.
        del color2
        del depth2
        del ir2
        self.assertIsNotNone(self.color)
        self.assertIsNotNone(self.depth)
        self.assertIsNotNone(self.ir)

    def test_functional_fast_api_image_deep_copy(self):
        color2 = copy.deepcopy(self.color)
        depth2 = copy.deepcopy(self.depth)
        ir2 = copy.deepcopy(self.ir)

        # Check that the images are not the same as in the original.
        self.check_copy(self.color, color2)
        self.check_copy(self.depth, depth2)
        self.check_copy(self.ir, ir2)

        # Check that the image handles are the same.
        self.assertIsNot(color2._image_handle, self.color._image_handle)
        self.assertIsNot(depth2._image_handle, self.depth._image_handle)
        self.assertIsNot(ir2._image_handle, self.ir._image_handle)

        # Check that modifying one does not modifies the other.
        self.color.white_balance = self.color.white_balance + 1
        self.assertNotEqual(color2.white_balance, self.color.white_balance)

        self.color.data[0, 0, 0] = self.color.data[0, 0, 0] + 1
        self.assertNotEqual(color2.data[0, 0, 0], self.color.data[0, 0, 0])

        # Check that the copy of capture is still valid even if the original
        # capture is deleted. This is because the original capture's reference
        # count is increased when the copy is made.
        del color2
        del depth2
        del ir2
        self.assertIsNotNone(self.color)
        self.assertIsNotNone(self.depth)
        self.assertIsNotNone(self.ir)

    def test_functional_fast_api_get_data(self):
        data = self.color.data
        self.assertIsNotNone(data)
        self.assertIsInstance(data, np.ndarray)

    def test_functional_fast_api_get_image_format(self):
        image_format = self.color.image_format
        self.assertIsNotNone(image_format)
        self.assertIsInstance(image_format, k4a.EImageFormat)

    def test_functional_fast_api_get_size_bytes(self):
        size_bytes = self.color.size_bytes
        self.assertIsNotNone(size_bytes)
        self.assertIsInstance(size_bytes, int)
        self.assertNotEqual(size_bytes, 0)

    def test_functional_fast_api_get_width_pixels(self):
        width_pixels = self.color.width_pixels
        self.assertIsNotNone(width_pixels)
        self.assertIsInstance(width_pixels, int)
        self.assertNotEqual(width_pixels, 0)

    def test_functional_fast_api_get_height_pixels(self):
        height_pixels = self.color.height_pixels
        self.assertIsNotNone(height_pixels)
        self.assertIsInstance(height_pixels, int)
        self.assertNotEqual(height_pixels, 0)

    def test_functional_fast_api_get_stride_bytes(self):
        stride_bytes = self.color.stride_bytes
        self.assertIsNotNone(stride_bytes)
        self.assertIsInstance(stride_bytes, int)
        self.assertNotEqual(stride_bytes, 0)

    def test_functional_fast_api_get_device_timestamp_usec(self):
        device_timestamp_usec = self.color.device_timestamp_usec
        self.assertIsNotNone(device_timestamp_usec)
        self.assertIsInstance(device_timestamp_usec, int)

    def test_functional_fast_api_get_system_timestamp_nsec(self):
        system_timestamp_nsec = self.color.system_timestamp_nsec
        self.assertIsNotNone(system_timestamp_nsec)
        self.assertIsInstance(system_timestamp_nsec, int)

    def test_functional_fast_api_get_exposure_usec(self):
        exposure_usec = self.color.exposure_usec
        self.assertIsNotNone(exposure_usec)
        self.assertIsInstance(exposure_usec, int)

    def test_functional_fast_api_get_white_balance(self):
        white_balance = self.color.white_balance
        self.assertIsNotNone(white_balance)
        self.assertIsInstance(white_balance, int)

    def test_functional_fast_api_get_iso_speed(self):
        iso_speed = self.color.iso_speed
        self.assertIsNotNone(iso_speed)
        self.assertIsInstance(iso_speed, int)

    def test_functional_fast_api_set_device_timestamp_usec(self):
        self.color.device_timestamp_usec = 10
        device_timestamp_usec = self.color.device_timestamp_usec
        self.assertIsNotNone(device_timestamp_usec)
        self.assertIsInstance(device_timestamp_usec, int)
        self.assertEqual(device_timestamp_usec, 10)

    def test_functional_fast_api_set_system_timestamp_nsec(self):
        self.color.system_timestamp_nsec = 10
        system_timestamp_nsec = self.color.system_timestamp_nsec
        self.assertIsNotNone(system_timestamp_nsec)
        self.assertIsInstance(system_timestamp_nsec, int)
        self.assertEqual(system_timestamp_nsec, 10)

    def test_functional_fast_api_set_exposure_usec(self):
        self.color.exposure_usec = 10
        exposure_usec = self.color.exposure_usec
        self.assertIsNotNone(exposure_usec)
        self.assertIsInstance(exposure_usec, int)
        self.assertEqual(exposure_usec, 10)

    def test_functional_fast_api_set_white_balance(self):
        self.color.white_balance = 1000
        white_balance = self.color.white_balance
        self.assertIsNotNone(white_balance)
        self.assertIsInstance(white_balance, int)
        self.assertEqual(white_balance, 1000)

    def test_functional_fast_api_set_iso_speed(self):
        self.color.iso_speed = 100
        iso_speed = self.color.iso_speed
        self.assertIsNotNone(iso_speed)
        self.assertIsInstance(iso_speed, int)
        self.assertEqual(iso_speed, 100)


class Test_Functional_API_Calibration_AzureKinect(unittest.TestCase):
    '''Test Calibration class for Azure Kinect device.
    '''

    @classmethod
    def setUpClass(cls):
        cls.device = k4a.Device.open()
        assert(cls.device is not None)

        cls.lock = test_config.glb_lock

    @classmethod
    def tearDownClass(cls):

        # Stop the cameras and imus before closing device.
        cls.device.stop_cameras()
        cls.device.stop_imu()
        cls.device.close()
        del cls.device

    def test_functional_fast_api_get_calibration_from_device(self):
    
        depth_modes = [
            k4a.EDepthMode.NFOV_2X2BINNED,
            k4a.EDepthMode.NFOV_UNBINNED,
            k4a.EDepthMode.WFOV_2X2BINNED,
            k4a.EDepthMode.WFOV_UNBINNED,
            k4a.EDepthMode.PASSIVE_IR,
        ]

        color_resolutions = [
            k4a.EColorResolution.RES_3072P,
            k4a.EColorResolution.RES_2160P,
            k4a.EColorResolution.RES_1536P,
            k4a.EColorResolution.RES_1440P,
            k4a.EColorResolution.RES_1080P,
            k4a.EColorResolution.RES_720P,
        ]
        
        for depth_mode in depth_modes:
            for color_resolution in color_resolutions:
                with self.subTest(
                    depth_mode = depth_mode, 
                    color_resolution = color_resolution):
    
                    calibration = self.device.get_calibration(
                        depth_mode,
                        color_resolution)
                    self.assertIsNotNone(calibration)
                    self.assertIsInstance(calibration, k4a.Calibration)

    def test_functional_fast_api_get_calibration_from_raw(self):
        raw_calibration = self.device.get_raw_calibration()
        self.assertIsNotNone(raw_calibration)

        depth_modes = [
            k4a.EDepthMode.NFOV_2X2BINNED,
            k4a.EDepthMode.NFOV_UNBINNED,
            k4a.EDepthMode.WFOV_2X2BINNED,
            k4a.EDepthMode.WFOV_UNBINNED,
            k4a.EDepthMode.PASSIVE_IR,
        ]

        color_resolutions = [
            k4a.EColorResolution.RES_3072P,
            k4a.EColorResolution.RES_2160P,
            k4a.EColorResolution.RES_1536P,
            k4a.EColorResolution.RES_1440P,
            k4a.EColorResolution.RES_1080P,
            k4a.EColorResolution.RES_720P,
        ]
        
        for depth_mode in depth_modes:
            for color_resolution in color_resolutions:
                with self.subTest(
                    depth_mode = depth_mode, 
                    color_resolution = color_resolution):

                    calibration = k4a.Calibration.create_from_raw(
                        raw_calibration,
                        depth_mode,
                        color_resolution)

                    self.assertIsNotNone(calibration)
                    self.assertIsInstance(calibration, k4a.Calibration)


class Test_Functional_API_Transformation_AzureKinect(unittest.TestCase):
    '''Test Transformation class for Azure Kinect device.
    '''

    depth_modes = [
        k4a.EDepthMode.NFOV_2X2BINNED,
        k4a.EDepthMode.NFOV_UNBINNED,
        k4a.EDepthMode.WFOV_2X2BINNED,
        k4a.EDepthMode.WFOV_UNBINNED,
        k4a.EDepthMode.PASSIVE_IR,
    ]

    color_resolutions = [
        k4a.EColorResolution.RES_3072P,
        k4a.EColorResolution.RES_2160P,
        k4a.EColorResolution.RES_1536P,
        k4a.EColorResolution.RES_1440P,
        k4a.EColorResolution.RES_1080P,
        k4a.EColorResolution.RES_720P,
    ]

    calibration_types = [
        k4a.ECalibrationType.COLOR,
        k4a.ECalibrationType.DEPTH
    ]

    @classmethod
    def setUpClass(cls):
        cls.device = k4a.Device.open()
        assert(cls.device is not None)

        cls.lock = test_config.glb_lock

        cls.calibration = cls.device.get_calibration(
            k4a.EDepthMode.WFOV_UNBINNED,
            k4a.EColorResolution.RES_2160P)

    @classmethod
    def tearDownClass(cls):

        # Stop the cameras and imus before closing device.
        cls.device.stop_cameras()
        cls.device.stop_imu()
        cls.device.close()
        del cls.device
        del cls.calibration
        
    def test_functional_fast_api_point_3d_to_point_3d(self):

        depth_mode = k4a.EDepthMode.NFOV_2X2BINNED
        color_resolution = k4a.EColorResolution.RES_720P
        source_camera = k4a.ECalibrationType.COLOR
        target_camera = k4a.ECalibrationType.DEPTH

        # Get calibration.
        calibration = self.device.get_calibration(
            depth_mode,
            color_resolution)

        # Create transformation.
        transformation = k4a.Transformation(calibration)

        # Apply transformation.
        (x, y, z) = transformation.point_3d_to_point_3d(
            (300.0, 300.0, 500.0),
            source_camera,
            target_camera)

        self.assertIsNotNone(x)
        self.assertIsNotNone(y)
        self.assertIsNotNone(z)

    def test_functional_fast_api_pixel_2d_to_point_3d(self):

        depth_mode = k4a.EDepthMode.NFOV_2X2BINNED
        color_resolution = k4a.EColorResolution.RES_720P
        source_camera = k4a.ECalibrationType.COLOR
        target_camera = k4a.ECalibrationType.DEPTH

        # Get calibration.
        calibration = self.device.get_calibration(
            depth_mode,
            color_resolution)

        # Create transformation.
        transformation = k4a.Transformation(calibration)

        # Apply transformation.
        (x, y, z) = transformation.pixel_2d_to_point_3d(
            (300.0, 300.0),
            500.0,
            source_camera,
            target_camera)

        self.assertIsNotNone(x)
        self.assertIsNotNone(y)
        self.assertIsNotNone(z)

    def test_functional_fast_api_point_3d_to_pixel_2d(self):

        depth_mode = k4a.EDepthMode.NFOV_2X2BINNED
        color_resolution = k4a.EColorResolution.RES_720P
        source_camera = k4a.ECalibrationType.COLOR
        target_camera = k4a.ECalibrationType.DEPTH

        # Get calibration.
        calibration = self.device.get_calibration(
            depth_mode,
            color_resolution)

        # Create transformation.
        transformation = k4a.Transformation(calibration)

        # Apply transformation.
        (x, y) = transformation.point_3d_to_pixel_2d(
            (300.0, 300.0, 500.0),
            source_camera,
            target_camera)

        self.assertIsNotNone(x)
        self.assertIsNotNone(y)

    def test_functional_fast_api_pixel_2d_to_pixel_2d(self):

        depth_mode = k4a.EDepthMode.NFOV_2X2BINNED
        color_resolution = k4a.EColorResolution.RES_720P
        source_camera = k4a.ECalibrationType.COLOR
        target_camera = k4a.ECalibrationType.DEPTH

        # Get calibration.
        calibration = self.device.get_calibration(
            depth_mode,
            color_resolution)

        # Create transformation.
        transformation = k4a.Transformation(calibration)

        # Apply transformation.
        (x, y) = transformation.pixel_2d_to_pixel_2d(
            (300.0, 300.0),
            500.0,
            source_camera,
            target_camera)

        self.assertIsNotNone(x)
        self.assertIsNotNone(y)
    
    def test_functional_fast_api_color_2d_to_depth_2d(self):

        depth_mode = k4a.EDepthMode.NFOV_2X2BINNED
        color_resolution = k4a.EColorResolution.RES_720P
        source_camera = k4a.ECalibrationType.COLOR
        target_camera = k4a.ECalibrationType.DEPTH

        # Get a depth image.
        device_config = k4a.DeviceConfiguration(
            color_format = k4a.EImageFormat.COLOR_BGRA32,
            color_resolution = color_resolution,
            depth_mode = depth_mode,
            camera_fps = k4a.EFramesPerSecond.FPS_15,
            synchronized_images_only = True,
            depth_delay_off_color_usec = 0,
            wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
            subordinate_delay_off_master_usec = 0,
            disable_streaming_indicator = False
        )
        self.device.start_cameras(device_config)
        capture = self.device.get_capture(-1)
        self.device.stop_cameras()

        # Get calibration.
        calibration = self.device.get_calibration(
            depth_mode,
            color_resolution)

        # Create transformation.
        transformation = k4a.Transformation(calibration)

        # Apply transformation.
        (x, y) = transformation.color_2d_to_depth_2d(
            (capture.color.height_pixels/4, capture.color.width_pixels/4),
            capture.depth)

        self.assertIsNotNone(x)
        self.assertIsNotNone(y)

    def test_functional_fast_api_depth_image_to_color_camera(self):

        depth_mode = k4a.EDepthMode.NFOV_2X2BINNED
        color_resolution = k4a.EColorResolution.RES_720P

        # Get a depth image.
        device_config = k4a.DeviceConfiguration(
            color_format = k4a.EImageFormat.COLOR_BGRA32,
            color_resolution = color_resolution,
            depth_mode = depth_mode,
            camera_fps = k4a.EFramesPerSecond.FPS_15,
            synchronized_images_only = True,
            depth_delay_off_color_usec = 0,
            wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
            subordinate_delay_off_master_usec = 0,
            disable_streaming_indicator = False
        )
        self.device.start_cameras(device_config)
        capture = self.device.get_capture(-1)
        depth = capture.depth
        self.device.stop_cameras()
        del capture

        # Get calibration.
        calibration = self.device.get_calibration(
            depth_mode,
            color_resolution)

        # Create transformation.
        transformation = k4a.Transformation(calibration)

        # Apply transformation.
        transformed_depth = transformation.depth_image_to_color_camera(depth)
        
        self.assertIsNotNone(transformed_depth)

    def test_functional_fast_api_depth_image_to_color_camera_custom(self):

        depth_mode = k4a.EDepthMode.NFOV_2X2BINNED
        color_resolution = k4a.EColorResolution.RES_720P

        # Get a depth image.
        device_config = k4a.DeviceConfiguration(
            color_format = k4a.EImageFormat.COLOR_BGRA32,
            color_resolution = color_resolution,
            depth_mode = depth_mode,
            camera_fps = k4a.EFramesPerSecond.FPS_15,
            synchronized_images_only = True,
            depth_delay_off_color_usec = 0,
            wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
            subordinate_delay_off_master_usec = 0,
            disable_streaming_indicator = False
        )                             
        self.device.start_cameras(device_config)
        capture = self.device.get_capture(-1)
        depth = capture.depth
        self.device.stop_cameras()
        del capture

        # Create a custom image.
        custom = k4a.Image.create(
            k4a.EImageFormat.CUSTOM16,
            depth.width_pixels,
            depth.height_pixels,
            depth.width_pixels * 2)

        # Get calibration.
        calibration = self.device.get_calibration(
            depth_mode,
            color_resolution)

        # Create transformation.
        transformation = k4a.Transformation(calibration)

        # Apply transformation.
        (transformed_depth, transformed_custom) = \
            transformation.depth_image_to_color_camera_custom(
                depth,
                custom,
                k4a.ETransformInterpolationType.LINEAR,
                0)
        
        self.assertIsNotNone(transformed_depth)
        self.assertIsNotNone(transformed_custom)

    def test_functional_fast_api_color_image_to_depth_camera(self):

        depth_mode = k4a.EDepthMode.NFOV_2X2BINNED
        color_resolution = k4a.EColorResolution.RES_720P

        # Get a depth and color image.
        device_config = k4a.DeviceConfiguration(
            color_format = k4a.EImageFormat.COLOR_BGRA32,
            color_resolution = color_resolution,
            depth_mode = depth_mode,
            camera_fps = k4a.EFramesPerSecond.FPS_15,
            synchronized_images_only = True,
            depth_delay_off_color_usec = 0,
            wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
            subordinate_delay_off_master_usec = 0,
            disable_streaming_indicator = False
        )
        self.device.start_cameras(device_config)
        capture = self.device.get_capture(-1)
        depth = capture.depth
        color = capture.color
        self.device.stop_cameras()

        # Get calibration.
        calibration = self.device.get_calibration(
            depth_mode,
            color_resolution)

        # Create transformation.
        transformation = k4a.Transformation(calibration)

        # Apply transformation.
        transformed_color = transformation.color_image_to_depth_camera(
            depth,
            color)
        
        self.assertIsNotNone(transformed_color)

    def test_functional_fast_api_depth_image_to_point_cloud(self):

        depth_mode = k4a.EDepthMode.NFOV_2X2BINNED
        color_resolution = k4a.EColorResolution.RES_720P

        # Get a depth image.
        device_config = k4a.DeviceConfiguration(
            color_format = k4a.EImageFormat.COLOR_BGRA32,
            color_resolution = color_resolution,
            depth_mode = depth_mode,
            camera_fps = k4a.EFramesPerSecond.FPS_15,
            synchronized_images_only = True,
            depth_delay_off_color_usec = 0,
            wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
            subordinate_delay_off_master_usec = 0,
            disable_streaming_indicator = False
        )
        self.device.start_cameras(device_config)
        capture = self.device.get_capture(-1)
        depth = capture.depth
        self.device.stop_cameras()
        del capture

        # Get calibration.
        calibration = self.device.get_calibration(
            depth_mode,
            color_resolution)

        # Create transformation.
        transformation = k4a.Transformation(calibration)

        # Apply transformation.
        point_cloud = transformation.depth_image_to_point_cloud(
            depth,
            k4a.ECalibrationType.DEPTH)
        
        self.assertIsNotNone(point_cloud)    
    
    #
    # The following tests may take a long time. 
    # It is not recommended to run them frequently.
    #

    def test_functional_api_point_3d_to_point_3d(self):

        for depth_mode in Test_Transformation_AzureKinect.depth_modes:
            for color_resolution in Test_Transformation_AzureKinect.color_resolutions:
                for source_camera in Test_Transformation_AzureKinect.calibration_types:
                    for target_camera in Test_Transformation_AzureKinect.calibration_types:
                        with self.subTest(depth_mode = depth_mode, 
                            color_resolution = color_resolution,
                            source_camera = source_camera,
                            target_camera = target_camera):

                            # Get calibration.
                            calibration = self.device.get_calibration(
                                depth_mode,
                                color_resolution)

                            # Create transformation.
                            transformation = k4a.Transformation(calibration)

                            # Apply transformation.
                            (x, y, z) = transformation.point_3d_to_point_3d(
                                (300.0, 300.0, 500.0),
                                source_camera,
                                target_camera)

                            self.assertIsNotNone(x)
                            self.assertIsNotNone(y)
                            self.assertIsNotNone(z)

    def test_functional_api_pixel_2d_to_point_3d(self):

        for depth_mode in Test_Transformation_AzureKinect.depth_modes:
            for color_resolution in Test_Transformation_AzureKinect.color_resolutions:
                for source_camera in Test_Transformation_AzureKinect.calibration_types:
                    for target_camera in Test_Transformation_AzureKinect.calibration_types:
                        with self.subTest(depth_mode = depth_mode, 
                            color_resolution = color_resolution,
                            source_camera = source_camera,
                            target_camera = target_camera):

                            # Get calibration.
                            calibration = self.device.get_calibration(
                                depth_mode,
                                color_resolution)

                            # Create transformation.
                            transformation = k4a.Transformation(calibration)

                            # Apply transformation.
                            (x, y, z) = transformation.pixel_2d_to_point_3d(
                                (300.0, 300.0),
                                500.0,
                                source_camera,
                                target_camera)

                            self.assertIsNotNone(x)
                            self.assertIsNotNone(y)
                            self.assertIsNotNone(z)

    def test_functional_api_point_3d_to_pixel_2d(self):

        for depth_mode in Test_Transformation_AzureKinect.depth_modes:
            for color_resolution in Test_Transformation_AzureKinect.color_resolutions:
                for source_camera in Test_Transformation_AzureKinect.calibration_types:
                    for target_camera in Test_Transformation_AzureKinect.calibration_types:
                        with self.subTest(depth_mode = depth_mode, 
                            color_resolution = color_resolution,
                            source_camera = source_camera,
                            target_camera = target_camera):

                            # Get calibration.
                            calibration = self.device.get_calibration(
                                depth_mode,
                                color_resolution)

                            # Create transformation.
                            transformation = k4a.Transformation(calibration)

                            # Apply transformation.
                            (x, y) = transformation.point_3d_to_pixel_2d(
                                (300.0, 300.0, 500.0),
                                source_camera,
                                target_camera)

                            self.assertIsNotNone(x)
                            self.assertIsNotNone(y)

    def test_functional_api_pixel_2d_to_pixel_2d(self):

        for depth_mode in Test_Transformation_AzureKinect.depth_modes:
            for color_resolution in Test_Transformation_AzureKinect.color_resolutions:
                for source_camera in Test_Transformation_AzureKinect.calibration_types:
                    for target_camera in Test_Transformation_AzureKinect.calibration_types:
                        with self.subTest(depth_mode = depth_mode, 
                            color_resolution = color_resolution,
                            source_camera = source_camera,
                            target_camera = target_camera):

                            # Get calibration.
                            calibration = self.device.get_calibration(
                                depth_mode,
                                color_resolution)

                            # Create transformation.
                            transformation = k4a.Transformation(calibration)

                            # Apply transformation.
                            (x, y) = transformation.pixel_2d_to_pixel_2d(
                                (300.0, 300.0),
                                500.0,
                                source_camera,
                                target_camera)

                            self.assertIsNotNone(x)
                            self.assertIsNotNone(y)
    
    def test_functional_api_color_2d_to_depth_2d(self):

        for depth_mode in Test_Transformation_AzureKinect.depth_modes[:4]:
            for color_resolution in Test_Transformation_AzureKinect.color_resolutions:
                for source_camera in Test_Transformation_AzureKinect.calibration_types:
                    for target_camera in Test_Transformation_AzureKinect.calibration_types:
                        with self.subTest(depth_mode = depth_mode, 
                            color_resolution = color_resolution,
                            source_camera = source_camera,
                            target_camera = target_camera):

                            # Get a depth image.
                            device_config = k4a.DeviceConfiguration(
                                color_format = k4a.EImageFormat.COLOR_BGRA32,
                                color_resolution = color_resolution,
                                depth_mode = depth_mode,
                                camera_fps = k4a.EFramesPerSecond.FPS_15,
                                synchronized_images_only = True,
                                depth_delay_off_color_usec = 0,
                                wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
                                subordinate_delay_off_master_usec = 0,
                                disable_streaming_indicator = False
                            )
                            self.device.start_cameras(device_config)
                            capture = self.device.get_capture(-1)
                            self.device.stop_cameras()

                            # Get calibration.
                            calibration = self.device.get_calibration(
                                depth_mode,
                                color_resolution)

                            # Create transformation.
                            transformation = k4a.Transformation(calibration)

                            # Apply transformation.
                            (x, y) = transformation.color_2d_to_depth_2d(
                                (capture.color.height_pixels/4, capture.color.width_pixels/4),
                                capture.depth)

                            self.assertIsNotNone(x)
                            self.assertIsNotNone(y)

    def test_functional_api_depth_image_to_color_camera(self):

        for depth_mode in Test_Transformation_AzureKinect.depth_modes[:4]:
            for color_resolution in Test_Transformation_AzureKinect.color_resolutions:
                with self.subTest(depth_mode = depth_mode, 
                    color_resolution = color_resolution):

                    # Get a depth image.
                    device_config = k4a.DeviceConfiguration(
                        color_format = k4a.EImageFormat.COLOR_BGRA32,
                        color_resolution = color_resolution,
                        depth_mode = depth_mode,
                        camera_fps = k4a.EFramesPerSecond.FPS_15,
                        synchronized_images_only = True,
                        depth_delay_off_color_usec = 0,
                        wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
                        subordinate_delay_off_master_usec = 0,
                        disable_streaming_indicator = False
                    )
                    self.device.start_cameras(device_config)
                    capture = self.device.get_capture(-1)
                    depth = capture.depth
                    self.device.stop_cameras()
                    del capture

                    # Get calibration.
                    calibration = self.device.get_calibration(
                        depth_mode,
                        color_resolution)

                    # Create transformation.
                    transformation = k4a.Transformation(calibration)

                    # Apply transformation.
                    transformed_depth = transformation.depth_image_to_color_camera(depth)
                    
                    self.assertIsNotNone(transformed_depth)

    def test_functional_api_depth_image_to_color_camera_custom(self):

        for depth_mode in Test_Transformation_AzureKinect.depth_modes[:4]:
            for color_resolution in Test_Transformation_AzureKinect.color_resolutions:
                with self.subTest(depth_mode = depth_mode, 
                    color_resolution = color_resolution):

                    # Get a depth image.
                    device_config = k4a.DeviceConfiguration(
                        color_format = k4a.EImageFormat.COLOR_BGRA32,
                        color_resolution = color_resolution,
                        depth_mode = depth_mode,
                        camera_fps = k4a.EFramesPerSecond.FPS_15,
                        synchronized_images_only = True,
                        depth_delay_off_color_usec = 0,
                        wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
                        subordinate_delay_off_master_usec = 0,
                        disable_streaming_indicator = False
                    )                             
                    self.device.start_cameras(device_config)
                    capture = self.device.get_capture(-1)
                    depth = capture.depth
                    self.device.stop_cameras()
                    del capture

                    # Create a custom image.
                    custom = k4a.Image.create(
                        k4a.EImageFormat.CUSTOM16,
                        depth.width_pixels,
                        depth.height_pixels,
                        depth.width_pixels * 2)

                    # Get calibration.
                    calibration = self.device.get_calibration(
                        depth_mode,
                        color_resolution)

                    # Create transformation.
                    transformation = k4a.Transformation(calibration)

                    # Apply transformation.
                    (transformed_depth, transformed_custom) = \
                        transformation.depth_image_to_color_camera_custom(
                            depth,
                            custom,
                            k4a.ETransformInterpolationType.LINEAR,
                            0)
                    
                    self.assertIsNotNone(transformed_depth)
                    self.assertIsNotNone(transformed_custom)

    def test_functional_api_color_image_to_depth_camera(self):

        for depth_mode in Test_Transformation_AzureKinect.depth_modes[:4]:
            for color_resolution in Test_Transformation_AzureKinect.color_resolutions:
                with self.subTest(depth_mode = depth_mode, 
                    color_resolution = color_resolution):

                    # Get a depth and color image.
                    device_config = k4a.DeviceConfiguration(
                        color_format = k4a.EImageFormat.COLOR_BGRA32,
                        color_resolution = color_resolution,
                        depth_mode = depth_mode,
                        camera_fps = k4a.EFramesPerSecond.FPS_15,
                        synchronized_images_only = True,
                        depth_delay_off_color_usec = 0,
                        wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
                        subordinate_delay_off_master_usec = 0,
                        disable_streaming_indicator = False
                    )
                    self.device.start_cameras(device_config)
                    capture = self.device.get_capture(-1)
                    depth = capture.depth
                    color = capture.color
                    self.device.stop_cameras()

                    # Get calibration.
                    calibration = self.device.get_calibration(
                        depth_mode,
                        color_resolution)

                    # Create transformation.
                    transformation = k4a.Transformation(calibration)

                    # Apply transformation.
                    transformed_color = transformation.color_image_to_depth_camera(
                        depth,
                        color)
                    
                    self.assertIsNotNone(transformed_color)

    def test_functional_api_depth_image_to_point_cloud(self):

        for depth_mode in Test_Transformation_AzureKinect.depth_modes[:4]:
            for color_resolution in Test_Transformation_AzureKinect.color_resolutions:
                with self.subTest(depth_mode = depth_mode, 
                    color_resolution = color_resolution):

                    # Get a depth image.
                    device_config = k4a.DeviceConfiguration(
                        color_format = k4a.EImageFormat.COLOR_BGRA32,
                        color_resolution = color_resolution,
                        depth_mode = depth_mode,
                        camera_fps = k4a.EFramesPerSecond.FPS_15,
                        synchronized_images_only = True,
                        depth_delay_off_color_usec = 0,
                        wired_sync_mode = k4a.EWiredSyncMode.STANDALONE,
                        subordinate_delay_off_master_usec = 0,
                        disable_streaming_indicator = False
                    )
                    self.device.start_cameras(device_config)
                    capture = self.device.get_capture(-1)
                    depth = capture.depth
                    self.device.stop_cameras()
                    del capture

                    # Get calibration.
                    calibration = self.device.get_calibration(
                        depth_mode,
                        color_resolution)

                    # Create transformation.
                    transformation = k4a.Transformation(calibration)

                    # Apply transformation.
                    point_cloud = transformation.depth_image_to_point_cloud(
                        depth,
                        k4a.ECalibrationType.DEPTH)
                    
                    self.assertIsNotNone(point_cloud)

if __name__ == '__main__':
    unittest.main()