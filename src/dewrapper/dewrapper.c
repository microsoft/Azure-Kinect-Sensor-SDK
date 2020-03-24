// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library is Depth Engine Wrapper
#include <k4ainternal/dewrapper.h>

// Dependent libraries
#include <k4ainternal/queue.h>
#include <k4ainternal/calibration.h>
#include <k4ainternal/deloader.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/condition.h>
#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/refcount.h>

// System dependencies
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DEWRAPPER_QUEUE_DEPTH ((uint32_t)2) // We should not need to store more than 1

typedef struct _dewrapper_context_t
{
    queue_t queue;

    uint8_t *calibration_memory;           // Calibration block passed durring create - we do not own this memory
    size_t calibration_memory_size;        // Calibration block size
    k4a_calibration_camera_t *calibration; // Copy of calibration passed in - we do not own this memory

    THREAD_HANDLE thread;
    LOCK_HANDLE lock;
    COND_HANDLE condition;
    volatile bool thread_started;
    volatile bool thread_stop;
    k4a_result_t thread_start_result;

    k4a_fps_t fps;
    k4a_depth_mode_t depth_mode;

    TICK_COUNTER_HANDLE tick;
    dewrapper_streaming_capture_cb_t *capture_ready_cb;
    void *capture_ready_cb_context;

    k4a_depth_engine_context_t *depth_engine;

} dewrapper_context_t;

typedef struct _shared_image_context_t
{
    // Overall shared buffer
    uint8_t *buffer;
    volatile long ref;
} shared_image_context_t;

K4A_DECLARE_CONTEXT(dewrapper_t, dewrapper_context_t);

static k4a_depth_engine_mode_t get_de_mode_from_depth_mode(k4a_depth_mode_t mode)
{
    k4a_depth_engine_mode_t de_mode;

    switch (mode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
        de_mode = K4A_DEPTH_ENGINE_MODE_LT_SW_BINNING;
        break;
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        de_mode = K4A_DEPTH_ENGINE_MODE_QUARTER_MEGA_PIXEL;
        break;
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        de_mode = K4A_DEPTH_ENGINE_MODE_LT_NATIVE;
        break;
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        de_mode = K4A_DEPTH_ENGINE_MODE_MEGA_PIXEL;
        break;
    case K4A_DEPTH_MODE_PASSIVE_IR:
        de_mode = K4A_DEPTH_ENGINE_MODE_PCM;
        break;
    default:
        assert(0);
        de_mode = K4A_DEPTH_ENGINE_MODE_UNKNOWN;
    }
    return de_mode;
}

static k4a_depth_engine_input_type_t get_input_format_from_depth_mode(k4a_depth_mode_t mode)
{
    k4a_depth_engine_mode_t de_mode = get_de_mode_from_depth_mode(mode);
    k4a_depth_engine_input_type_t format;

    format = K4A_DEPTH_ENGINE_INPUT_TYPE_12BIT_COMPRESSED;
    if (de_mode == K4A_DEPTH_ENGINE_MODE_MEGA_PIXEL)
    {
        format = K4A_DEPTH_ENGINE_INPUT_TYPE_8BIT_COMPRESSED;
    }
    return format;
}

/** Depth engine uses 1 large allocation to write two images; depth & IR. We then create 2 k4a_image_t's to manage the
 * lifetime. This function is the destroy callback when each of the two images is destroyed. Once both have been
 * destroyed this function will destroy the shared memory between the two.
 */
static void free_shared_depth_image(void *buffer, void *context)
{
    RETURN_VALUE_IF_ARG(VOID_VALUE, buffer == NULL);
    RETURN_VALUE_IF_ARG(VOID_VALUE, context == NULL);

    // The buffer being passed in may be the beginning or the middle of the
    // overall shared buffer
    (void)buffer;

    shared_image_context_t *shared_context = (shared_image_context_t *)context;

    long count = DEC_REF_VAR(shared_context->ref);

    if (count == 0)
    {
        allocator_free(shared_context->buffer);
        free(context);
    }
}

