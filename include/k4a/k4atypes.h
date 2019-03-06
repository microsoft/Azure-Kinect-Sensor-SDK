/** \file k4atypes.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK Type definitions.
 */

#ifndef K4ATYPES_H
#define K4ATYPES_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstddef>
#include <cstring>
#else
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Internal use only. Declare an opaque handle type.
 *
 * \param _handle_name_ The type name of the handle
 *
 * \remarks This is used to define the public handle types for the k4a APIs.
 * The macro should not be used outside of the k4a headers.
 */
#define K4A_DECLARE_HANDLE(_handle_name_)                                                                              \
    typedef struct _##_handle_name_                                                                                    \
    {                                                                                                                  \
        size_t _rsvd; /**< Reserved, do not use. */                                                                    \
    } * _handle_name_;

/** \class k4a_device_t k4a.h <k4a/k4a.h>
 * Handle to a k4a device.
 *
 * Handles are created with k4a_device_open() and closed
 * with k4a_device_close().
 * Invalid handles are set to 0.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_DECLARE_HANDLE(k4a_device_t);

/** \class k4a_capture_t k4a.h <k4a/k4a.h>
 * Handle to a k4a capture.
 *
 * Handles are created with k4a_device_get_capture() or k4a_capture_create() and closed with k4a_capture_release().
 * Invalid handles are set to 0.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_DECLARE_HANDLE(k4a_capture_t);

/** \class k4a_image_t k4a.h <k4a/k4a.h>
 * Handle to a k4a image.
 *
 * Handles are created with k4a_capture_get_color_image(), k4a_capture_get_depth_image(), k4a_capture_get_ir_image(),
 * k4a_image_create(), or k4a_image_create_from_buffer() and closed with k4a_image_release(). Invalid handles are set to
 * 0.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_DECLARE_HANDLE(k4a_image_t);

/** \class k4a_transformation_t k4a.h <k4a/k4a.h>
 * Handle to a k4a transformation context.
 *
 * Handles are created with k4a_transformation_create() and closed with
 * k4a_transformation_destroy(). Invalid handles are set to 0.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_DECLARE_HANDLE(k4a_transformation_t);

/*
 * environment variables
 * K4A_ENABLE_LOG_TO_A_FILE =
 *    0    - completely disable logging to a file
 *    log\custom.log - log all messages to the path and file specified - must end in '.log' to
 *                     be considered a valid entry
 *    ** When enabled this takes precedence over the value of K4A_ENABLE_LOG_TO_STDOUT
 *
 * K4A_ENABLE_LOG_TO_STDOUT =
 *    0    - disable logging to stdout
 *    all else  - log all messages to stdout
 *
 * K4A_LOG_LEVEL =
 *    'c'  - log all messages of level 'critical' criticality
 *    'e'  - log all messages of level 'error' or higher criticality
 *    'w'  - log all messages of level 'warning' or higher criticality
 *    'i'  - log all messages of level 'info' or higher criticality
 *    't'  - log all messages of level 'trace' or higher criticality
 *    DEFAULT - log all message of level 'error' or higher criticality
 */

/** Indicates the caller of \ref k4a_device_open wants to open the default sensor
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define K4A_DEVICE_DEFAULT (0) // Open the default K4A device

/** An infinite wait time for functions that take a timeout parameter
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define K4A_WAIT_INFINITE (-1)

/** A timestamp value that should never be returned by the hardware.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define K4A_INVALID_TIMESTAMP ((uint64_t)0)

/** callback function for a memory object being destroyed
 *
 * \param buffer
 * The buffer pointer that was supplied by the caller in k4a_memory_description_t.
 *
 * \param context
 * The context for the memory object that needs to be destroyed. This was given to k4a in
 * k4a_memory_description_t.buffer_release_cb_context.
 *
 * \remarks
 * We all references for the memory object are released, this callback will be invoked as the final destroy for the
 * given memory.
 */
typedef void(k4a_memory_destroy_cb_t)(void *buffer, void *context);

/** Return codes returned by k4a APIs
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_RESULT_SUCCEEDED = 0, /**< The result was successful */
    K4A_RESULT_FAILED,        /**< The result was a failure */
} k4a_result_t;

/** Return codes returned by k4a APIs
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_BUFFER_RESULT_SUCCEEDED = 0, /**< The result was successful */
    K4A_BUFFER_RESULT_FAILED,        /**< The result was a failure */
    K4A_BUFFER_RESULT_TOO_SMALL,     /**< The input buffer was too small */
} k4a_buffer_result_t;

/** Return codes returned by k4a APIs
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_WAIT_RESULT_SUCCEEDED = 0, /**< The result was successful */
    K4A_WAIT_RESULT_FAILED,        /**< The result was a failure */
    K4A_WAIT_RESULT_TIMEOUT,       /**< The operation timed out */
} k4a_wait_result_t;

