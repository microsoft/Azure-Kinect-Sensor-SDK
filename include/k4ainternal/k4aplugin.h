/** \file k4aplugin.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure Depth Engine Plugin API.
 * Defines the API which must be defined by the depth engine plugin to be used
 * by the Azure Kinect SDK.
 */

#ifndef K4A_PLUGIN_H
#define K4A_PLUGIN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef _WIN32
#define __stdcall /**< __stdcall not defined in Linux */
#define __cdecl   /**< __cdecl not defined in Linux */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Current Version of the Azure Kinect Depth Engine Plugin Interface
 *
 * \remarks
 * When the depth engine plugin interface (k4a_plugin_t) is updated, this version should be increased.
 *
 * \remarks
 * The depth version binary name has the plugin version appended to it to signify the compatibility between the plugin
 * and the depth engine.
 */
#define K4A_PLUGIN_VERSION 2 /**< Azure Kinect plugin version */

/**
 * Expected name of plugin's dynamic library
 *
 * \remarks When the Azure Kinect SDK tried to load the depth engine, it will attempt to
 * load a dynamic library with name contains "depthengine". The name contains a
 * version number to bind with the matching depth engine plugin interface.
 */
#define K4A_PLUGIN_DYNAMIC_LIBRARY_NAME "depthengine"

/**
 * Name of the function all plugins must export in a dynamic library
 *
 * \remarks Upon finding a dynamic library named "depthengine", the k4aplugin
 * loader will attempt to find a symbol named k4a_register_plugin. Please see
 * \ref k4a_register_plugin_fn for the signature of that function.
 */
#define K4A_PLUGIN_EXPORTED_FUNCTION "k4a_register_plugin"

/** Supported Depth Engine modes
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_DEPTH_ENGINE_MODE_UNKNOWN = -1,           /**< Unknown Depth Engine mode */
    K4A_DEPTH_ENGINE_MODE_ST = 0,                 /**< Internal use only */
    K4A_DEPTH_ENGINE_MODE_LT_HW_BINNING = 1,      /**< Internal use only */
    K4A_DEPTH_ENGINE_MODE_LT_SW_BINNING = 2,      /**< Translates to K4A_DEPTH_MODE_NFOV_2X2BINNED */
    K4A_DEPTH_ENGINE_MODE_PCM = 3,                /**< Translates to K4A_DEPTH_MODE_PASSIVE_IR */
    K4A_DEPTH_ENGINE_MODE_LT_NATIVE = 4,          /**< Translates to K4A_DEPTH_MODE_NFOV_UNBINNED */
    K4A_DEPTH_ENGINE_MODE_MEGA_PIXEL = 5,         /**< Translates to K4A_DEPTH_MODE_WFOV_UNBINNED */
    K4A_DEPTH_ENGINE_MODE_QUARTER_MEGA_PIXEL = 7, /**< Translates to K4A_DEPTH_MODE_WFOV_2X2BINNED */
} k4a_depth_engine_mode_t;

/** Depth Engine output types
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_DEPTH_ENGINE_OUTPUT_TYPE_Z_DEPTH = 0,  /**< Output z depth */
    K4A_DEPTH_ENGINE_OUTPUT_TYPE_RADIAL_DEPTH, /**< Output radial depth */
    K4A_DEPTH_ENGINE_OUTPUT_TYPE_PCM,          /**< Output passive ir */
} k4a_depth_engine_output_type_t;

/** Depth Engine supported input formats
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_DEPTH_ENGINE_INPUT_TYPE_UNKNOWN = 0,          /**< Unknown Depth Engine input type */
    K4A_DEPTH_ENGINE_INPUT_TYPE_16BIT_LINEAR = 1,     /**< Internal use only */
    K4A_DEPTH_ENGINE_INPUT_TYPE_12BIT_RAW = 2,        /**< Internal use only */
    K4A_DEPTH_ENGINE_INPUT_TYPE_12BIT_COMPRESSED = 3, /**< 12bit compressed */
    K4A_DEPTH_ENGINE_INPUT_TYPE_8BIT_COMPRESSED = 4,  /**< 8bit compressed */
} k4a_depth_engine_input_type_t;

/** Transform Engine types
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_TRANSFORM_ENGINE_TYPE_COLOR_TO_DEPTH = 0,      /**< Transforms color image into the geometry of depth camera */
    K4A_TRANSFORM_ENGINE_TYPE_DEPTH_TO_COLOR,          /**< Transforms depth image into the geometry of color camera */
    K4A_TRANSFORM_ENGINE_TYPE_DEPTH_CUSTOM8_TO_COLOR,  /**< Transforms depth + 8 bit custom data into color camera */
    K4A_TRANSFORM_ENGINE_TYPE_DEPTH_CUSTOM16_TO_COLOR, /**< Transforms depth + 16 bit custom data into color camera */
} k4a_transform_engine_type_t;