static k4a_result_t depth_engine_start_helper(dewrapper_context_t *dewrapper,
                                              k4a_fps_t fps,
                                              k4a_depth_mode_t depth_mode,
                                              int *depth_engine_max_compute_time_ms,
                                              size_t *depth_engine_output_buffer_size)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, fps < K4A_FRAMES_PER_SECOND_5 || fps > K4A_FRAMES_PER_SECOND_30);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, depth_mode <= K4A_DEPTH_MODE_OFF || depth_mode > K4A_DEPTH_MODE_PASSIVE_IR);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    assert(dewrapper->depth_engine == NULL);
    assert(dewrapper->calibration_memory != NULL);

    // Max comput time is the configured FPS
    *depth_engine_max_compute_time_ms = HZ_TO_PERIOD_MS(k4a_convert_fps_to_uint(fps));
    result = K4A_RESULT_FROM_BOOL(*depth_engine_max_compute_time_ms != 0);

    if (K4A_SUCCEEDED(result))
    {
        k4a_depth_engine_result_code_t deresult =
            deloader_depth_engine_create_and_initialize(&dewrapper->depth_engine,
                                                        dewrapper->calibration_memory_size,
                                                        dewrapper->calibration_memory,
                                                        get_de_mode_from_depth_mode(depth_mode),
                                                        get_input_format_from_depth_mode(depth_mode),
                                                        dewrapper->calibration, // k4a_calibration_camera_t*
                                                        NULL,                   // Callback
                                                        NULL);                  // Callback Context
        if (deresult != K4A_DEPTH_ENGINE_RESULT_SUCCEEDED)
        {
            LOG_ERROR("Depth engine create and initialize failed with error code: %d.", deresult);
            if (deresult == K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_OPENGL_CONTEXT)
            {
                LOG_ERROR("OpenGL 4.4 context creation failed. You could try updating your graphics drivers.", 0);
            }
        }
        result = K4A_RESULT_FROM_BOOL(deresult == K4A_DEPTH_ENGINE_RESULT_SUCCEEDED);
    }

    if (K4A_SUCCEEDED(result))
    {
        *depth_engine_output_buffer_size = deloader_depth_engine_get_output_frame_size(dewrapper->depth_engine);
        result = K4A_RESULT_FROM_BOOL(0 != *depth_engine_output_buffer_size);
    }

    return result;
}

static void depth_engine_stop_helper(dewrapper_context_t *dewrapper)
{
    if (dewrapper->depth_engine != NULL)
    {
        deloader_depth_engine_destroy(&dewrapper->depth_engine);
        dewrapper->depth_engine = NULL;
    }
}

