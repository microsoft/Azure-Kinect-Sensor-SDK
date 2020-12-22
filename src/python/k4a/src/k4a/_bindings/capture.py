'''
capture.py

Defines a Capture class that is a container for a single capture of data
from an Azure Kinect device.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import ctypes as _ctypes
import copy as _copy

from .k4atypes import _CaptureHandle, EStatus, _ImageHandle

from .k4a import k4a_capture_create, k4a_capture_release, k4a_capture_reference, \
    k4a_capture_get_color_image, k4a_capture_set_color_image, \
    k4a_capture_get_depth_image, k4a_capture_set_depth_image, \
    k4a_capture_get_ir_image, k4a_capture_set_ir_image, \
    k4a_capture_get_temperature_c, k4a_capture_set_temperature_c

from .image import Image

class Capture:

    def __init__(self, capture_handle:_CaptureHandle=None):
        self._capture_handle = capture_handle
        self._color = None
        self._depth = None
        self._ir = None
        self._temperature = None

    @staticmethod
    def create():
        capture = None

        # Create a capture.
        capture_handle = _CaptureHandle()
        status = k4a_capture_create(_ctypes.byref(capture_handle))

        if status == EStatus.SUCCEEDED:
            capture = Capture(capture_handle=capture_handle)

        return capture

    def _release(self):
        k4a_capture_release(self._capture_handle)

    def _reference(self):
        k4a_capture_reference(self._capture_handle)

    def __copy__(self):
         # Create a shallow copy.
        new_capture = Capture(self._capture_handle)
        new_capture.color = _copy.copy(self.color)
        new_capture.depth = _copy.copy(self.depth)
        new_capture.ir = _copy.copy(self.ir)
        new_capture.temperature = _copy.copy(self.temperature)

        # Update reference count.
        new_capture._reference()

        return new_capture

    def __deepcopy__(self, memo):
        
        # Create a new capture.
        new_capture = Capture.create()

        # Deep copy the images.
        new_capture.color = _copy.deepcopy(self.color, memo)
        new_capture.depth = _copy.deepcopy(self.depth, memo)
        new_capture.ir = _copy.deepcopy(self.ir, memo)

        new_capture.temperature = self.temperature

        # Since it is a completely different capture, there is no need to
        # increment the reference count. Just return the deep copy.
        return new_capture
    
    def __enter__(self):
        return self

    def __exit__(self):
        del self

    def __del__(self):
        # Release the handle first.
        self._release()

        del self._capture_handle
        del self._color
        del self._depth
        del self._ir
        del self._temperature

        self._capture_handle = None
        self._color = None
        self._depth = None
        self._ir = None
        self._temperature = None

    # Define properties and get/set functions. ############### 
    @property
    def color(self):
        image_handle = k4a_capture_get_color_image(self._capture_handle)
        self._color = Image._create_from_existing_image_handle(image_handle)
        return self._color

    @color.setter
    def color(self, image:Image):
        if image is not None and isinstance(image, Image):
            del self._color

            k4a_capture_set_color_image(
                self._capture_handle, 
                image._image_handle)

            self._color = image

    @color.deleter
    def color(self):
        del self._color

    @property
    def depth(self):
        image_handle = k4a_capture_get_depth_image(self._capture_handle)
        self._depth = Image._create_from_existing_image_handle(image_handle)
        return self._depth

    @depth.setter
    def depth(self, image:Image):
        if image is not None and isinstance(image, Image):
            del self._depth

            k4a_capture_set_depth_image(
                self._capture_handle, 
                image._image_handle)

            self._depth = image

    @depth.deleter
    def depth(self):
        del self._depth

    @property
    def ir(self):
        image_handle = k4a_capture_get_ir_image(self._capture_handle)
        self._ir = Image._create_from_existing_image_handle(image_handle)
        return self._ir

    @ir.setter
    def ir(self, image:Image):
        if image is not None and isinstance(image, Image):
            del self._ir

            k4a_capture_set_ir_image(
                self._capture_handle, 
                image._image_handle)

            self._ir = image

    @ir.deleter
    def ir(self):
        del self._ir

    @property
    def temperature(self):
        self._temperature = k4a_capture_get_temperature_c(self._capture_handle)
        return self._temperature

    @temperature.setter
    def temperature(self, temperature:float):
        if temperature is not None and isinstance(temperature, float):
            del self._temperature

            k4a_capture_set_temperature_c(
                self._capture_handle, 
                _ctypes.c_float(temperature))

            self._temperature = temperature

    @temperature.deleter
    def temperature(self):
        del self._temperature
    # ###############