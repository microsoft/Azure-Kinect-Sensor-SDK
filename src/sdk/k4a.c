// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4a/k4a.h>

// Dependent libraries
#include <k4ainternal/common.h>
#include <k4ainternal/capture.h>
#include <k4ainternal/depth.h>
#include <k4ainternal/imu.h>
#include <k4ainternal/color.h>
#include <k4ainternal/color_mcu.h>
#include <k4ainternal/depth_mcu.h>
#include <k4ainternal/calibration.h>
#include <k4ainternal/capturesync.h>
#include <k4ainternal/transformation.h>
#include <k4ainternal/logging.h>
#include <azure_c_shared_utility/tickcounter.h>

// System dependencies
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

char K4A_ENV_VAR_LOG_TO_A_FILE[] = K4A_ENABLE_LOG_TO_A_FILE;

typedef struct _k4a_context_t
{
    TICK_COUNTER_HANDLE tick_handle;

    calibration_t calibration;

    depthmcu_t depthmcu;
    colormcu_t colormcu;

    capturesync_t capturesync;

    imu_t imu;
    color_t color;
    depth_t depth;

    bool depth_started;
    bool color_started;
    bool imu_started;
} k4a_context_t;

K4A_DECLARE_CONTEXT(k4a_device_t, k4a_context_t);

#define DEPTH_CAPTURE (false)
#define COLOR_CAPTURE (true)
#define TRANSFORM_ENABLE_GPU_OPTIMIZATION (true)
#define K4A_DEPTH_MODE_TO_STRING_CASE(depth_mode)                                                                      \
    case depth_mode:                                                                                                   \
        return #depth_mode
#define K4A_COLOR_RESOLUTION_TO_STRING_CASE(color_resolution)                                                          \
    case color_resolution:                                                                                             \
        return #color_resolution
#define K4A_IMAGE_FORMAT_TO_STRING_CASE(image_format)                                                                  \
    case image_format:                                                                                                 \
        return #image_format
#define K4A_FPS_TO_STRING_CASE(fps)                                                                                    \
    case fps:                                                                                                          \
        return #fps

uint32_t k4a_device_get_installed_count(void)
{
    uint32_t device_count = 0;
    usb_cmd_get_device_count(&device_count);
    return device_count;
}

k4a_result_t k4a_set_debug_message_handler(k4a_logging_message_cb_t *message_cb,
                                           void *message_cb_context,
                                           k4a_log_level_t min_level)
{
    return logger_register_message_callback(message_cb, message_cb_context, min_level);
}

k4a_result_t k4a_set_allocator(k4a_memory_allocate_cb_t allocate, k4a_memory_destroy_cb_t free)
{
    return allocator_set_allocator(allocate, free);
}

depth_cb_streaming_capture_t depth_capture_ready;
color_cb_streaming_capture_t color_capture_ready;

void depth_capture_ready(k4a_result_t result, k4a_capture_t capture_handle, void *callback_context)
{
    k4a_device_t device_handle = (k4a_device_t)callback_context;
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);
    capturesync_add_capture(device->capturesync, result, capture_handle, DEPTH_CAPTURE);
}

void color_capture_ready(k4a_result_t result, k4a_capture_t capture_handle, void *callback_context)
{
    k4a_device_t device_handle = (k4a_device_t)callback_context;
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);
    capturesync_add_capture(device->capturesync, result, capture_handle, COLOR_CAPTURE);
}

k4a_result_t k4a_device_open(uint32_t index, k4a_device_t *device_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, device_handle == NULL);
    k4a_context_t *device = NULL;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    k4a_device_t handle = NULL;
    const guid_t *container_id = NULL;
    char serial_number[MAX_SERIAL_NUMBER_LENGTH];
    size_t serial_number_size = sizeof(serial_number);

    allocator_initialize();

    device = k4a_device_t_create(&handle);
    result = K4A_RESULT_FROM_BOOL(device != NULL);

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL((device->tick_handle = tickcounter_create()) != NULL);
    }

    // Create MCU modules
    if (K4A_SUCCEEDED(result))
    {
        // This will block until the depth process is ready to receive commands
        result = TRACE_CALL(depthmcu_create(index, &device->depthmcu));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL((container_id = depthmcu_get_container_id(device->depthmcu)) != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        if (TRACE_BUFFER_CALL(depthmcu_get_serialnum(device->depthmcu, serial_number, &serial_number_size) !=
                              K4A_BUFFER_RESULT_SUCCEEDED))
        {
            result = K4A_RESULT_FAILED;
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(colormcu_create(container_id, &device->colormcu));
    }

    // Create calibration module - ensure we can read calibration before proceeding
    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(calibration_create(device->depthmcu, &device->calibration));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(capturesync_create(&device->capturesync));
    }

    // Open Depth Module
    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(
            depth_create(device->depthmcu, device->calibration, depth_capture_ready, handle, &device->depth));
    }

    // Create color Module
    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(color_create(
            device->tick_handle, container_id, serial_number, color_capture_ready, handle, &device->color));
    }

    // Create imu Module
    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(imu_create(device->tick_handle, device->colormcu, device->calibration, &device->imu));
    }

    if (K4A_FAILED(result))
    {
        k4a_device_close(handle);
        handle = NULL;
    }
    else
    {
        *device_handle = handle;
    }

    return result;
}

