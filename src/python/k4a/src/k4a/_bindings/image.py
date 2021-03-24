'''!
@file image.py

Defines an Image class that is a container for a single image
from an Azure Kinect device.

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
Kinect For Azure SDK.
'''

import ctypes as _ctypes
import numpy as _np
import copy as _copy

from .k4atypes import _ImageHandle, EStatus, EImageFormat

from .k4a import k4a_image_create, k4a_image_create_from_buffer, \
    k4a_image_release, k4a_image_get_buffer, \
    k4a_image_reference, k4a_image_release, k4a_image_get_format, \
    k4a_image_get_size, k4a_image_get_width_pixels, \
    k4a_image_get_height_pixels, k4a_image_get_stride_bytes, \
    k4a_image_get_device_timestamp_usec, k4a_image_set_device_timestamp_usec, \
    k4a_image_get_system_timestamp_nsec, k4a_image_set_system_timestamp_nsec, \
    k4a_image_get_exposure_usec, k4a_image_set_exposure_usec, \
    k4a_image_get_white_balance, k4a_image_set_white_balance, \
    k4a_image_get_iso_speed, k4a_image_set_iso_speed

class Image:
    '''! A class that represents an image from an imaging sensor in an
    Azure Kinect device.

    <table>
    <tr style="background-color: #323C7D; color: #ffffff;">
    <td> Property Name </td><td> Type </td><td> R/W </td><td> Description </td>
    </tr>
    
    <tr>
    <td> data </td>
    <td> numpy.ndarray </td>
    <td> R/W </td>
    <td> The image data as a numpy ndarray. </td>
    </tr>

    <tr>
    <td> image_format </td>
    <td> EImageFormat </td>
    <td> R </td>
    <td> The format of the image. </td>
    </tr>

    <tr>
    <td> size_bytes </td>
    <td> int </td>
    <td> R </td>
    <td> The size in bytes of the data. </td>
    </tr>

    <tr>
    <td> width_pixels </td>
    <td> int </td>
    <td> R </td>
    <td> The width in pixels of the data. </td>
    </tr>

    <tr>
    <td> height_pixels </td>
    <td> int </td>
    <td> R </td>
    <td> The height in pixels of the data. </td>
    </tr>

    <tr>
    <td> stride_bytes </td>
    <td> int </td>
    <td> R </td>
    <td> The stride in bytes of the rows. </td>
    </tr>

    <tr>
    <td> device_timestamp_usec </td>
    <td> int </td>
    <td> R/W </td>
    <td> The device timestamp in microseconds.
         - Timestamps are recorded by the device and represent the mid-point of
            exposure. They may be used for relative comparison, but their 
            absolute value has no defined meaning.
         - If the Image is invalid or if no timestamp was set for the image,
            this value will be 0. It is also possible for a 0 value to be a
            valid timestamp originating from the beginning of a recording or
            the start of streaming.
    </td>
    </tr>

    <tr>
    <td> system_timestamp_nsec </td>
    <td> int </td>
    <td> R/W </td>
    <td> The device timestamp in nanoseconds.
         - The system timestamp is a high performance and increasing clock 
            (from boot). The timestamp represents the time immediately after 
            the image buffer was read by the host PC.
         - Timestamps are recorded by the host. They may be used for relative
            comparision, as they are relative to the corresponding system 
            clock. The absolute value is a monotonic count from an arbitrary 
            point in the past.
         - The system timestamp is captured at the moment host PC finishes 
            receiving the image.
         - On Linux the system timestamp is read from 
            clock_gettime(CLOCK_MONOTONIC), which measures realtime and is not
            impacted by adjustments to the system clock. It starts from an 
            arbitrary point in the past. On Windows the system timestamp is 
            read from QueryPerformanceCounter(). It also measures realtime and
            is not impacted by adjustments to the system clock. It also starts
            from an arbitrary point in the past.
         - If the Image is invalid or if no timestamp was set for the image,
            this value will be 0. It is also possible for a 0 value to be a
            valid timestamp originating from the beginning of a recording or
            the start of streaming.
    </td>
    </tr>

    <tr>
    <td> exposure_usec </td>
    <td> int </td>
    <td> R/W </td>
    <td> The image exposure in microseconds.
         - This is only supported on color image formats.
         - If the Image is invalid or if no exposure was set for the image,
            this value will be 0. An exposure time of 0 is considered invalid.
    </td>
    </tr>

    <tr>
    <td> white_balance </td>
    <td> int </td>
    <td> R/W </td>
    <td> The image white balance in Kelvin.
         - This function is only valid for color captures, and not for depth or
            IR captures.
         - If the Image is invalid or if no white balance was set for the image,
            this value will be 0. A white balance of 0 is considered invalid.
    </td>
    </tr>

    <tr>
    <td> iso_speed </td>
    <td> int </td>
    <td> R/W </td>
    <td> The image ISO speed.
         - This function is only valid for color captures, and not for depth or
            IR captures.
         - If the Image is invalid or if no white balance was set for the image,
            this value will be 0. If the image is read from the device and this
            value is 0, this indicates the ISO speed was not available or an
            error occurred. An ISO of 0 is considered invalid.
    </td>
    </tr>

    </table>

    @remarks 
    - An Image manages an image buffer and associated metadata.
    
    @remarks 
    - Do not use the Image() constructor to get an Image instance. It
        will return an object that does not have a handle to the image
        resources held by the SDK. Instead, use the create() function.

    @remarks 
    - The memory associated with the image buffer in an Image may have been
    allocated by the Azure Kinect APIs or by the application. If the 
    Image was created by an Azure Kinect API, its memory will be freed when all
    references to the Image are released. All images retrieved directly from a
    device will have been created by the API. An application can create an
    Image using memory that it has allocated using create_from_ndarray() and
    passing an numpy ndarray with a pre-allocated memory. In both cases, the
    memory is released when the all references to the Image object are
    released, either explicitly with del or it goes out of scope. Users do not
    need to worry about memory allocation and deallocation other than the
    normal rules for Python objects.

    @remarks
    - An image has a number of metadata properties that can be set or 
    retrieved. Not all properties are applicable to images of all types. 
    See the documentation for the individual properties for more information on
    their applicability and meaning.

    @remarks
    - Images may be of one of the standard EImageFormat formats, or may be of 
    format EImageFormat.CUSTOM. The format defines how the underlying image 
    buffer should be interpreted.

    @remarks
    - Images from a device are retrieved through a Capture object returned by 
    Device.get_capture().

    @remarks
    - Images stored in a capture are referenced by the capture until they are
    replaced or the capture is destroyed.

    @remarks 
    - An Image object may be copied or deep copied. A shallow copy
        shares the same data as the original, and any changes in one will
        affect the other. A deep copy does not share any resources with the
        original, and changes in one will not affect the other. In both shallow
        and deep copies, deleting one will have no effects on the other.
    '''

    def __init__(self, image_handle:_ImageHandle=None):
        self.__image_handle = image_handle

        # The _data property is a numpy ndarray. The buffer backing the ndarray
        # can be user-owned or sdk-owned, depending on whether this image
        # object is created by the user with a backing buffer or not.
        self._data = None

        self._image_format = None
        self._size_bytes = None
        self._width_pixels = None
        self._height_pixels = None
        self._stride_bytes = None
        self._device_timestamp_usec = None
        self._system_timestamp_nsec = None
        self._exposure_usec = None
        self._white_balance = None
        self._iso_speed = None

    @staticmethod
    def _get_array_type_from_format(
        image_format:EImageFormat,
        buffer_size:int, 
        width_pixels:int, 
        height_pixels:int):

        array_type = None
        array_len_bytes = 0

        if image_format == EImageFormat.COLOR_MJPG:
            array_type = _ctypes.c_ubyte * buffer_size
            array_len_bytes = buffer_size
        elif image_format == EImageFormat.COLOR_NV12:
            array_type = ((_ctypes.c_ubyte * 1) * width_pixels) * (height_pixels + int(height_pixels/2)) 
            array_len_bytes = width_pixels * (height_pixels + int(height_pixels/2))
        elif image_format == EImageFormat.COLOR_YUY2:
            array_type = (_ctypes.c_ubyte * width_pixels*2) * height_pixels
            array_len_bytes = width_pixels * height_pixels * 2
        elif image_format == EImageFormat.COLOR_BGRA32:
            array_type = ((_ctypes.c_ubyte * 4) * width_pixels) * height_pixels
            array_len_bytes = width_pixels * height_pixels * 4
        elif image_format == EImageFormat.DEPTH16:
            array_type = (_ctypes.c_uint16 * width_pixels) * height_pixels
            array_len_bytes = width_pixels * height_pixels * 2
        elif image_format == EImageFormat.IR16:
            array_type = (_ctypes.c_uint16 * width_pixels) * height_pixels
            array_len_bytes = width_pixels * height_pixels * 2
        elif image_format == EImageFormat.CUSTOM16:
            array_type = (_ctypes.c_uint16 * width_pixels) * height_pixels
            array_len_bytes = width_pixels * height_pixels * 2
        elif image_format == EImageFormat.CUSTOM8:
            array_type = (_ctypes.c_uint8 * width_pixels) * height_pixels
            array_len_bytes = width_pixels * height_pixels
        elif image_format == EImageFormat.CUSTOM:
            array_type = _ctypes.c_ubyte * buffer_size
            array_len_bytes = buffer_size

        return (array_type, array_len_bytes)

    # This static method should not be called by users.
    # It is an internal-only function for instantiating an Image object.
    @staticmethod
    def _create_from_existing_image_handle(
        image_handle:_ImageHandle):

        # Create an Image object.
        image = Image(image_handle=image_handle)

        # Get the data property to force reading of the data from sdk.
        data = image.data

        return image

    @staticmethod
    def create(
        image_format:EImageFormat,
        width_pixels:int,
        height_pixels:int,
        stride_bytes:int):
        '''! Create an image.

        @param image_format (EImageFormat): The format of the image that will
            be stored in this image container.

        @param width_pixels (int): Width in pixels.

        @param height_pixels (int): Height in pixels.

        @param stride_bytes (int): The number of bytes per horizontal line of
            the image. If set to 0, the stride will be set to the minimum size
            given the image format and width_pixels.

        @returns Image: An Image instance with an allocated data buffer. If an
            error occurs, then None is returned.

        @remarks
        - This function is used to create images of formats that have 
            consistent stride. The function is not suitable for compressed
            formats that may not be represented by the same number of bytes per
            line.

        @remarks
        - For most image formats, the function will allocate an image buffer of
            size @p height_pixels * @p stride_bytes. Buffers with image format
            of EImageFormat.COLOR_NV12 will allocate an additional 
            @p height_pixels / 2 set of lines (each of @p stride_bytes). This 
            function cannot be used to allocate EImageFormat.COLOR_MJPG buffer.

        @remarks
        - To create an image object without the API allocating memory, or to 
            represent an image that has a non-deterministic stride, use 
            create_from_ndarray().

        @remarks
        - If an error occurs, then None is returned.
        '''

        image = None

        assert(isinstance(image_format, EImageFormat)), "image_format parameter must be an EImageFormat."
        assert(width_pixels > 0), "width_pixels must be greater than zero."
        assert(height_pixels > 0), "height_pixels must be greater than zero."
        assert(stride_bytes > 0), "stride_bytes must be greater than zero."
        assert(stride_bytes > width_pixels), "stride_bytes must be greater than width_pixels."
        assert(stride_bytes > height_pixels), "stride_bytes must be greater than height_pixels."

        # Create an image.
        image_handle = _ImageHandle()
        status = k4a_image_create(
            _ctypes.c_int(image_format),
            _ctypes.c_int(width_pixels),
            _ctypes.c_int(height_pixels),
            _ctypes.c_int(stride_bytes),
            _ctypes.byref(image_handle))

        if status == EStatus.SUCCEEDED:
            image = Image._create_from_existing_image_handle(image_handle)

        return image

    @staticmethod
    def create_from_ndarray(
        image_format:EImageFormat,
        arr:_np.ndarray,
        stride_bytes_custom:int=0,
        size_bytes_custom:int=0,
        width_pixels_custom:int=0,
        height_pixels_custom:int=0):
        '''! Create an image from a pre-allocated buffer.

        @param image_format (EImageFormat): The format of the image that will
            be stored in this image container.

        @param arr (numpy.ndarray): A pre-allocated numpy ndarray.

        @param stride_bytes_custom (int, optional): User-defined stride in bytes.

        @param size_bytes_custom (int): User-defined size in bytes of the data.

        @param width_pixels_custom (int): User-defined width in pixels of the data.

        @param height_pixels_custom (int): User-defined height in pixels of the data.

        @returns Image: An Image instance using the pre-allocated data buffer. If an
            error occurs, then None is returned.

        @remarks
        - This function is used to create images of formats that have 
            consistent stride. The function is not suitable for compressed
            formats that may not be represented by the same number of bytes per
            line.

        @remarks
        - The numpy ndarray keeps a record of the per-dimension stride in bytes
            to get to the next sample in the same dimension. The definition of
            stride in the Azure Kinect SDK is the number of bytes to get to the
            next sample in the row, so it only has one stride. As such, the
            stride that is used to pass to the SDK is the first stride in the
            ndarray. In cases where the geometry of the ndarray does not map
            well to this scheme, the user can use the optional
            @p stride_bytes_custom to pass in a custom stride to use.

        @remarks
        - Users can pass in their own size, width, height, and stride to use in
            cases where the ndarray shape and size metadata does not map well
            to the predefined EImageFormat types. They supercede any of the
            corresponding metadata in the ndarray.

        @remarks
        - If an error occurs, then None is returned.
        '''
        image = None

        assert(isinstance(arr, _nd.ndarray)), "arr must be a numpy ndarray object."
        assert(isinstance(image_format, EImageFormat)), "image_format parameter must be an EImageFormat."

        # Get buffer pointer and sizes of the numpy ndarray.
        buffer_ptr = ctypes.cast(
            _ctypes.addressof(np.ctypeslib.as_ctypes(arr)), 
            _ctypes.POINTER(_ctypes.c_uint8))

        width_pixels = _ctypes.c_int(width_pixels_custom)
        height_pixels = _ctypes.c_int(height_pixels_custom)
        stride_bytes = _ctypes.c_int(stride_bytes_custom)
        size_bytes = _ctypes.c_size_t(size_bytes_custom)

        # Use the ndarray sizes if the custom size info is not passed in.
        if width_pixels == 0:
            width_pixels = _ctypes.c_int(arr.shape[0])

        if height_pixels == 0:
            height_pixels = _ctypes.c_int(arr.shape[1])

        if size_bytes == 0:
            size_bytes = _ctypes.c_size_t(arr.itemsize * arr.size)

        if len(arr.shape) > 2 and stride_bytes == 0:
            stride_bytes = _ctypes.c_int(arr.shape[2])

        # Create image from the numpy buffer.
        image_handle = _ImageHandle()
        status = k4a_image_create_from_buffer(
            image_format,
            width_pixels,
            height_pixels,
            stride_bytes,
            buffer_ptr,
            size_bytes,
            None,
            None,
            _ctypes.byref(image_handle))

        if status == EStatus.SUCCEEDED:
            image = Image._create_from_existing_image_handle(image_handle)

        return image

    def _release(self):
        k4a_image_release(self.__image_handle)

    def _reference(self):
        k4a_image_reference(self.__image_handle)

    def __del__(self):

        # Deleting the _image_handle will release the image reference.
        del self._image_handle

        del self.data
        del self.image_format
        del self.size_bytes
        del self.width_pixels
        del self.height_pixels
        del self.stride_bytes
        del self.device_timestamp_usec
        del self.system_timestamp_nsec
        del self.exposure_usec
        del self.white_balance
        del self.iso_speed

    def __copy__(self):

        # Create a shallow copy.
        new_image = Image(self._image_handle)
        new_image._data = self._data.view()
        new_image._image_format = self._image_format
        new_image._size_bytes = self._size_bytes
        new_image._width_pixels = self._width_pixels
        new_image._height_pixels = self._height_pixels
        new_image._stride_bytes = self._stride_bytes
        new_image._device_timestamp_usec = self._device_timestamp_usec
        new_image._system_timestamp_nsec = self._system_timestamp_nsec
        new_image._exposure_usec =self._exposure_usec
        new_image._white_balance = self._white_balance
        new_image._iso_speed = self._iso_speed

        # Update reference count.
        new_image._reference()

        return new_image

    def __deepcopy__(self, memo):

        # Create a deep copy. This requires an entirely new image
        # backed by an entirely new buffer in the SDK.
        new_image = Image.create(
            self.image_format,
            self.width_pixels,
            self.height_pixels,
            self.stride_bytes)

        # Copy the ndarray data to the new buffer.
        _np.copyto(new_image._data, self._data)

        # Copy the other image metadata.
        new_image.device_timestamp_usec = self.device_timestamp_usec
        new_image.system_timestamp_nsec = self.system_timestamp_nsec
        new_image.exposure_usec = self.exposure_usec
        new_image.white_balance = self.white_balance
        new_image.iso_speed = self.iso_speed

        # Do not need to update reference count. Just return the copy.
        return new_image
    
    def __enter__(self):
        return self

    def __exit__(self):
        del self

    def __str__(self):
        return ''.join([
            'data=%s, ',
            'image_format=%d, ',
            'size_bytes=%d, ',
            'width_pixels=%d, ',
            'height_pixels=%d, ',
            'stride_bytes=%d, ',
            'device_timestamp_usec=%d, ',
            'system_timestamp_nsec=%d, ',
            'exposure_usec=%d, ',
            'white_balance=%d, ',
            'iso_speed=%d, ']) % (
            self.data.__str__(),
            self.image_format,
            self.size_bytes,
            self.width_pixels,
            self.height_pixels,
            self.stride_bytes,
            self.device_timestamp_usec,
            self.system_timestamp_nsec,
            self.exposure_usec,
            self.white_balance,
            self.iso_speed)

    # Define properties and get/set functions. ############### 
    @property
    def _image_handle(self):
        return self.__image_handle

    @_image_handle.deleter
    def _image_handle(self):
        
        # Release the image before deleting.
        if isinstance(self._data, _np.ndarray):
            if not self._data.flags.owndata:
                k4a_image_release(self.__image_handle)

        del self.__image_handle
        self.__image_handle = None

    @property
    def data(self):
        # Create a numpy.ndarray from the image buffer.

        if self._data is None:
            # First, get a pointer to the buffer.
            buffer_ptr = k4a_image_get_buffer(self.__image_handle)
            buffer_size = k4a_image_get_size(self.__image_handle)
            image_format = k4a_image_get_format(self.__image_handle)
            width_pixels = k4a_image_get_width_pixels(self.__image_handle)
            height_pixels = k4a_image_get_height_pixels(self.__image_handle)
            stride_bytes = k4a_image_get_stride_bytes(self.__image_handle)

            assert(buffer_size > 0), "buffer_size must be greater than zero."
            assert(width_pixels > 0), "width_pixels must be greater than zero."
            assert(height_pixels > 0), "height_pixels must be greater than zero."
            assert(stride_bytes > 0), "stride_bytes must be greater than zero."
            assert(stride_bytes > width_pixels), "stride_bytes must be greater than width_pixels."
            assert(stride_bytes > height_pixels), "stride_bytes must be greater than height_pixels."

            # Construct a descriptor of the data in the buffer.
            (array_type, array_len_bytes) = Image._get_array_type_from_format(
                image_format, buffer_size, width_pixels, height_pixels)
            assert(array_type is not None), "Unrecognized image format."
            assert(array_len_bytes <= buffer_size), "ndarray size should be less than buffer size in bytes."

            self._data = _np.ctypeslib.as_array(array_type.from_address(
                _ctypes.c_void_p.from_buffer(buffer_ptr).value))

        return self._data

    @data.deleter
    def data(self):
        del self._data
        self._data = None

    @property
    def image_format(self):
        self._image_format = k4a_image_get_format(self.__image_handle)
        return self._image_format

    @image_format.deleter
    def image_format(self):
        del self._image_format
        self._image_format = None

    @property
    def size_bytes(self):
        self._size_bytes = k4a_image_get_size(self.__image_handle)
        return self._size_bytes

    @size_bytes.deleter
    def size_bytes(self):
        del self._size_bytes
        self._size_bytes = None

    @property
    def width_pixels(self):
        self._width_pixels = k4a_image_get_width_pixels(self.__image_handle)
        return self._width_pixels

    @width_pixels.deleter
    def width_pixels(self):
        del self._width_pixels
        self._width_pixels = None

    @property
    def height_pixels(self):
        self._height_pixels = k4a_image_get_height_pixels(self.__image_handle)
        return self._height_pixels

    @height_pixels.deleter
    def height_pixels(self):
        del self._height_pixels
        self._height_pixels = None

    @property
    def stride_bytes(self):
        self._stride_bytes = k4a_image_get_stride_bytes(self.__image_handle)
        return self._stride_bytes

    @stride_bytes.deleter
    def stride_bytes(self):
        del self._stride_bytes
        self._stride_bytes = None

    @property
    def device_timestamp_usec(self):
        self._device_timestamp_usec = k4a_image_get_device_timestamp_usec(self.__image_handle)
        return self._device_timestamp_usec

    @device_timestamp_usec.setter
    def device_timestamp_usec(self, value:int):
        k4a_image_set_device_timestamp_usec(
            self.__image_handle, 
            _ctypes.c_ulonglong(value))
        self._device_timestamp_usec = value

    @device_timestamp_usec.deleter
    def device_timestamp_usec(self):
        del self._device_timestamp_usec
        self._device_timestamp_usec = None

    @property
    def system_timestamp_nsec(self):
        self._system_timestamp_nsec = k4a_image_get_system_timestamp_nsec(self.__image_handle)
        return self._system_timestamp_nsec

    @system_timestamp_nsec.setter
    def system_timestamp_nsec(self, value:int):
        k4a_image_set_system_timestamp_nsec(
            self.__image_handle, 
            _ctypes.c_ulonglong(value))
        self._system_timestamp_nsec = value

    @system_timestamp_nsec.deleter
    def system_timestamp_nsec(self):
        del self._system_timestamp_nsec
        self._system_timestamp_nsec = None

    @property
    def exposure_usec(self):
        self._exposure_usec = k4a_image_get_exposure_usec(self.__image_handle)
        return self._exposure_usec

    @exposure_usec.setter
    def exposure_usec(self, value:int):
        k4a_image_set_exposure_usec(
            self.__image_handle, 
            _ctypes.c_ulonglong(value))
        self._exposure_usec = value

    @exposure_usec.deleter
    def exposure_usec(self):
        del self._exposure_usec
        self._exposure_usec = None

    @property
    def white_balance(self):
        self._white_balance = k4a_image_get_white_balance(self.__image_handle)
        return self._white_balance

    @white_balance.setter
    def white_balance(self, value:int):
        k4a_image_set_white_balance(
            self.__image_handle, 
            _ctypes.c_uint32(value))
        self._white_balance = value

    @white_balance.deleter
    def white_balance(self):
        del self._white_balance
        self._white_balance = None

    @property
    def iso_speed(self):
        self._iso_speed = k4a_image_get_iso_speed(self.__image_handle)
        return self._iso_speed

    @iso_speed.setter
    def iso_speed(self, value:int):
        k4a_image_set_iso_speed(
            self.__image_handle, 
            _ctypes.c_uint32(value))
        self._iso_speed = value

    @iso_speed.deleter
    def iso_speed(self):
        del self._iso_speed
        self._iso_speed = None
    
    # ###############