static int depth_engine_thread(void *param)
{
    dewrapper_context_t *dewrapper = (dewrapper_context_t *)param;

    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    size_t depth_engine_output_buffer_size;
    int depth_engine_max_compute_time_ms;
    bool received_valid_image = false;

    result = TRACE_CALL(depth_engine_start_helper(dewrapper,
                                                  dewrapper->fps,
                                                  dewrapper->depth_mode,
                                                  &depth_engine_max_compute_time_ms,
                                                  &depth_engine_output_buffer_size));

    // The Start routine is blocked waiting for this thread to complete startup, so we signal it here and share our
    // startup status.
    Lock(dewrapper->lock);
    dewrapper->thread_started = true;
    dewrapper->thread_start_result = result;
    Condition_Post(dewrapper->condition);
    Unlock(dewrapper->lock);

    // NOTE: Failures after this point are reported to the user via the k4a_device_get_capture()

    while (result != K4A_RESULT_FAILED && dewrapper->thread_stop == false)
    {
        k4a_capture_t capture = NULL;
        k4a_capture_t capture_raw = NULL;
        k4a_image_t image_raw = NULL;
        k4a_depth_engine_output_frame_info_t outputCaptureInfo = { 0 };
        uint8_t *capture_byte_ptr = NULL;
        bool cleanup_capture_byte_ptr = true;
        shared_image_context_t *shared_image_context = NULL;
        uint8_t *raw_image_buffer = NULL;
        size_t raw_image_buffer_size = 0;
        bool dropped = false;

        k4a_wait_result_t wresult = queue_pop(dewrapper->queue, K4A_WAIT_INFINITE, &capture_raw);
        if (wresult != K4A_WAIT_RESULT_SUCCEEDED)
        {
            result = K4A_RESULT_FAILED;
        }

        if (K4A_SUCCEEDED(result))
        {
            image_raw = capture_get_ir_image(capture_raw);
            result = K4A_RESULT_FROM_BOOL(image_raw != NULL);
        }

        if (K4A_SUCCEEDED(result))
        {
            raw_image_buffer = image_get_buffer(image_raw);
            raw_image_buffer_size = image_get_size(image_raw);

            // Allocate 1 buffer for depth engine to write depth and IR images to
            assert(depth_engine_output_buffer_size != 0);
            capture_byte_ptr = allocator_alloc(ALLOCATION_SOURCE_DEPTH, depth_engine_output_buffer_size);
            if (capture_byte_ptr == NULL)
            {
                LOG_ERROR("Depth streaming callback failed to allocate output buffer", 0);
                result = K4A_RESULT_FAILED;
            }
        }

        if (K4A_SUCCEEDED(result))
        {
            tickcounter_ms_t start_time = 0;
            tickcounter_ms_t stop_time = 0;

            tickcounter_get_current_ms(dewrapper->tick, &start_time);
            k4a_depth_engine_result_code_t deresult =
                deloader_depth_engine_process_frame(dewrapper->depth_engine,
                                                    raw_image_buffer,
                                                    raw_image_buffer_size,
                                                    K4A_DEPTH_ENGINE_OUTPUT_TYPE_Z_DEPTH,
                                                    capture_byte_ptr,
                                                    depth_engine_output_buffer_size,
                                                    &outputCaptureInfo,
                                                    NULL);
            tickcounter_get_current_ms(dewrapper->tick, &stop_time);
            if (deresult == K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_WAIT_PROCESSING_COMPLETE_FAILED ||
                deresult == K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_TIMEOUT)
            {
                LOG_ERROR("Timeout during depth engine process frame.", 0);
                LOG_ERROR("SDK should be restarted since it looks like GPU has encountered an unrecoverable error.", 0);
                dropped = true;
                result = K4A_RESULT_FAILED;
            }
            else if (deresult != K4A_DEPTH_ENGINE_RESULT_SUCCEEDED)
            {
                LOG_ERROR("Depth engine process frame failed with error code: %d.", deresult);
                result = K4A_RESULT_FAILED;
            }
            else if ((stop_time - start_time) > (unsigned)depth_engine_max_compute_time_ms)
            {
                LOG_WARNING("Depth image processing is too slow at %lldms (this may be transient).",
                            stop_time - start_time);
            }
        }

        if (K4A_SUCCEEDED(result) && received_valid_image && outputCaptureInfo.center_of_exposure_in_ticks == 0)
        {
            // We drop samples with a timestamp of zero when starting up.
            LOG_WARNING("Dropping depth image due to bad timestamp at startup", 0);
            dropped = true;
            result = K4A_RESULT_FAILED;
        }

        if (K4A_SUCCEEDED(result))
        {
            shared_image_context = (shared_image_context_t *)malloc(sizeof(shared_image_context_t));
            result = K4A_RESULT_FROM_BOOL(shared_image_context != NULL);
        }

        if (K4A_SUCCEEDED(result))
        {
            shared_image_context->ref = 0;
            shared_image_context->buffer = capture_byte_ptr;

            result = TRACE_CALL(capture_create(&capture));
        }

        bool depth16_present = (dewrapper->depth_mode == K4A_DEPTH_MODE_NFOV_2X2BINNED ||
                                dewrapper->depth_mode == K4A_DEPTH_MODE_NFOV_UNBINNED ||
                                dewrapper->depth_mode == K4A_DEPTH_MODE_WFOV_2X2BINNED ||
                                dewrapper->depth_mode == K4A_DEPTH_MODE_WFOV_UNBINNED);

        if (K4A_SUCCEEDED(result) & depth16_present)
        {
            k4a_image_t image;
            int stride_bytes = (int)outputCaptureInfo.output_width * (int)sizeof(uint16_t);
            result = TRACE_CALL(image_create_from_buffer(K4A_IMAGE_FORMAT_DEPTH16,
                                                         outputCaptureInfo.output_width,
                                                         outputCaptureInfo.output_height,
                                                         stride_bytes,
                                                         capture_byte_ptr,
                                                         (size_t)stride_bytes * (size_t)outputCaptureInfo.output_height,
                                                         free_shared_depth_image,
                                                         shared_image_context,
                                                         &image));
            if (K4A_SUCCEEDED(result))
            {
                cleanup_capture_byte_ptr = false; // buffer is now owned by image;
                INC_REF_VAR(shared_image_context->ref);
                image_set_device_timestamp_usec(image,
                                                K4A_90K_HZ_TICK_TO_USEC(outputCaptureInfo.center_of_exposure_in_ticks));
                image_set_system_timestamp_nsec(image, image_get_system_timestamp_nsec(image_raw));
                capture_set_depth_image(capture, image);
                image_dec_ref(image);
            }
        }

        if (K4A_SUCCEEDED(result))
        {
            k4a_image_t image;
            int stride_bytes = (int)outputCaptureInfo.output_width * (int)sizeof(uint16_t);
            uint8_t *image_buf = capture_byte_ptr;
            if (depth16_present)
            {
                image_buf = image_buf + stride_bytes * outputCaptureInfo.output_height;
            }

            result = TRACE_CALL(image_create_from_buffer(K4A_IMAGE_FORMAT_IR16,
                                                         outputCaptureInfo.output_width,
                                                         outputCaptureInfo.output_height,
                                                         stride_bytes,
                                                         image_buf,
                                                         (size_t)stride_bytes * (size_t)outputCaptureInfo.output_height,
                                                         free_shared_depth_image,
                                                         shared_image_context,
                                                         &image));
            if (K4A_SUCCEEDED(result))
            {
                cleanup_capture_byte_ptr = false; // buffer is now owned by image;
                INC_REF_VAR(shared_image_context->ref);
                image_set_device_timestamp_usec(image,
                                                K4A_90K_HZ_TICK_TO_USEC(outputCaptureInfo.center_of_exposure_in_ticks));
                image_set_system_timestamp_nsec(image, image_get_system_timestamp_nsec(image_raw));
                capture_set_ir_image(capture, image);
                image_dec_ref(image);
            }
        }

        if (K4A_SUCCEEDED(result))
        {
            // set capture attributes
            capture_set_temperature_c(capture, outputCaptureInfo.sensor_temp);

            received_valid_image = true;
            dewrapper->capture_ready_cb(result, capture, dewrapper->capture_ready_cb_context);
        }

        if (shared_image_context && shared_image_context->ref == 0)
        {
            // It didn't get used due to a failure
            free(shared_image_context);
        }

        if (capture)
        {
            capture_dec_ref(capture);
        }
        if (capture_raw)
        {
            capture_dec_ref(capture_raw);
        }
        if (image_raw)
        {
            image_dec_ref(image_raw);
            image_raw = NULL;
        }

        if (capture_byte_ptr && cleanup_capture_byte_ptr)
        {
            allocator_free(capture_byte_ptr);
        }

        if (dropped)
        {
            // It is not a fatal error when we drop a frame, so we reset 'result' so that we can continue to run.
            result = K4A_RESULT_SUCCEEDED;
        }
    }

    if (K4A_FAILED(result))
    {
        dewrapper->capture_ready_cb(result, NULL, dewrapper->capture_ready_cb_context);
    }

    depth_engine_stop_helper(dewrapper);

    // This will always return failure, because stop is trigged by the queue being disabled
    return (int)result;
}