void k4a_device_close(k4a_device_t device_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    if (device->capturesync)
    {
        // Stop capturesync first so that imu, depth, and color can destroy cleanly
        capturesync_stop(device->capturesync);
    }

    // Destroy modules in the reverse order they were created
    if (device->imu)
    {
        imu_destroy(device->imu);
        device->imu = NULL;
    }
    if (device->color)
    {
        color_destroy(device->color);
        device->color = NULL;
    }
    if (device->depth)
    {
        depth_destroy(device->depth);
        device->depth = NULL;
    }

    // depth & color call into capturesync, so they need to be destroyed first.
    if (device->capturesync)
    {
        capturesync_destroy(device->capturesync);
        device->capturesync = NULL;
    }

    // calibration rely's on depthmcu, so it needs to be destroyed first.
    if (device->calibration)
    {
        calibration_destroy(device->calibration);
        device->calibration = NULL;
    }

    if (device->depthmcu)
    {
        depthmcu_destroy(device->depthmcu);
        device->depthmcu = NULL;
    }
    if (device->colormcu)
    {
        colormcu_destroy(device->colormcu);
        device->colormcu = NULL;
    }

    if (device->tick_handle)
    {
        tickcounter_destroy(device->tick_handle);
        device->tick_handle = NULL;
    }

    k4a_device_t_destroy(device_handle);
    allocator_deinitialize();
}

k4a_wait_result_t k4a_device_get_capture(k4a_device_t device_handle,
                                         k4a_capture_t *capture_handle,
                                         int32_t timeout_in_ms)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_WAIT_RESULT_FAILED, k4a_device_t, device_handle);
    RETURN_VALUE_IF_ARG(K4A_WAIT_RESULT_FAILED, capture_handle == NULL);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);
    return TRACE_WAIT_CALL(capturesync_get_capture(device->capturesync, capture_handle, timeout_in_ms));
}

k4a_wait_result_t k4a_device_get_imu_sample(k4a_device_t device_handle,
                                            k4a_imu_sample_t *imu_sample,
                                            int32_t timeout_in_ms)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_WAIT_RESULT_FAILED, k4a_device_t, device_handle);
    RETURN_VALUE_IF_ARG(K4A_WAIT_RESULT_FAILED, imu_sample == NULL);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);
    return TRACE_WAIT_CALL(imu_get_sample(device->imu, imu_sample, timeout_in_ms));
}

k4a_result_t k4a_device_start_imu(k4a_device_t device_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, device->imu_started == true);

    if (device->depth_started == false && device->color_started == false)
    {
        // Color camera resets the IMU timestamp so we avoid letting the IMU run without the camera already running.
        LOG_ERROR("k4a_device_start_imu called while the color/depth camera is not running is not supported", 0);
        result = K4A_RESULT_FAILED;
    }

    if (K4A_SUCCEEDED(result))
    {
        LOG_TRACE("k4a_device_start_imu starting", 0);
        result = TRACE_CALL(imu_start(device->imu, color_get_sensor_start_time_tick(device->color)));
    }

    if (K4A_SUCCEEDED(result))
    {
        device->imu_started = true;
    }

    if (K4A_FAILED(result) && device->imu_started == true)
    {
        k4a_device_stop_imu(device_handle);
    }
    LOG_INFO("k4a_device_start_imu started", 0);

    return result;
}

void k4a_device_stop_imu(k4a_device_t device_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    LOG_INFO("k4a_device_stop_imu stopping", 0);
    if (device->imu)
    {
        imu_stop(device->imu);
        device->imu_started = false;
    }
    LOG_TRACE("k4a_device_stop_imu stopped", 0);
}

k4a_result_t k4a_capture_create(k4a_capture_t *capture_handle)
{
    return capture_create(capture_handle);
}

void k4a_capture_release(k4a_capture_t capture_handle)
{
    capture_dec_ref(capture_handle);
}

void k4a_capture_reference(k4a_capture_t capture_handle)
{
    capture_inc_ref(capture_handle);
}

float k4a_capture_get_temperature_c(k4a_capture_t capture_handle)
{
    return capture_get_temperature_c(capture_handle);
}

k4a_image_t k4a_capture_get_color_image(k4a_capture_t capture_handle)
{
    return capture_get_color_image(capture_handle);
}

k4a_image_t k4a_capture_get_depth_image(k4a_capture_t capture_handle)
{
    return capture_get_depth_image(capture_handle);
}

k4a_image_t k4a_capture_get_ir_image(k4a_capture_t capture_handle)
{
    return capture_get_ir_image(capture_handle);
}

void k4a_capture_set_color_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    capture_set_color_image(capture_handle, image_handle);
}

void k4a_capture_set_depth_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    capture_set_depth_image(capture_handle, image_handle);
}

void k4a_capture_set_ir_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    capture_set_ir_image(capture_handle, image_handle);
}

void k4a_capture_set_temperature_c(k4a_capture_t capture_handle, float temperature_c)
{
    capture_set_temperature_c(capture_handle, temperature_c);
}

