"""
test_create_image.py

Tests for the creating Image objects using pre-allocated arrays.

"""

import numpy as np
from numpy.random import default_rng
import unittest

import k4a


class TestImages(unittest.TestCase):
    """Test allocation of images.
    """

    @classmethod
    def setUpClass(cls):
        pass

    @classmethod
    def tearDownClass(cls):
        pass

    def test_unit_create_from_ndarray(self):
        # from inspect import currentframe, getframeinfo    # For debugging if needed

        # Create an image with the default memory allocation and free
        an_image = k4a.Image.create(k4a.EImageFormat.CUSTOM16, 640, 576, 640*2)
        self.assertEqual(an_image.height_pixels, 576)
        del an_image    # this will call the default image_release function

        for index in range(5):
            # We want a diverse source for the values, but also we want it to be deterministic
            seed = 12345 + index
            rng = default_rng(seed=seed)
            two_channel_uint16 = rng.integers(0, high=65536, size=(576, 640), dtype=np.uint16)
            # Uncomment these two lines to change the image buffer to show that the verification below works
            # two_channel_uint16[0, 0] = (0x12 << 8) + 0x34
            # two_channel_uint16[0, 1] = (0x56 << 8) + 0x78

            custom_image = k4a.Image.create_from_ndarray(k4a.EImageFormat.CUSTOM16, two_channel_uint16)
            two_channel_uint16 = rng.integers(0, high=32767, size=(576, 640), dtype=np.uint16)
            # Ensure that the original array is no longer known locally, but the data is still available through custom_image
            self.assertFalse(np.all(two_channel_uint16 == custom_image.data))
            del two_channel_uint16

            # Regenerate the values of the custom_image using the same seed for comparison
            rng = default_rng(seed=seed)
            verify_two_channel_uint16 = rng.integers(0, high=65536, size=(576, 640), dtype=np.uint16)

            self.assertTrue(np.all(verify_two_channel_uint16 == custom_image.data))
            del custom_image


if __name__ == '__main__':
    unittest.main()
