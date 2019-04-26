// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4a/k4aversion.h>
#include <k4ainternal/depth.h>

// System dependencies
#include <assert.h>
#include <stdbool.h>

// Dependent libraries
#include <k4ainternal/common.h>
#include <k4ainternal/dewrapper.h>

// System dependencies
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEPTH_CALIBRATION_DATA_SIZE 2000000

static k4a_version_t g_min_fw_version_rgb = { 1, 5, 92 };             // 1.5.92
static k4a_version_t g_min_fw_version_depth = { 1, 5, 66 };           // 1.5.66
static k4a_version_t g_min_fw_version_audio = { 1, 5, 14 };           // 1.5.14
static k4a_version_t g_min_fw_version_depth_config = { 5006, 27, 0 }; // 5006.27 (iteration is not used, set to zero)

#define MINOR_VERSION_OFFSET_1 100 // Some variants of development FW offset minor version with 100
#define MINOR_VERSION_OFFSET_2 200 // Some variants of development FW offset minor version with 200

typedef struct _depth_context_t
{
    depthmcu_t depthmcu;
    dewrapper_t dewrapper;

    uint8_t *calibration_memory;
    size_t calibration_memory_size;
    bool calibration_init;

    bool running;
    k4a_hardware_version_t version;
    k4a_calibration_camera_t calibration;

    depth_cb_streaming_capture_t *capture_ready_cb;
    void *capture_ready_cb_context;
} depth_context_t;

K4A_DECLARE_CONTEXT(depth_t, depth_context_t);

depthmcu_stream_cb_t depth_capture_available;

static void log_device_info(depth_context_t *depth);
static void depth_stop_internal(depth_t depth_handle, bool quiet);
bool is_fw_version_compatable(const char *fw_type, k4a_version_t *fw_version, k4a_version_t *fw_min_version);

bool is_fw_version_compatable(const char *fw_type, k4a_version_t *fw_version, k4a_version_t *fw_min_version)
{
    typedef enum
    {
        FW_OK,
        FW_TOO_LOW,
        FW_UNKNOWN
    } fw_check_state_t;

    fw_check_state_t fw = FW_UNKNOWN;

    // Check major version
    if (fw_version->major > fw_min_version->major)
    {
        fw = FW_OK;
    }
    else if (fw_version->major < fw_min_version->major)
    {
        fw = FW_TOO_LOW;
    }

    // Check minor version
    if (fw == FW_UNKNOWN)
    {
        uint32_t minor = fw_version->minor;
        if (fw_version->minor > MINOR_VERSION_OFFSET_2)
        {
            minor = fw_version->minor - MINOR_VERSION_OFFSET_2;
        }
        else if (fw_version->minor > MINOR_VERSION_OFFSET_1)
        {
            minor = fw_version->minor - MINOR_VERSION_OFFSET_1;
        }

        if (minor > fw_min_version->minor)
        {
            fw = FW_OK;
        }
        else if (minor < fw_min_version->minor)
        {
            fw = FW_TOO_LOW;
        }
    }

    // Check iteration version
    if (fw == FW_UNKNOWN)
    {
        fw = FW_TOO_LOW;
        if (fw_version->iteration >= fw_min_version->iteration)
        {
            fw = FW_OK;
        }
    }

    if (fw != FW_OK)
    {
        LOG_ERROR("ERROR Firmware version for %s is %d.%d.%d is not current enough. Use %d.%d.%d or newer.",
                  fw_type,
                  fw_version->major,
                  fw_version->minor,
                  fw_version->iteration,
                  fw_min_version->major,
                  fw_min_version->minor,
                  fw_min_version->iteration);
    }
    return (fw == FW_OK);
}