k4a_result_t k4a_image_create(k4a_image_format_t format,
                              int width_pixels,
                              int height_pixels,
                              int stride_bytes,
                              k4a_image_t *image_handle)
{
    return image_create(format, width_pixels, height_pixels, stride_bytes, ALLOCATION_SOURCE_USER, image_handle);
}

k4a_result_t k4a_image_create_from_buffer(k4a_image_format_t format,
                                          int width_pixels,
                                          int height_pixels,
                                          int stride_bytes,
                                          uint8_t *buffer,
                                          size_t buffer_size,
                                          k4a_memory_destroy_cb_t *buffer_release_cb,
                                          void *buffer_release_cb_context,
                                          k4a_image_t *image_handle)
{
    return image_create_from_buffer(format,
                                    width_pixels,
                                    height_pixels,
                                    stride_bytes,
                                    buffer,
                                    buffer_size,
                                    buffer_release_cb,
                                    buffer_release_cb_context,
                                    image_handle);
}

uint8_t *k4a_image_get_buffer(k4a_image_t image_handle)
{
    return image_get_buffer(image_handle);
}

size_t k4a_image_get_size(k4a_image_t image_handle)
{
    return image_get_size(image_handle);
}

k4a_image_format_t k4a_image_get_format(k4a_image_t image_handle)
{
    return image_get_format(image_handle);
}
int k4a_image_get_width_pixels(k4a_image_t image_handle)
{
    return image_get_width_pixels(image_handle);
}

int k4a_image_get_height_pixels(k4a_image_t image_handle)
{
    return image_get_height_pixels(image_handle);
}

int k4a_image_get_stride_bytes(k4a_image_t image_handle)
{
    return image_get_stride_bytes(image_handle);
}

// Deprecated
uint64_t k4a_image_get_timestamp_usec(k4a_image_t image_handle)
{
    return image_get_device_timestamp_usec(image_handle);
}

uint64_t k4a_image_get_device_timestamp_usec(k4a_image_t image_handle)
{
    return image_get_device_timestamp_usec(image_handle);
}

uint64_t k4a_image_get_system_timestamp_nsec(k4a_image_t image_handle)
{
    return image_get_system_timestamp_nsec(image_handle);
}

uint64_t k4a_image_get_exposure_usec(k4a_image_t image_handle)
{
    return image_get_exposure_usec(image_handle);
}

uint32_t k4a_image_get_white_balance(k4a_image_t image_handle)
{
    return image_get_white_balance(image_handle);
}

uint32_t k4a_image_get_iso_speed(k4a_image_t image_handle)
{
    return image_get_iso_speed(image_handle);
}

void k4a_image_set_device_timestamp_usec(k4a_image_t image_handle, uint64_t timestamp_usec)
{
    image_set_device_timestamp_usec(image_handle, timestamp_usec);
}

// Deprecated
void k4a_image_set_timestamp_usec(k4a_image_t image_handle, uint64_t timestamp_usec)
{
    image_set_device_timestamp_usec(image_handle, timestamp_usec);
}

void k4a_image_set_system_timestamp_nsec(k4a_image_t image_handle, uint64_t timestamp_nsec)
{
    image_set_system_timestamp_nsec(image_handle, timestamp_nsec);
}

// Deprecated
void k4a_image_set_exposure_time_usec(k4a_image_t image_handle, uint64_t exposure_usec)
{
    image_set_exposure_usec(image_handle, exposure_usec);
}

void k4a_image_set_exposure_usec(k4a_image_t image_handle, uint64_t exposure_usec)
{
    image_set_exposure_usec(image_handle, exposure_usec);
}

void k4a_image_set_white_balance(k4a_image_t image_handle, uint32_t white_balance)
{
    image_set_white_balance(image_handle, white_balance);
}

void k4a_image_set_iso_speed(k4a_image_t image_handle, uint32_t iso_speed)
{
    image_set_iso_speed(image_handle, iso_speed);
}

void k4a_image_reference(k4a_image_t image_handle)
{
    image_inc_ref(image_handle);
}

void k4a_image_release(k4a_image_t image_handle)
{
    image_dec_ref(image_handle);
}

static const char *k4a_depth_mode_to_string(k4a_depth_mode_t depth_mode)
{
    switch (depth_mode)
    {
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_OFF);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_NFOV_2X2BINNED);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_NFOV_UNBINNED);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_WFOV_2X2BINNED);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_WFOV_UNBINNED);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_PASSIVE_IR);
    }
    return "Unexpected k4a_depth_mode_t value.";
}

static const char *k4a_color_resolution_to_string(k4a_color_resolution_t resolution)
{
    switch (resolution)
    {
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_OFF);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_720P);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_1080P);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_1440P);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_1536P);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_2160P);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_3072P);
    }
    return "Unexpected k4a_color_resolution_t value.";
}

static const char *k4a_image_format_to_string(k4a_image_format_t image_format)
{
    switch (image_format)
    {
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_COLOR_MJPG);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_COLOR_NV12);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_COLOR_YUY2);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_COLOR_BGRA32);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_DEPTH16);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_IR16);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_CUSTOM8);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_CUSTOM16);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_CUSTOM);
    }
    return "Unexpected k4a_image_format_t value.";
}