/** Validate that a k4a_result_t is successful
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define K4A_SUCCEEDED(_result_) (_result_ == K4A_RESULT_SUCCEEDED)

/** Validate that a k4a_result_t is failed
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define K4A_FAILED(_result_) (!K4A_SUCCEEDED(_result_))

/** Depth Sensor capture modes
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_DEPTH_MODE_OFF = 0,        /**< Depth sensor will be turned off with this setting */
    K4A_DEPTH_MODE_NFOV_2X2BINNED, /**< RES: 320x288 */
    K4A_DEPTH_MODE_NFOV_UNBINNED,  /**< RES: 640x576 */
    K4A_DEPTH_MODE_WFOV_2X2BINNED, /**< RES: 512x512 */
    K4A_DEPTH_MODE_WFOV_UNBINNED,  /**< RES: 1024x1024 */
    K4A_DEPTH_MODE_PASSIVE_IR,     /**< RES: 1024x1024 */
} k4a_depth_mode_t;

/** Color (RGB) sensor resolutions
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_COLOR_RESOLUTION_OFF = 0, /**< Color camera will be turned off with this setting */
    K4A_COLOR_RESOLUTION_720P,    /**< 1280 * 720  16:9 */
    K4A_COLOR_RESOLUTION_1080P,   /**< 1920 * 1080 16:9 */
    K4A_COLOR_RESOLUTION_1440P,   /**< 2560 * 1440 16:9 */
    K4A_COLOR_RESOLUTION_1536P,   /**< 2048 * 1536 4:3  */
    K4A_COLOR_RESOLUTION_2160P,   /**< 3840 * 2160 16:9 */
    K4A_COLOR_RESOLUTION_3072P,   /**< 4096 * 3072 4:3  */
} k4a_color_resolution_t;

/** Image types
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_IMAGE_FORMAT_COLOR_MJPG = 0, /**< Color image type MJPG*/
    K4A_IMAGE_FORMAT_COLOR_NV12,     /**< Color image type NV12*/
    K4A_IMAGE_FORMAT_COLOR_YUY2,     /**< Color image type YUY2*/
    K4A_IMAGE_FORMAT_COLOR_BGRA32,   /**< Color image type BGRA (8 bits per channel) */
    K4A_IMAGE_FORMAT_DEPTH16,        /**< Depth image type Depth16*/
    K4A_IMAGE_FORMAT_IR16,           /**< Depth image type IR16*/
    K4A_IMAGE_FORMAT_CUSTOM,         /**< Used inconjuction with user created images */
} k4a_image_format_t;

/** Color and Depth sensor frame rate
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_FRAMES_PER_SECOND_5 = 0, /**< 5 FPS */
    K4A_FRAMES_PER_SECOND_15,    /**< 15 FPS */
    K4A_FRAMES_PER_SECOND_30,    /**< 30 FPS */
} k4a_fps_t;

/** Color (RGB) sensor control commands
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE = 0, /**< Auto, Manual (in uSec unit) */
    K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY,     /**< Manual (0: framerate priority, 1: exposure priority) */
    K4A_COLOR_CONTROL_BRIGHTNESS,                 /**< Manual */
    K4A_COLOR_CONTROL_CONTRAST,                   /**< Manual */
    K4A_COLOR_CONTROL_SATURATION,                 /**< Manual */
    K4A_COLOR_CONTROL_SHARPNESS,                  /**< Manual */
    K4A_COLOR_CONTROL_WHITEBALANCE,               /**< Auto, Manual (in degrees Kelvin) */
    K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION,     /**< Manual (0: disable, 1: enable) */
    K4A_COLOR_CONTROL_GAIN,                       /**< Manual */
    K4A_COLOR_CONTROL_POWERLINE_FREQUENCY         /**< Manual (1: 50Hz, 2:60Hz) */
} k4a_color_control_command_t;

/** Color (RGB) sensor control mode
 *
 * Used in conjunction with k4a_color_control_command_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_COLOR_CONTROL_MODE_AUTO = 0, /**< set the associated k4a_color_control_command_t to auto*/
    K4A_COLOR_CONTROL_MODE_MANUAL,   /**< set the associated k4a_color_control_command_t to manual*/
} k4a_color_control_mode_t;

/** Calibration types
 * Specifies the calibration type.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_CALIBRATION_TYPE_UNKNOWN = -1, /**< Calibration type is unknown */
    K4A_CALIBRATION_TYPE_DEPTH,        /**< Depth sensor */
    K4A_CALIBRATION_TYPE_COLOR,        /**< Color sensor */
    K4A_CALIBRATION_TYPE_GYRO,         /**< Gyro sensor */
    K4A_CALIBRATION_TYPE_ACCEL,        /**< Accel sensor */
    K4A_CALIBRATION_TYPE_NUM,          /**< Number of types excluding unknown type*/
} k4a_calibration_type_t;

