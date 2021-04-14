'''!
@file transformation.py

Defines a Transformation class for point and image transformations.

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
Kinect For Azure SDK.
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
    '''! A class that represents an image transform between the cameras in an
    Azure Kinect device.

    @remarks
    - The Transformation class is used to transform images from the coordinate
        system of one camera into the other. Each transformation requires some
        pre-computed resources to be allocated, which are retained until the
        object is deleted.

    @remarks 
    - Do not use the Transformation() constructor to get an instance. 
        Instead, use the create() function.

    @remarks
    - A Calibration object is needed to instantiate a Transformation object.
        The calibration object is in turn dependent on a specific depth mode
        and color resolution. 
        
    @remarks
    - The depth mode and color resolution of the calibration object does not 
        need to be identical to the depth mode and color resolution of the 
        captured data. Transformations requiring image data will be transformed
        to the resolution of the calibration object used to create the
        Transformation object.
    '''

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

    @staticmethod
    def create(calibration:Calibration):
        '''! Create a transformation object.

        @param calibration (Calibration): A calibration object obtained by
            Device.get_calibration().

        @returns Transformation: A Transformation instance. If an error occurs,
            then None is returned.

        @remarks
        - The transformation is used to transform images from the coordinate
            system of one camera into the other. Each transformation requires 
            some pre-computed resources to be allocated, which are retained 
            until the object is deleted or goes out of scope.

        @remarks
        - A Calibration object is needed to instantiate a Transformation object.
            The calibration object is in turn dependent on a specific depth mode
            and color resolution. The depth mode and color resolution should be
            identical to the depth mode and color resolution of the captured data.

        @remarks
        - If an error occurs, then None is returned.
        '''
        return Transformation(calibration)

    def point_3d_to_point_3d(self,
        source_point_3d:(float, float, float),
        source_camera:ECalibrationType,
        target_camera:ECalibrationType)->(float, float, float):
        '''! Transform a 3D point of a source coordinate system into a 3D point
            of the target coordinate system.

        @param source_point_3d (float, float, float): The 3D coordinates in 
            millimeters representing a point in @p source_camera.

        @param source_camera (ECalibrationType): The source camera.

        @param target_camera (ECalibrationType): The target camera.

        @returns (float, float, float): The new 3D coordinates of the input 
            point in the coordinate space of @p target_camera in millimeters.
            If an error occurs, then (None, None, None) is returned.

        @remarks
        - This function is used to transform 3D points between depth and color
            camera coordinate systems. The function uses the extrinsic camera 
            calibration. It computes the output via multiplication with a 
            precomputed matrix encoding a 3D rotation and a 3D translation. 
            If @p source_camera and @p target_camera are the same, then 
            the output 3D point will be identical to @p source_point_3d.

        @remarks
        - If an error occurs, then (None, None, None) is returned.
        '''

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

    def pixel_2d_to_point_3d(self,
        source_pixel_2d:(float, float),
        source_depth_mm:float,
        source_camera:ECalibrationType,
        target_camera:ECalibrationType)->(float, float, float):
        '''! Transform a 2D pixel of a source camera with an associated depth
            value into a 3D point of the target coordinate system.

        @param source_pixel_2d (float, float): The 2D pixel in @p source_camera
            coordinates.

        @param source_depth_mm (float): The depth of @p source_pixel_2d in 
            millimeters. One way to derive the depth value in the color camera
            geometry is to use the function depth_image_to_color_camera().

        @param source_camera (ECalibrationType): The source camera.

        @param target_camera (ECalibrationType): The target camera.

        @returns (float, float, float): The new 3D coordinates of the input 
            pixel and depth in the coordinate space of @p target_camera in 
            millimeters. If an error occurs, then (None, None, None) is returned.

        @remarks
        - This function applies the intrinsic calibration of @p source_camera
            to compute the 3D ray from the focal point of the camera through
            pixel @p source_pixel_2d. The 3D point on this ray is then found 
            using @p source_depth_mm. If @p target_camera is different from 
            @p source_camera, the 3D point is transformed to @p target_camera 
            using point_3d_to_point_3d(). In practice, @p source_camera and 
            @p target_camera will often be identical. In this case, no 3D to 
            3D transformation is applied.

        @remarks
        - If an error occurs or if the point is not valid in the target camera
            coordinate, then (None, None, None) is returned.
        '''

        target_point = (None, None, None)

        src_pt = _Float2(
            x = source_pixel_2d[0],
            y = source_pixel_2d[1])
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

    def point_3d_to_pixel_2d(self,
        source_point_3d:(float, float, float),
        source_camera:ECalibrationType,
        target_camera:ECalibrationType)->(float, float):
        '''! Transform a 3D point of a source camera with into a 2D pixel 
        coordinate of the target camera

        @param source_point_3d (float, float, float): The 3D coordinates in 
            millimeters representing a point in @p source_camera.

        @param source_camera (ECalibrationType): The source camera.

        @param target_camera (ECalibrationType): The target camera.

        @returns (float, float): The 2D pixel coordinates of the input 
            point in the coordinate space of @p target_camera. If an error 
            occurs, then (None, None) is returned.

        @remarks
        - If @p target_camera is different from @p source_camera, 
            @p source_point_3d is transformed to @p target_camera using
            point_3d_to_point_3d(). In practice, @p source_camera and 
            @p target_camera will often be identical. In this case, no 3D to 3D
            transformation is applied. The 3D point in the coordinate system of
            @p target_camera is then projected onto the image plane using the 
            intrinsic calibration of @p target_camera.

        @remarks
        - If an error occurs or if the point is not valid in the target camera
            coordinate, then (None, None, None) is returned.
        '''

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

    def pixel_2d_to_pixel_2d(self,
        source_pixel_2d:(float, float),
        source_depth_mm:float,
        source_camera:ECalibrationType,
        target_camera:ECalibrationType)->(float, float):
        '''! Transform a 2D pixel coordinate with an associated depth value of
            the source camera into a 2D pixel coordinate of the target camera.

        @param source_pixel_2d (float, float): The 2D pixel in @p source_camera 
            coordinates.

        @param source_depth_mm (float): The depth of @p source_pixel_2d
            in millimeters. One way to derive the depth value in the color 
            camera geometry is to use the function depth_image_to_color_camera().

        @param source_camera (ECalibrationType): The source camera.

        @param target_camera (ECalibrationType): The target camera.

        @returns (float, float): The 2D pixel in @p target_camera coordinates.
            If an error occurs, then (None, None) is returned.

        @remarks
        - This function maps a pixel between the coordinate systems of the
            depth and color cameras. It is equivalent to calling 
            pixel_2d_to_point_3d() to compute the 3D point corresponding to 
            @p source_point2d and then using point_3d_to_pixel_2d() to map the
            3D point into the coordinate system of the @p target_camera.

        @remarks
        - If @p source_camera and @p target_camera are identical, the function
            immediately sets the output to @p source_pixel_2d and returns 
            without computing any transformations.

        @remarks
        - If an error occurs or if the point is not valid in the target camera
            coordinate, then (None, None) is returned.
        '''

        target_point = (None, None)

        src_pt = _Float2(
            x = source_pixel_2d[0],
            y = source_pixel_2d[1])
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
        source_pixel_2d:(float, float),
        depth:Image)->(float, float):
        '''! Transform a 2D pixel coordinate from color camera into a 2D pixel
            coordinate of the depth camera.

        @param source_pixel_2d (float, float): The 2D pixel in color camera 
            coordinates.

        @param depth (Image): A depth image.

        @returns (float, float): The 2D pixel in @p depth camera coordinates.
            If an error occurs, then (None, None) is returned.

        @remarks
        - This function represents an alternative to pixel_2d_to_pixel_2d() if
            the number of pixels that need to be transformed is small. This 
            function searches along an epipolar line in the depth image to find
            the corresponding depth pixel. If a larger number of pixels need to
            be transformed, it might be computationally cheaper to call
            depth_image_to_color_camera() to get correspondence depth values
            for these color pixels, then call the function pixel_2d_to_pixel_2d().
        '''

        target_point = (None, None)

        src_pt = _Float2(
            x = source_pixel_2d[0],
            y = source_pixel_2d[1])
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

    def depth_image_to_color_camera(self, depth:Image)->Image:
        '''! Transforms the depth map into the geometry of the color camera.

        @param depth (Image): The depth image.

        @returns (Image): The transformed depth image in the color camera
            coordinates. If an error occurs, then None is returned.

        @remarks
        - This produces a depth image for which each pixel matches the 
            corresponding pixel coordinates of the color camera.

        @remarks
        - @p depth_image must be of format EImageFormat.DEPTH16, and the
        output transformed depth image will also have the same format.

        @remarks
        - The output transformed depth image will have a width and height 
            matching the width and height of the color camera in the mode
            specified by the Calibration used to create the Transformation.
        '''
        
        width_pixels = self._calibration.color_cam_cal.resolution_width
        height_pixels = self._calibration.color_cam_cal.resolution_height

        # Create an output image.
        transformed_depth_image = Image.create(
            depth.image_format,
            width_pixels,
            height_pixels,
            width_pixels * 2)

        status = k4a_transformation_depth_image_to_color_camera(
            self.__transform_handle,
            depth._image_handle,
            transformed_depth_image._image_handle)

        if (status != EStatus.SUCCEEDED):
            transformed_depth_image = None
        else:
            buffer_ptr = k4a_image_get_buffer(transformed_depth_image._image_handle)
            array_type = (_ctypes.c_uint16 * height_pixels) * width_pixels
            self._data = _np.ctypeslib.as_array(array_type.from_address(
                _ctypes.c_void_p.from_buffer(buffer_ptr).value))

        return transformed_depth_image

    def depth_image_to_color_camera_custom(self,
        depth:Image,
        custom:Image,
        interp_type:ETransformInterpolationType,
        invalid_value:int)->(Image, Image):
        '''! Transforms depth map and a custom image into the geometry of the
        color camera.

        @param depth (Image): The depth image.

        @param custom (Image): A custom image.

        @param interp_type (ETransformInterpolationType): Parameter that 
            controls how pixels in @p custom_image should be interpolated when 
            transformed to color camera space. 
            - ETransformInterpolationType.LINEAR if linear interpolation should
                be used.
            - ETransformInterpolationType.NEAREST if nearest neighbor 
                interpolation should be used.

        @param invalid_value (int): Defines the custom image pixel value that 
            should be written to the transformed images in case the corresponding
            depth pixel can not be transformed into the color camera space.

        @returns (Image, Image): The transformed depth image and transformed
            custom image in the color camera coordinates. If an error occurs, 
            then (None, None) is returned.

        @remarks
        - This produces a depth image and a corresponding custom image for 
            which each pixel matches the corresponding pixel coordinates of the
            color camera.

        @remarks
        - @p depth must be of format EImageFormat.DEPTH16 and the
        output transformed depth image will also have the same format.

         @remarks
        - @p custom must be of format EImageFormat.CUSTOM8 or 
            EImageFormat.CUSTOM16, and the output transformed custom image will
            also have the same format.

        @remarks
        - The output transformed depth image and transformed custom image will
            have a width and height matching the width and height of the color
            camera in the mode specified by the Calibration used to create the 
            Transformation.
        '''

        width_pixels = self._calibration.color_cam_cal.resolution_width
        height_pixels = self._calibration.color_cam_cal.resolution_height

        custom_stride_bytes = width_pixels
        if custom.image_format == EImageFormat.CUSTOM16:
            custom_stride_bytes = width_pixels * 2

        # Create an output image.
        transformed_depth_image = Image.create(
            depth.image_format,
            width_pixels,
            height_pixels,
            width_pixels * 2)

        # Create an output image.
        transformed_custom_image = Image.create(
            custom.image_format,
            width_pixels,
            height_pixels,
            custom_stride_bytes)

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
        '''! Transforms a color image into the geometry of the depth camera.

        @param depth (Image): The depth image.

        @param color (Image): The color image to transform.

        @returns (Image): The transformed color image in the depth camera 
            coordinates. If an error occurs, then None is returned.

        @remarks
        - This produces a color image for which each pixel matches the 
            corresponding pixel coordinates of the depth camera.

        @remarks
        - @p depth and @p color need to represent the same moment in time. The 
            depth data will be applied to the color image to properly warp the 
            color data to the perspective of the depth camera.

         @remarks
        - @p depth must be of type EImageFormat.DEPTH16. @p color must be of 
            format EImageFormat.COLOR_BGRA32.

        @remarks
        - The output transformed color image will have a width and height 
            matching the width and height of the depth camera in the mode 
            specified by the Calibration used to create the Transformation.
        '''

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
        '''! Transforms the depth image into 3 planar images representing 
            X, Y and Z-coordinates of corresponding 3D points.

        @param depth (Image): The depth image.

        @param camera_type (ECalibrationType): Geometry in which depth map was 
            computed.

        @returns (Image): The output XYZ point cloud image in the depth camera 
            coordinates. If an error occurs, then None is returned.

        @remarks
        - The @p camera_type parameter tells the function what the perspective 
            of the @p depth is. If the @p depth was captured directly from the 
            depth camera, the value should be ECalibrationType.DEPTH. If the 
            @p depth is the result of a transformation into the color camera's 
            coordinate space using depth_image_to_color_camera(), the value 
            should be ECalibrationType.COLOR.

         @remarks
        - The format of the output image is EImageFormat.CUSTOM. The width and 
            height of the output image will be equal to the width and height of 
            @p depth image. The output image will have a stride in bytes of 6 
            times its width in pixels.

        @remarks
        - Each plane of the output image consists of the X, Y, and Z planar images of int16 samples.
          If out is the output image, then out[:,:,1] corresponds to the X coordinates, 
          out[:,:,2] corresponds to the Y coordinates, and out[:,:,3] is the Z depth image.
        '''

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
            array_type = ((_ctypes.c_int16 * 3) * depth.width_pixels) * depth.height_pixels
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