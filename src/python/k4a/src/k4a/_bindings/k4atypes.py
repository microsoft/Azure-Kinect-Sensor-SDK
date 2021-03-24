'''! 
@file k4atypes.py

Defines common enums and structures used in the Azure Kinect SDK.
These enums defined here are analogous to those defined in k4a.h.

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
Kinect For Azure SDK.
'''


from enum import IntEnum as _IntEnum
from enum import unique as _unique
from enum import auto as _auto
import ctypes as _ctypes
from os import linesep as _newline
import platform as _platform


# Determine the calling convention to use.
_IS_WINDOWS = 'Windows' == _platform.system()
if _IS_WINDOWS:
    _FUNCTYPE = _ctypes.CFUNCTYPE
else:
    _FUNCTYPE = _ctypes.CFUNCTYPE


@_unique
class EStatus(_IntEnum):
    '''! Result code returned by Azure Kinect APIs.

    Name              | Description
    ----------------- | -------------------------------------------------------
    EStatus.SUCCEEDED | Successful status.
    EStatus.FAILED    | Failed status.
    '''
    SUCCEEDED = 0
    FAILED = _auto()


@_unique
class EBufferStatus(_IntEnum):
    '''! Result code returned by Azure Kinect APIs.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    EBufferStatus.SUCCEEDED         | Successful buffer request status.
    EBufferStatus.FAILED            | Failed buffer request status.
    EBufferStatus.BUFFER_TOO_SMALL  | Buffer is too small.
    '''
    SUCCEEDED = 0
    FAILED = _auto()
    BUFFER_TOO_SMALL = _auto()


@_unique
class EWaitStatus(_IntEnum):
    '''! Result code returned by Azure Kinect APIs.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    EWaitStatus.SUCCEEDED           | Successful result status.
    EWaitStatus.FAILED              | Failed result status.
    EWaitStatus.TIMEOUT             | The request timed out.
    '''
    SUCCEEDED = 0
    FAILED = _auto()
    TIMEOUT = _auto()


@_unique
class ELogLevel(_IntEnum):
    '''! Verbosity levels of debug messaging.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    ELogLevel.CRITICAL              | Most severe level of debug messaging.
    ELogLevel.ERROR                 | 2nd most severe level of debug messaging.
    ELogLevel.WARNING               | 3nd most severe level of debug messaging.
    ELogLevel.INFO                  | 2nd least severe level of debug messaging.
    ELogLevel.TRACE                 | Least severe level of debug messaging.
    ELogLevel.OFF                   | No logging is performed.
    '''
    CRITICAL = 0
    ERROR = _auto()
    WARNING = _auto()
    INFO = _auto()
    TRACE = _auto()
    OFF = _auto()


@_unique
class EDepthMode(_IntEnum):
    '''! Depth sensor capture modes.

    See the hardware specification for additional details on the field of view
    and supported frame rates for each mode.

    NFOV and WFOV denote Narrow and Wide Field of View configurations.

    Binned modes reduce the captured camera resolution by combining adjacent
    sensor pixels into a bin.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    EDepthMode.OFF                  | Depth sensor will be turned off.
    EDepthMode.NFOV_2X2BINNED       | Depth and Active IR captured at 320x288.
    EDepthMode.NFOV_UNBINNED        | Depth and Active IR captured at 640x576.
    EDepthMode.WFOV_2X2BINNED       | Depth and Active IR captured at 512x512.
    EDepthMode.WFOV_UNBINNED        | Depth and Active IR captured at 1024x1024.
    EDepthMode.PASSIVE_IR           | Passive IR only, captured at 1024x1024.
    '''
    OFF = 0
    NFOV_2X2BINNED = _auto()
    NFOV_UNBINNED = _auto()
    WFOV_2X2BINNED = _auto()
    WFOV_UNBINNED = _auto()
    PASSIVE_IR = _auto()


@_unique
class EColorResolution(_IntEnum):
    '''! Color sensor resolutions, width * height and aspect ratio.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    EColorResolution.OFF            | Color camera will be turned off.
    EColorResolution.RES_720P       | 1280 * 720  16:9.
    EColorResolution.RES_1080P      | 1920 * 1080 16:9.
    EColorResolution.RES_1440P      | 2560 * 1440 16:9.
    EColorResolution.RES_1536P      | 2048 * 1536 4:3.
    EColorResolution.RES_2160P      | 3840 * 2160 16:9.
    EColorResolution.RES_3072P      | 4096 * 3072 4:3.
    '''
    OFF = 0
    RES_720P = _auto()
    RES_1080P = _auto()
    RES_1440P = _auto()
    RES_1536P = _auto()
    RES_2160P = _auto()
    RES_3072P = _auto()


