// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/color.h>

// Dependent libraries

// System dependencies
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <new>
#include <array>

#include "color_priv.h"

#ifdef _WIN32
#include "mfcamerareader.h"
#else
#include "uvc_camerareader.h"
#include <memory>
#endif

#ifdef __cplusplus
extern "C" {
#endif

color_cb_stream_t color_capture_available;

typedef struct _color_context_t
{
    TICK_COUNTER_HANDLE tick;
    color_cb_streaming_capture_t *capture_ready_cb;
    void *capture_ready_cb_context;
    tickcounter_ms_t sensor_start_time_tick;
    std::array<color_control_cap_t, K4A_COLOR_CONTROL_POWERLINE_FREQUENCY + 1> control_cap = {};
#ifdef _WIN32
    Microsoft::WRL::ComPtr<CMFCameraReader> m_spCameraReader;
#else
    std::unique_ptr<UVCCameraReader> m_spCameraReader;
#endif
} color_context_t;

K4A_DECLARE_CONTEXT(color_t, color_context_t);

k4a_result_t color_create(TICK_COUNTER_HANDLE tick_handle,
                          const guid_t *container_id,
                          const char *serial_number,
                          color_cb_streaming_capture_t capture_ready,
                          void *capture_ready_context,
                          color_t *color_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, tick_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, color_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, container_id == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, serial_number == NULL);

    color_context_t *color = color_t_create(color_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(color != NULL);

    if (K4A_SUCCEEDED(result))
    {
        color->capture_ready_cb = capture_ready;
        color->capture_ready_cb_context = capture_ready_context;
        color->sensor_start_time_tick = 0;
        color->tick = tick_handle;

#ifdef _WIN32
        (void)(serial_number);
        static_assert(sizeof(guid_t) == sizeof(GUID), "Windows GUID and this guid_t are not the same");
        result = K4A_RESULT_FROM_BOOL(SUCCEEDED(
            Microsoft::WRL::MakeAndInitialize<CMFCameraReader>(&color->m_spCameraReader, (GUID *)container_id)));
#else
        (void)(container_id);
        color->m_spCameraReader.reset(new (std::nothrow) UVCCameraReader);
        if (!color->m_spCameraReader)
        {
            result = K4A_RESULT_FAILED;
        }
        else
        {
            result = TRACE_CALL(color->m_spCameraReader->Init(serial_number));
        }
#endif
    }

    if (K4A_FAILED(result))
    {
        color_destroy(*color_handle);
        *color_handle = NULL;
    }
    return result;
}

void color_destroy(color_t color_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, color_t, color_handle);

    color_stop(color_handle);

    color_context_t *color = color_t_get_context(color_handle);
    if (color->m_spCameraReader)
    {
        color->m_spCameraReader->Shutdown();
        color->m_spCameraReader = nullptr;
    }
    color_t_destroy(color_handle);
}

void color_capture_available(k4a_result_t result, k4a_capture_t capture_handle, void *context)
{
    color_context_t *color = (color_context_t *)context;

    if (color->capture_ready_cb)
    {
        color->capture_ready_cb(result, capture_handle, color->capture_ready_cb_context);
    }

    if (result != K4A_RESULT_SUCCEEDED)
    {
        assert(capture_handle == NULL);
        LOG_WARNING("A streaming color transfer failed", 0);
        return;
    }
}