dewrapper_t dewrapper_create(k4a_calibration_camera_t *calibration,
                             dewrapper_streaming_capture_cb_t *capture_ready_cb,
                             void *capture_ready_context)
{
    RETURN_VALUE_IF_ARG(NULL, capture_ready_cb == NULL);
    RETURN_VALUE_IF_ARG(NULL, calibration == NULL);

    k4a_result_t result;
    dewrapper_t dewrapper_handle;
    dewrapper_context_t *dewrapper = dewrapper_t_create(&dewrapper_handle);

    dewrapper->calibration = calibration;
    dewrapper->capture_ready_cb = capture_ready_cb;
    dewrapper->capture_ready_cb_context = capture_ready_context;
    dewrapper->thread_start_result = K4A_RESULT_FAILED;
    dewrapper->tick = tickcounter_create();
    result = K4A_RESULT_FROM_BOOL(NULL != dewrapper->tick);

    if (K4A_SUCCEEDED(result))
    {
        dewrapper->lock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(dewrapper->lock != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        dewrapper->condition = Condition_Init();
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(queue_create(DEWRAPPER_QUEUE_DEPTH, "dewrapper", &dewrapper->queue));
    }

    if (K4A_FAILED(result))
    {
        dewrapper_t_destroy(dewrapper_handle);
        dewrapper_handle = NULL;
    }

    return dewrapper_handle;
}

void dewrapper_destroy(dewrapper_t dewrapper_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, dewrapper_t, dewrapper_handle);
    dewrapper_context_t *dewrapper = dewrapper_t_get_context(dewrapper_handle);

    dewrapper_stop(dewrapper_handle);

    if (dewrapper->queue)
    {
        queue_destroy(dewrapper->queue);
    }

    if (dewrapper->tick)
    {
        tickcounter_destroy(dewrapper->tick);
    }

    if (dewrapper->condition)
    {
        Condition_Deinit(dewrapper->condition);
    }

    if (dewrapper->lock)
    {
        Lock_Deinit(dewrapper->lock);
    }
    dewrapper_t_destroy(dewrapper_handle);
}