static const char *k4a_fps_to_string(k4a_fps_t fps)
{
    switch (fps)
    {
        K4A_FPS_TO_STRING_CASE(K4A_FRAMES_PER_SECOND_5);
        K4A_FPS_TO_STRING_CASE(K4A_FRAMES_PER_SECOND_15);
        K4A_FPS_TO_STRING_CASE(K4A_FRAMES_PER_SECOND_30);
    }
    return "Unexpected k4a_fps_t value.";
}

static k4a_result_t validate_configuration(k4a_context_t *device, const k4a_device_configuration_t *config)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, device == NULL);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    bool depth_enabled = false;
    bool color_enabled = false;

    if (config->color_format != K4A_IMAGE_FORMAT_COLOR_MJPG && config->color_format != K4A_IMAGE_FORMAT_COLOR_YUY2 &&
        config->color_format != K4A_IMAGE_FORMAT_COLOR_NV12 && config->color_format != K4A_IMAGE_FORMAT_COLOR_BGRA32)
    {
        result = K4A_RESULT_FAILED;
        LOG_ERROR("The configured color_format is not a valid k4a_color_format_t value.", 0);
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->color_resolution < K4A_COLOR_RESOLUTION_OFF ||
            config->color_resolution > K4A_COLOR_RESOLUTION_3072P)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("The configured color_resolution is not a valid k4a_color_resolution_t value.", 0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->depth_mode < K4A_DEPTH_MODE_OFF || config->depth_mode > K4A_DEPTH_MODE_PASSIVE_IR)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("The configured depth_mode is not a valid k4a_depth_mode_t value.", 0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->camera_fps != K4A_FRAMES_PER_SECOND_5 && config->camera_fps != K4A_FRAMES_PER_SECOND_15 &&
            config->camera_fps != K4A_FRAMES_PER_SECOND_30)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("The configured camera_fps is not a valid k4a_fps_t value.", 0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->wired_sync_mode < K4A_WIRED_SYNC_MODE_STANDALONE ||
            config->wired_sync_mode > K4A_WIRED_SYNC_MODE_SUBORDINATE)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("The configured wired_sync_mode is not a valid k4a_wired_sync_mode_t value.", 0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE ||
            config->wired_sync_mode == K4A_WIRED_SYNC_MODE_MASTER)
        {
            bool sync_in_cable_present;
            bool sync_out_cable_present;

            result = colormcu_get_external_sync_jack_state(device->colormcu,
                                                           &sync_in_cable_present,
                                                           &sync_out_cable_present);

            if (K4A_SUCCEEDED(result))
            {
                if (config->wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE && !sync_in_cable_present)
                {
                    result = K4A_RESULT_FAILED;
                    LOG_ERROR("Failure to detect presence of sync in cable with wired sync mode "
                              "K4A_WIRED_SYNC_MODE_SUBORDINATE.",
                              0);
                }

                if (config->wired_sync_mode == K4A_WIRED_SYNC_MODE_MASTER)
                {
                    if (!sync_out_cable_present)
                    {
                        result = K4A_RESULT_FAILED;
                        LOG_ERROR("Failure to detect presence of sync out cable with wired sync mode "
                                  "K4A_WIRED_SYNC_MODE_MASTER.",
                                  0);
                    }

                    if (config->color_resolution == K4A_COLOR_RESOLUTION_OFF)
                    {
                        result = K4A_RESULT_FAILED;
                        LOG_ERROR(
                            "Device wired_sync_mode is set to K4A_WIRED_SYNC_MODE_MASTER, so color camera must be used "
                            "on master device. Color_resolution can not be set to K4A_COLOR_RESOLUTION_OFF.",
                            0);
                    }
                }
            }
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE &&
            config->subordinate_delay_off_master_usec != 0)
        {
            uint32_t fps_in_usec = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config->camera_fps));
            if (config->subordinate_delay_off_master_usec > fps_in_usec)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR("The configured subordinate device delay from the master device cannot exceed one frame "
                          "interval of %d. User requested %d",
                          fps_in_usec,
                          config->subordinate_delay_off_master_usec);
            }
        }

        if (config->wired_sync_mode != K4A_WIRED_SYNC_MODE_SUBORDINATE &&
            config->subordinate_delay_off_master_usec != 0)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("When wired_sync_mode is K4A_WIRED_SYNC_MODE_STANDALONE or K4A_WIRED_SYNC_MODE_MASTER, the "
                      "subordinate_delay_off_master_usec must be 0.",
                      0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->depth_mode != K4A_DEPTH_MODE_OFF)
        {
            depth_enabled = true;
        }

        if (config->color_resolution != K4A_COLOR_RESOLUTION_OFF)
        {
            color_enabled = true;
        }

        if (depth_enabled && color_enabled)
        {
            int64_t fps = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config->camera_fps));
            if (config->depth_delay_off_color_usec < -fps || config->depth_delay_off_color_usec > fps)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR("The configured depth_delay_off_color_usec must be within +/- one frame interval of %d. User "
                          "requested %d",
                          fps,
                          config->depth_delay_off_color_usec);
            }
        }
        else if (!depth_enabled && !color_enabled)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("Neither depth camera nor color camera are enabled in the configuration, at least one needs to "
                      "be enabled.",
                      0);
        }
        else
        {
            if (config->depth_delay_off_color_usec != 0)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR("If depth_delay_off_color_usec is not 0, both depth camera and color camera must be enabled.",
                          0);
            }

            if (config->synchronized_images_only)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR(
                    "To enable synchronized_images_only, both depth camera and color camera must also be enabled.", 0);
            }
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (depth_enabled)
        {
            struct _depth_configuration
            {
                k4a_depth_mode_t mode;
                k4a_fps_t max_fps;
            } supported_depth_configs[] = {
                { K4A_DEPTH_MODE_NFOV_2X2BINNED, K4A_FRAMES_PER_SECOND_30 },
                { K4A_DEPTH_MODE_NFOV_UNBINNED, K4A_FRAMES_PER_SECOND_30 },
                { K4A_DEPTH_MODE_WFOV_2X2BINNED, K4A_FRAMES_PER_SECOND_30 },
                { K4A_DEPTH_MODE_WFOV_UNBINNED, K4A_FRAMES_PER_SECOND_15 },
                { K4A_DEPTH_MODE_PASSIVE_IR, K4A_FRAMES_PER_SECOND_30 },
            };

            bool depth_fps_and_mode_supported = false;
            for (unsigned int x = 0; x < COUNTOF(supported_depth_configs); x++)
            {
                if (supported_depth_configs[x].mode == config->depth_mode &&
                    supported_depth_configs[x].max_fps >= config->camera_fps)
                {
                    depth_fps_and_mode_supported = true;
                    break;
                }
            }

            if (!depth_fps_and_mode_supported)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR("The configured depth_mode %s does not support the configured camera_fps %s.",
                          k4a_depth_mode_to_string(config->depth_mode),
                          k4a_fps_to_string(config->camera_fps));
            }
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (color_enabled)
        {
            struct _color_configuration
            {
                k4a_color_resolution_t res;
                k4a_image_format_t format;
                k4a_fps_t max_fps;
            } supported_color_configs[] = {
                { K4A_COLOR_RESOLUTION_2160P, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_1440P, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_1080P, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_720P, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_720P, K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_720P, K4A_IMAGE_FORMAT_COLOR_NV12, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_3072P, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_FRAMES_PER_SECOND_15 },
                { K4A_COLOR_RESOLUTION_1536P, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_2160P, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_1440P, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_1080P, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_720P, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_FRAMES_PER_SECOND_30 },
                { K4A_COLOR_RESOLUTION_3072P, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_FRAMES_PER_SECOND_15 },
                { K4A_COLOR_RESOLUTION_1536P, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_FRAMES_PER_SECOND_30 },
            };

            bool color_fps_and_res_and_format_supported = false;
            for (unsigned int x = 0; x < COUNTOF(supported_color_configs); x++)
            {
                if (supported_color_configs[x].res == config->color_resolution &&
                    supported_color_configs[x].max_fps >= config->camera_fps &&
                    supported_color_configs[x].format == config->color_format)
                {
                    color_fps_and_res_and_format_supported = true;
                    break;
                }
            }

            if (!color_fps_and_res_and_format_supported)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR("The combination of color_resolution at %s, color_format at %s, and camera_fps at %s is not "
                          "supported.",
                          k4a_color_resolution_to_string(config->color_resolution),
                          k4a_image_format_to_string(config->color_format),
                          k4a_fps_to_string(config->camera_fps));
            }
        }
    }
    return result;
}

