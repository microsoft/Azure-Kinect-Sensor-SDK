/** \file k4aplugin.h
 * Kinect For Azure Depth Engine Plugin API.
 * Defines the API which must be defined by the depth engine plugin to be used
 * by the K4A SDK.
 */

#ifndef K4A_PLUGIN_H
#define K4A_PLUGIN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef _WIN32
#define __stdcall /**< __stdcall not defined in Linux */
#endif

/**
 * Current Version of the k4a Depth Engine Plugin API
 *
 * \remarks The MAJOR version must be updated to denote any breaking change
 * that would cause an older SDK to not be able to use this plugin.
 */
#define K4A_PLUGIN_MAJOR_VERSION 1 /**< K4A plugin major version */
#define K4A_PLUGIN_MINOR_VERSION 0 /**< K4A plugin minor version */
#define K4A_PLUGIN_PATCH_VERSION 0 /**< K4A plugin patch version */

/**
 * Expected name of plugin's dynamic library
 *
 * \remarks When the K4A SDK tried to load the depth engine, it will attempt to
 * load a dynamic library named "depthengine"
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

/** Valid Depth Engine modes
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_DEPTH_ENGINE_MODE_UNKNOWN = -1,
    K4A_DEPTH_ENGINE_MODE_ST = 0,
    K4A_DEPTH_ENGINE_MODE_LT_HW_BINNING = 1,
    K4A_DEPTH_ENGINE_MODE_LT_SW_BINNING = 2,
    K4A_DEPTH_ENGINE_MODE_PCM = 3,
    K4A_DEPTH_ENGINE_MODE_LT_NATIVE = 4,
    K4A_DEPTH_ENGINE_MODE_MEGA_PIXEL = 5,
    K4A_DEPTH_ENGINE_MODE_QUARTER_MEGA_PIXEL = 7
} k4a_depth_engine_mode_t;

/** Depth Engine output formats
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_DEPTH_ENGINE_OUTPUT_TYPE_Z_DEPTH = 0,
    K4A_DEPTH_ENGINE_OUTPUT_TYPE_RADIAL_DEPTH,
    K4A_DEPTH_ENGINE_OUTPUT_TYPE_PCM
} k4a_depth_engine_output_type_t;

/** Depth Engine valid input formats
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_DEPTH_ENGINE_INPUT_TYPE_UNKNOWN = 0,
    K4A_DEPTH_ENGINE_INPUT_TYPE_16BIT_LINEAR,
    K4A_DEPTH_ENGINE_INPUT_TYPE_12BIT_RAW,
    K4A_DEPTH_ENGINE_INPUT_TYPE_12BIT_COMPRESSED,
    K4A_DEPTH_ENGINE_INPUT_TYPE_8BIT_COMPRESSED
} k4a_depth_engine_input_type_t;

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
    uint16_t output_width;                /**< Outputted frame width */
    uint16_t output_height;               /**< Outputted frame height */
    float sensor_temp;                    /**< Sensor temperature degrees in C */
    float laser_temp[2];                  /**< Laser temperature degrees in C */
    uint64_t center_of_exposure_in_ticks; /**< Tick timestamp with the center of exposure */
    uint64_t usb_sof_tick;                /**< Tick timestamp with the USB SoF was seen */
} k4a_depth_engine_output_frame_info_t;

/** Depth Engine input frame information
 *
 * /remarks At Runtime, please set to NULL, we parse these information from a
 * raw 12bit compressed input. Some playback testing may use this to pass in
 * temperature info
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

/** Depth Engine Result codes
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_DEPTH_ENGINE_RESULT_SUCCEEDED = 0,

    // Frame data error, require caller to retry with expected frame data
    K4A_DEPTH_ENGINE_RESULT_DATA_ERROR_INVALID_INPUT_BUFFER_SIZE = 1,
    K4A_DEPTH_ENGINE_RESULT_DATA_ERROR_INVALID_OUTPUT_BUFFER_SIZE = 2,
    K4A_DEPTH_ENGINE_RESULT_DATA_ERROR_INVALID_CAPTURE_SEQUENCE = 3,

    // System fatal error, require caller to restart depth engine
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_NULL_ENGINE_POINTER = 101,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_INITIALIZE_ENGINE_FAILED = 102,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_UPDATE_CALIBRATION_FAILED = 103,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_PROCESS_PCM_FAILED = 104,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_BIN_INPUT_FAILED = 105,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_WAIT_PROCESSING_COMPLETE_FAILED = 106,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_NULL_CAMERA_CALIBRATION_POINTER = 107,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_ENGINE_NOT_LOADED = 108,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_OUT_OF_MEMORY = 201,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_INVALID_PARAMETER = 202,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_INVALID_CALIBRATION = 203,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_FROM_API = 204,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_INTERNAL = 205,
    K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_SHADER_COMPILATION = 206,

    // Frame dropped during asynchronous call, only be sent to async caller with event listener
    K4A_DEPTH_ENGINE_RESULT_FRAME_DROPPED_ASYNC = 301,
} k4a_depth_engine_result_code_t;

/** Depth Engine Plugin Version
 *
 * /remarks On load, k4a will validate that major versions match between the SDK
 * and the plugin
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

/** Opaque struct to be implemented by plugin
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct k4a_depth_engine_context_t k4a_depth_engine_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/** callback function for depth engine finishing processing
 *
 * \param context
 * The context passed into k4a_de_process_frame_fn_t
 *
 * \param status
 * The result of the processing. 0 is a success
 *
 * \param output_frame
 * The final processed buffer passed back out to the user
 */
