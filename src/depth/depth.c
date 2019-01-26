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

typedef struct _depth_context_t
{
    depthmcu_t depthmcu;
    dewrapper_t dewrapper;

    uint8_t *calibration_memory;
    size_t calibration_memory_size;
    bool calibration_init;

    bool running;

    k4a_calibration_camera_t calibration;

    depth_cb_streaming_capture_t *capture_ready_cb;
    void *capture_ready_cb_context;
} depth_context_t;

K4A_DECLARE_CONTEXT(depth_t, depth_context_t);

depthmcu_stream_cb_t depth_capture_available;

static void log_device_info(depth_context_t *depth);
static void depth_stop_internal(depth_t depth_handle, bool quiet);

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
        result = TRACE_CALL(
            calibration_get_camera(calibration_handle, K4A_CALIBRATION_TYPE_DEPTH, &depth->calibration));
    }

    log_device_info(depth);

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
    if (!logger_is_file_based())
    {
        // only log this info if we are logging to a file
        return;
    }

    // Log device information at a 'critical' level so that even at that no matter what level the logger
    // is set to, we will always of this critical information about version configuration and hardware
    // revision (hardware revision)
    logger_critical(LOGGER_API, "******************** Device Info ********************");
    logger_critical(LOGGER_API, "K4A SDK version:     %s", K4A_VERSION_STR);

    char serial_number[128];
    size_t size = sizeof(serial_number);
    if (depthmcu_get_serialnum(depth->depthmcu, serial_number, &size) == K4A_BUFFER_RESULT_SUCCEEDED)
    {
        logger_critical(LOGGER_API, "Serial Number:       %s", serial_number);
    }

    depthmcu_firmware_versions_t version;
    if (depthmcu_get_version(depth->depthmcu, &version) == K4A_RESULT_SUCCEEDED)
    {
        logger_critical(LOGGER_API,
                        "RGB Sensor Version:  %d.%d.%d",
                        version.rgb_major,
                        version.rgb_minor,
                        version.rgb_build);
        logger_critical(LOGGER_API,
                        "Depth Sensor Version:%d.%d.%d",
                        version.depth_major,
                        version.depth_minor,
                        version.depth_build);
        logger_critical(LOGGER_API,
                        "Mic Array Version:   %d.%d.%d",
                        version.audio_major,
                        version.audio_minor,
                        version.audio_build);
        logger_critical(LOGGER_API,
                        "Sensor Config:       %d.%d",
                        version.depth_sensor_cfg_major,
                        version.depth_sensor_cfg_minor);
        logger_critical(LOGGER_API, "Build type:          %s", version.build_config == 0 ? "Release" : "Debug");
        logger_critical(LOGGER_API,
                        "Signature type:      %s",
                        version.signature_type == K4A_FIRMWARE_SIGNATURE_MSFT ?
                            "MSFT" :
                            (version.signature_type == K4A_FIRMWARE_SIGNATURE_TEST ? "Test" : "Unsigned"));
    }

    // TODO add hardware info here too

    logger_critical(LOGGER_API, "****************************************************");
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
        version->depth_sensor.minor = mcu_version.depth_sensor_cfg_major;
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

    // It is ok to call this multiple times, so no lock. Only doing it once is an optimization to not stop if the sensor
    // was never started.
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