k4a_result_t k4a_device_start_cameras(k4a_device_t device_handle, const k4a_device_configuration_t *config)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    LOG_TRACE("k4a_device_start_cameras starting", 0);
    if (device->depth_started == true || device->color_started == true)
    {
        LOG_ERROR("k4a_device_start_cameras called while one of the sensors are running, depth:%d color:%d",
                  device->depth_started,
                  device->color_started);
        result = K4A_RESULT_FAILED;
    }

    if (device->imu_started == true)
    {
        // Color camera resets the IMU timestamp so we avoid that condition.
        LOG_ERROR("k4a_device_start_cameras called while the IMU is running is not supported, stop the IMU", 0);
        result = K4A_RESULT_FAILED;
    }

    if (K4A_SUCCEEDED(result))
    {
        LOG_INFO("Starting camera's with the following config.", 0);
        LOG_INFO("    color_format:%d", config->color_format);
        LOG_INFO("    color_resolution:%d", config->color_resolution);
        LOG_INFO("    depth_mode:%d", config->depth_mode);
        LOG_INFO("    camera_fps:%d", config->camera_fps);
        LOG_INFO("    synchronized_images_only:%d", config->synchronized_images_only);
        LOG_INFO("    depth_delay_off_color_usec:%d", config->depth_delay_off_color_usec);
        LOG_INFO("    wired_sync_mode:%d", config->wired_sync_mode);
        LOG_INFO("    subordinate_delay_off_master_usec:%d", config->subordinate_delay_off_master_usec);
        LOG_INFO("    disable_streaming_indicator:%d", config->disable_streaming_indicator);
        result = TRACE_CALL(validate_configuration(device, config));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(colormcu_set_multi_device_mode(device->colormcu, config));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(capturesync_start(device->capturesync, config));
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->depth_mode != K4A_DEPTH_MODE_OFF)
        {
            result = TRACE_CALL(depth_start(device->depth, config));
        }
        if (K4A_SUCCEEDED(result))
        {
            device->depth_started = true;
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->color_resolution != K4A_COLOR_RESOLUTION_OFF)
        {
            // NOTE: Color must be started before depth and IMU as it triggers the sync of PTS. If it starts after
            // depth or IMU, the user will see timestamps reset back to zero when the color camera is started.
            result = TRACE_CALL(color_start(device->color, config));
        }
        if (K4A_SUCCEEDED(result))
        {
            device->color_started = true;
        }
    }
    LOG_INFO("k4a_device_start_cameras started", 0);

    if (K4A_FAILED(result))
    {
        k4a_device_stop_cameras(device_handle);
    }

    return result;
}