/** Calibration model type
 *
 * The type of calibration performed on the sensor
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_UNKNOWN = 0,   /**< Calibration model is unknown */
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_THETA,         /**< Calibration model is Theta (arctan) */
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_POLYNOMIAL_3K, /**< Calibration model Polynomial 3K */
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT,  /**< Calibration model Rational 6KT */
    K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY, /**< Calibration model Brown Conrady (compatible with OpenCV)
                                                          */
} k4a_calibration_model_type_t;

/** Firmware built type
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_FIRMWARE_BUILD_RELEASE, /**< Production firmware */
    K4A_FIRMWARE_BUILD_DEBUG    /**< Pre-production firmware */
} k4a_firmware_build_t;

/** Firmware signature
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_FIRMWARE_SIGNATURE_MSFT,    /**< Microsoft signed firmware */
    K4A_FIRMWARE_SIGNATURE_TEST,    /**< Test signed firmware */
    K4A_FIRMWARE_SIGNATURE_UNSIGNED /**< Unsigned firmware */
} k4a_firmware_signature_t;

/** Synchronization mode when connecting 2 or more devices together.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_WIRED_SYNC_MODE_STANDALONE, /**< Neither Sync In or Sync Out connections are used. */
    K4A_WIRED_SYNC_MODE_MASTER,     /**< The 'Sync Out' jack is enabled and synchronization data it driven out the
                                       connected wire.*/
    K4A_WIRED_SYNC_MODE_SUBORDINATE /**< The 'Sync In' jack is used for synchronization and 'Sync Out' is driven for the
                                       next device in the chain. 'Sync Out' is just a mirror of 'Sync In' for this mode.
                                     */
} k4a_wired_sync_mode_t;

/** Version information
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_version_t
{
    uint32_t major;     /**< Major version; represents a breaking change */
    uint32_t minor;     /**< Minor version; represents additional features, no regression from lower versions with same
                           major version */
    uint32_t iteration; /**< Reserved */
} k4a_version_t;

/** Structure to define hardware version
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_hardware_version_t
{
    k4a_version_t rgb;          /**< RGB version*/
    k4a_version_t depth;        /**< Depth version*/
    k4a_version_t audio;        /**< Audio version*/
    k4a_version_t depth_sensor; /**< Depth Sensor*/

    k4a_firmware_build_t firmware_build;         /**< Firmware reported hardware build */
    k4a_firmware_signature_t firmware_signature; /**< Firmware reported signature */
} k4a_hardware_version_t;

/** Structure to define configuration for k4a sensor
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_device_configuration_t
{
    /** Image format to capture with the color camera. The color camera does not natively produce BGRA32 images.
     * Setting BGRA32 for color_format will result in higher CPU utilization. */
    k4a_image_format_t color_format;

    /** Image resolution to capture with the color camera */
    k4a_color_resolution_t color_resolution;

    /** Capture mode for the depth camera */
    k4a_depth_mode_t depth_mode;

    /** Frame rate for the color and depth camera */
    k4a_fps_t camera_fps;

    /** Only report k4a_capture_t objects if they contain synchronized color and depth images. If set to false a
     * capture may be reported that contains only a single image. NOTE: this can only be enabled if both color camera
     * and depth camera's are configured to run*/
    bool synchronized_images_only;

    /**
     * Delay after the capture of the color image and the capture of the depth image. Can be any
     * value 1 period faster than the color capture (a negative value) upto 1 period after the color capture.
     */
    int32_t depth_delay_off_color_usec;

    /** The external synchronization mode*/
    k4a_wired_sync_mode_t wired_sync_mode;

    /**
     * If this camera is a subordinate, this sets the capture delay between the color camera capture and the input
     * pulse. If this is not a subordinate, then this value is ignored. */
    uint32_t subordinate_delay_off_master_usec;

    /**
     * Streaming indicator automatically turns on when the color or depth camera's are in use. This setting disables
     * that behavior and keeps the LED in an off state.*/
    bool disable_streaming_indicator;
} k4a_device_configuration_t;

/** Initial configuration setting for disabling all sensors
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
static const k4a_device_configuration_t K4A_DEVICE_CONFIG_INIT_DISABLE_ALL = { K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                                               K4A_COLOR_RESOLUTION_OFF,
                                                                               K4A_DEPTH_MODE_OFF,
                                                                               K4A_FRAMES_PER_SECOND_30,
                                                                               false,
                                                                               0,
                                                                               K4A_WIRED_SYNC_MODE_STANDALONE,
                                                                               0,
                                                                               false };

/** k4a_float2_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef union
{
    /** XY or array representation of vector */
    struct _xy
    {
        float x; /**< X representation of a vector */
        float y; /**< Y representation of a vector */
    } xy;        /**< X, Y representation of a vector */
    float v[2];  /**< Array representation of a vector */
} k4a_float2_t;