k4a_result_t depth_create(depthmcu_t depthmcu,
                          calibration_t calibration_handle,
                          depth_cb_streaming_capture_t *capture_ready,
                          void *capture_ready_context,
                          depth_t *depth_handle)
{
    depth_context_t *depth = NULL;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, depthmcu == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, calibration_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, depth_handle == NULL);

    depth = depth_t_create(depth_handle);
    depth->depthmcu = depthmcu;
    depth->capture_ready_cb = capture_ready;
    depth->capture_ready_cb_context = capture_ready_context;

    if (K4A_SUCCEEDED(result))
    {
        depth->calibration_memory = (uint8_t *)malloc(DEPTH_CALIBRATION_DATA_SIZE);
        result = K4A_RESULT_FROM_BOOL(depth->calibration_memory != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(depthmcu_wait_is_ready(depth->depthmcu));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(depth_get_device_version(*depth_handle, &depth->version));
    }

    if (K4A_SUCCEEDED(result))
    {
        log_device_info(depth);

#ifndef K4A_MTE_VERSION
        if (!is_fw_version_compatable("RGB", &depth->version.rgb, &g_min_fw_version_rgb) ||
            !is_fw_version_compatable("Depth", &depth->version.depth, &g_min_fw_version_depth) ||
            !is_fw_version_compatable("Audio", &depth->version.audio, &g_min_fw_version_audio) ||
            !is_fw_version_compatable("Depth Config", &depth->version.depth_sensor, &g_min_fw_version_depth_config))
        {
            result = K4A_RESULT_FAILED;
        }
#endif
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(
            calibration_get_camera(calibration_handle, K4A_CALIBRATION_TYPE_DEPTH, &depth->calibration));
    }

    if (K4A_SUCCEEDED(result))
    {
        depth->dewrapper = dewrapper_create(&depth->calibration, capture_ready, capture_ready_context);
        result = K4A_RESULT_FROM_BOOL(depth->dewrapper != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        // SDK may have crashed last session, so call stop
        depth->running = true;
        bool quiet = true;
        depth_stop_internal(*depth_handle, quiet);
    }

    if (K4A_FAILED(result))
    {
        depth_destroy(*depth_handle);
        *depth_handle = NULL;
    }

    return result;
}

void depth_destroy(depth_t depth_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, depth_t, depth_handle);
    depth_context_t *depth = depth_t_get_context(depth_handle);

    bool quiet = false;
    depth_stop_internal(depth_handle, quiet);

    if (depth->dewrapper)
    {
        dewrapper_destroy(depth->dewrapper);
    }
    if (depth->calibration_memory != NULL)
    {
        free(depth->calibration_memory);
    }

    depth_t_destroy(depth_handle);
}

static void log_device_info(depth_context_t *depth)
{
    k4a_log_level_t level;

    if (logger_is_file_based())
    {
        // Log device information at a 'critical' level so that even at that no matter what level the logger
        // is set to, we will always of this critical information about version configuration and hardware
        // revision (hardware revision)
        level = K4A_LOG_LEVEL_CRITICAL;
    }
    else
    {
        // Log device information to stdout at an 'info' level so that users and maintain a simple level of debug
        // output. If the user want more detailed info in a stdout log then the verbosity needs to be turned up to info
        // or higher
        level = K4A_LOG_LEVEL_INFO;
    }

    logger_log(level, __FILE__, __LINE__, "******************** Device Info ********************");
    logger_log(level, __FILE__, __LINE__, "K4A SDK version:     %s", K4A_VERSION_STR);

    char serial_number[128];
    size_t size = sizeof(serial_number);
    if (depthmcu_get_serialnum(depth->depthmcu, serial_number, &size) == K4A_BUFFER_RESULT_SUCCEEDED)
    {
        logger_log(level, __FILE__, __LINE__, "Serial Number:       %s", serial_number);
    }

    k4a_version_t *ver = &depth->version.rgb;
    logger_log(level, __FILE__, __LINE__, "RGB Sensor Version:  %d.%d.%d", ver->major, ver->minor, ver->iteration);

    ver = &depth->version.depth;
    logger_log(level, __FILE__, __LINE__, "Depth Sensor Version:%d.%d.%d", ver->major, ver->minor, ver->iteration);

    ver = &depth->version.audio;
    logger_log(level, __FILE__, __LINE__, "Mic Array Version:   %d.%d.%d", ver->major, ver->minor, ver->iteration);

    ver = &depth->version.depth_sensor;
    logger_log(level, __FILE__, __LINE__, "Sensor Config:       %d.%d", ver->major, ver->minor);
    logger_log(level,
               __FILE__,
               __LINE__,
               "Build type:          %s",
               depth->version.firmware_build == 0 ? "Release" : "Debug");
    logger_log(level,
               __FILE__,
               __LINE__,
               "Signature type:      %s",
               depth->version.firmware_signature == K4A_FIRMWARE_SIGNATURE_MSFT ?
                   "MSFT" :
                   (depth->version.firmware_signature == K4A_FIRMWARE_SIGNATURE_TEST ? "Test" : "Unsigned"));

    logger_log(level, __FILE__, __LINE__, "****************************************************");
}

/** see documentation for depthmcu_stream_cb_t
 *
 * \related depthmcu_stream_cb_t
 */
void depth_capture_available(k4a_result_t cb_result, k4a_image_t image_raw, void *context)
{
    depth_context_t *depth = (depth_context_t *)context;
    k4a_capture_t capture_raw = NULL;

    if (K4A_SUCCEEDED(cb_result))
    {
        cb_result = TRACE_CALL(capture_create(&capture_raw));
    }

    if (K4A_SUCCEEDED(cb_result))
    {
        capture_set_ir_image(capture_raw, image_raw);
    }

    dewrapper_post_capture(cb_result, capture_raw, depth->dewrapper);

    if (capture_raw)
    {
        capture_dec_ref(capture_raw);
    }
}