@_unique
class EImageFormat(_IntEnum):
    '''! Image format type.
    
    The image format indicates how the buffer image data is interpreted.

    <table>
    <tr style="background-color: #323C7D; color: #ffffff;">
    <td> Name </td><td> Description </td></tr>
    
    <tr><td> EImageFormat.COLOR_MJPG </td>
    <td> Color image type MJPG. 
    - The buffer for each image is encoded as a JPEG and can be
      decoded by a JPEG decoder.
    - Because the image is compressed, the stride parameter for the
      image is not applicable.
    - Each MJPG encoded image in a stream may be of differing size
      depending on the compression efficiency.
    </td></tr>

    <tr><td> EImageFormat.COLOR_NV12 </td>
    <td> Color image type NV12.
    - NV12 images separate the luminance and chroma data such that
      all the luminance is at the beginning of the buffer, and the
      chroma lines follow immediately after.
    - Stride indicates the length of each line in bytes and should
      be used to determine the start location of each line of the
      image in memory. Chroma has half as many lines of height and
      half the width in pixels of the luminance. Each chroma line
      has the same width in bytes as a luminance line.
      </td></tr>
    
    <tr><td> EImageFormat.COLOR_YUY2 </td>
    <td> Color image type YUY2.
    - YUY2 stores chroma and luminance data in interleaved pixels.
    - Stride indicates the length of each line in bytes and should
      be used to determine the start location of each line of the
      image in memory.
    </td></tr>

    <tr><td> EImageFormat.COLOR_BGRA32 </td>
    <td> Color image type BGRA32.
    - Each pixel of BGRA32 data is four bytes. The first three 
      bytes represent Blue, Green, and Red data. The fourth byte
      is the alpha channel and is unused in Azure Kinect APIs.
    - Stride indicates the length of each line in bytes and should
      be used to determine the start location of each line of the
      image in memory.
    - The Azure Kinect device does not natively capture in this
      format. Requesting images of this format requires additional
      computation in the API.
    </td></tr>

    <tr><td> EImageFormat.DEPTH16 </td>
    <td> Depth image type DEPTH16.
    - Each pixel of DEPTH16 data is two bytes of little endian
      unsigned depth data. The unit of the data is in millimeters
      from the origin of the camera.
    - Stride indicates the length of each line in bytes and should
      be used to determine the start location of each line of the
      image in memory.
    </td></tr>

    <tr><td> EImageFormat.IR16 </td>
    <td> Image type IR16.
    - Each pixel of IR16 data is two bytes of little endian 
      unsigned depth data. The value of the data represents 
      brightness.
    - This format represents infrared light and is captured by the
      depth camera.
    - Stride indicates the length of each line in bytes and should
      be used to determine the start location of each line of the
      image in memory.
    </td></tr>

    <tr><td> EImageFormat.CUSTOM8 </td>
    <td> Single channel image type CUSTOM8.
    - Each pixel of CUSTOM8 is a single channel one byte of 
      unsigned data.
    - Stride indicates the length of each line in bytes and should
      be used to determine the start location of each line of the
      image in memory.
    </td></tr>

    <tr><td> EImageFormat.CUSTOM16 </td>
    <td> Single channel image type CUSTOM16.
    - Each pixel of CUSTOM16 is a single channel two byte of 
      unsigned data.
    - Stride indicates the length of each line in bytes and should
      be used to determine the start location of each line of the
      image in memory.
    </td></tr> 

    <tr><td> EImageFormat.CUSTOM </td>
    <td> Custom image format.
    - Used in conjunction with user created images or images 
      packing non-standard data.
    - See the originator of the custom formatted image for 
      information on how to interpret the data.
    </td></tr>
    </table>
    '''

    
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
    '''! Transformation interpolation type.

    Interpolation type used with transformation from depth image to color
    camera custom.

    Name                                | Description
    ----------------------------------- | -------------------------------------
    ETransformInterpolationType.NEAREST | Nearest neighbor interpolation.
    ETransformInterpolationType.LINEAR  | Linear interpolation.
    '''
    NEAREST = 0
    LINEAR = _auto()


@_unique
class EFramesPerSecond(_IntEnum):
    '''! Color and depth sensor frame rate.

    This enumeration is used to select the desired frame rate to operate the
    cameras. The actual frame rate may vary slightly due to dropped data, 
    synchronization variation between devices, clock accuracy, or if the camera
    exposure priority mode causes reduced frame rate.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    EFramesPerSecond.FPS_5          | 5 frames per second.
    EFramesPerSecond.FPS_15         | 15 frames per second.
    EFramesPerSecond.FPS_30         | 30 frames per second.
    '''
    FPS_5 = 0
    FPS_15 = _auto()
    FPS_30 = _auto()


