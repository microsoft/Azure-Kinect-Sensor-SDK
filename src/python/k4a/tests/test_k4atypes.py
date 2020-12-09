'''Tests for the k4a types and enums.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import unittest
    
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

    def test_k4a_result_t(self):
        enum_name = k4a.k4a_result_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_buffer_result_t(self):
        enum_name = k4a.k4a_buffer_result_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_wait_result_t(self):
        enum_name = k4a.k4a_wait_result_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_log_level_t(self):
        enum_name = k4a.k4a_log_level_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_depth_mode_t(self):
        enum_name = k4a.k4a_depth_mode_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_color_resolution_t(self):
        enum_name = k4a.k4a_color_resolution_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_image_format_t(self):
        enum_name = k4a.k4a_image_format_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_transformation_interpolation_type_t(self):
        enum_name = k4a.k4a_transformation_interpolation_type_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_fps_t(self):
        enum_name = k4a.k4a_fps_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_color_control_command_t(self):
        enum_name = k4a.k4a_color_control_command_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_color_control_mode_t(self):
        enum_name = k4a.k4a_color_control_mode_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_wired_sync_mode_t(self):
        enum_name = k4a.k4a_wired_sync_mode_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_calibration_type_t(self):
        enum_name = k4a.k4a_calibration_type_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_calibration_model_type_t(self):
        enum_name = k4a.k4a_calibration_model_type_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_firmware_build_t(self):
        enum_name = k4a.k4a_firmware_build_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

    def test_k4a_firmware_signature_t(self):
        enum_name = k4a.k4a_firmware_signature_t
        enum_values = get_enum_values(len(enum_name))
        enum_list = list(enum_name)
        for e in enum_list:
            self.assertEqual(e.value, next(enum_values))

if __name__ == '__main__':
    unittest.main()