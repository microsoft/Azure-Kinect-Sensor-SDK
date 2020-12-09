'''
k4atypes.py

Defines common enums and structures used in the Azure Kinect SDK.
These enums defined here are analogous to those defined in k4a.h.

Copyright (C) Microsoft Corporation. All rights reserved.
'''

from enum import IntEnum, unique, auto


@unique
class k4a_result_t(IntEnum):
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
    K4A_RESULT_FAILED = auto()


@unique
class k4a_buffer_result_t(IntEnum):
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
    K4A_BUFFER_RESULT_FAILED = auto()
    K4A_BUFFER_RESULT_TOO_SMALL = auto()


@unique
class k4a_wait_result_t(IntEnum):
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
    K4A_WAIT_RESULT_FAILED = auto()
    K4A_WAIT_RESULT_TIMEOUT = auto()


@unique
class k4a_log_level_t(IntEnum):
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
    K4A_LOG_LEVEL_ERROR = auto()
    K4A_LOG_LEVEL_WARNING = auto()
    K4A_LOG_LEVEL_INFO = auto()
    K4A_LOG_LEVEL_TRACE = auto()
    K4A_LOG_LEVEL_OFF = auto()


@unique
class k4a_depth_mode_t(IntEnum):
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
    K4A_DEPTH_MODE_NFOV_2X2BINNED = auto()
    K4A_DEPTH_MODE_NFOV_UNBINNED = auto()
    K4A_DEPTH_MODE_WFOV_2X2BINNED = auto()
    K4A_DEPTH_MODE_WFOV_UNBINNED = auto()
    K4A_DEPTH_MODE_PASSIVE_IR = auto()


@unique
class k4a_color_resolution_t(IntEnum):
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
    K4A_COLOR_RESOLUTION_720P = auto()
    K4A_COLOR_RESOLUTION_1080P = auto()
    K4A_COLOR_RESOLUTION_1440P = auto()
    K4A_COLOR_RESOLUTION_1536P = auto()
    K4A_COLOR_RESOLUTION_2160P = auto()


@unique
class k4a_image_format_t(IntEnum):
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
    K4A_IMAGE_FORMAT_COLOR_NV12 = auto()
    K4A_IMAGE_FORMAT_COLOR_YUY2 = auto()
    K4A_IMAGE_FORMAT_COLOR_BGRA32 = auto()
    K4A_IMAGE_FORMAT_DEPTH16 = auto()
    K4A_IMAGE_FORMAT_IR16 = auto()
    K4A_IMAGE_FORMAT_CUSTOM8 = auto()
    K4A_IMAGE_FORMAT_CUSTOM16 = auto()
    K4A_IMAGE_FORMAT_CUSTOM = auto()


@unique
class k4a_transformation_interpolation_type_t(IntEnum):
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
    K4A_TRANSFORMATION_INTERPOLATION_TYPE_LINEAR = auto()


@unique
class k4a_fps_t(IntEnum):
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
    K4A_FRAMES_PER_SECOND_15 = auto()
    K4A_FRAMES_PER_SECOND_30 = auto()


@unique
class k4a_color_control_command_t(IntEnum):
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
                                             - May be set to K4A_COLOR_CONTROL_MODE_AUTO
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
                                             - May be set to K4A_COLOR_CONTROL_MODE_AUTO 
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
    K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY = auto()
    K4A_COLOR_CONTROL_BRIGHTNESS = auto()
    K4A_COLOR_CONTROL_CONTRAST = auto()
    K4A_COLOR_CONTROL_SATURATION = auto()
    K4A_COLOR_CONTROL_SHARPNESS = auto()
    K4A_COLOR_CONTROL_WHITEBALANCE = auto()
    K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION = auto()
    K4A_COLOR_CONTROL_GAIN = auto()
    K4A_COLOR_CONTROL_POWERLINE_FREQUENCY = auto()


@unique
class k4a_color_control_mode_t(IntEnum):
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
    K4A_COLOR_CONTROL_MODE_MANUAL = auto()


@unique
class k4a_wired_sync_mode_t(IntEnum):
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
    K4A_WIRED_SYNC_MODE_MASTER = auto()
    K4A_WIRED_SYNC_MODE_SUBORDINATE = auto()


@unique
class k4a_calibration_type_t(IntEnum):
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
    K4A_CALIBRATION_TYPE_DEPTH = auto()
    K4A_CALIBRATION_TYPE_COLOR = auto()
    K4A_CALIBRATION_TYPE_GYRO = auto()
    K4A_CALIBRATION_TYPE_ACCEL = auto()
    K4A_CALIBRATION_TYPE_NUM = auto()


@unique
class k4a_calibration_model_type_t(IntEnum):
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
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_THETA = auto()
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_POLYNOMIAL_3K = auto()
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT = auto()
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY = auto()


@unique
class k4a_firmware_build_t(IntEnum):
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
    K4A_FIRMWARE_BUILD_DEBUG = auto()


@unique
class k4a_firmware_signature_t(IntEnum):
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
    K4A_FIRMWARE_SIGNATURE_TEST = auto()
    K4A_FIRMWARE_SIGNATURE_UNSIGNED = auto()