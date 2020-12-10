'''
k4atypes.py

Defines common enums and structures used in the Azure Kinect SDK.
These enums defined here are analogous to those defined in k4a.h.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

from enum import IntEnum as _IntEnum
from enum import unique as _unique
from enum import auto as _auto
import ctypes as _ctypes
from os import linesep as _newline


@_unique
class k4a_result_t(_IntEnum):
    """
    Result code returned by Azure Kinect APIs.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_RESULT_SUCCEEDED             Successful status.
    K4A_RESULT_FAILED                Failed status.
    ================================ ==========================================
    """
    K4A_RESULT_SUCCEEDED = 0
    K4A_RESULT_FAILED = _auto()


@_unique
class k4a_buffer_result_t(_IntEnum):
    """
    Result code returned by Azure Kinect APIs.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_BUFFER_RESULT_SUCCEEDED      Successful buffer request status.
    K4A_BUFFER_RESULT_FAILED         Failed buffer request status.
    K4A_BUFFER_RESULT_TOO_SMALL      Buffer is too small.
    ================================ ==========================================
    """
    K4A_BUFFER_RESULT_SUCCEEDED = 0
    K4A_BUFFER_RESULT_FAILED = _auto()
    K4A_BUFFER_RESULT_TOO_SMALL = _auto()


@_unique
class k4a_wait_result_t(_IntEnum):
    """
    Result code returned by Azure Kinect APIs.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_WAIT_RESULT_SUCCEEDED        Successful result status.
    K4A_WAIT_RESULT_FAILED           Failed result status.
    K4A_WAIT_RESULT_TIMEOUT          The request timed out.
    ================================ ==========================================
    """
    K4A_WAIT_RESULT_SUCCEEDED = 0
    K4A_WAIT_RESULT_FAILED = _auto()
    K4A_WAIT_RESULT_TIMEOUT = _auto()


@_unique
class k4a_log_level_t(_IntEnum):
    """
    Verbosity levels of debug messaging.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_LOG_LEVEL_CRITICAL           Most severe level of debug messaging.
    K4A_LOG_LEVEL_ERROR              2nd most severe level of debug messaging.
    K4A_LOG_LEVEL_WARNING            3nd most severe level of debug messaging.
    K4A_LOG_LEVEL_INFO               2nd least severe level of debug messaging.
    K4A_LOG_LEVEL_TRACE              Least severe level of debug messaging.
    K4A_LOG_LEVEL_OFF                No logging is performed.
    ================================ ==========================================
    """
    K4A_LOG_LEVEL_CRITICAL = 0
    K4A_LOG_LEVEL_ERROR = _auto()
    K4A_LOG_LEVEL_WARNING = _auto()
    K4A_LOG_LEVEL_INFO = _auto()
    K4A_LOG_LEVEL_TRACE = _auto()
    K4A_LOG_LEVEL_OFF = _auto()


@_unique
class k4a_depth_mode_t(_IntEnum):
    """
    Depth sensor capture modes.

    See the hardware specification for additional details on the field of view
    and supported frame rates for each mode.

    NFOV and WFOV denote Narrow and Wide Field of View configurations.

    Binned modes reduce the captured camera resolution by combining adjacent
    sensor pixels into a bin.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_DEPTH_MODE_OFF               Depth sensor will be turned off with this 
                                     setting.

    K4A_DEPTH_MODE_NFOV_2X2BINNED    Depth captured at 320x288. Passive IR is
                                     also captured at 320x288.

    K4A_DEPTH_MODE_NFOV_UNBINNED     Depth captured at 640x576. Passive IR is
                                     also captured at 640x576.

    K4A_DEPTH_MODE_WFOV_2X2BINNED    Depth captured at 512x512. Passive IR is
                                     also captured at 512x512.

    K4A_DEPTH_MODE_WFOV_UNBINNED     Depth captured at 1024x1024. Passive IR
                                     is also captured at 1024x1024.

    K4A_DEPTH_MODE_PASSIVE_IR        Passive IR only, captured at 1024x1024.
    ================================ ==========================================
    """
    K4A_DEPTH_MODE_OFF = 0
    K4A_DEPTH_MODE_NFOV_2X2BINNED = _auto()
    K4A_DEPTH_MODE_NFOV_UNBINNED = _auto()
    K4A_DEPTH_MODE_WFOV_2X2BINNED = _auto()
    K4A_DEPTH_MODE_WFOV_UNBINNED = _auto()
    K4A_DEPTH_MODE_PASSIVE_IR = _auto()


@_unique
class k4a_color_resolution_t(_IntEnum):
    """
    Color sensor resolutions.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_COLOR_RESOLUTION_OFF         Color camera will be turned off.
    K4A_COLOR_RESOLUTION_720P        1280 * 720  16:9.
    K4A_COLOR_RESOLUTION_1080P       1920 * 1080 16:9.
    K4A_COLOR_RESOLUTION_1440P       2560 * 1440 16:9.
    K4A_COLOR_RESOLUTION_1536P       2048 * 1536 4:3.
    K4A_COLOR_RESOLUTION_2160P       4096 * 3072 4:3
    ================================ ==========================================
    """
    K4A_COLOR_RESOLUTION_OFF = 0
    K4A_COLOR_RESOLUTION_720P = _auto()
    K4A_COLOR_RESOLUTION_1080P = _auto()
    K4A_COLOR_RESOLUTION_1440P = _auto()
    K4A_COLOR_RESOLUTION_1536P = _auto()
    K4A_COLOR_RESOLUTION_2160P = _auto()


@_unique
class k4a_image_format_t(_IntEnum):
    """
    Image format type.
    
    The image format indicates how the buffer image data is interpreted.

    ============================= =============================================
    Name                             Definition
    ============================= =============================================
    K4A_IMAGE_FORMAT_COLOR_MJPG   Color image type MJPG.
                                  - The buffer for each image is encoded as a 
                                  JPEG and can be decoded by a JPEG decoder.
                                  - Because the image is compressed, the stride 
                                  parameter for the image is not applicable.
                                  - Each MJPG encoded image in a stream may be
                                  of differing size depending on the 
                                  compression efficiency.

    K4A_IMAGE_FORMAT_COLOR_NV12   Color image type NV12.
                                  - NV12 images separate the luminance and
                                  chroma data such that all the luminance is at
                                  the beginning of the buffer, and the chroma
                                  lines follow immediately after.
                                  - Stride indicates the length of each line in
                                  bytes and should be used to determine the
                                  start location of each line of the image in
                                  memory. Chroma has half as many lines of 
                                  height and half the width in pixels of the
                                  luminance. Each chroma line has the same
                                  width in bytes as a luminance line.

    K4A_IMAGE_FORMAT_COLOR_YUY2   Color image type YUY2.
                                  - YUY2 stores chroma and luminance data in
                                  interleaved pixels.
                                  - Stride indicates the length of each line in
                                  bytes and should be used to determine the
                                  start location of each line of the image in
                                  memory.

    K4A_IMAGE_FORMAT_COLOR_BGRA32 Color image type BGRA32.
                                  - Each pixel of BGRA32 data is four bytes. 
                                  The first three bytes represent Blue, Green,
                                  and Red data. The fourth byte is the alpha 
                                  channel and is unused in Azure Kinect APIs.
                                  - Stride indicates the length of each line in
                                  bytes and should be used to determine the
                                  start location of each line of the image in 
                                  memory.
                                  - The Azure Kinect device does not natively
                                  capture in this format. Requesting images of
                                  this format requires additional computation
                                  in the API.

    K4A_IMAGE_FORMAT_DEPTH16      Depth image type DEPTH16.
                                  - Each pixel of DEPTH16 data is two bytes of
                                  little endian unsigned depth data. The unit
                                  of the data is in millimeters from the origin
                                  of the camera.
                                  - Stride indicates the length of each line in
                                  bytes and should be used to determine the
                                  start location of each line of the image in 
                                  memory.

    K4A_IMAGE_FORMAT_IR16         Image type IR16.
                                  - Each pixel of IR16 data is two bytes of 
                                  little endian unsigned depth data. The value
                                  of the data represents brightness.
                                  - This format represents infrared light and
                                  is captured by the depth camera.
                                  - Stride indicates the length of each line in
                                  bytes and should be used to determine the
                                  start location of each line of the image in 
                                  memory.

    K4A_IMAGE_FORMAT_CUSTOM8      Single channel image type CUSTOM8.
                                  - Each pixel of CUSTOM8 is a single channel
                                  one byte of unsigned data.
                                  - Stride indicates the length of each line in
                                  bytes and should be used to determine the
                                  start location of each line of the image in 
                                  memory.

    K4A_IMAGE_FORMAT_CUSTOM16     Single channel image type CUSTOM16.
                                  - Each pixel of CUSTOM16 is a single channel
                                  two byte of unsigned data.
                                  - Stride indicates the length of each line in
                                  bytes and should be used to determine the
                                  start location of each line of the image in 
                                  memory.

    K4A_IMAGE_FORMAT_CUSTOM       Custom image format.
                                  - Used in conjunction with user created 
                                  images or images packing non-standard data.
                                  - See the originator of the custom formatted 
                                  image for information on how to interpret the
                                  data.
    ============================= =============================================
    """
    K4A_IMAGE_FORMAT_COLOR_MJPG = 0
    K4A_IMAGE_FORMAT_COLOR_NV12 = _auto()
    K4A_IMAGE_FORMAT_COLOR_YUY2 = _auto()
    K4A_IMAGE_FORMAT_COLOR_BGRA32 = _auto()
    K4A_IMAGE_FORMAT_DEPTH16 = _auto()
    K4A_IMAGE_FORMAT_IR16 = _auto()
    K4A_IMAGE_FORMAT_CUSTOM8 = _auto()
    K4A_IMAGE_FORMAT_CUSTOM16 = _auto()
    K4A_IMAGE_FORMAT_CUSTOM = _auto()


@_unique
class k4a_transformation_interpolation_type_t(_IntEnum):
    """
    Transformation interpolation type.

    Interpolation type used with transformation from depth image to color
    camera custom.

    ============================================= =============================
    Name                                          Definition
    ============================================= =============================
    K4A_TRANSFORMATION_INTERPOLATION_TYPE_NEAREST Nearest neighbor interpolation.
    K4A_TRANSFORMATION_INTERPOLATION_TYPE_LINEAR  Linear interpolation.
    ============================================= =============================
    """
    K4A_TRANSFORMATION_INTERPOLATION_TYPE_NEAREST = 0
    K4A_TRANSFORMATION_INTERPOLATION_TYPE_LINEAR = _auto()


@_unique
class k4a_fps_t(_IntEnum):
    """
    Color and depth sensor frame rate.

    This enumeration is used to select the desired frame rate to operate the
    cameras. The actual frame rate may vary slightly due to dropped data, 
    synchronization variation between devices, clock accuracy, or if the camera
    exposure priority mode causes reduced frame rate.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_FRAMES_PER_SECOND_5          5 FPS
    K4A_FRAMES_PER_SECOND_15         15 FPS
    K4A_FRAMES_PER_SECOND_30         30 FPS
    ================================ ==========================================
    """
    K4A_FRAMES_PER_SECOND_5 = 0
    K4A_FRAMES_PER_SECOND_15 = _auto()
    K4A_FRAMES_PER_SECOND_30 = _auto()


@_unique
class k4a_color_control_command_t(_IntEnum):
    """
    Color sensor control commands

    The current settings can be read with k4a_device_get_color_control(). The
    settings can be set with k4a_device_set_color_control().

    Control values set on a device are reset only when the device is power 
    cycled. The device will retain the settings even if the k4a_device_t is 
    closed or the application is restarted.

    ======================================== ==========================================
    Name                                     Definition
    ======================================== ==========================================
    K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE Exposure time setting.
                                             - May be set to K4A_COLOR_CONTROL_MODE__auto
                                             or K4A_COLOR_CONTROL_MODE_MANUAL.
                                             - The Azure Kinect supports a limited number 
                                             of fixed expsore settings. When setting this, 
                                             expect the exposure to be rounded up to the 
                                             nearest setting. Exceptions are:
                                                1) The last value in the table is the upper 
                                                limit, so a value larger than this will be 
                                                overridden to the largest entry in the table. 
                                                2) The exposure time cannot be larger than 
                                                the equivelent FPS. So expect 100ms exposure 
                                                time to be reduced to 30ms or 33.33ms when 
                                                the camera is started. 
                                             - The most recent copy of the table 
                                             'device_exposure_mapping' is in 
                                             https://github.com/microsoft/Azure-Kinect-Sensor-SDK/blob/develop/src/color/color_priv.h
                                             - Exposure time is measured in microseconds.

    K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY Exposure or Framerate priority setting.
                                             - May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
                                             - Value of 0 means framerate priority. 
                                             Value of 1 means exposure priority.
                                             - Using exposure priority may impact the framerate
                                             of both the color and depth cameras.
                                             - Deprecated starting in 1.2.0. Please discontinue usage, 
                                             firmware does not support this.

    K4A_COLOR_CONTROL_BRIGHTNESS             Brightness setting.
                                             - May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
                                             - The valid range is 0 to 255. The default value is 128.

    K4A_COLOR_CONTROL_CONTRAST               Contrast setting.
                                             - May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.

    K4A_COLOR_CONTROL_SATURATION             Saturation setting.
                                             - May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.

    K4A_COLOR_CONTROL_SHARPNESS              Sharpness setting.
                                             - May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.

    K4A_COLOR_CONTROL_WHITEBALANCE           White balance setting.
                                             - May be set to K4A_COLOR_CONTROL_MODE__auto 
                                             or K4A_COLOR_CONTROL_MODE_MANUAL.
                                             - The unit is degrees Kelvin. The setting must be set
                                             to a value evenly divisible by 10 degrees.

    K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION Backlight compensation setting.
                                             - May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
                                             - Value of 0 means backlight compensation is disabled. 
                                             Value of 1 means backlight compensation is enabled.

    K4A_COLOR_CONTROL_GAIN                   Gain setting.
                                             - May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.   

    K4A_COLOR_CONTROL_POWERLINE_FREQUENCY    Powerline frequency setting.
                                             - May only be set to ::K4A_COLOR_CONTROL_MODE_MANUAL.
                                             - Value of 1 sets the powerline compensation to 50 Hz. 
                                             Value of 2 sets the powerline compensation to 60 Hz.
    ======================================== ==========================================
    """
    K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE = 0
    K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY = _auto()
    K4A_COLOR_CONTROL_BRIGHTNESS = _auto()
    K4A_COLOR_CONTROL_CONTRAST = _auto()
    K4A_COLOR_CONTROL_SATURATION = _auto()
    K4A_COLOR_CONTROL_SHARPNESS = _auto()
    K4A_COLOR_CONTROL_WHITEBALANCE = _auto()
    K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION = _auto()
    K4A_COLOR_CONTROL_GAIN = _auto()
    K4A_COLOR_CONTROL_POWERLINE_FREQUENCY = _auto()


@_unique
class k4a_color_control_mode_t(_IntEnum):
    """
    Color sensor control mode

    The current settings can be read with k4a_device_get_color_control(). The
    settings can be set with k4a_device_set_color_control().

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_COLOR_CONTROL_MODE_AUTO      Set k4a_color_control_command_t to auto.
    K4A_COLOR_CONTROL_MODE_MANUAL    Set k4a_color_control_command_t to manual.
    ================================ ==========================================
    """
    K4A_COLOR_CONTROL_MODE_AUTO = 0
    K4A_COLOR_CONTROL_MODE_MANUAL = _auto()


@_unique
class k4a_wired_sync_mode_t(_IntEnum):
    """
    Synchronization mode when connecting two or more devices together.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_WIRED_SYNC_MODE_STANDALONE   Neither 'Sync In' or 'Sync Out' 
                                     connections are used.

    K4A_WIRED_SYNC_MODE_MASTER       The 'Sync Out' jack is enabled and 
                                     synchronization data it driven out the
                                     connected wire. While in master mode the 
                                     color camera must be enabled as part of 
                                     the multi device sync signalling logic. 
                                     Even if the color image is not needed, the
                                     color camera must be running.

    K4A_WIRED_SYNC_MODE_SUBORDINATE  The 'Sync In' jack is used for 
                                     synchronization and 'Sync Out' is driven 
                                     for the next device in the chain. 
                                     'Sync Out' is a mirror of 'Sync In' for 
                                     this mode.
    ================================ ==========================================
    """
    K4A_WIRED_SYNC_MODE_STANDALONE = 0
    K4A_WIRED_SYNC_MODE_MASTER = _auto()
    K4A_WIRED_SYNC_MODE_SUBORDINATE = _auto()


@_unique
class k4a_calibration_type_t(_IntEnum):
    """
    Calibration types.

    Specifies a type of calibration.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_CALIBRATION_TYPE_UNKNOWN     Calibration type is unknown.
    K4A_CALIBRATION_TYPE_DEPTH       Depth sensor.
    K4A_CALIBRATION_TYPE_COLOR       Color sensor.
    K4A_CALIBRATION_TYPE_GYRO        Gyroscope sensor.
    K4A_CALIBRATION_TYPE_ACCEL       Acceleremeter sensor.
    K4A_CALIBRATION_TYPE_NUM         Number of types excluding unknown type.
    ================================ ==========================================
    """
    K4A_CALIBRATION_TYPE_UNKNOWN = 0
    K4A_CALIBRATION_TYPE_DEPTH = _auto()
    K4A_CALIBRATION_TYPE_COLOR = _auto()
    K4A_CALIBRATION_TYPE_GYRO = _auto()
    K4A_CALIBRATION_TYPE_ACCEL = _auto()
    K4A_CALIBRATION_TYPE_NUM = _auto()


@_unique
class k4a_calibration_model_type_t(_IntEnum):
    """
    Calibration model type.

    The model used interpret the calibration parameters.

    =================================================== ==========================================
    Name                             Definition
    =================================================== ==========================================
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_UNKNOWN       Calibration model is unknown.

    K4A_CALIBRATION_LENS_DISTORTION_MODEL_THETA         Deprecated (not supported). 
                                                        Calibration model is Theta (arctan).

    K4A_CALIBRATION_LENS_DISTORTION_MODEL_POLYNOMIAL_3K Deprecated (not supported). 
                                                        Calibration model is Polynomial 3K.

    K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT  Deprecated (only supported early internal devices).
                                                        Calibration model is Rational 6KT.

    K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY  Calibration model is Brown Conrady
                                                        (compatible with OpenCV).
    =================================================== ==========================================
    """
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_UNKNOWN = 0
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_THETA = _auto()
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_POLYNOMIAL_3K = _auto()
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT = _auto()
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY = _auto()


@_unique
class k4a_firmware_build_t(_IntEnum):
    """
    Firmware build type.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_FIRMWARE_BUILD_RELEASE       Production firmware.
    K4A_FIRMWARE_BUILD_DEBUG         Pre-production firmware.
    ================================ ==========================================
    """
    K4A_FIRMWARE_BUILD_RELEASE = 0
    K4A_FIRMWARE_BUILD_DEBUG = _auto()


@_unique
class k4a_firmware_signature_t(_IntEnum):
    """
    Firmware signature type.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    K4A_FIRMWARE_SIGNATURE_MSFT      Microsoft signed firmware.
    K4A_FIRMWARE_SIGNATURE_TEST      Test signed firmware.
    K4A_FIRMWARE_SIGNATURE_UNSIGNED  Unsigned firmware.
    ================================ ==========================================
    """
    K4A_FIRMWARE_SIGNATURE_MSFT = 0
    K4A_FIRMWARE_SIGNATURE_TEST = _auto()
    K4A_FIRMWARE_SIGNATURE_UNSIGNED = _auto()


#define K4A_SUCCEEDED(_result_) (_result_ == K4A_RESULT_SUCCEEDED)
def K4A_SUCCEEDED(result):
    return result == k4a_result_t.K4A_RESULT_SUCCEEDED


#define K4A_FAILED(_result_) (!K4A_SUCCEEDED(_result_))
def K4A_FAILED(result):
    return not K4A_SUCCEEDED(result)


#typedef void(k4a_logging_message_cb_t)(void *context,
#                                       k4a_log_level_t level,
#                                       const char *file,
#                                       const int line,
#                                       const char *message);
k4a_logging_message_cb_t = _ctypes.CFUNCTYPE(
    _ctypes.c_void_p, _ctypes.c_int, _ctypes.POINTER(_ctypes.c_char), 
    _ctypes.c_int, _ctypes.POINTER(_ctypes.c_char))


#typedef void(k4a_memory_destroy_cb_t)(void *buffer, void *context);
k4a_memory_destroy_cb_t = _ctypes.CFUNCTYPE(
    None, _ctypes.c_void_p, _ctypes.c_void_p)


#typedef uint8_t *(k4a_memory_allocate_cb_t)(int size, void **context);
k4a_memory_allocate_cb_t = _ctypes.CFUNCTYPE(
    _ctypes.c_uint8, _ctypes.c_int, _ctypes.POINTER(_ctypes.c_void_p))


# K4A_DECLARE_HANDLE(k4a_device_t);
class _handle_k4a_device_t(_ctypes.Structure):
     _fields_= [
        ("_rsvd", _ctypes.c_size_t),
    ]
k4a_device_t = _ctypes.POINTER(_handle_k4a_device_t)


# K4A_DECLARE_HANDLE(k4a_capture_t);
class _handle_k4a_capture_t(_ctypes.Structure):
     _fields_= [
        ("_rsvd", _ctypes.c_size_t),
    ]
k4a_capture_t = _ctypes.POINTER(_handle_k4a_capture_t)


# K4A_DECLARE_HANDLE(k4a_image_t);
class _handle_k4a_image_t(_ctypes.Structure):
     _fields_= [
        ("_rsvd", _ctypes.c_size_t),
    ]
k4a_image_t = _ctypes.POINTER(_handle_k4a_image_t)


# K4A_DECLARE_HANDLE(k4a_transformation_t);
class _handle_k4a_transformation_t(_ctypes.Structure):
     _fields_= [
        ("_rsvd", _ctypes.c_size_t),
    ]
k4a_transformation_t = _ctypes.POINTER(_handle_k4a_transformation_t)


class k4a_device_configuration_t(_ctypes.Structure):
    _fields_= [
        ("color_format", _ctypes.c_int),
        ("color_resolution", _ctypes.c_int),
        ("depth_mode", _ctypes.c_int),
        ("camera_fps", _ctypes.c_int),
        ("synchronized_images_only", _ctypes.c_bool),
        ("depth_delay_off_color_usec", _ctypes.c_int32),
        ("wired_sync_mode", _ctypes.c_int),
        ("subordinate_delay_off_master_usec", _ctypes.c_uint32),
        ("disable_streaming_indicator", _ctypes.c_bool),
    ]

    def __repr__(self):
        return ''.join(['<%s.%s object at %s>:',
            _newline,
            'color_format=%d, ',
            'color_resolution=%d, ',
            'depth_mode=%d, ',
            'camera_fps=%d, ',
            'synchronized_images_only=%s, ',
            'depth_delay_off_color_usec=%d, ',
            'wired_sync_mode=%d, ',
            'subordinate_delay_off_master_usec=%d, ',
            'disable_streaming_indicator=%s']) % (
            self.__class__.__module__,
            self.__class__.__name__,
            hex(id(self)),
            self.color_format,
            self.color_resolution,
            self.depth_mode,
            self.camera_fps,
            self.synchronized_images_only,
            self.depth_delay_off_color_usec,
            self.wired_sync_mode,
            self.subordinate_delay_off_master_usec,
            self.disable_streaming_indicator)


class k4a_calibration_extrinsics_t(_ctypes.Structure):
    _fields_= [
        ("rotation", _ctypes.c_float * 9),
        ("translation", _ctypes.c_float * 3),
    ]

    def __repr__(self):
        return ''.join(['<%s.%s object at %s>:',
            _newline,
            'rotation=[[%f,%f,%f][%f,%f,%f][%f,%f,%f]] ',
            'translation=[%f,%f,%f]']) % (
            self.__class__.__module__,
            self.__class__.__name__,
            hex(id(self)),
            self.rotation[0], self.rotation[1], self.rotation[2],
            self.rotation[3], self.rotation[4], self.rotation[5],
            self.rotation[6], self.rotation[7], self.rotation[8],
            self.translation[0], self.translation[1], self.translation[2])


class _k4a_calibration_intrinsic_param(_ctypes.Structure):
    _fields_ = [
        ("cx", _ctypes.c_float),
        ("cy", _ctypes.c_float),
        ("fx", _ctypes.c_float),
        ("fy", _ctypes.c_float),
        ("k1", _ctypes.c_float),
        ("k2", _ctypes.c_float),
        ("k3", _ctypes.c_float),
        ("k4", _ctypes.c_float),
        ("k5", _ctypes.c_float),
        ("k6", _ctypes.c_float),
        ("codx", _ctypes.c_float),
        ("cody", _ctypes.c_float),
        ("p2", _ctypes.c_float),
        ("p1", _ctypes.c_float),
        ("metric_radius", _ctypes.c_float),
    ]

    def __repr__(self):
        return ''.join(['<%s.%s object at %s>:',
            _newline,
            'cx=%f, cy=%f, ',
            'fx=%f, fy=%f, ',
            'k1=%f, k2=%f, k3=%f, k4=%f, k5=%f, k6=%f, ',
            'codx=%f, cody=%f, ',
            'p2=%f, p1=%f, ',
            'metric_radius=%f']) % (
            self.__class__.__module__,
            self.__class__.__name__,
            hex(id(self)),
            self.cx, self.cy,
            self.fx, self.fy,
            self.k1, self.k2, self.k3, self.k4, self.k5, self.k6,
            self.codx, self.cody,
            self.p2, self.p1,
            self.metric_radius)

class _k4a_calibration_intrinsic_parameters_t(_ctypes.Union):
    _fields_= [
        ("param", _k4a_calibration_intrinsic_param),
        ("v", _ctypes.c_float * 15),
    ]

    def __repr__(self):
        return self.param.__repr__()


class k4a_calibration_intrinsics_t(_ctypes.Structure):
    _fields_= [
        ("type", _ctypes.c_int),
        ("parameter_count", _ctypes.c_uint),
        ("parameters", _k4a_calibration_intrinsic_parameters_t),
    ]

    def __repr__(self):
        return ''.join(['<%s.%s object at %s>:',
            _newline,
            'type=%d, ',
            'parameter_count=%d, ',
            'parameters=%s']) % (
            self.__class__.__module__,
            self.__class__.__name__,
            hex(id(self)),
            self.type,
            self.parameter_count,
            self.parameters.__repr__())


class k4a_calibration_camera_t(_ctypes.Structure):
    _fields_= [
        ("extrinsics", k4a_calibration_extrinsics_t),
        ("intrinsics", k4a_calibration_intrinsics_t),
        ("resolution_width", _ctypes.c_int),
        ("resolution_height", _ctypes.c_int),
        ("metric_radius", _ctypes.c_float),
    ]

    def __repr__(self):
        return ''.join(['<%s.%s object at %s>:',
            _newline,
            'extrinsics=%s, ',
            'intrinsics=%s, ',
            'resolution_width=%d, ',
            'resolution_height=%d, ',
            'metric_radius=%f',]) % (
            self.__class__.__module__,
            self.__class__.__name__,
            hex(id(self)),
            self.extrinsics.__repr__(),
            self.intrinsics.__repr__(),
            self.resolution_width,
            self.resolution_height,
            self.metric_radius)


class k4a_calibration_t(_ctypes.Structure):
    _fields_= [
        ("depth_camera_calibration", k4a_calibration_camera_t),
        ("color_camera_calibration", k4a_calibration_camera_t),
        ("extrinsics", (k4a_calibration_extrinsics_t * k4a_calibration_type_t.K4A_CALIBRATION_TYPE_NUM) * k4a_calibration_type_t.K4A_CALIBRATION_TYPE_NUM),
        ("depth_mode", _ctypes.c_int),
        ("color_resolution", _ctypes.c_int),
    ]

    def __repr__(self):
        s = ''.join(['<%s.%s object at %s>:',
            _newline,
            'depth_camera_calibration=%s, ',
            'color_camera_calibration=%s, ']) % (
                self.__class__.__module__,
                self.__class__.__name__,
                hex(id(self)),
                self.depth_camera_calibration.__repr__(),
                self.color_camera_calibration.__repr__(),
            )
        
        for r in range(k4a_calibration_type_t.K4A_CALIBRATION_TYPE_NUM):
            for c in range(k4a_calibration_type_t.K4A_CALIBRATION_TYPE_NUM):
                s = ''.join([s, 'extrinsics[%d][%d]=%s, ']) % (r, c, self.extrinsics[r][c].__repr__())

        s = ''.join([s,
            'depth_mode=%d, ',
            'color_resolution=%d']) % (
                self.depth_mode,
                self.color_resolution
            )

        return s


class k4a_version_t(_ctypes.Structure):
    _fields_= [
        ("major", _ctypes.c_uint32),
        ("minor", _ctypes.c_uint32),
        ("iteration", _ctypes.c_uint32),
    ]

    def __repr__(self):
        return ''.join(['<%s.%s object at %s>:',
            'major=%d, ',
            'minor=%d, ',
            'iteration=%d']) % (
            self.__class__.__module__,
            self.__class__.__name__,
            hex(id(self)),
            self.major,
            self.minor,
            self.iteration)


class k4a_hardware_version_t(_ctypes.Structure):
    _fields_= [
        ("rgb", k4a_version_t),
        ("depth", k4a_version_t),
        ("audio", k4a_version_t),
        ("depth_sensor", k4a_version_t),
        ("firmware_build", _ctypes.c_int),
        ("firmware_signature", _ctypes.c_int),
    ]

    def __repr__(self):
        return ''.join(['<%s.%s object at %s>:', _newline,
            'rgb=%s, ', _newline,
            'depth=%s, ', _newline,
            'audio=%s, ', _newline,
            'depth_sensor=%s, ', _newline,
            'firmware_build=%d, ',
            'firmware_signature=%d']) % (
            self.__class__.__module__,
            self.__class__.__name__,
            hex(id(self)),
            self.rgb,
            self.depth,
            self.audio,
            self.depth_sensor,
            self.firmware_build,
            self.firmware_signature)


class _k4a_xy(_ctypes.Structure):
    _fields_= [
        ("x", _ctypes.c_float),
        ("y", _ctypes.c_float),
        ]

    def __repr__(self):
        return ''.join(['x=%f, ', 'y=%f']) % (self.x, self.y)


class k4a_float2_t(_ctypes.Union):
    _fields_= [
        ("xy", _k4a_xy),
        ("v", _ctypes.c_float * 2),
        ]

    def __repr__(self):
        return self.xy.__repr__()


class _k4a_xyz(_ctypes.Structure):
    _fields_= [
        ("x", _ctypes.c_float),
        ("y", _ctypes.c_float),
        ("z", _ctypes.c_float),
    ]

    def __repr__(self):
        return ''.join(['x=%f, ', 'y=%f, ', 'z=%f']) % (self.x, self.y, self.z)


class k4a_float3_t(_ctypes.Union):
    _fields_= [
        ("xyz", _k4a_xyz),
        ("v", _ctypes.c_float * 3)
    ]

    def __repr__(self):
        return self.xyz.__repr__()


class k4a_imu_sample_t(_ctypes.Structure):
    _fields_= [
        ("temperature", _ctypes.c_float),
        ("acc_sample", k4a_float3_t),
        ("acc_timestamp_usec", _ctypes.c_uint64),
        ("gyro_sample", k4a_float3_t),
        ("gyro_timestamp_usec", _ctypes.c_uint64),
    ]

    def __repr__(self):
        return ''.join(['<%s.%s object at %s>:',
            _newline,
            'temperature=%f, ',
            'acc_sample=%s, ',
            'acc_timestamp_usec=%lu, ',
            'gyro_sample=%s, ',
            'gyro_timestamp_usec=%lu']) % (
            self.__class__.__module__,
            self.__class__.__name__,
            hex(id(self)),
            self.temperature,
            self.acc_sample.__repr__(),
            self.acc_timestamp_usec,
            self.gyro_sample.__repr__(),
            self.gyro_timestamp_usec)


# A static instance of a device configuration where everything is disabled.
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL = k4a_device_configuration_t()
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.color_format = k4a_image_format_t.K4A_IMAGE_FORMAT_COLOR_MJPG
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.color_resolution = k4a_color_resolution_t.K4A_COLOR_RESOLUTION_OFF
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.depth_mode = k4a_depth_mode_t.K4A_DEPTH_MODE_OFF
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.camera_fps = k4a_fps_t.K4A_FRAMES_PER_SECOND_30
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.synchronized_images_only = False
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.depth_delay_off_color_usec = 0
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.wired_sync_mode = k4a_wired_sync_mode_t.K4A_WIRED_SYNC_MODE_STANDALONE
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.subordinate_delay_off_master_usec = 0
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.disable_streaming_indicator = False


del _IntEnum
del _unique
del _auto
del _ctypes