typedef void(__stdcall k4a_processing_complete_cb_t)(void *context, int status, void *output_frame);

/** Function for creating and initialzing the depth engine
 *
 * \param context
 * An opaque pointer to be passed around to the rest of the depth engine calls.
 *
 * \param cal_block_size_in_bytes
 * Size of the calibration block being passed in
 *
 * \param cal_block
 * The cal_block being passed into the device
 *
 * \param mode
 * The \ref k4a_depth_engine_mode_t to initialize the depth engine
 *
 * \param input_format
 * The \ref k4a_depth_engine_input_type_t being passed into this depth engine
 *
 * \param camera_calibration
 * Camera calibration blob
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
    void *camera_calibration, // fallback to use
                              // intrinsics from
                              // CCB when
                              // camera_calibration
                              // is NULL
    k4a_processing_complete_cb_t *callback,
    void *callback_context);

/** Function to process depth frame
 *
 * \param context
 * context created by \ref k4a_de_create_and_initialize_fn_t
 *
 * \param input_frame
 * Frame buffer containing depth engine data
 *
 * \param input_frame_size
 * Size of of the input_frame buffer
 *
 * \param output_type
 * The type of frame the depth engine should output
 *
 * \param output_frame
 * The buffer of the outputted frame
 *
 * \param output_frame_size
 * The size of the output_frame buffer
 *
 * \param output_frame_info
 * TODO
 *
 * \param input_frame_info
 * TODO
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

/** Get the size of the output frame in bytes
 *
 * \param context
 * context created by \ref k4a_de_create_and_initialize_fn_t
 *
 * \returns
 * The size of the output frame in bytes (or 0 if passed a null context)
 */
typedef size_t(__stdcall *k4a_de_get_output_frame_size_fn_t)(k4a_depth_engine_context_t *context);

/** Destroys the depth engine context
 *
 * \param context
 * context created by \ref k4a_de_create_and_initialize_fn_t
 */
typedef void(__stdcall *k4a_de_destroy_fn_t)(k4a_depth_engine_context_t **context);

/** Plugin API which must be populated on plugin registration.
 *
 * \remarks
 * The K4A SDK will call k4a_register_plugin, and pass in a pointer to a \ref
 * k4a_plugin_t. The plugin must properly fill out all fields of the plugin for
 * the K4A SDK to accept the plugin.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4aplugin.h (include k4a/k4aplugin.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_plugin_t
{
    k4a_plugin_version_t version; /**< Version this plugin was written against */
    k4a_de_create_and_initialize_fn_t depth_engine_create_and_initialize; /**< function pointer to a
                                                                             depth_engine_create_and_initialize function
                                                                           */
    k4a_de_process_frame_fn_t depth_engine_process_frame; /**< function pointer to a depth_engine_process_frame function
                                                           */
    k4a_de_get_output_frame_size_fn_t depth_engine_get_output_frame_size; /**< function pointer to a
                                                                             depth_engine_get_output_frame_size function
                                                                           */
    k4a_de_destroy_fn_t depth_engine_destroy; /**< function pointer to a depth_engine_destroy function */
} k4a_plugin_t;

/** Function signature for \ref K4A_PLUGIN_EXPORTED_FUNCTION.
 *
 * \remarks
 * Plugins must implement a function named "k4a_register_plugin" which will
 * fill out all fields in the passed in \ref k4a_plugin_t
 *
 * \param plugin
 * function pointers and version of the plugin to filled out
 *
 * \returns
 * True if the plugin believes it successfully registered, false otherwise.
 */
typedef bool (*k4a_register_plugin_fn)(k4a_plugin_t *plugin);

#ifdef __cplusplus
}
#endif

#endif /* K4A_PLUGIN_H */
