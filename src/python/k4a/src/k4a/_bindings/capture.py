'''!
@file capture.py

Defines a Capture class that is a container for a single capture of data
from an Azure Kinect device.

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
Kinect For Azure SDK.
'''

import ctypes as _ctypes
import copy as _copy
from os import linesep as _newline

from .k4atypes import _CaptureHandle, EStatus, _ImageHandle

from .k4a import k4a_capture_create, k4a_capture_release, k4a_capture_reference, \
    k4a_capture_get_color_image, k4a_capture_set_color_image, \
    k4a_capture_get_depth_image, k4a_capture_set_depth_image, \
    k4a_capture_get_ir_image, k4a_capture_set_ir_image, \
    k4a_capture_get_temperature_c, k4a_capture_set_temperature_c

from .image import Image

class Capture:
    '''! A class that represents a capture from an Azure Kinect device.

    Property Name | Type  | R/W | Description
    ------------- | ----- | --- | -----------------------------------------
    color         | Image | R/W | The color image container.
    depth         | Image | R/W | The depth image container.
    ir            | Image | R/W | The IR image container.
    temperature   | float | R/W | The temperature

    @remarks 
    - A capture represents a set of images that were captured by a 
        device at approximately the same time. A capture may have a color, IR,
        and depth image. A capture may have up to one image of each type. A 
        capture may have no image for a given type as well.
    
    @remarks 
    - Do not use the Capture() constructor to get a Capture instance. It
        will return an object that does not have a handle to the capture
        resources held by the SDK. Instead, use the create() function.

    @remarks 
    - Captures also store a temperature value which represents the 
        temperature of the device at the time of the capture.

    @remarks 
    - While all the images associated with the capture were collected at
        approximately the same time, each image has an individual timestamp 
        which may differ from each other. If the device was configured to 
        capture depth and color images separated by a delay, 
        Device.get_capture() will return a capture containing both image types 
        separated by the configured delay.

    @remarks 
    - Empty captures are created with create().

    @remarks 
    - Captures can be obtained from a Device object using get_capture().

    @remarks 
    - A Capture object may be copied or deep copied. A shallow copy
        shares the same images as the original, and any changes in one will
        affect the other. A deep copy does not share any resources with the
        original, and changes in one will not affect the other. In both shallow
        and deep copies, deleting one will have no effects on the other.
    '''

    def __init__(self, capture_handle:_CaptureHandle):
        self._capture_handle = capture_handle
        self._color = None
        self._depth = None
        self._ir = None
        self._temperature = None

    @staticmethod
    def create():
        '''! Create an empty capture object.
                
        @returns Capture instance with empty contents.
        
        @remarks 
        - If unsuccessful, None is returned.
        '''

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

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

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

    def __str__(self):
        return ''.join([
            'color=%s, ', _newline,
            'depth=%s, ', _newline,
            'ir=%s, ', _newline,
            'temperature_C=%f, ']) % (
            self.color.__str__(),
            self.depth.__str__(),
            self.ir.__str__(),
            self.temperature)

    # Define properties and get/set functions. ############### 
    @property
    def color(self):
        if self._color is None:
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
        if self._depth is None:
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
        if self._ir is None:
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