k4a_buffer_result_t depth_get_device_serialnum(depth_t depth_handle, char *serial_number, size_t *serial_number_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, depth_t, depth_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, serial_number_size == NULL);

    depth_context_t *depth = depth_t_get_context(depth_handle);

    return TRACE_BUFFER_CALL(depthmcu_get_serialnum(depth->depthmcu, serial_number, serial_number_size));
}

k4a_result_t depth_get_device_version(depth_t depth_handle, k4a_hardware_version_t *version)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depth_t, depth_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, version == NULL);

    depth_context_t *depth = depth_t_get_context(depth_handle);
    depthmcu_firmware_versions_t mcu_version;

    result = TRACE_CALL(depthmcu_get_version(depth->depthmcu, &mcu_version));

    if (K4A_SUCCEEDED(result))
    {
        version->rgb.major = mcu_version.rgb_major;
        version->rgb.minor = mcu_version.rgb_minor;
        version->rgb.iteration = mcu_version.rgb_build;

        version->depth.major = mcu_version.depth_major;
        version->depth.minor = mcu_version.depth_minor;
        version->depth.iteration = mcu_version.depth_build;

        version->audio.major = mcu_version.audio_major;
        version->audio.minor = mcu_version.audio_minor;
        version->audio.iteration = mcu_version.audio_build;

        version->depth_sensor.major = mcu_version.depth_sensor_cfg_major;
        version->depth_sensor.minor = mcu_version.depth_sensor_cfg_minor;
        version->depth_sensor.iteration = 0;

        switch (mcu_version.build_config)
        {
        case 0:
            version->firmware_build = K4A_FIRMWARE_BUILD_RELEASE;
            break;
        case 1:
            version->firmware_build = K4A_FIRMWARE_BUILD_DEBUG;
            break;
        default:
            LOG_WARNING("Hardware reported unknown firmware build: %d", mcu_version.build_config);
            version->firmware_build = K4A_FIRMWARE_BUILD_DEBUG;
            break;
        }

        switch (mcu_version.signature_type)
        {
        case 0:
            version->firmware_signature = K4A_FIRMWARE_SIGNATURE_MSFT;
            break;
        case 1:
            version->firmware_signature = K4A_FIRMWARE_SIGNATURE_TEST;
            break;
        case 2:
            version->firmware_signature = K4A_FIRMWARE_SIGNATURE_UNSIGNED;
            break;
        default:
            LOG_WARNING("Hardware reported unknown signature type: %d", mcu_version.signature_type);
            version->firmware_signature = K4A_FIRMWARE_SIGNATURE_UNSIGNED;
            break;
        }
    }

    return result;
}

k4a_result_t depth_start(depth_t depth_handle, const k4a_device_configuration_t *config)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depth_t, depth_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);

    depth_context_t *depth = depth_t_get_context(depth_handle);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    // Turn on depth sensor (needed for most depth operations)

    if (K4A_SUCCEEDED(result))
    {
        depth->running = true; // set to true once we know we need to call depth_stop to unwind
        result = TRACE_CALL(depthmcu_depth_set_capture_mode(depth->depthmcu, config->depth_mode));
    }

    if (K4A_SUCCEEDED(result) && depth->calibration_init == false)
    {
        // Device power must be on for this to succeed
        result = TRACE_CALL(depthmcu_get_cal(depth->depthmcu,
                                             depth->calibration_memory,
                                             DEPTH_CALIBRATION_DATA_SIZE,
                                             &depth->calibration_memory_size));
        if (K4A_SUCCEEDED(result))
        {
            depth->calibration_init = true;
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        // Note: Depth Engine Start must be called after the mode is set in the sensor due to the sensor calibration
        // dependency on the mode of operation
        result = TRACE_CALL(
            dewrapper_start(depth->dewrapper, config, depth->calibration_memory, depth->calibration_memory_size));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(depthmcu_depth_set_fps(depth->depthmcu, config->camera_fps));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(depthmcu_depth_start_streaming(depth->depthmcu, depth_capture_available, depth));
    }

    if (K4A_FAILED(result))
    {
        depth_stop(depth_handle);
    }

    return result;
}

void depth_stop(depth_t depth_handle)
{
    bool quiet = false;
    depth_stop_internal(depth_handle, quiet);
}

void depth_stop_internal(depth_t depth_handle, bool quiet)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, depth_t, depth_handle);

    depth_context_t *depth = depth_t_get_context(depth_handle);

    // It is ok to call this multiple times, so no lock. Only doing it once is an optimization to not stop if the
    // sensor was never started.
    if (depth->running)
    {
        depthmcu_depth_stop_streaming(depth->depthmcu, quiet);

        dewrapper_stop(depth->dewrapper);
    }
    depth->running = false;
}

#ifdef __cplusplus
}
#endif