k4a_result_t color_start(color_t color_handle, const k4a_device_configuration_t *config)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, color_t, color_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    color_context_t *color = color_t_get_context(color_handle);
    uint32_t width = 0;
    uint32_t height = 0;
    float fps = 1.0f;

    switch (config->color_resolution)
    {
    case K4A_COLOR_RESOLUTION_720P:
        width = 1280;
        height = 720;
        break;
    case K4A_COLOR_RESOLUTION_2160P:
        width = 3840;
        height = 2160;
        break;
    case K4A_COLOR_RESOLUTION_1440P:
        width = 2560;
        height = 1440;
        break;
    case K4A_COLOR_RESOLUTION_1080P:
        width = 1920;
        height = 1080;
        break;
    case K4A_COLOR_RESOLUTION_3072P:
        width = 4096;
        height = 3072;
        break;
    case K4A_COLOR_RESOLUTION_1536P:
        width = 2048;
        height = 1536;
        break;
    default:
        LOG_ERROR("color_resolution %d is invalid", config->color_resolution);
        return K4A_RESULT_FAILED;
    }

    switch (config->camera_fps)
    {
    case K4A_FRAMES_PER_SECOND_30:
        fps = 30.0f;
        break;
    case K4A_FRAMES_PER_SECOND_15:
        fps = 15.0f;
        break;
    case K4A_FRAMES_PER_SECOND_5:
        fps = 5.0f;
        break;
    default:
        LOG_ERROR("camera_fps %d is invalid", config->camera_fps);
        return K4A_RESULT_FAILED;
    }

    result = K4A_RESULT_FROM_BOOL(tickcounter_get_current_ms(color->tick, &color->sensor_start_time_tick) == 0);

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(color->m_spCameraReader->Start(width,
                                                           height,                   // Resolution
                                                           fps,                      // Framerate
                                                           config->color_format,     // Color format enum
                                                           &color_capture_available, // Callback
                                                           color));                  // Callback context
    }

    if (K4A_FAILED(result))
    {
        color_stop(color_handle);
    }

    return result;
}

void color_stop(color_t color_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, color_t, color_handle);
    color_context_t *color = color_t_get_context(color_handle);
    color->sensor_start_time_tick = 0;

    // Request stop streaming and wait until clean up and flushing.
    // After this call, no more sample callback will be called.

    if (color->m_spCameraReader)
    {
        color->m_spCameraReader->Stop();
    }

    return;
}

tickcounter_ms_t color_get_sensor_start_time_tick(const color_t handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, color_t, handle);
    color_context_t *color = color_t_get_context(handle);
    return color->sensor_start_time_tick;
}

k4a_result_t color_get_control_capabilities(const color_t handle,
                                            const k4a_color_control_command_t command,
                                            bool *supports_auto,
                                            int32_t *min_value,
                                            int32_t *max_value,
                                            int32_t *step_value,
                                            int32_t *default_value,
                                            k4a_color_control_mode_t *default_mode)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, color_t, handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED,
                        command < K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE ||
                            command > K4A_COLOR_CONTROL_POWERLINE_FREQUENCY);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, supports_auto == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, min_value == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, max_value == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, step_value == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, default_value == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, default_mode == NULL);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    color_context_t *color = color_t_get_context(handle);

    if (color->control_cap[command].valid == false)
    {
        result = color->m_spCameraReader->GetCameraControlCapabilities(command, &(color->control_cap[command]));
    }

    if (K4A_SUCCEEDED(result))
    {
        *supports_auto = color->control_cap[command].supportAuto;
        *min_value = color->control_cap[command].minValue;
        *max_value = color->control_cap[command].maxValue;
        *step_value = color->control_cap[command].stepValue;
        *default_value = color->control_cap[command].defaultValue;
        *default_mode = color->control_cap[command].defaultMode;
    }

    return result;
}

k4a_result_t color_get_control(const color_t handle,
                               const k4a_color_control_command_t command,
                               k4a_color_control_mode_t *mode,
                               int32_t *value)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, color_t, handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED,
                        command < K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE ||
                            command > K4A_COLOR_CONTROL_POWERLINE_FREQUENCY);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, mode == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, value == NULL);
    color_context_t *color = color_t_get_context(handle);

    return color->m_spCameraReader->GetCameraControl(command, mode, value);
}

k4a_result_t color_set_control(const color_t handle,
                               const k4a_color_control_command_t command,
                               const k4a_color_control_mode_t mode,
                               int32_t value)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, color_t, handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED,
                        command < K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE ||
                            command > K4A_COLOR_CONTROL_POWERLINE_FREQUENCY);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED,
                        mode != K4A_COLOR_CONTROL_MODE_AUTO && mode != K4A_COLOR_CONTROL_MODE_MANUAL);
    color_context_t *color = color_t_get_context(handle);

    return color->m_spCameraReader->SetCameraControl(command, mode, value);
}

#ifdef __cplusplus
}
#endif