@_unique
class EColorControlCommand(_IntEnum):
    '''! Color sensor control commands

    The current settings can be read with k4a_device_get_color_control(). The
    settings can be set with k4a_device_set_color_control().

    Control values set on a device are reset only when the device is power 
    cycled. The device will retain the settings even if the _DeviceHandle is 
    closed or the application is restarted.

    <table>
    <tr style="background-color: #323C7D; color: #ffffff;">
    <td> Name </td><td> Description </td></tr>
    
    <tr><td> EColorControlCommand.EXPOSURE_TIME_ABSOLUTE </td>
    <td> Exposure time setting.
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
    </td></tr>

    <tr><td> EColorControlCommand.AUTO_EXPOSURE_PRIORITY </td>
    <td> Exposure or Framerate priority setting.
    - May only be set to EColorControlMode.MANUAL.
    - Value of 0 means framerate priority. 
      Value of 1 means exposure priority.
    - Using exposure priority may impact the framerate
      of both the color and depth cameras.
    - Deprecated starting in 1.2.0. Please discontinue 
      usage, firmware does not support this.
    </td></tr>

    <tr><td> EColorControlCommand.BRIGHTNESS </td>
    <td> Brightness setting.
    - May only be set to EColorControlMode.MANUAL.
    - The valid range is 0 to 255. The default value is 128.
    </td></tr>

    <tr><td> EColorControlCommand.CONTRAST </td>
    <td> Contrast setting.
    - May only be set to EColorControlMode.MANUAL.
    </td></tr>

    <tr><td> EColorControlCommand.SATURATION </td>
    <td> Saturation setting.
    - May only be set to EColorControlMode.MANUAL.
    </td></tr>

    <tr><td> EColorControlCommand.SHARPNESS </td>
    <td> Sharpness setting.
    - May only be set to EColorControlMode.MANUAL.
    </td></tr>

    <tr><td> EColorControlCommand.WHITEBALANCE </td>
    <td> White balance setting.
    - May be set to EColorControlMode.AUTO or 
      EColorControlMode.MANUAL.
    - The unit is degrees Kelvin. The setting must be
      set to a value evenly divisible by 10 degrees.
    </td></tr>

    <tr><td> EColorControlCommand.BACKLIGHT_COMPENSATION </td>
    <td> Backlight compensation setting.
    - May only be set to EColorControlMode.MANUAL.
    - Value of 0 means backlight compensation is disabled. 
      Value of 1 means backlight compensation is enabled.
    </td></tr>

    <tr><td> EColorControlCommand.GAIN </td>
    <td> Gain setting.
    - May only be set to EColorControlMode.MANUAL. 
    </td></tr>

    <tr><td> EColorControlCommand.POWERLINE_FREQUENCY </td>
    <td> Powerline frequency setting.
    - May only be set to EColorControlMode.MANUAL.
    - Value of 1 sets the powerline compensation to 50 Hz. 
      Value of 2 sets the powerline compensation to 60 Hz.
    </td></tr>
    </table>
    '''
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
    '''! Color sensor control mode.

    The current settings can be read with k4a_device_get_color_control(). The
    settings can be set with k4a_device_set_color_control().

    Name                            | Description
    ------------------------------- | -----------------------------------------
    EColorControlMode.AUTO          | Set EColorControlCommand to auto.
    EColorControlMode.MANUAL        | Set EColorControlCommand to manual.
    '''
    AUTO = 0
    MANUAL = _auto()


@_unique
class EWiredSyncMode(_IntEnum):
    '''! Synchronization mode when connecting two or more devices together.

    <table>
    <tr style="background-color: #323C7D; color: #ffffff;">
    <td> Name </td><td> Description </td></tr>
    
    <tr><td> EWiredSyncMode.STANDALONE </td>
    <td> Neither 'Sync In' or 'Sync Out' connections are used. </td></tr>

    <tr><td> EWiredSyncMode.MASTER </td>
    <td> The 'Sync Out' jack is enabled and synchronization data it
         driven out the connected wire. While in master mode the color
         camera must be enabled as part of the multi device sync
         signalling logic. Even if the color image is not needed, the
         color camera must be running.
    </td></tr>

    <tr><td> EWiredSyncMode.SUBORDINATE </td>
    <td> The 'Sync In' jack is used for synchronization and 'Sync Out'
         is driven for the next device in the chain. 'Sync Out' is a
         mirror of 'Sync In' for this mode.
    </td></tr>
    </table>
    '''
    STANDALONE = 0
    MASTER = _auto()
    SUBORDINATE = _auto()


@_unique
class ECalibrationType(_IntEnum):
    '''! Calibration types.

    Specifies a type of calibration.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    ECalibrationType.UNKNOWN        | Calibration type is unknown.
    ECalibrationType.DEPTH          | Depth sensor.
    ECalibrationType.COLOR          | Color sensor.
    ECalibrationType.GYRO           | Gyroscope sensor.
    ECalibrationType.ACCEL          | Acceleremeter sensor.
    ECalibrationType.NUM_TYPES      | Number of types excluding unknown type.
    '''
    UNKNOWN = -1
    DEPTH = _auto()
    COLOR = _auto()
    GYRO = _auto()
    ACCEL = _auto()
    NUM_TYPES = _auto()


