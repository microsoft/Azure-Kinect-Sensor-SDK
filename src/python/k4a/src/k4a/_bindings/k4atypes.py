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
class EStatus(_IntEnum):
    """
    Result code returned by Azure Kinect APIs.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    SUCCEEDED                        Successful status.
    FAILED                           Failed status.
    ================================ ==========================================
    """
    SUCCEEDED = 0
    FAILED = _auto()


@_unique
class EBufferStatus(_IntEnum):
    """
    Result code returned by Azure Kinect APIs.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    SUCCEEDED                        Successful buffer request status.
    FAILED                           Failed buffer request status.
    BUFFER_TOO_SMALL                 Buffer is too small.
    ================================ ==========================================
    """
    SUCCEEDED = 0
    FAILED = _auto()
    BUFFER_TOO_SMALL = _auto()


@_unique
class EWaitStatus(_IntEnum):
    """
    Result code returned by Azure Kinect APIs.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    SUCCEEDED                        Successful result status.
    FAILED                           Failed result status.
    TIMEOUT                          The request timed out.
    ================================ ==========================================
    """
    SUCCEEDED = 0
    FAILED = _auto()
    TIMEOUT = _auto()


@_unique
class ELogLevel(_IntEnum):
    """
    Verbosity levels of debug messaging.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    CRITICAL                         Most severe level of debug messaging.
    ERROR                            2nd most severe level of debug messaging.
    WARNING                          3nd most severe level of debug messaging.
    INFO                             2nd least severe level of debug messaging.
    TRACE                            Least severe level of debug messaging.
    OFF                              No logging is performed.
    ================================ ==========================================
    """
    CRITICAL = 0
    ERROR = _auto()
    WARNING = _auto()
    INFO = _auto()
    TRACE = _auto()
    OFF = _auto()