void k4a_device_stop_cameras(k4a_device_t device_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    LOG_INFO("k4a_device_stop_cameras stopping", 0);

    // Capturesync needs to stop before color so that all queues will purged
    if (device->capturesync)
    {
        capturesync_stop(device->capturesync);
    }

    if (device->depth)
    {
        depth_stop(device->depth);
        device->depth_started = false;
    }

    if (device->color)
    {
        // This call will block waiting for all outstanding allocations to be released
        color_stop(device->color);
        device->color_started = false;
    }

    LOG_INFO("k4a_device_stop_cameras stopped", 0);
}

k4a_buffer_result_t k4a_device_get_serialnum(k4a_device_t device_handle,
                                             char *serial_number,
                                             size_t *serial_number_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    return TRACE_BUFFER_CALL(depth_get_device_serialnum(device->depth, serial_number, serial_number_size));
}

k4a_result_t k4a_device_get_version(k4a_device_t device_handle, k4a_hardware_version_t *version)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    return TRACE_CALL(depth_get_device_version(device->depth, version));
}

k4a_result_t k4a_device_get_sync_jack(k4a_device_t device_handle,
                                      bool *sync_in_jack_connected,
                                      bool *sync_out_jack_connected)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    return TRACE_CALL(
        colormcu_get_external_sync_jack_state(device->colormcu, sync_in_jack_connected, sync_out_jack_connected));
}

k4a_result_t k4a_device_get_color_control_capabilities(k4a_device_t device_handle,
                                                       k4a_color_control_command_t command,
                                                       bool *supports_auto,
                                                       int32_t *min_value,
                                                       int32_t *max_value,
                                                       int32_t *step_value,
                                                       int32_t *default_value,
                                                       k4a_color_control_mode_t *default_mode)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    return TRACE_CALL(color_get_control_capabilities(
        device->color, command, supports_auto, min_value, max_value, step_value, default_value, default_mode));
}

k4a_result_t k4a_device_get_color_control(k4a_device_t device_handle,
                                          k4a_color_control_command_t command,
                                          k4a_color_control_mode_t *mode,
                                          int32_t *value)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    return TRACE_CALL(color_get_control(device->color, command, mode, value));
}

k4a_result_t k4a_device_set_color_control(k4a_device_t device_handle,
                                          k4a_color_control_command_t command,
                                          k4a_color_control_mode_t mode,
                                          int32_t value)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    return TRACE_CALL(color_set_control(device->color, command, mode, value));
}

k4a_buffer_result_t k4a_device_get_raw_calibration(k4a_device_t device_handle, uint8_t *data, size_t *data_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    return calibration_get_raw_data(device->calibration, data, data_size);
}

k4a_result_t k4a_device_get_calibration(k4a_device_t device_handle,
                                        const k4a_depth_mode_t depth_mode,
                                        const k4a_color_resolution_t color_resolution,
                                        k4a_calibration_t *calibration)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_context_t *device = k4a_device_t_get_context(device_handle);

    k4a_calibration_camera_t depth_calibration;
    if (K4A_FAILED(
            TRACE_CALL(calibration_get_camera(device->calibration, K4A_CALIBRATION_TYPE_DEPTH, &depth_calibration))))
    {
        return K4A_RESULT_FAILED;
    }

    k4a_calibration_camera_t color_calibration;
    if (K4A_FAILED(
            TRACE_CALL(calibration_get_camera(device->calibration, K4A_CALIBRATION_TYPE_COLOR, &color_calibration))))
    {
        return K4A_RESULT_FAILED;
    }

    k4a_calibration_extrinsics_t *gyro_extrinsics = imu_get_gyro_extrinsics(device->imu);
    k4a_calibration_extrinsics_t *accel_extrinsics = imu_get_accel_extrinsics(device->imu);

    return TRACE_CALL(transformation_get_mode_specific_calibration(&depth_calibration,
                                                                   &color_calibration,
                                                                   gyro_extrinsics,
                                                                   accel_extrinsics,
                                                                   depth_mode,
                                                                   color_resolution,
                                                                   calibration));
}