void dewrapper_post_capture(k4a_result_t cb_result, k4a_capture_t capture_raw, void *context)
{
    dewrapper_t dewrapper_handle = (dewrapper_t)context;
    dewrapper_context_t *dewrapper = dewrapper_t_get_context(dewrapper_handle);
    k4a_capture_t capture = NULL;

    if (K4A_SUCCEEDED(cb_result))
    {
        queue_push(dewrapper->queue, capture_raw);
    }
    else
    {
        LOG_WARNING("A streaming depth transfer failed", 0);
        dewrapper->capture_ready_cb(cb_result, capture, dewrapper->capture_ready_cb_context);
        queue_stop(dewrapper->queue);
    }
}

k4a_result_t dewrapper_start(dewrapper_t dewrapper_handle,
                             const k4a_device_configuration_t *config,
                             uint8_t *calibration_memory,
                             size_t calibration_memory_size)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, calibration_memory == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, calibration_memory_size == 0);
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, dewrapper_t, dewrapper_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);
    dewrapper_context_t *dewrapper = dewrapper_t_get_context(dewrapper_handle);

    dewrapper->calibration_memory = calibration_memory;
    dewrapper->calibration_memory_size = calibration_memory_size;
    dewrapper->thread_start_result = K4A_RESULT_FAILED;

    k4a_result_t result = K4A_RESULT_FROM_BOOL(dewrapper->thread == NULL);

    if (K4A_SUCCEEDED(result))
    {
        bool locked = false;
        queue_enable(dewrapper->queue);

        // NOTE: do not copy config ptr, it may be freed after this call
        dewrapper->fps = config->camera_fps;
        dewrapper->depth_mode = config->depth_mode;
        dewrapper->thread_stop = false;
        dewrapper->thread_started = false;

        THREADAPI_RESULT tresult = ThreadAPI_Create(&dewrapper->thread, depth_engine_thread, dewrapper);
        result = K4A_RESULT_FROM_BOOL(tresult == THREADAPI_OK);

        if (K4A_SUCCEEDED(result))
        {
            Lock(dewrapper->lock);
            locked = true;
            if (!dewrapper->thread_started)
            {
                int infinite_timeout = 0;
                COND_RESULT cond_result = Condition_Wait(dewrapper->condition, dewrapper->lock, infinite_timeout);
                result = K4A_RESULT_FROM_BOOL(cond_result == COND_OK);
            }
        }

        if (K4A_SUCCEEDED(result) && K4A_FAILED(dewrapper->thread_start_result))
        {
            LOG_ERROR("Depth Engine thread failed to start", 0);
            result = dewrapper->thread_start_result;
        }

        if (locked)
        {
            Unlock(dewrapper->lock);
            locked = false;
        }
    }

    if (K4A_FAILED(result))
    {
        dewrapper_stop(dewrapper_handle);
    }
    return result;
}

void dewrapper_stop(dewrapper_t dewrapper_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, dewrapper_t, dewrapper_handle);
    dewrapper_context_t *dewrapper = dewrapper_t_get_context(dewrapper_handle);

    dewrapper->thread_stop = true;
    queue_disable(dewrapper->queue);

    Lock(dewrapper->lock);
    THREAD_HANDLE thread = dewrapper->thread;
    dewrapper->thread = NULL;
    Unlock(dewrapper->lock);

    if (thread)
    {
        int thread_result; // We ignore this result, errors are reported to user via get_capture call.
        THREADAPI_RESULT tresult = ThreadAPI_Join(thread, &thread_result);
        (void)K4A_RESULT_FROM_BOOL(tresult == THREADAPI_OK); // Trace the issue, but we don't return a failure

        dewrapper->fps = (k4a_fps_t)-1;
        dewrapper->depth_mode = K4A_DEPTH_MODE_OFF;
    }

    queue_disable(dewrapper->queue);
}