/** Transform Engine interpolation type
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_TRANSFORM_ENGINE_INTERPOLATION_NEAREST = 0, /**< Nearest neighbor interpolation */
    K4A_TRANSFORM_ENGINE_INTERPOLATION_LINEAR       /**< Linear interpolation */
} k4a_transform_engine_interpolation_t;

/** Depth Engine output frame information
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_depth_engine_output_frame_info_t
{
    uint16_t output_width;                /**< Output frame width */
    uint16_t output_height;               /**< Output frame height */
    float sensor_temp;                    /**< Sensor temperature degrees in C */
    float laser_temp[2];                  /**< Laser temperature degrees in C */
    uint64_t center_of_exposure_in_ticks; /**< Tick timestamp with the center of exposure */
    uint64_t usb_sof_tick;                /**< Tick timestamp with the USB SoF was seen */
} k4a_depth_engine_output_frame_info_t;

/** Depth Engine input frame information
 *
 * /remarks At Runtime, please set to null, we parse these information from a
 * raw compressed input. Some internal testing may use this to pass in temperature info.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_depth_engine_input_frame_info_t
{
    float sensor_temp;                    /**< Sensor Temperature in Deg C */
    float laser_temp[2];                  /**< Laser Temperature in Deg C */
    uint64_t center_of_exposure_in_ticks; /**< Tick timestamp with the center of exposure */
    uint64_t usb_sof_tick;                /**< Tick timestamp with the USB SoF was seen */
} k4a_depth_engine_input_frame_info_t;

/** Depth Engine return codes
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_DEPTH_ENGINE_RESULT_SUCCEEDED = 0, /**< Result succeeded */

    // Frame data error, require caller to retry with expected frame data
    K4A_DEPTH_ENGINE_RESULT_DATA_ERROR_INVALID_INPUT_BUFFER_SIZE = 1,  /**< Invalid input buffer size */
    K4A_DEPTH_ENGINE_RESULT_DATA_ERROR_INVALID_OUTPUT_BUFFER_SIZE = 2, /**< Invalid output buffer size */
    K4A_DEPTH_ENGINE_RESULT_DATA_ERROR_INVALID_CAPTURE_SEQUENCE = 3,   /**< Invalid input capture data */
    K4A_DEPTH_ENGINE_RESULT_DATA_ERROR_NULL_INPUT_BUFFER = 4,          /**< Invalid input buffer pointer */
    K4A_DEPTH_ENGINE_RESULT_DATA_ERROR_NULL_OUTPUT_BUFFER = 5,         /**< Invalid output buffer pointer */

    // System fatal error, require caller to restart depth engine
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_NULL_ENGINE_POINTER = 101,       /**< Depth Engine was not initialized */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_INITIALIZE_ENGINE_FAILED = 102,  /**< Failed to initialize Depth Engine */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_UPDATE_CALIBRATION_FAILED = 103, /**< Failed to create depth calibration */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_PROCESS_PCM_FAILED = 104,        /**< Failed to process passive ir */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_BIN_INPUT_FAILED = 105,          /**< Failed to bin the input pixels */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_WAIT_PROCESSING_COMPLETE_FAILED = 106, /**< Failed to wait for processing
                                                                                  complete */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_NULL_CAMERA_CALIBRATION_POINTER = 107, /**< Camera calibration was null */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_ENGINE_NOT_LOADED = 108,       /**< Depth Engine plugin was not loaded */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_OUT_OF_MEMORY = 201,       /**< Failed to allocate memory */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_INVALID_PARAMETER = 202,   /**< Invalid input parameter */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_INVALID_CALIBRATION = 203, /**< Invalid depth calibration */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_FROM_API = 204,            /**< GPU API returns failure */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_INTERNAL = 205,            /**< GPU processing internal error */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_SHADER_COMPILATION = 206,  /**< GPU shader compilation error */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_OPENGL_CONTEXT = 207,      /**< OpenGL context creation error */
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_TIMEOUT = 208,             /**< GPU processing timed out */

    // Frame dropped during asynchronous call, only be sent to async caller with event listener
    K4A_DEPTH_ENGINE_RESULT_FRAME_DROPPED_ASYNC = 301, /**< Frame dropped during asynchronous call */
} k4a_depth_engine_result_code_t;

/** Depth Engine plugin version
 *
 * /remarks On load, Azure Kinect will validate that major versions match between the SDK
 * and the plugin.
 *
 * \xmlonly
 * <requirements>k4a_depth
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct
{
    uint32_t major; /**< Plugin Major Version */
    uint32_t minor; /**< Plugin Minor Version */
    uint32_t patch; /**< Plugin Patch Version */
} k4a_plugin_version_t;