k4a_result_t k4a_calibration_get_from_raw(char *raw_calibration,
                                          size_t raw_calibration_size,
                                          const k4a_depth_mode_t depth_mode,
                                          const k4a_color_resolution_t color_resolution,
                                          k4a_calibration_t *calibration)
{
    k4a_calibration_camera_t depth_calibration;
    k4a_calibration_camera_t color_calibration;
    k4a_calibration_imu_t gyro_calibration;
    k4a_calibration_imu_t accel_calibration;
    k4a_result_t result;

    result = TRACE_CALL(calibration_create_from_raw(raw_calibration,
                                                    raw_calibration_size,
                                                    &depth_calibration,
                                                    &color_calibration,
                                                    &gyro_calibration,
                                                    &accel_calibration));

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(transformation_get_mode_specific_calibration(&depth_calibration,
                                                                         &color_calibration,
                                                                         &gyro_calibration.depth_to_imu,
                                                                         &accel_calibration.depth_to_imu,
                                                                         depth_mode,
                                                                         color_resolution,
                                                                         calibration));
    }
    return result;
}

k4a_result_t k4a_calibration_3d_to_3d(const k4a_calibration_t *calibration,
                                      const k4a_float3_t *source_point3d_mm,
                                      const k4a_calibration_type_t source_camera,
                                      const k4a_calibration_type_t target_camera,
                                      k4a_float3_t *target_point3d_mm)
{
    return TRACE_CALL(
        transformation_3d_to_3d(calibration, source_point3d_mm->v, source_camera, target_camera, target_point3d_mm->v));
}

k4a_result_t k4a_calibration_2d_to_3d(const k4a_calibration_t *calibration,
                                      const k4a_float2_t *source_point2d,
                                      const float source_depth_mm,
                                      const k4a_calibration_type_t source_camera,
                                      const k4a_calibration_type_t target_camera,
                                      k4a_float3_t *target_point3d_mm,
                                      int *valid)
{
    return TRACE_CALL(transformation_2d_to_3d(
        calibration, source_point2d->v, source_depth_mm, source_camera, target_camera, target_point3d_mm->v, valid));
}

k4a_result_t k4a_calibration_3d_to_2d(const k4a_calibration_t *calibration,
                                      const k4a_float3_t *source_point3d_mm,
                                      const k4a_calibration_type_t source_camera,
                                      const k4a_calibration_type_t target_camera,
                                      k4a_float2_t *target_point2d,
                                      int *valid)
{
    return TRACE_CALL(transformation_3d_to_2d(
        calibration, source_point3d_mm->v, source_camera, target_camera, target_point2d->v, valid));
}

k4a_result_t k4a_calibration_2d_to_2d(const k4a_calibration_t *calibration,
                                      const k4a_float2_t *source_point2d,
                                      const float source_depth_mm,
                                      const k4a_calibration_type_t source_camera,
                                      const k4a_calibration_type_t target_camera,
                                      k4a_float2_t *target_point2d,
                                      int *valid)
{
    return TRACE_CALL(transformation_2d_to_2d(
        calibration, source_point2d->v, source_depth_mm, source_camera, target_camera, target_point2d->v, valid));
}

k4a_result_t k4a_calibration_color_2d_to_depth_2d(const k4a_calibration_t *calibration,
                                                  const k4a_float2_t *source_point2d,
                                                  const k4a_image_t depth_image,
                                                  k4a_float2_t *target_point2d,
                                                  int *valid)
{
    return TRACE_CALL(
        transformation_color_2d_to_depth_2d(calibration, source_point2d->v, depth_image, target_point2d->v, valid));
}

k4a_transformation_t k4a_transformation_create(const k4a_calibration_t *calibration)
{
    return transformation_create(calibration, TRANSFORM_ENABLE_GPU_OPTIMIZATION);
}

void k4a_transformation_destroy(k4a_transformation_t transformation_handle)
{
    transformation_destroy(transformation_handle);
}

static k4a_transformation_image_descriptor_t k4a_image_get_descriptor(const k4a_image_t image)
{
    k4a_transformation_image_descriptor_t descriptor;
    descriptor.width_pixels = k4a_image_get_width_pixels(image);
    descriptor.height_pixels = k4a_image_get_height_pixels(image);
    descriptor.stride_bytes = k4a_image_get_stride_bytes(image);
    descriptor.format = k4a_image_get_format(image);
    return descriptor;
}

k4a_result_t k4a_transformation_depth_image_to_color_camera(k4a_transformation_t transformation_handle,
                                                            const k4a_image_t depth_image,
                                                            k4a_image_t transformed_depth_image)
{
    k4a_transformation_image_descriptor_t depth_image_descriptor = k4a_image_get_descriptor(depth_image);
    k4a_transformation_image_descriptor_t transformed_depth_image_descriptor = k4a_image_get_descriptor(
        transformed_depth_image);

    uint8_t *depth_image_buffer = k4a_image_get_buffer(depth_image);
    uint8_t *transformed_depth_image_buffer = k4a_image_get_buffer(transformed_depth_image);

    // Both k4a_transformation_depth_image_to_color_camera and k4a_transformation_depth_image_to_color_camera_custom
    // call the same implementation of transformation_depth_image_to_color_camera_custom. The below parameters need
    // to be passed in but they will be ignored in the internal implementation.
    k4a_transformation_image_descriptor_t dummy_descriptor = { 0 };
    uint8_t *custom_image_buffer = NULL;
    uint8_t *transformed_custom_image_buffer = NULL;
    k4a_transformation_interpolation_type_t interpolation_type = K4A_TRANSFORMATION_INTERPOLATION_TYPE_LINEAR;
    uint32_t invalid_custom_value = 0;

    return TRACE_CALL(transformation_depth_image_to_color_camera_custom(transformation_handle,
                                                                        depth_image_buffer,
                                                                        &depth_image_descriptor,
                                                                        custom_image_buffer,
                                                                        &dummy_descriptor,
                                                                        transformed_depth_image_buffer,
                                                                        &transformed_depth_image_descriptor,
                                                                        transformed_custom_image_buffer,
                                                                        &dummy_descriptor,
                                                                        interpolation_type,
                                                                        invalid_custom_value));
}