@_unique
class EDepthMode(_IntEnum):
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
    OFF                              Depth sensor will be turned off.
    NFOV_2X2BINNED                   Depth and Active IR captured at 320x288.
    NFOV_UNBINNED                    Depth and Active IR captured at 640x576.
    WFOV_2X2BINNED                   Depth and Active IR captured at 512x512.
    WFOV_UNBINNED                    Depth and Active IR captured at 1024x1024.
    PASSIVE_IR                       Passive IR only, captured at 1024x1024.
    ================================ ==========================================
    """
    OFF = 0
    NFOV_2X2BINNED = _auto()
    NFOV_UNBINNED = _auto()
    WFOV_2X2BINNED = _auto()
    WFOV_UNBINNED = _auto()
    PASSIVE_IR = _auto()


@_unique
class EColorResolution(_IntEnum):
    """
    Color sensor resolutions.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    OFF                              Color camera will be turned off.
    RES_720P                         1280 * 720  16:9.
    RES_1080P                        1920 * 1080 16:9.
    RES_1440P                        2560 * 1440 16:9.
    RES_1536P                        2048 * 1536 4:3.
    RES_2160P                        3840 * 2160 16:9.
    RES_3072P                        4096 * 3072 4:3.
    ================================ ==========================================
    """
    OFF = 0
    RES_720P = _auto()
    RES_1080P = _auto()
    RES_1440P = _auto()
    RES_1536P = _auto()
    RES_2160P = _auto()
    RES_3072P = _auto()


@_unique
class EImageFormat(_IntEnum):
    """
    Image format type.
    
    The image format indicates how the buffer image data is interpreted.

    ============ ==============================================================
    Name                             Definition
    ============ ==============================================================
    COLOR_MJPG   Color image type MJPG.
                 - The buffer for each image is encoded as a JPEG and can be 
                   decoded by a JPEG decoder.
                 - Because the image is compressed, the stride parameter for the
                   image is not applicable.
                 - Each MJPG encoded image in a stream may be of differing size
                   depending on the compression efficiency.

    COLOR_NV12   Color image type NV12.
                 - NV12 images separate the luminance and chroma data such that
                   all the luminance is at the beginning of the buffer, and the
                   chroma lines follow immediately after.
                 - Stride indicates the length of each line in bytes and should
                   be used to determine the start location of each line of the
                   image in memory. Chroma has half as many lines of height and
                   half the width in pixels of the luminance. Each chroma line
                   has the same width in bytes as a luminance line.

    COLOR_YUY2   Color image type YUY2.
                 - YUY2 stores chroma and luminance data in interleaved pixels.
                 - Stride indicates the length of each line in bytes and should
                   be used to determine the start location of each line of the
                   image in memory.

    COLOR_BGRA32 Color image type BGRA32.
                 - Each pixel of BGRA32 data is four bytes. The first three 
                   bytes represent Blue, Green, and Red data. The fourth byte
                   is the alpha channel and is unused in Azure Kinect APIs.
                 - Stride indicates the length of each line in bytes and should
                   be used to determine the start location of each line of the
                   image in memory.
                 - The Azure Kinect device does not natively capture in this
                   format. Requesting images of this format requires additional
                   computation in the API.

    DEPTH16      Depth image type DEPTH16.
                 - Each pixel of DEPTH16 data is two bytes of little endian
                   unsigned depth data. The unit of the data is in millimeters
                   from the origin of the camera.
                 - Stride indicates the length of each line in bytes and should
                   be used to determine the start location of each line of the
                   image in memory.

    IR16         Image type IR16.
                 - Each pixel of IR16 data is two bytes of little endian 
                   unsigned depth data. The value of the data represents 
                   brightness.
                 - This format represents infrared light and is captured by the
                   depth camera.
                 - Stride indicates the length of each line in bytes and should
                   be used to determine the start location of each line of the
                   image in memory.

    CUSTOM8      Single channel image type CUSTOM8.
                 - Each pixel of CUSTOM8 is a single channel one byte of 
                   unsigned data.
                 - Stride indicates the length of each line in bytes and should
                   be used to determine the start location of each line of the
                   image in memory.

    CUSTOM16     Single channel image type CUSTOM16.
                 - Each pixel of CUSTOM16 is a single channel two byte of 
                   unsigned data.
                 - Stride indicates the length of each line in bytes and should
                   be used to determine the start location of each line of the
                   image in memory.

    CUSTOM       Custom image format.
                 - Used in conjunction with user created images or images 
                   packing non-standard data.
                 - See the originator of the custom formatted image for 
                   information on how to interpret the data.
    ============ ==============================================================
    """
    COLOR_MJPG = 0
    COLOR_NV12 = _auto()
    COLOR_YUY2 = _auto()
    COLOR_BGRA32 = _auto()
    DEPTH16 = _auto()
    IR16 = _auto()
    CUSTOM8 = _auto()
    CUSTOM16 = _auto()
    CUSTOM = _auto()


@_unique
class ETransformInterpolationType(_IntEnum):
    """
    Transformation interpolation type.

    Interpolation type used with transformation from depth image to color
    camera custom.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    NEAREST                          Nearest neighbor interpolation.
    LINEAR                           Linear interpolation.
    ================================ ==========================================
    """
    NEAREST = 0
    LINEAR = _auto()


@_unique
class EFramePerSecond(_IntEnum):
    """
    Color and depth sensor frame rate.

    This enumeration is used to select the desired frame rate to operate the
    cameras. The actual frame rate may vary slightly due to dropped data, 
    synchronization variation between devices, clock accuracy, or if the camera
    exposure priority mode causes reduced frame rate.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    FPS_5                            5 frames per second.
    FPS_15                           15 frames per second.
    FPS_30                           30 frames per second.
    ================================ ==========================================
    """
    FPS_5 = 0
    FPS_15 = _auto()
    FPS_30 = _auto()


@_unique
class EColorControlCommand(_IntEnum):
    """
    Color sensor control commands

    The current settings can be read with k4a_device_get_color_control(). The
    settings can be set with k4a_device_set_color_control().

    Control values set on a device are reset only when the device is power 
    cycled. The device will retain the settings even if the _DeviceHandle is 
    closed or the application is restarted.

    ====================== ====================================================
    Name                                     Definition
    ====================== ====================================================
    EXPOSURE_TIME_ABSOLUTE Exposure time setting.
                           - May be set to EColorControlMode.AUTO or 
                             EColorControlMode.MANUAL.
                           - The Azure Kinect supports a limited number of 
                             fixed expsore settings. When setting this, expect
                             the exposure to be rounded up to the nearest 
                             setting. Exceptions are:
                             1) The last value in the table is the upper limit,
                                so a value larger than this will be overridden
                                to the largest entry in the table. 
                             2) The exposure time cannot be larger than the 
                                equivelent FPS. So expect 100ms exposure time
                                to be reduced to 30ms or 33.33ms when the 
                                camera is started. 
                           - The most recent copy of the table 
                             'device_exposure_mapping' is in 
                             https://github.com/microsoft/Azure-Kinect-Sensor-SDK/blob/develop/src/color/color_priv.h
                           - Exposure time is measured in microseconds.

    AUTO_EXPOSURE_PRIORITY Exposure or Framerate priority setting.
                           - May only be set to EColorControlMode.MANUAL.
                           - Value of 0 means framerate priority. 
                             Value of 1 means exposure priority.
                           - Using exposure priority may impact the framerate
                             of both the color and depth cameras.
                           - Deprecated starting in 1.2.0. Please discontinue 
                             usage, firmware does not support this.

    BRIGHTNESS             Brightness setting.
                           - May only be set to EColorControlMode.MANUAL.
                           - The valid range is 0 to 255. The default value is 128.

    CONTRAST               Contrast setting.
                           - May only be set to EColorControlMode.MANUAL.

    SATURATION             Saturation setting.
                           - May only be set to EColorControlMode.MANUAL.

    SHARPNESS              Sharpness setting.
                           - May only be set to EColorControlMode.MANUAL.

    WHITEBALANCE           White balance setting.
                           - May be set to EColorControlMode.AUTO or 
                             EColorControlMode.MANUAL.
                           - The unit is degrees Kelvin. The setting must be
                             set to a value evenly divisible by 10 degrees.

    BACKLIGHT_COMPENSATION Backlight compensation setting.
                           - May only be set to EColorControlMode.MANUAL.
                           - Value of 0 means backlight compensation is disabled. 
                             Value of 1 means backlight compensation is enabled.

    GAIN                   Gain setting.
                           - May only be set to EColorControlMode.MANUAL. 

    POWERLINE_FREQUENCY    Powerline frequency setting.
                           - May only be set to EColorControlMode.MANUAL.
                           - Value of 1 sets the powerline compensation to 50 Hz. 
                             Value of 2 sets the powerline compensation to 60 Hz.
    ====================== ====================================================
    """
    EXPOSURE_TIME_ABSOLUTE = 0
    AUTO_EXPOSURE_PRIORITY = _auto()
    BRIGHTNESS = _auto()
    CONTRAST = _auto()
    SATURATION = _auto()
    SHARPNESS = _auto()
    WHITEBALANCE = _auto()
    BACKLIGHT_COMPENSATION = _auto()
    GAIN = _auto()
    POWERLINE_FREQUENCY = _auto()


@_unique
class EColorControlMode(_IntEnum):
    """
    Color sensor control mode.

    The current settings can be read with k4a_device_get_color_control(). The
    settings can be set with k4a_device_set_color_control().

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    AUTO                             Set EColorControlCommand to auto.
    MANUAL                           Set EColorControlCommand to manual.
    ================================ ==========================================
    """
    AUTO = 0
    MANUAL = _auto()


@_unique
class EWiredSyncMode(_IntEnum):
    """
    Synchronization mode when connecting two or more devices together.

    ============ ==============================================================
    Name                             Definition
    ============ ==============================================================
    STANDALONE   Neither 'Sync In' or 'Sync Out' connections are used.

    MASTER       The 'Sync Out' jack is enabled and synchronization data it 
                 driven out the connected wire. While in master mode the color
                 camera must be enabled as part of the multi device sync 
                 signalling logic. Even if the color image is not needed, the
                 color camera must be running.

    SUBORDINATE  The 'Sync In' jack is used for synchronization and 'Sync Out'
                 is driven for the next device in the chain. 'Sync Out' is a 
                 mirror of 'Sync In' for this mode.
    ============ ==============================================================
    """
    STANDALONE = 0
    MASTER = _auto()
    SUBORDINATE = _auto()


@_unique
class ECalibrationType(_IntEnum):
    """
    Calibration types.

    Specifies a type of calibration.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    UNKNOWN                          Calibration type is unknown.
    DEPTH                            Depth sensor.
    COLOR                            Color sensor.
    GYRO                             Gyroscope sensor.
    ACCEL                            Acceleremeter sensor.
    NUM_TYPES                        Number of types excluding unknown type.
    ================================ ==========================================
    """
    UNKNOWN = 0
    DEPTH = _auto()
    COLOR = _auto()
    GYRO = _auto()
    ACCEL = _auto()
    NUM_TYPES = _auto()


@_unique
class ECalibrationModelType(_IntEnum):
    """
    Calibration model type.

    The model used interpret the calibration parameters.

    =================================== =======================================
    Name                                Definition
    =================================== =======================================
    LENS_DISTORTION_MODEL_UNKNOWN       Calibration model is unknown.

    LENS_DISTORTION_MODEL_THETA         Deprecated (not supported). 
                                        Calibration model is Theta (arctan).

    LENS_DISTORTION_MODEL_POLYNOMIAL_3K Deprecated (not supported). 
                                        Calibration model is Polynomial 3K.

    LENS_DISTORTION_MODEL_RATIONAL_6KT  Deprecated (only supported early internal devices).
                                        Calibration model is Rational 6KT.

    LENS_DISTORTION_MODEL_BROWN_CONRADY Calibration model is Brown Conrady
                                        (compatible with OpenCV).
    =================================== =======================================
    """
    LENS_DISTORTION_MODEL_UNKNOWN = 0
    LENS_DISTORTION_MODEL_THETA = _auto()
    LENS_DISTORTION_MODEL_POLYNOMIAL_3K = _auto()
    LENS_DISTORTION_MODEL_RATIONAL_6KT = _auto()
    LENS_DISTORTION_MODEL_BROWN_CONRADY = _auto()


@_unique
class EFirmwareBuild(_IntEnum):
    """
    Firmware build type.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    RELEASE                          Production firmware.
    DEBUG                            Pre-production firmware.
    ================================ ==========================================
    """
    RELEASE = 0
    DEBUG = _auto()


@_unique
class EFirmwareSignature(_IntEnum):
    """
    Firmware signature type.

    ================================ ==========================================
    Name                             Definition
    ================================ ==========================================
    MSFT                             Microsoft signed firmware.
    TEST                             Test signed firmware.
    UNSIGNED                         Unsigned firmware.
    ================================ ==========================================
    """
    MSFT = 0
    TEST = _auto()
    UNSIGNED = _auto()


#define K4A_SUCCEEDED(_result_) (_result_ == SUCCEEDED)
def K4A_SUCCEEDED(result):
    return result == EStatus.SUCCEEDED


#define K4A_FAILED(_result_) (!K4A_SUCCEEDED(_result_))
def K4A_FAILED(result):
    return not K4A_SUCCEEDED(result)


#typedef void(k4a_logging_message_cb_t)(void *context,
#                                       ELogLevel level,
#                                       const char *file,
#                                       const int line,
#                                       const char *message);
logging_message_cb = _ctypes.WINFUNCTYPE(None,
    _ctypes.c_void_p, _ctypes.c_int, _ctypes.POINTER(_ctypes.c_char), 
    _ctypes.c_int, _ctypes.POINTER(_ctypes.c_char))


#typedef void(k4a_memory_destroy_cb_t)(void *buffer, void *context);
_memory_destroy_cb = _ctypes.WINFUNCTYPE(
    None, _ctypes.c_void_p, _ctypes.c_void_p)


#typedef uint8_t *(k4a_memory_allocate_cb_t)(int size, void **context);
_memory_allocate_cb = _ctypes.WINFUNCTYPE(
    _ctypes.c_uint8, _ctypes.c_int, _ctypes.POINTER(_ctypes.c_void_p))


# K4A_DECLARE_HANDLE(handle_k4a_device_t);
class __handle_k4a_device_t(_ctypes.Structure):
     _fields_= [
        ("_rsvd", _ctypes.c_size_t),
    ]
_DeviceHandle = _ctypes.POINTER(__handle_k4a_device_t)


# K4A_DECLARE_HANDLE(handle_k4a_capture_t);
class __handle_k4a_capture_t(_ctypes.Structure):
     _fields_= [
        ("_rsvd", _ctypes.c_size_t),
    ]
_CaptureHandle = _ctypes.POINTER(__handle_k4a_capture_t)


# K4A_DECLARE_HANDLE(handle_k4a_image_t);
class __handle_k4a_image_t(_ctypes.Structure):
     _fields_= [
        ("_rsvd", _ctypes.c_size_t),
    ]
_ImageHandle = _ctypes.POINTER(__handle_k4a_image_t)


# K4A_DECLARE_HANDLE(k4a_transformation_t);
class __handle_k4a_transformation_t(_ctypes.Structure):
     _fields_= [
        ("_rsvd", _ctypes.c_size_t),
    ]
_TransformationHandle = _ctypes.POINTER(__handle_k4a_transformation_t)


class DeviceConfiguration(_ctypes.Structure):
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


class CalibrationExtrinsics(_ctypes.Structure):
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


class CalibrationIntrinsicParam(_ctypes.Structure):
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

class _CalibrationIntrinsicParameters(_ctypes.Union):
    _fields_= [
        ("param", CalibrationIntrinsicParam),
        ("v", _ctypes.c_float * 15),
    ]

    def __repr__(self):
        return self.param.__repr__()


class CalibrationIntrinsics(_ctypes.Structure):
    _fields_= [
        ("type", _ctypes.c_int),
        ("parameter_count", _ctypes.c_uint),
        ("parameters", _CalibrationIntrinsicParameters),
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


class CalibrationCamera(_ctypes.Structure):
    _fields_= [
        ("extrinsics", CalibrationExtrinsics),
        ("intrinsics", CalibrationIntrinsics),
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


class Calibration(_ctypes.Structure):
    _fields_= [
        ("depth_camera_calibration", CalibrationCamera),
        ("color_camera_calibration", CalibrationCamera),
        ("extrinsics", (CalibrationExtrinsics * ECalibrationType.NUM_TYPES) * ECalibrationType.NUM_TYPES),
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
        
        for r in range(ECalibrationType.NUM_TYPES):
            for c in range(ECalibrationType.NUM_TYPES):
                s = ''.join([s, 'extrinsics[%d][%d]=%s, ']) % (r, c, self.extrinsics[r][c].__repr__())

        s = ''.join([s,
            'depth_mode=%d, ',
            'color_resolution=%d']) % (
                self.depth_mode,
                self.color_resolution
            )

        return s


class Version(_ctypes.Structure):
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


class HardwareVersion(_ctypes.Structure):
    _fields_= [
        ("rgb", Version),
        ("depth", Version),
        ("audio", Version),
        ("depth_sensor", Version),
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


class _XY(_ctypes.Structure):
    _fields_= [
        ("x", _ctypes.c_float),
        ("y", _ctypes.c_float),
        ]

    def __init__(self, x=0, y=0):
        self.x = x
        self.y = y

    def __repr__(self):
        return ''.join(['x=%f, ', 'y=%f']) % (self.x, self.y)


class Float2(_ctypes.Union):
    _fields_= [
        ("xy", _XY),
        ("v", _ctypes.c_float * 2),
        ]

    def __init__(self, x=0, y=0):
        self.xy = _XY(x, y)

    def __repr__(self):
        return self.xy.__repr__()


class _XYZ(_ctypes.Structure):
    _fields_= [
        ("x", _ctypes.c_float),
        ("y", _ctypes.c_float),
        ("z", _ctypes.c_float),
    ]

    def __init__(self, x=0, y=0, z=0):
        self.x = x
        self.y = y
        self.z = z

    def __repr__(self):
        return ''.join(['x=%f, ', 'y=%f, ', 'z=%f']) % (self.x, self.y, self.z)


class Float3(_ctypes.Union):
    _fields_= [
        ("xyz", _XYZ),
        ("v", _ctypes.c_float * 3)
    ]

    def __init__(self, x=0, y=0, z=0):
        self.xyz = _XYZ(x, y, z)

    def __repr__(self):
        return self.xyz.__repr__()


class ImuSample(_ctypes.Structure):
    _fields_= [
        ("temperature", _ctypes.c_float),
        ("acc_sample", Float3),
        ("acc_timestamp_usec", _ctypes.c_uint64),
        ("gyro_sample", Float3),
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
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL = DeviceConfiguration()
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.color_format = EImageFormat.COLOR_MJPG
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.color_resolution = EColorResolution.OFF
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.depth_mode = EDepthMode.OFF
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.camera_fps = EFramePerSecond.FPS_30
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.synchronized_images_only = False
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.depth_delay_off_color_usec = 0
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.wired_sync_mode = EWiredSyncMode.STANDALONE
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.subordinate_delay_off_master_usec = 0
K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.disable_streaming_indicator = False


del _IntEnum
del _unique
del _auto
del _ctypes
