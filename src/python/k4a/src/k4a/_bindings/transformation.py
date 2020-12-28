'''
transformation.py

Defines a Transformation class that is a container for a transform.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

import ctypes as _ctypes
import numpy as _np
import copy as _copy

from .k4atypes import EStatus, ECalibrationType, _Float2, _Float3, \
    ETransformInterpolationType, EImageFormat

from .k4a import k4a_transformation_create, k4a_transformation_destroy, \
    k4a_calibration_3d_to_3d, k4a_calibration_2d_to_3d, \
    k4a_calibration_3d_to_2d, k4a_calibration_2d_to_2d, \
    k4a_calibration_color_2d_to_depth_2d, \
    k4a_transformation_depth_image_to_color_camera, \
    k4a_transformation_depth_image_to_color_camera_custom, \
    k4a_transformation_color_image_to_depth_camera, \
    k4a_transformation_depth_image_to_point_cloud, \
    k4a_image_get_buffer

from .calibration import Calibration
from .image import Image

class Transformation:

    def __init__(self, 
        calibration:Calibration):
        
        self._calibration = calibration
        self.__transform_handle = k4a_transformation_create(
            _ctypes.byref(calibration._calibration))

    def __del__(self):
        k4a_transformation_destroy(self.__transform_handle)
        del self._calibration
        del self.__transform_handle

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    def __str__(self):
        return self._calibration.__str__()

    def point_3d_to_point_3d(self,
        source_point_3d:(float, float, float),
        source_camera:ECalibrationType,
        target_camera:ECalibrationType)->(float, float, float):

        target_point = (None, None, None)

        src_pt = _Float3(
            x = source_point_3d[0],
            y = source_point_3d[1],
            z = source_point_3d[2])

        tgt_pt = _Float3()

        status = k4a_calibration_3d_to_3d(
            self._calibration._calibration,
            _ctypes.byref(src_pt),
            source_camera,
            target_camera,
            _ctypes.byref(tgt_pt))

        if (status == EStatus.SUCCEEDED):
            target_point = (tgt_pt.xyz.x, tgt_pt.xyz.y, tgt_pt.xyz.z)

        return target_point

    def point_2d_to_point_3d(self,
        source_point_2d:(float, float),
        source_depth_mm:float,
        source_camera:ECalibrationType,
        target_camera:ECalibrationType)->(float, float, float):

        target_point = (None, None, None)

        src_pt = _Float2(
            x = source_point_2d[0],
            y = source_point_2d[1])
        tgt_pt = _Float3()
        valid_int_flag = _ctypes.c_int(0)

        status = k4a_calibration_2d_to_3d(
            self._calibration._calibration,
            _ctypes.byref(src_pt),
            _ctypes.c_float(source_depth_mm),
            source_camera,
            target_camera,
            _ctypes.byref(tgt_pt),
            _ctypes.byref(valid_int_flag))

        if (status == EStatus.SUCCEEDED and valid_int_flag.value == 1):
            target_point = (tgt_pt.xyz.x, tgt_pt.xyz.y, tgt_pt.xyz.z)

        return target_point

    def point_3d_to_point_2d(self,
        source_point_3d:(float, float, float),
        source_camera:ECalibrationType,
        target_camera:ECalibrationType)->(float, float):

        target_point = (None, None)

        src_pt = _Float3(
            x = source_point_3d[0],
            y = source_point_3d[1],
            z = source_point_3d[2])
        tgt_pt = _Float2()
        valid_int_flag = _ctypes.c_int(0)

        status = k4a_calibration_3d_to_2d(
            self._calibration._calibration,
            _ctypes.byref(src_pt),
            source_camera,
            target_camera,
            _ctypes.byref(tgt_pt),
            _ctypes.byref(valid_int_flag))

        if (status == EStatus.SUCCEEDED and valid_int_flag.value == 1):
            target_point = (tgt_pt.xy.x, tgt_pt.xy.y)

        return target_point

    def point_2d_to_point_2d(self,
        source_point_2d:(float, float),
        source_depth_mm:float,
        source_camera:ECalibrationType,
        target_camera:ECalibrationType)->(float, float):

        target_point = (None, None)

        src_pt = _Float2(
            x = source_point_2d[0],
            y = source_point_2d[1])
        tgt_pt = _Float2()
        valid_int_flag = _ctypes.c_int(0)

        status = k4a_calibration_2d_to_2d(
            self._calibration._calibration,
            _ctypes.byref(src_pt),
            _ctypes.c_float(source_depth_mm),
            source_camera,
            target_camera,
            _ctypes.byref(tgt_pt),
            _ctypes.byref(valid_int_flag))

        if (status == EStatus.SUCCEEDED and valid_int_flag.value == 1):
            target_point = (tgt_pt.xy.x, tgt_pt.xy.y)

        return target_point

    def color_2d_to_depth_2d(self,
        source_point_2d:(float, float),
        depth:Image)->(float, float):

        target_point = (None, None)

        src_pt = _Float2(
            x = source_point_2d[0],
            y = source_point_2d[1])
        tgt_pt = _Float2()
        valid_int_flag = _ctypes.c_int(0)

        status = k4a_calibration_color_2d_to_depth_2d(
            self._calibration._calibration,
            _ctypes.byref(src_pt),
            depth._image_handle,
            _ctypes.byref(tgt_pt),
            _ctypes.byref(valid_int_flag))

        if (status == EStatus.SUCCEEDED and valid_int_flag.value == 1):
            target_point = (tgt_pt.xy.x, tgt_pt.xy.y)

        return target_point

    def depth_image_to_color_camera(self,
        depth:Image,
        color:Image)->Image:

        # Create an output image.
        transformed_depth_image = Image.create(
            depth.image_format,
            color.width_pixels,
            color.height_pixels,
            color.width_pixels * 2)

        status = k4a_transformation_depth_image_to_color_camera(
            self.__transform_handle,
            depth._image_handle,
            transformed_depth_image._image_handle)

        if (status != EStatus.SUCCEEDED):
            transformed_depth_image = None

        return transformed_depth_image

    def depth_image_to_color_camera_custom(self,
        depth:Image,
        custom:Image,
        color:Image,
        interp_type:ETransformInterpolationType,
        invalid_value:int)->(Image, Image):

        # Create an output image.
        transformed_depth_image = Image.create(
            depth.image_format,
            color.width_pixels,
            color.height_pixels,
            color.width_pixels * 2)

        # Create an output image.
        transformed_custom_image = Image.create(
            custom.image_format,
            color.width_pixels,
            color.height_pixels,
            color.width_pixels * 2)

        status = k4a_transformation_depth_image_to_color_camera_custom(
            self.__transform_handle,
            depth._image_handle,
            custom._image_handle,
            transformed_depth_image._image_handle,
            transformed_custom_image._image_handle,
            interp_type,
            _ctypes.c_uint32(invalid_value))

        if (status != EStatus.SUCCEEDED):
            transformed_depth_image = None
            transformed_custom_image = None

        return (transformed_depth_image, transformed_custom_image)

    def color_image_to_depth_camera(self,
        depth:Image,
        color:Image)->Image:

        # Create an output image.
        transformed_color_image = Image.create(
            color.image_format,
            depth.width_pixels,
            depth.height_pixels,
            depth.width_pixels * 4)

        status = k4a_transformation_color_image_to_depth_camera(
            self.__transform_handle,
            depth._image_handle,
            color._image_handle,
            transformed_color_image._image_handle)

        if (status != EStatus.SUCCEEDED):
            transformed_color_image = None

        return transformed_color_image

    def depth_image_to_point_cloud(self,
        depth:Image, 
        camera_type:ECalibrationType)->Image:

        # Create a custom image.
        point_cloud_image = Image.create(
            EImageFormat.CUSTOM,
            depth.width_pixels,
            depth.height_pixels,
            depth.width_pixels * 6)

        status = k4a_transformation_depth_image_to_point_cloud(
            self.__transform_handle,
            depth._image_handle,
            camera_type,
            point_cloud_image._image_handle)

        if (status != EStatus.SUCCEEDED):
            point_cloud_image = None
        else:
            # The ndarray for a CUSTOM image format is a flat buffer.
            # Rewrite the ndarray to have 3 dimensions for X, Y, and Z points.
            buffer_ptr = k4a_image_get_buffer(point_cloud_image._image_handle)
            array_type = ((_ctypes.c_uint16 * 3) * depth.width_pixels) * depth.height_pixels
            point_cloud_image._data = _np.ctypeslib.as_array(array_type.from_address(
                _ctypes.c_void_p.from_buffer(buffer_ptr).value))

        return point_cloud_image

    @property
    def calibration(self):
        return self._calibration

    @calibration.deleter
    def calibration(self):
        del self._calibration
        self._calibration = None