/** Depth Engine context handle to be implemented by plugin
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct k4a_depth_engine_context_t k4a_depth_engine_context_t;

/** Transform Engine context handle to be implemented by plugin
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct k4a_transform_engine_context_t k4a_transform_engine_context_t;

/** Callback function for depth engine finishing processing.
 *
 * \param context
 * The context passed into k4a_de_process_frame_fn_t
 *
 * \param status
 * The result of the processing. 0 is a success
 *
 * \param output_frame
 * The final processed buffer passed back out to the user
 *
 * \param output_frame2
 * Second output buffer passed back out to the user or null
 */
typedef void(__stdcall k4a_processing_complete_cb_t)(void *context,
                                                     int status,
                                                     void *output_frame,
                                                     void *output_frame2);

/** Function for creating and initializing the depth engine.
 *
 * \param context
 * An opaque pointer to be passed around to the rest of the depth engine calls.
 *
 * \param cal_block_size_in_bytes
 * Size of the depth calibration blob being passed in, in bytes
 *
 * \param cal_block
 * The depth calibration blob
 *
 * \param mode
 * The \ref k4a_depth_engine_mode_t to initialize the depth engine
 *
 * \param input_format
 * The \ref k4a_depth_engine_input_type_t being passed in
 *
 * \param camera_calibration
 * The depth camera calibration blob
 *
 * \param callback
 * Callback to call when processing is complete
 *
 * \param callback_context
 * An optional context to be passed back to the callback
 *
 * \returns
 * K4A_DEPTH_ENGINE_RESULT_SUCCEEDED on success, or the proper failure code on
 * failure
 */
typedef k4a_depth_engine_result_code_t(__stdcall *k4a_de_create_and_initialize_fn_t)(
    k4a_depth_engine_context_t **context,
    size_t cal_block_size_in_bytes,
    void *cal_block,
    k4a_depth_engine_mode_t mode,
    k4a_depth_engine_input_type_t input_format,
    void *camera_calibration, // Fallback to use ccb intrinsics if it is null
    k4a_processing_complete_cb_t *callback,
    void *callback_context);

/** Function to process depth frame.
 *
 * \param context
 * context created by \ref k4a_de_create_and_initialize_fn_t
 *
 * \param input_frame
 * Input frame buffer containing depth raw captured data
 *
 * \param input_frame_size
 * Size of the input_frame buffer in bytes
 *
 * \param output_type
 * The type of frame the depth engine should output
 *
 * \param output_frame
 * The buffer of the output frame
 *
 * \param output_frame_size in bytes
 * The size of the output_frame buffer
 *
 * \param output_frame_info
 * The depth frame output information
 *
 * \param input_frame_info
 * The input frame information (internal use only), should pass in null at runtime
 *
 * \returns
 * K4A_DEPTH_ENGINE_RESULT_SUCCEEDED on success, or the proper failure code on
 * failure
 */
typedef k4a_depth_engine_result_code_t(__stdcall *k4a_de_process_frame_fn_t)(
    k4a_depth_engine_context_t *context,
    void *input_frame,
    size_t input_frame_size,
    k4a_depth_engine_output_type_t output_type,
    void *output_frame,
    size_t output_frame_size,
    k4a_depth_engine_output_frame_info_t *output_frame_info,
    k4a_depth_engine_input_frame_info_t *input_frame_info);

/** Get the size of the output frame in bytes.
 *
 * \param context
 * context created by \ref k4a_de_create_and_initialize_fn_t
 *
 * \returns
 * The size of the output frame in bytes (or 0 if passed a null context)
 */
typedef size_t(__stdcall *k4a_de_get_output_frame_size_fn_t)(k4a_depth_engine_context_t *context);

/** Destroys the depth engine context.
 *
 * \param context
 * context created by \ref k4a_de_create_and_initialize_fn_t
 */
typedef void(__stdcall *k4a_de_destroy_fn_t)(k4a_depth_engine_context_t **context);

/** Function for creating and initializing the transform engine.
 *
 * \param context
 * An opaque pointer to be passed around to the rest of the transform engine calls
 *
 * \param camera_calibration
 * The transform engine calibration blob
 *
 * \param callback
 * Callback to call when processing is complete
 *
 * \param callback_context
 * An optional context to be passed back to the callback
 *
 * \returns
 * K4A_DEPTH_ENGINE_RESULT_SUCCEEDED on success, or the proper failure code on
 * failure
 */
typedef k4a_depth_engine_result_code_t(__stdcall *k4a_te_create_and_initialize_fn_t)(
    k4a_transform_engine_context_t **context,
    void *camera_calibration,
    k4a_processing_complete_cb_t *callback,
    void *callback_context);