/** k4a_float3_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef union
{
    /** XYZ or array representation of vector */
    struct _xyz
    {
        float x; /**< X representation of a vector */
        float y; /**< Y representation of a vector */
        float z; /**< Z representation of a vector */
    } xyz;       /**< X, Y, Z representation of a vector */
    float v[3];  /**< Array representation of a vector */
} k4a_float3_t;

/** IMU frame structure
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_imu_sample_t
{
    float temperature;            /**< Temperature reading of this sample (Celsius) */
    k4a_float3_t acc_sample;      /**< Accelerometer sample in meters per second squared */
    uint64_t acc_timestamp_usec;  /**< Timestamp in uSec */
    k4a_float3_t gyro_sample;     /**< Gyro sample in radians per second */
    uint64_t gyro_timestamp_usec; /**< Timestamp in uSec */
} k4a_imu_sample_t;

/** Camera sensor extrinsic calibration data
 *
 * \relates k4a_calibration_camera_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_calibration_extrinsics_t
{
    float rotation[9];    /**< Rotation matrix*/
    float translation[3]; /**< Translation vector*/
} k4a_calibration_extrinsics_t;

/** k4a_calibration_intrinsic_parameters_t
 *
 * \remarks
 * Compatible with OpenCV. See OpenCV for documentation of individual parameters.
 *
 * \relates k4a_calibration_camera_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef union
{
    /** individual parameter or array representation of intrinsic model. */
    struct _param
    {
        float cx;            /**< Principal point in image, x */
        float cy;            /**< Principal point in image, y */
        float fx;            /**< Focal length x */
        float fy;            /**< Focal length y */
        float k1;            /**< k1 radial distortion coefficient */
        float k2;            /**< k2 radial distortion coefficient */
        float k3;            /**< k3 radial distortion coefficient */
        float k4;            /**< k4 radial distortion coefficient */
        float k5;            /**< k5 radial distortion coefficient */
        float k6;            /**< k6 radial distortion coefficient */
        float codx;          /**< Center of distortion in Z=1 plane, x (only used for Rational6KT) */
        float cody;          /**< Center of distortion in Z=1 plane, y (only used for Rational6KT) */
        float p2;            /**< Tangential distortion coefficient 2 */
        float p1;            /**< Tangential distortion coefficient 1 */
        float metric_radius; /**< Metric radius */
    } param;                 /**< Individual parameter representation of intrinsic model */
    float v[15];             /**< Array representation of intrinsic model parameters */
} k4a_calibration_intrinsic_parameters_t;

/** Camera sensor intrinsic calibration data
 *
 * \relates k4a_calibration_camera_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_calibration_intrinsics_t
{
    k4a_calibration_model_type_t type;                 /**< Type of calibration model used*/
    unsigned int parameter_count;                      /**< Number of valid entries in parameters*/
    k4a_calibration_intrinsic_parameters_t parameters; /**< Calibration parameters*/
} k4a_calibration_intrinsics_t;

/** Camera calibration contains intrinsic and extrinsic calibration information
 *
 * \relates k4a_calibration_camera_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_calibration_camera_t
{
    k4a_calibration_extrinsics_t extrinsics; /**< Extrinsic calibration data */
    k4a_calibration_intrinsics_t intrinsics; /**< Intrinsic calibration data*/
    int resolution_width;                    /**< Resolution width of the calibration sensor*/
    int resolution_height;                   /**< Resolution height of the calibration sensor*/
    float metric_radius;                     /**< Max FOV of the camera */
} k4a_calibration_camera_t;

/** Calibration type representing device calibration. It contains (1) the depth and color cameras' intrinsic
 * calibrations, (2) extrinsic transformation parameters (3) depth and color modes. The extrinsic parameters allow 3d
 * coordinate conversions between depth camera, color camera, the imu's gyro and the imu's accelerometer. For
 * transforming from a source to a target 3d coordinate system, use the parameters stored under
 * extrinsics[source][target]. *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_calibration_t
{
    k4a_calibration_camera_t depth_camera_calibration; /**< Depth camera calibration */
    k4a_calibration_camera_t color_camera_calibration; /**< Color camera calibration */
    k4a_calibration_extrinsics_t extrinsics[K4A_CALIBRATION_TYPE_NUM][K4A_CALIBRATION_TYPE_NUM]; /**< Extrinsic
                                                                                                    transformation */
    k4a_depth_mode_t depth_mode;             /**< Depth camera mode for which calibration was obtained */
    k4a_color_resolution_t color_resolution; /**< Color camera resolution for which calibration was obtained */
} k4a_calibration_t;

#ifdef __cplusplus
}
#endif

#endif /* K4ATYPES_H */