k4a_result_t
k4a_transformation_depth_image_to_color_camera_custom(k4a_transformation_t transformation_handle,
                                                      const k4a_image_t depth_image,
                                                      const k4a_image_t custom_image,
                                                      k4a_image_t transformed_depth_image,
                                                      k4a_image_t transformed_custom_image,
                                                      k4a_transformation_interpolation_type_t interpolation_type,
                                                      uint32_t invalid_custom_value)
{
    k4a_transformation_image_descriptor_t depth_image_descriptor = k4a_image_get_descriptor(depth_image);
    k4a_transformation_image_descriptor_t custom_image_descriptor = k4a_image_get_descriptor(custom_image);
    k4a_transformation_image_descriptor_t transformed_depth_image_descriptor = k4a_image_get_descriptor(
        transformed_depth_image);
    k4a_transformation_image_descriptor_t transformed_custom_image_descriptor = k4a_image_get_descriptor(
        transformed_custom_image);

    uint8_t *depth_image_buffer = k4a_image_get_buffer(depth_image);
    uint8_t *custom_image_buffer = k4a_image_get_buffer(custom_image);
    uint8_t *transformed_depth_image_buffer = k4a_image_get_buffer(transformed_depth_image);
    uint8_t *transformed_custom_image_buffer = k4a_image_get_buffer(transformed_custom_image);

    return TRACE_CALL(transformation_depth_image_to_color_camera_custom(transformation_handle,
                                                                        depth_image_buffer,
                                                                        &depth_image_descriptor,
                                                                        custom_image_buffer,
                                                                        &custom_image_descriptor,
                                                                        transformed_depth_image_buffer,
                                                                        &transformed_depth_image_descriptor,
                                                                        transformed_custom_image_buffer,
                                                                        &transformed_custom_image_descriptor,
                                                                        interpolation_type,
                                                                        invalid_custom_value));
}

k4a_result_t k4a_transformation_color_image_to_depth_camera(k4a_transformation_t transformation_handle,
                                                            const k4a_image_t depth_image,
                                                            const k4a_image_t color_image,
                                                            k4a_image_t transformed_color_image)
{
    k4a_transformation_image_descriptor_t depth_image_descriptor = k4a_image_get_descriptor(depth_image);
    k4a_transformation_image_descriptor_t color_image_descriptor = k4a_image_get_descriptor(color_image);
    k4a_transformation_image_descriptor_t transformed_color_image_descriptor = k4a_image_get_descriptor(
        transformed_color_image);

    k4a_image_format_t color_image_format = k4a_image_get_format(color_image);
    k4a_image_format_t transformed_color_image_format = k4a_image_get_format(transformed_color_image);
    if (!(color_image_format == K4A_IMAGE_FORMAT_COLOR_BGRA32 &&
          transformed_color_image_format == K4A_IMAGE_FORMAT_COLOR_BGRA32))
    {
        LOG_ERROR("Require color image and transformed color image both have bgra32 format.", 0);
        return K4A_RESULT_FAILED;
    }

    uint8_t *depth_image_buffer = k4a_image_get_buffer(depth_image);
    uint8_t *color_image_buffer = k4a_image_get_buffer(color_image);
    uint8_t *transformed_color_image_buffer = k4a_image_get_buffer(transformed_color_image);

    return TRACE_CALL(transformation_color_image_to_depth_camera(transformation_handle,
                                                                 depth_image_buffer,
                                                                 &depth_image_descriptor,
                                                                 color_image_buffer,
                                                                 &color_image_descriptor,
                                                                 transformed_color_image_buffer,
                                                                 &transformed_color_image_descriptor));
}

k4a_result_t k4a_transformation_depth_image_to_point_cloud(k4a_transformation_t transformation_handle,
                                                           const k4a_image_t depth_image,
                                                           const k4a_calibration_type_t camera,
                                                           k4a_image_t xyz_image)
{
    k4a_transformation_image_descriptor_t depth_image_descriptor = k4a_image_get_descriptor(depth_image);
    k4a_transformation_image_descriptor_t xyz_image_descriptor = k4a_image_get_descriptor(xyz_image);

    uint8_t *depth_image_buffer = k4a_image_get_buffer(depth_image);
    uint8_t *xyz_image_buffer = k4a_image_get_buffer(xyz_image);

    return TRACE_CALL(transformation_depth_image_to_point_cloud(transformation_handle,
                                                                depth_image_buffer,
                                                                &depth_image_descriptor,
                                                                camera,
                                                                xyz_image_buffer,
                                                                &xyz_image_descriptor));
}

#ifdef __cplusplus
}
#endif