/** Function to transform between depth and color frame.
 *
 * \param context
 * context created by \ref k4a_te_create_and_initialize_fn_t
 *
 * \param type
 * The type of transform engine
 *
 * \param interpolation
 * The interpolation type for aux frame data
 *
 * \param invalid_value
 * The desired value for invalid pixel data
 *
 * \param depth_frame
 * Frame buffer containing depth frame data
 *
 * \param depth_frame_size
 * Size of the depth_frame buffer in bytes
 *
 * \param frame2
 * Frame buffer containing color or aux frame data
 *
 * \param frame2_size
 * Size of the frame2 buffer in bytes
 *
 * \param output_frame
 * The buffer of the output frame
 *
 * \param output_frame_size
 * Size of the output_frame buffer in bytes
 *
 * \param output_frame2
 * Second output buffer or null if there is none
 *
 * \param output_frame2_size
 * Size of the output_frame2 buffer in bytes
 *
 * \returns
 * K4A_DEPTH_ENGINE_RESULT_SUCCEEDED on success, or the proper failure code on
 * failure
 */
typedef k4a_depth_engine_result_code_t(__stdcall *k4a_te_process_frame_fn_t)(
    k4a_transform_engine_context_t *context,
    k4a_transform_engine_type_t type,
    k4a_transform_engine_interpolation_t interpolation,
    uint32_t invalid_value,
    const void *depth_frame,
    size_t depth_frame_size,
    const void *frame2,
    size_t frame2_size,
    void *output_frame,
    size_t output_frame_size,
    void *output_frame2,
    size_t output_frame2_size);

/** Get the size of the output frame in bytes.
 *
 * \param context
 * context created by \ref k4a_te_create_and_initialize_fn_t
 *
 * \param type
 * The type of transform engine
 *
 * \returns
 * The size of the output frame in bytes (or 0 if passed a null context)
 */
typedef size_t(__stdcall *k4a_te_get_output_frame_size_fn_t)(k4a_transform_engine_context_t *context,
                                                             k4a_transform_engine_type_t type);

/** Destroys the transform engine context.
 *
 * \param context
 * context created by \ref k4a_te_create_and_initialize_fn_t
 */
typedef void(__stdcall *k4a_te_destroy_fn_t)(k4a_transform_engine_context_t **context);

/** Plugin API which must be populated on plugin registration.
 *
 * \remarks
 * The Azure Kinect SDK will call k4a_register_plugin, and pass in a pointer to a \ref
 * k4a_plugin_t. The plugin must properly fill out all fields of the plugin for
 * the Azure Kinect SDK to accept the plugin.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_plugin_t
{
    k4a_plugin_version_t version;                                         /**< Depth Engine Version */
    k4a_de_create_and_initialize_fn_t depth_engine_create_and_initialize; /**< Function pointer to a
                                                                             depth_engine_create_and_initialize function
                                                                           */
    k4a_de_process_frame_fn_t depth_engine_process_frame; /**< Function pointer to a depth_engine_process_frame function
                                                           */
    k4a_de_get_output_frame_size_fn_t depth_engine_get_output_frame_size; /**< Function pointer to a
                                                                             depth_engine_get_output_frame_size function
                                                                           */
    k4a_de_destroy_fn_t depth_engine_destroy; /**< Function pointer to a depth_engine_destroy function */
    k4a_te_create_and_initialize_fn_t transform_engine_create_and_initialize; /**< Function pointer to a
                                                                                 transform_engine_create_and_initialize
                                                                                 function */
    k4a_te_process_frame_fn_t transform_engine_process_frame; /**< Function pointer to a transform_engine_process_frame
                                                                 function */
    k4a_te_get_output_frame_size_fn_t transform_engine_get_output_frame_size; /**< Function pointer to a
                                                                                 transform_engine_get_output_frame_size
                                                                                 function */
    k4a_te_destroy_fn_t transform_engine_destroy; /**< Function pointer to a transform_engine_destroy function */
} k4a_plugin_t;

/** Function signature for \ref K4A_PLUGIN_EXPORTED_FUNCTION.
 *
 * \remarks
 * Plugins must implement a function named "k4a_register_plugin" which will
 * fill out all fields in the passed in \ref k4a_plugin_t.
 *
 * \param plugin
 * function pointers and version of the plugin to filled out
 *
 * \returns
 * True if the plugin believes it successfully registered, false otherwise.
 */
typedef bool(__cdecl *k4a_register_plugin_fn)(k4a_plugin_t *plugin);

#ifdef __cplusplus
}
#endif

#endif /* K4A_PLUGIN_H */