@_unique
class ECalibrationModelType(_IntEnum):
    '''! Calibration model type.

    The model used interpret the calibration parameters.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    ECalibrationModelType.LENS_DISTORTION_MODEL_UNKNOWN | Calibration model is unknown.
    ECalibrationModelType.LENS_DISTORTION_MODEL_THETA |Deprecated (not supported). Calibration model is Theta (arctan).
    ECalibrationModelType.LENS_DISTORTION_MODEL_POLYNOMIAL_3K | Deprecated (not supported). Calibration model is Polynomial 3K.
    ECalibrationModelType.LENS_DISTORTION_MODEL_RATIONAL_6KT | Deprecated (only supported early internal devices). Calibration model is Rational 6KT.
    ECalibrationModelType.LENS_DISTORTION_MODEL_BROWN_CONRADY | Calibration model is Brown Conrady (compatible with OpenCV).
    '''
    LENS_DISTORTION_MODEL_UNKNOWN = 0
    LENS_DISTORTION_MODEL_THETA = _auto()
    LENS_DISTORTION_MODEL_POLYNOMIAL_3K = _auto()
    LENS_DISTORTION_MODEL_RATIONAL_6KT = _auto()
    LENS_DISTORTION_MODEL_BROWN_CONRADY = _auto()


@_unique
class EFirmwareBuild(_IntEnum):
    '''! Firmware build type.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    EFirmwareBuild.RELEASE          | Production firmware.
    EFirmwareBuild.DEBUG            | Pre-production firmware.
    '''
    RELEASE = 0
    DEBUG = _auto()


@_unique
class EFirmwareSignature(_IntEnum):
    '''! Firmware signature type.

    Name                            | Description
    ------------------------------- | -----------------------------------------
    EFirmwareSignature.MSFT         | Microsoft signed firmware.
    EFirmwareSignature.TEST         | Test signed firmware.
    EFirmwareSignature.UNSIGNED     | Unsigned firmware.
    '''
    MSFT = 0
    TEST = _auto()
    UNSIGNED = _auto()


#define K4A_SUCCEEDED(_result_) (_result_ == SUCCEEDED)
def K4A_SUCCEEDED(result:EStatus):
    '''! Validate that an EStatus is successful.

    @param result (EStatus): An EStatus returned by some functions in the K4A API.
                
    @returns True if result is a successful status, False otherwise.
    '''
    return result == EStatus.SUCCEEDED


#define K4A_FAILED(_result_) (!K4A_SUCCEEDED(_result_))
def K4A_FAILED(result):
    '''! Validate that an EStatus is failed.

    @param result (EStatus): An EStatus returned by some functions in the K4A API.
                
    @returns True if result is a failed status, False otherwise.
    '''
    return not K4A_SUCCEEDED(result)


#typedef void(k4a_logging_message_cb_t)(void *context,
#                                       ELogLevel level,
#                                       const char *file,
#                                       const int line,
#                                       const char *message);
logging_message_cb = _FUNCTYPE(None,
    _ctypes.c_void_p, _ctypes.c_int, _ctypes.POINTER(_ctypes.c_char), 
    _ctypes.c_int, _ctypes.POINTER(_ctypes.c_char))


#typedef void(k4a_memory_destroy_cb_t)(void *buffer, void *context);
_memory_destroy_cb = _FUNCTYPE(
    None, _ctypes.c_void_p, _ctypes.c_void_p)


