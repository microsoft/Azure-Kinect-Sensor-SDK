'''
test_device_azurekinect.py

Tests for the Device class for Azure Kinect device.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import unittest
from threading import Lock
import copy
import matplotlib.pyplot as plt
from time import sleep

import k4a


class Test_Device_AzureKinect(unittest.TestCase):
    '''Test Device class for Azure Kinect device.
    '''

    @classmethod
    def setUpClass(cls):
        cls.device = k4a.Device.open()
        assert(cls.device is not None)

        cls.lock = Lock()

    @classmethod
    def tearDownClass(cls):

        # Stop the cameras and imus before closing device.
        cls.device.stop_cameras()
        cls.device.stop_imu()
        cls.device.close()
        del cls.device

    def test_open_twice_expected_fail(self):
        device2 = k4a.Device.open()
        self.assertIsNone(device2)

        device2 = k4a.Device.open(1000000)
        self.assertIsNone(device2)

    def test_get_serial_number(self):
        serial_number = self.device.serial_number
        self.assertIsInstance(serial_number, str)
        self.assertGreater(len(serial_number), 0)

    # Helper method for test_set_serial_number().
    @staticmethod
    def set_serial_number(device:k4a.Device, serial_number:str):
        device.serial_number = serial_number

    def test_set_serial_number(self):
        self.assertRaises(AttributeError, 
            Test_Device_AzureKinect.set_serial_number, 
            self.device, "not settable")

    def test_get_capture(self):

        # Start the cameras.
        status = self.device.start_cameras(
            k4a.DEVICE_CONFIG_BGRA32_4K_WFOV_UNBINNED)
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        # Get a capture, waiting indefinitely.
        capture = self.device.get_capture(-1)
        self.assertIsNotNone(capture)

        # Stop the cameras.
        self.device.stop_cameras()

    def test_get_serial_number(self):
        pass

    def test_get_hardware_version(self):
        pass

    def test_get_color_ctrl_cap(self):
        pass

    def test_get_sync_out_connected(self):
        pass

    def test_get_sync_in_connected(self):
        pass

    def test_start_stop_cameras(self):
        pass

    def test_start_stop_imu(self):
        pass

    def test_get_color_control(self):
        pass

    def test_set_color_control(self):
        pass

    def test_get_raw_calibration(self):
        pass

    def test_get_calibration(self):
        pass


class Test_Capture_AzureKinect(unittest.TestCase):
    '''Test Capture class for Azure Kinect device.
    '''

    @classmethod
    def setUpClass(cls):
        cls.device = k4a.Device.open()
        assert(cls.device is not None)

        cls.lock = Lock()

    @classmethod
    def tearDownClass(cls):

        # Stop the cameras and imus before closing device.
        cls.device.stop_cameras()
        cls.device.stop_imu()
        cls.device.close()
        del cls.device

    def setUp(self):
        status = self.device.start_cameras(
            k4a.DEVICE_CONFIG_BGRA32_4K_WFOV_UNBINNED)
        self.assertEqual(status, k4a.EStatus.SUCCEEDED)

        self.capture = self.device.get_capture(-1)
        self.assertIsNotNone(self.capture)

    def tearDown(self):
        self.device.stop_cameras()
        self.device.stop_imu()
        del self.capture

    def check_copy(self, image1, image2):
        
        # Check that the images are not the same instance.
        self.assertIsNot(image1, image2)
        self.assertIsNot(image1._image_handle, image2._image_handle)

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

    def test_shallow_copy(self):
        capture2 = copy.copy(self.capture)

        # Check that the copy of the capture is not the same as the original.
        self.assertIsNotNone(capture2)
        self.assertIsNot(capture2, self.capture)

        # Check that the images are not the same as in the original.
        self.check_copy(capture2.color, self.capture.color)
        self.check_copy(capture2.depth, self.capture.depth)
        self.check_copy(capture2.ir, self.capture.ir)
        self.assertAlmostEqual(capture2.temperature, self.capture.temperature, 4)

        # Check that modifying one also modifies the other.
        self.capture.temperature = self.capture.temperature + 1
        self.assertAlmostEqual(capture2.temperature, self.capture.temperature, 4)

        self.capture.color.white_balance = self.capture.color.white_balance + 1
        self.assertAlmostEqual(capture2.color.white_balance, self.capture.color.white_balance, 4)

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

    def test_deep_copy(self):
        capture2 = copy.deepcopy(self.capture)

        # Check that the copy of the capture is not the same as the original.
        self.assertIsNotNone(capture2)
        self.assertIsNot(capture2, self.capture)

        # Check that the images are not the same as in the original.
        self.check_copy(capture2.color, self.capture.color)
        self.check_copy(capture2.depth, self.capture.depth)
        self.check_copy(capture2.ir, self.capture.ir)
        self.assertAlmostEqual(capture2.temperature, self.capture.temperature, 4)

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

    def test_get_color(self):
        color = self.capture.color
        self.assertIsNotNone(color)
        self.assertEqual(color.width_pixels, 3840)
        self.assertEqual(color.height_pixels, 2160)
        self.assertEqual(color.image_format, k4a.EImageFormat.COLOR_BGRA32)

    def test_get_depth(self):
        depth = self.capture.depth
        self.assertIsNotNone(depth)
        self.assertEqual(depth.width_pixels, 512)
        self.assertEqual(depth.height_pixels, 512)
        self.assertEqual(depth.image_format, k4a.EImageFormat.DEPTH16)

    def test_get_ir(self):
        ir = self.capture.ir
        self.assertIsNotNone(ir)
        self.assertEqual(ir.width_pixels, 512)
        self.assertEqual(ir.height_pixels, 512)
        self.assertEqual(ir.image_format, k4a.EImageFormat.IR16)

    def test_set_color(self):
        pass

    def test_set_depth(self):
        pass

    def test_set_ir(self):
        pass


class Test_Image_AzureKinect(unittest.TestCase):
    '''Test Image class for Azure Kinect device.
    '''

    @classmethod
    def setUpClass(cls):
        cls.device = k4a.Device.open()
        assert(cls.device is not None)

        cls.lock = Lock()

    @classmethod
    def tearDownClass(cls):

        # Stop the cameras and imus before closing device.
        cls.device.stop_cameras()
        cls.device.stop_imu()
        cls.device.close()
        del cls.device

    def setUp(self):
        status = self.device.start_cameras(
            k4a.DEVICE_CONFIG_BGRA32_4K_WFOV_UNBINNED)
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

    def test_shallow_copy(self):
        pass

    def test_deep_copy(self):
        pass

    def test_get_data(self):
        pass

    def test_get_image_format(self):
        pass

    def test_get_size_bytes(self):
        pass

    def test_get_width_pixels(self):
        pass

    def test_get_height_pixels(self):
        pass

    def test_get_stride_bytes(self):
        pass

    def test_get_device_timestamp_usec(self):
        pass

    def test_get_system_timestamp_nsec(self):
        pass

    def test_get_exposure_usec(self):
        pass

    def test_get_white_balance(self):
        pass

    def test_get_iso_speed(self):
        pass

    def test_set_device_timestamp_usec(self):
        pass

    def test_set_system_timestamp_nsec(self):
        pass

    def test_set_exposure_usec(self):
        pass

    def test_set_white_balance(self):
        pass

    def test_set_iso_speed(self):
        pass