#typedef uint8_t *(k4a_memory_allocate_cb_t)(int size, void **context);
_memory_allocate_cb = _FUNCTYPE(
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
    '''! Configuration parameters for an Azure Kinect device.

    @remarks 
    - Used by Device.start_cameras() to specify the configuration of the
        data capture.

    <table>
    <tr style="background-color: #323C7D; color: #ffffff;">
    <td> Field Name </td><td> Type </td><td> Description </td>
    </tr>
    
    <tr>
    <td> color_format </td>
    <td> EImageFormat </td>
    <td> Image format to capture with the color camera.
         - The color camera does not natively produce BGRA32 images.
           Setting to EImageFormat.BGRA32 for color_format will result in
           higher CPU utilization.
    </td>
    </tr>

    <tr>
    <td> color_resolution </td>
    <td> EColorResolution </td>
    <td> Image resolution to capture with the color camera.
    </td>
    </tr>

    <tr>
    <td> depth_mode </td>
    <td> EDepthMode </td>
    <td> Capture mode for the depth camera.
    </td>
    </tr>

    <tr>
    <td> camera_fps </td>
    <td> EFramesPerSecond </td>
    <td> Desired frame rate for the color and depth camera.
    </td>
    </tr>

    <tr>
    <td> synchronized_images_only </td>
    <td> bool </td>
    <td> Only produce k4a_capture_t objects if they contain synchronized color
         and depth images.
         - This setting controls the behavior in which images are dropped when
           images are produced faster than they can be read, or if there are
           errors in reading images from the device.
         - If set to True, Capture objects will only be produced with both
           color and depth images. If set to False, Capture objects may be
           produced with only a single image when the corresponding image is
           dropped.
         - Setting this to False ensures that the caller receives all of the
           images received from the camera, regardless of whether the 
           corresponding images expected in the capture are available.
         - If either the color or depth camera are disabled, this setting has
           no effect.
    </td>
    </tr>

    <tr>
    <td> depth_delay_off_color_usec </td>
    <td> int </td>
    <td> Desired delay in microseconds between the capture of the color image 
         and the capture of the depth image.
         - A negative value indicates that the depth image should be captured
           before the color image.
         - Any value between negative and positive one capture period is valid.
    </td>
    </tr>

    <tr>
    <td> wired_sync_mode </td>
    <td> EWiredSyncMode </td>
    <td> The external synchronization mode.
    </td>
    </tr>

    <tr>
    <td> subordinate_delay_off_master_usec </td>
    <td> int </td>
    <td> The external synchronization timing.
         - If this camera is a subordinate, this sets the capture delay between
           the color camera capture and the external input pulse. A setting of
           zero indicates that the master and subordinate color images should
           be aligned.
         - This setting does not effect the 'Sync out' connection.
         - This value must be positive and range from zero to one capture period.
         - If this is not a subordinate, then this value is ignored.
    </td>
    </tr>

    <tr>
    <td> disable_streaming_indicator </td>
    <td> bool </td>
    <td> Streaming indicator automatically turns on when the color or depth
         cameras are in use.
         - This setting disables that behavior and keeps the LED in an off state.
    </td>
    </tr>
    
    </table>
    '''
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

    def __init__(self, 
        color_format:EImageFormat=EImageFormat.CUSTOM,
        color_resolution:EColorResolution=EColorResolution.RES_720P,
        depth_mode:EDepthMode=EDepthMode.OFF,
        camera_fps:EFramesPerSecond=EFramesPerSecond.FPS_5,
        synchronized_images_only:bool=True,
        depth_delay_off_color_usec:int=0,
        wired_sync_mode:EWiredSyncMode=EWiredSyncMode.STANDALONE,
        subordinate_delay_off_master_usec:int=0,
        disable_streaming_indicator:bool=False):

        self.color_format = color_format
        self.color_resolution = color_resolution
        self.depth_mode = depth_mode
        self.camera_fps = camera_fps
        self.synchronized_images_only = synchronized_images_only
        self.depth_delay_off_color_usec = depth_delay_off_color_usec
        self.wired_sync_mode = wired_sync_mode
        self.subordinate_delay_off_master_usec = subordinate_delay_off_master_usec
        self.disable_streaming_indicator = disable_streaming_indicator

    def __str__(self):
        return ''.join([
            'color_format=%d, ',
            'color_resolution=%d, ',
            'depth_mode=%d, ',
            'camera_fps=%d, ',
            'synchronized_images_only=%s, ',
            'depth_delay_off_color_usec=%d, ',
            'wired_sync_mode=%d, ',
            'subordinate_delay_off_master_usec=%d, ',
            'disable_streaming_indicator=%s']) % (
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
    '''! Extrinsic calibration data.

    @remarks 
    - Extrinsic calibration defines the physical relationship between
        two separate devices.

    Name          | Type       | Description
    ------------- | ---------- | ----------------------------------------------
    rotation      | float * 9  | 3x3 Rotation matrix stored in row major order.
    translation   | float * 3  | Translation vector, x,y,z (in millimeters).
    '''
    _fields_= [
        ("rotation", (_ctypes.c_float * 3) * 3),
        ("translation", _ctypes.c_float * 3),
    ]

    def __str__(self):
        return ''.join([
            'rotation=[[%f,%f,%f][%f,%f,%f][%f,%f,%f]] ',
            'translation=[%f,%f,%f]']) % (
            self.rotation[0][0], self.rotation[0][1], self.rotation[0][2],
            self.rotation[1][0], self.rotation[1][1], self.rotation[1][2],
            self.rotation[2][0], self.rotation[2][1], self.rotation[2][2],
            self.translation[0], self.translation[1], self.translation[2])


class CalibrationIntrinsicParam(_ctypes.Structure):
    '''! Camera intrinsic calibration data.

    @remarks 
    - Intrinsic calibration represents the internal optical properties
        of the camera.

    @remarks 
    - Azure Kinect devices are calibrated with Brown Conrady which is
        compatible with OpenCV.

    Name          | Type       | Description
    ------------- | ---------- | ----------------------------------------------
    cx            | float      | Principal point in image, x.
    cy            | float      | Principal point in image, y.
    fx            | float      | Focal length x.
    fy            | float      | Focal length y.
    k1            | float      | k1 radial distortion coefficient.
    k2            | float      | k2 radial distortion coefficient.
    k3            | float      | k3 radial distortion coefficient.
    k4            | float      | k4 radial distortion coefficient.
    k5            | float      | k5 radial distortion coefficient.
    k6            | float      | k6 radial distortion coefficient.
    codx          | float      | Center of distortion in Z=1 plane, x (only used for Rational6KT).
    cody          | float      | Center of distortion in Z=1 plane, y (only used for Rational6KT).
    p2            | float      | Tangential distortion coefficient 2.
    p1            | float      | Tangential distortion coefficient 1.
    metric_radius | float      | Metric radius.
    '''
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

    def __str__(self):
        return ''.join([
            'cx=%f, cy=%f, ',
            'fx=%f, fy=%f, ',
            'k1=%f, k2=%f, k3=%f, k4=%f, k5=%f, k6=%f, ',
            'codx=%f, cody=%f, ',
            'p2=%f, p1=%f, ',
            'metric_radius=%f']) % (
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

    def __str__(self):
        return self.param.__str__()


class CalibrationIntrinsics(_ctypes.Structure):
    '''! Camera sensor intrinsic calibration data.

    @remarks 
    - Intrinsic calibration represents the internal optical properties
        of the camera.

    @remarks 
    - Azure Kinect devices are calibrated with Brown Conrady which is
        compatible with OpenCV.

    Name            | Type             | Description
    --------------- | ---------------- | ----------------------------------------------
    type            | ECalibrationType | Type of calibration model used.
    parameter_count | int              | Number of valid entries in parameters.
    parameters      | struct           | Calibration parameters.
    '''
    _fields_= [
        ("type", _ctypes.c_int),
        ("parameter_count", _ctypes.c_uint),
        ("parameters", _CalibrationIntrinsicParameters),
    ]

    def __str__(self):
        return ''.join([
            'type=%d, ',
            'parameter_count=%d, ',
            'parameters=%s']) % (
            self.type,
            self.parameter_count,
            self.parameters.__str__())


class CalibrationCamera(_ctypes.Structure):
    '''! Camera calibration contains intrinsic and extrinsic calibration
    information for a camera.

    Name              | Type                  | Description
    ----------------- | --------------------- | ----------------------------------------------
    extrinsics        | CalibrationExtrinsics | Extrinsic calibration data.
    intrinsics        | CalibrationIntrinsics | Intrinsic calibration data.
    resolution_width  | int                   | Resolution width of the calibration sensor.
    resolution_height | int                   | Resolution height of the calibration sensor.
    metric_radius     | float                 | Max FOV of the camera.
    '''
    _fields_= [
        ("extrinsics", CalibrationExtrinsics),
        ("intrinsics", CalibrationIntrinsics),
        ("resolution_width", _ctypes.c_int),
        ("resolution_height", _ctypes.c_int),
        ("metric_radius", _ctypes.c_float),
    ]

    def __str__(self):
        return ''.join([
            'extrinsics=%s, ',
            'intrinsics=%s, ',
            'resolution_width=%d, ',
            'resolution_height=%d, ',
            'metric_radius=%f',]) % (
            self.extrinsics.__str__(),
            self.intrinsics.__str__(),
            self.resolution_width,
            self.resolution_height,
            self.metric_radius)


class _Calibration(_ctypes.Structure):
    _fields_= [
        ("depth_camera_calibration", CalibrationCamera),
        ("color_camera_calibration", CalibrationCamera),
        ("extrinsics", (CalibrationExtrinsics * ECalibrationType.NUM_TYPES) * ECalibrationType.NUM_TYPES),
        ("depth_mode", _ctypes.c_int),
        ("color_resolution", _ctypes.c_int),
    ]

    def __str__(self):
        s = ''.join([
            'depth_camera_calibration=%s, ',
            'color_camera_calibration=%s, ']) % (
                self.depth_camera_calibration.__str__(),
                self.color_camera_calibration.__str__(),
            )
        
        for r in range(ECalibrationType.NUM_TYPES):
            for c in range(ECalibrationType.NUM_TYPES):
                s = ''.join([s, 'extrinsics[%d][%d]=%s, ']) % (r, c, self.extrinsics[r][c].__str__())

        s = ''.join([s,
            'depth_mode=%d, ',
            'color_resolution=%d']) % (
                self.depth_mode,
                self.color_resolution
            )

        return s


class Version(_ctypes.Structure):
    '''! Version information.

    Name       | Type    | Description
    ---------- | ------- | ----------------------------------------------
    major      | int     | Major version; represents a breaking change.
    minor      | int     | Minor version; represents additional features, no regression from lower versions with same major version.
    iteration  | int     | Reserved.
    '''
    _fields_= [
        ("major", _ctypes.c_uint32),
        ("minor", _ctypes.c_uint32),
        ("iteration", _ctypes.c_uint32),
    ]

    def __str__(self):
        return ''.join(['%d.%d.%d']) % (
            self.major,
            self.minor,
            self.iteration)


class HardwareVersion(_ctypes.Structure):
    '''! Structure to define hardware version.

    Name               | Type    | Description
    ------------------ | ------- | ----------------------------------------------
    rgb                | Version | Color camera firmware version.
    depth              | Version | Depth camera firmware version.
    audio              | Version | Audio device firmware version.
    depth              | Version | Depth sensor firmware version.
    firmware_build     | int     | Build type reported by the firmware.
    firmware_signature | int     | Signature type of the firmware.
    '''
    _fields_= [
        ("rgb", Version),
        ("depth", Version),
        ("audio", Version),
        ("depth_sensor", Version),
        ("firmware_build", _ctypes.c_int),
        ("firmware_signature", _ctypes.c_int),
    ]

    def __str__(self):
        return ''.join([
            'rgb=%s, ', _newline,
            'depth=%s, ', _newline,
            'audio=%s, ', _newline,
            'depth_sensor=%s, ', _newline,
            'firmware_build=%d, ', _newline,
            'firmware_signature=%d']) % (
            self.rgb.__str__(),
            self.depth.__str__(),
            self.audio.__str__(),
            self.depth_sensor.__str__(),
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

    def __str__(self):
        return ''.join(['x=%f, ', 'y=%f']) % (self.x, self.y)


class _Float2(_ctypes.Union):
    _fields_= [
        ("xy", _XY),
        ("v", _ctypes.c_float * 2),
        ]

    def __init__(self, x=0, y=0):
        self.xy = _XY(x, y)

    def __str__(self):
        return self.xy.__str__()


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

    def __str__(self):
        return ''.join(['x=%f, ', 'y=%f, ', 'z=%f']) % (self.x, self.y, self.z)


class _Float3(_ctypes.Union):
    _fields_= [
        ("xyz", _XYZ),
        ("v", _ctypes.c_float * 3)
    ]

    def __init__(self, x=0, y=0, z=0):
        self.xyz = _XYZ(x, y, z)

    def __str__(self):
        return self.xyz.__str__()


class ImuSample(_ctypes.Structure):
    '''! IMU Sample.

    Name                | Type      | Description
    ------------------- | --------- | ----------------------------------------------
    temperature         | float     | Temperature reading of this sample (Celsius).
    acc_sample          | float * 3 | Accelerometer sample in meters per second squared.
    acc_timestamp_usec  | int       | Timestamp of the accelerometer in microseconds.
    gyro_sample         | float * 3 | Gyro sample in radians per second.
    gyro_timestamp_usec | int       | Timestamp of the gyroscope in microseconds.
    '''
    _fields_= [
        ("temperature", _ctypes.c_float),
        ("acc_sample", _Float3),
        ("acc_timestamp_usec", _ctypes.c_uint64),
        ("gyro_sample", _Float3),
        ("gyro_timestamp_usec", _ctypes.c_uint64),
    ]

    def __str__(self):
        return ''.join([
            'temperature=%f, ',
            'acc_sample=%s, ',
            'acc_timestamp_usec=%lu, ',
            'gyro_sample=%s, ',
            'gyro_timestamp_usec=%lu']) % (
            self.temperature,
            self.acc_sample.__str__(),
            self.acc_timestamp_usec,
            self.gyro_sample.__str__(),
            self.gyro_timestamp_usec)


# An empty class for appending fields dynamically.
class _EmptyClass:
    
    def __str__(self):
        keys = list(self.__dict__.keys())
        tempstr = ''

        if len(keys) > 0:
            for n in range(len(keys)-1):
                tempstr = tempstr + str(keys[n]) + "=" + str(self.__dict__[keys[n]]) + ", "
            tempstr = tempstr + str(keys[len(keys)-1]) + "=" + str(self.__dict__[keys[len(keys)-1]])

        return tempstr


# ############# Define global instances of device configurations. #############

DEVICE_CONFIG_DISABLE_ALL = DeviceConfiguration(
    color_format = EImageFormat.COLOR_MJPG,
    color_resolution = EColorResolution.OFF,
    depth_mode = EDepthMode.OFF,
    camera_fps = EFramesPerSecond.FPS_30,
    synchronized_images_only = False,
    depth_delay_off_color_usec = 0,
    wired_sync_mode = EWiredSyncMode.STANDALONE,
    subordinate_delay_off_master_usec = 0,
    disable_streaming_indicator = False)

DEVICE_CONFIG_BGRA32_2160P_WFOV_UNBINNED_FPS15 = DeviceConfiguration(
    color_format = EImageFormat.COLOR_BGRA32,
    color_resolution = EColorResolution.RES_2160P,
    depth_mode = EDepthMode.WFOV_UNBINNED,
    camera_fps = EFramesPerSecond.FPS_15,
    synchronized_images_only = True,
    depth_delay_off_color_usec = 0,
    wired_sync_mode = EWiredSyncMode.STANDALONE,
    subordinate_delay_off_master_usec = 0,
    disable_streaming_indicator = False)

DEVICE_CONFIG_BGRA32_2160P_WFOV_2X2BINNED_FPS15 = DeviceConfiguration(
    color_format = EImageFormat.COLOR_BGRA32,
    color_resolution = EColorResolution.RES_2160P,
    depth_mode = EDepthMode.WFOV_2X2BINNED,
    camera_fps = EFramesPerSecond.FPS_15,
    synchronized_images_only = True,
    depth_delay_off_color_usec = 0,
    wired_sync_mode = EWiredSyncMode.STANDALONE,
    subordinate_delay_off_master_usec = 0,
    disable_streaming_indicator = False)

DEVICE_CONFIG_BGRA32_2160P_NFOV_UNBINNED_FPS15 = DeviceConfiguration(
    color_format = EImageFormat.COLOR_BGRA32,
    color_resolution = EColorResolution.RES_2160P,
    depth_mode = EDepthMode.NFOV_UNBINNED,
    camera_fps = EFramesPerSecond.FPS_15,
    synchronized_images_only = True,
    depth_delay_off_color_usec = 0,
    wired_sync_mode = EWiredSyncMode.STANDALONE,
    subordinate_delay_off_master_usec = 0,
    disable_streaming_indicator = False)

DEVICE_CONFIG_BGRA32_2160P_NFOV_2X2BINNED_FPS15 = DeviceConfiguration(
    color_format = EImageFormat.COLOR_BGRA32,
    color_resolution = EColorResolution.RES_2160P,
    depth_mode = EDepthMode.NFOV_2X2BINNED,
    camera_fps = EFramesPerSecond.FPS_15,
    synchronized_images_only = True,
    depth_delay_off_color_usec = 0,
    wired_sync_mode = EWiredSyncMode.STANDALONE,
    subordinate_delay_off_master_usec = 0,
    disable_streaming_indicator = False)

DEVICE_CONFIG_BGRA32_1080P_WFOV_UNBINNED_FPS15 = DeviceConfiguration(
    color_format = EImageFormat.COLOR_BGRA32,
    color_resolution = EColorResolution.RES_1080P,
    depth_mode = EDepthMode.WFOV_UNBINNED,
    camera_fps = EFramesPerSecond.FPS_15,
    synchronized_images_only = True,
    depth_delay_off_color_usec = 0,
    wired_sync_mode = EWiredSyncMode.STANDALONE,
    subordinate_delay_off_master_usec = 0,
    disable_streaming_indicator = False)

DEVICE_CONFIG_BGRA32_1080P_WFOV_2X2BINNED_FPS15 = DeviceConfiguration(
    color_format = EImageFormat.COLOR_BGRA32,
    color_resolution = EColorResolution.RES_1080P,
    depth_mode = EDepthMode.WFOV_2X2BINNED,
    camera_fps = EFramesPerSecond.FPS_15,
    synchronized_images_only = True,
    depth_delay_off_color_usec = 0,
    wired_sync_mode = EWiredSyncMode.STANDALONE,
    subordinate_delay_off_master_usec = 0,
    disable_streaming_indicator = False)

DEVICE_CONFIG_BGRA32_1080P_NFOV_UNBINNED_FPS15 = DeviceConfiguration(
    color_format = EImageFormat.COLOR_BGRA32,
    color_resolution = EColorResolution.RES_1080P,
    depth_mode = EDepthMode.NFOV_UNBINNED,
    camera_fps = EFramesPerSecond.FPS_15,
    synchronized_images_only = True,
    depth_delay_off_color_usec = 0,
    wired_sync_mode = EWiredSyncMode.STANDALONE,
    subordinate_delay_off_master_usec = 0,
    disable_streaming_indicator = False)

DEVICE_CONFIG_BGRA32_1080P_NFOV_2X2BINNED_FPS15 = DeviceConfiguration(
    color_format = EImageFormat.COLOR_BGRA32,
    color_resolution = EColorResolution.RES_1080P,
    depth_mode = EDepthMode.NFOV_2X2BINNED,
    camera_fps = EFramesPerSecond.FPS_15,
    synchronized_images_only = True,
    depth_delay_off_color_usec = 0,
    wired_sync_mode = EWiredSyncMode.STANDALONE,
    subordinate_delay_off_master_usec = 0,
    disable_streaming_indicator = False)
