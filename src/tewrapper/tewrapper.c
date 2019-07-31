// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library is Transform Engine Wrapper
#include <k4ainternal/tewrapper.h>

// Dependent libraries
#include <k4ainternal/deloader.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/condition.h>
#include <azure_c_shared_utility/lock.h>

// System dependencies
#include <stdlib.h>
#include <stdbool.h>

typedef struct _tewrapper_context_t
{
    k4a_transform_engine_calibration_t *transform_engine_calibration; // Copy of transform engine calibration passed in
                                                                      // - we do not own this memory
    k4a_transform_engine_context_t *transform_engine;

    THREAD_HANDLE thread;
    LOCK_HANDLE main_lock;
    COND_HANDLE main_condition;
    LOCK_HANDLE worker_lock;
    COND_HANDLE worker_condition;
    volatile bool thread_started;
    volatile bool thread_stop;
    volatile bool worker_locked;
    k4a_result_t thread_start_result;
    k4a_result_t thread_processing_result;

    k4a_transform_engine_type_t type;
    const void *depth_image_data;
    size_t depth_image_size;
    const void *image2_data;
    size_t image2_size;
    void *transformed_image_data;
    size_t transformed_image_size;
    void *transformed_image2_data;
    size_t transformed_image2_size;
    k4a_transform_engine_interpolation_t interpolation;
    uint32_t invalid_value;

} tewrapper_context_t;

K4A_DECLARE_CONTEXT(tewrapper_t, tewrapper_context_t);

static k4a_result_t transform_engine_start_helper(tewrapper_context_t *tewrapper)
{
    assert(tewrapper->transform_engine == NULL);

    k4a_depth_engine_result_code_t teresult =
        deloader_transform_engine_create_and_initialize(&tewrapper->transform_engine,
                                                        tewrapper->transform_engine_calibration,
                                                        NULL,  // Callback
                                                        NULL); // Callback Context
    if (teresult != K4A_DEPTH_ENGINE_RESULT_SUCCEEDED)
    {
        LOG_ERROR("Transform engine create and initialize failed with error code: %d.", teresult);
        if (teresult == K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_OPENGL_CONTEXT)
        {
            LOG_ERROR("OpenGL 4.4 context creation failed. You could try updating your graphics drivers.", 0);
        }
    }

    return K4A_RESULT_FROM_BOOL(teresult == K4A_DEPTH_ENGINE_RESULT_SUCCEEDED);
}

static void transform_engine_stop_helper(tewrapper_context_t *tewrapper)
{
    if (tewrapper->transform_engine != NULL)
    {
        deloader_transform_engine_destroy(&tewrapper->transform_engine);
        tewrapper->transform_engine = NULL;
    }
}

static int transform_engine_thread(void *param)
{
    tewrapper_context_t *tewrapper = (tewrapper_context_t *)param;

    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    result = TRACE_CALL(transform_engine_start_helper(tewrapper));

    // The Start routine is blocked waiting for this thread to complete startup, so we signal it here and share our
    // startup status.
    Lock(tewrapper->main_lock);
    tewrapper->thread_started = true;
    tewrapper->thread_start_result = result;
    tewrapper->worker_locked = false;
    Condition_Post(tewrapper->main_condition);
    Unlock(tewrapper->main_lock);

    while (result != K4A_RESULT_FAILED && tewrapper->thread_stop == false)
    {
        // Waiting the main thread to request processing a frame
        Lock(tewrapper->worker_lock);
        tewrapper->worker_locked = true;
        Condition_Post(tewrapper->main_condition);
        int infinite_timeout = 0;
        COND_RESULT cond_result = Condition_Wait(tewrapper->worker_condition, tewrapper->worker_lock, infinite_timeout);
        result = K4A_RESULT_FROM_BOOL(cond_result == COND_OK);

        if (tewrapper->thread_stop == false)
        {
            if (K4A_SUCCEEDED(result))
            {
                if (tewrapper->type == K4A_TRANSFORM_ENGINE_TYPE_DEPTH_TO_COLOR ||
                    tewrapper->type == K4A_TRANSFORM_ENGINE_TYPE_COLOR_TO_DEPTH)
                {
                    size_t transform_engine_output_buffer_size =
                        deloader_transform_engine_get_output_frame_size(tewrapper->transform_engine, tewrapper->type);
                    if (tewrapper->transformed_image_size != transform_engine_output_buffer_size)
                    {
                        LOG_ERROR("Transform engine output buffer size not expected. Expect: %d, Actual: %d.",
                                  transform_engine_output_buffer_size,
                                  tewrapper->transformed_image_size);
                        result = K4A_RESULT_FAILED;
                    }
                }
                else if (tewrapper->type == K4A_TRANSFORM_ENGINE_TYPE_DEPTH_CUSTOM8_TO_COLOR ||
                         tewrapper->type == K4A_TRANSFORM_ENGINE_TYPE_DEPTH_CUSTOM16_TO_COLOR)
                {
                    size_t transform_engine_output_buffer_size =
                        deloader_transform_engine_get_output_frame_size(tewrapper->transform_engine,
                                                                        K4A_TRANSFORM_ENGINE_TYPE_DEPTH_TO_COLOR);
                    if (tewrapper->transformed_image_size != transform_engine_output_buffer_size)
                    {
                        LOG_ERROR("Transform engine output buffer size not expected. Expect: %d, Actual: %d.",
                                  transform_engine_output_buffer_size,
                                  tewrapper->transformed_image_size);
                        result = K4A_RESULT_FAILED;
                    }

                    size_t transform_engine_output_buffer2_size =
                        deloader_transform_engine_get_output_frame_size(tewrapper->transform_engine, tewrapper->type);
                    if (tewrapper->transformed_image2_size != transform_engine_output_buffer2_size)
                    {
                        LOG_ERROR("Transform engine output buffer 2 size not expected. Expect: %d, Actual: %d.",
                                  transform_engine_output_buffer2_size,
                                  tewrapper->transformed_image2_size);
                        result = K4A_RESULT_FAILED;
                    }
                }
            }

            if (K4A_SUCCEEDED(result))
            {
                k4a_depth_engine_result_code_t teresult =
                    deloader_transform_engine_process_frame(tewrapper->transform_engine,
                                                            tewrapper->type,
                                                            tewrapper->depth_image_data,
                                                            tewrapper->depth_image_size,
                                                            tewrapper->image2_data,
                                                            tewrapper->image2_size,
                                                            tewrapper->transformed_image_data,
                                                            tewrapper->transformed_image_size,
                                                            tewrapper->transformed_image2_data,
                                                            tewrapper->transformed_image2_size,
                                                            tewrapper->interpolation,
                                                            tewrapper->invalid_value);
                if (teresult == K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_WAIT_PROCESSING_COMPLETE_FAILED ||
                    teresult == K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_GPU_TIMEOUT)
                {
                    LOG_ERROR("Timeout during depth engine process frame.", 0);
                    LOG_ERROR("SDK should be restarted since it looks like GPU has encountered an unrecoverable error.",
                              0);
                    result = K4A_RESULT_FAILED;
                }
                else if (teresult != K4A_DEPTH_ENGINE_RESULT_SUCCEEDED)
                {
                    LOG_ERROR("Transform engine process frame failed with error code: %d.", teresult);
                    result = K4A_RESULT_FAILED;
                }
            }
        }

        // Notify the API thread that transform engine thread completed a frame processing
        tewrapper->thread_processing_result = result;
        Condition_Post(tewrapper->main_condition);
        Unlock(tewrapper->worker_lock);
        tewrapper->worker_locked = false;
    }

    transform_engine_stop_helper(tewrapper);

    return (int)result;
}

k4a_result_t tewrapper_process_frame(tewrapper_t tewrapper_handle,
                                     k4a_transform_engine_type_t type,
                                     const void *depth_image_data,
                                     size_t depth_image_size,
                                     const void *image2_data,
                                     size_t image2_size,
                                     void *transformed_image_data,
                                     size_t transformed_image_size,
                                     void *transformed_image2_data,
                                     size_t transformed_image2_size,
                                     k4a_transform_engine_interpolation_t interpolation,
                                     uint32_t invalid_value)
{
    tewrapper_context_t *tewrapper = tewrapper_t_get_context(tewrapper_handle);

    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    Lock(tewrapper->main_lock);

    // Ensure the worker thread loop to acquire the worker lock before this function acquires the worker lock to avoid
    // dead lock
    if (!tewrapper->worker_locked)
    {
        int infinite_timeout = 0;
        COND_RESULT cond_result = Condition_Wait(tewrapper->main_condition, tewrapper->main_lock, infinite_timeout);
        result = K4A_RESULT_FROM_BOOL(cond_result == COND_OK);
    }

    if (K4A_SUCCEEDED(result))
    {
        // Notify the transform engine thread to process a frame
        Lock(tewrapper->worker_lock);
        tewrapper->type = type;
        tewrapper->depth_image_data = depth_image_data;
        tewrapper->depth_image_size = depth_image_size;
        tewrapper->image2_data = image2_data;
        tewrapper->image2_size = image2_size;
        tewrapper->transformed_image_data = transformed_image_data;
        tewrapper->transformed_image_size = transformed_image_size;
        tewrapper->transformed_image2_data = transformed_image2_data;
        tewrapper->transformed_image2_size = transformed_image2_size;
        tewrapper->interpolation = interpolation;
        tewrapper->invalid_value = invalid_value;
        Condition_Post(tewrapper->worker_condition);

        // Waiting the transform engine thread to finish processing
        int infinite_timeout = 0;
        COND_RESULT cond_result = Condition_Wait(tewrapper->main_condition, tewrapper->worker_lock, infinite_timeout);
        result = K4A_RESULT_FROM_BOOL(cond_result == COND_OK);

        if (K4A_SUCCEEDED(result) && K4A_FAILED(tewrapper->thread_processing_result))
        {
            LOG_ERROR("Transform Engine thread failed to process", 0);
            result = tewrapper->thread_processing_result;
        }

        Unlock(tewrapper->worker_lock);
    }

    Unlock(tewrapper->main_lock);

    return result;
}

tewrapper_t tewrapper_create(k4a_transform_engine_calibration_t *transform_engine_calibration)
{
    RETURN_VALUE_IF_ARG(NULL, transform_engine_calibration == NULL);

    tewrapper_t tewrapper_handle;
    tewrapper_context_t *tewrapper = tewrapper_t_create(&tewrapper_handle);

    tewrapper->transform_engine_calibration = transform_engine_calibration;
    tewrapper->thread_start_result = K4A_RESULT_FAILED;

    tewrapper->main_lock = Lock_Init();
    k4a_result_t result = K4A_RESULT_FROM_BOOL(tewrapper->main_lock != NULL);
    if (K4A_SUCCEEDED(result))
    {
        tewrapper->worker_lock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(tewrapper->worker_lock != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        tewrapper->main_condition = Condition_Init();
        result = K4A_RESULT_FROM_BOOL(tewrapper->main_condition != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        tewrapper->worker_condition = Condition_Init();
        result = K4A_RESULT_FROM_BOOL(tewrapper->worker_condition != NULL);
    }

    // Start transform engine thread
    if (K4A_SUCCEEDED(result))
    {
        bool locked = false;
        tewrapper->thread_stop = false;
        tewrapper->thread_started = false;

        THREADAPI_RESULT tresult = ThreadAPI_Create(&tewrapper->thread, transform_engine_thread, tewrapper);
        result = K4A_RESULT_FROM_BOOL(tresult == THREADAPI_OK);

        if (K4A_SUCCEEDED(result))
        {
            Lock(tewrapper->main_lock);
            locked = true;
            if (!tewrapper->thread_started)
            {
                int infinite_timeout = 0;
                COND_RESULT cond_result = Condition_Wait(tewrapper->main_condition,
                                                         tewrapper->main_lock,
                                                         infinite_timeout);
                result = K4A_RESULT_FROM_BOOL(cond_result == COND_OK);
            }
        }

        if (K4A_SUCCEEDED(result) && K4A_FAILED(tewrapper->thread_start_result))
        {
            LOG_ERROR("Transform Engine thread failed to start", 0);
            result = tewrapper->thread_start_result;
        }

        if (locked)
        {
            Unlock(tewrapper->main_lock);
            locked = false;
        }
    }

    if (K4A_FAILED(result))
    {
        tewrapper_destroy(tewrapper_handle);
        tewrapper_handle = NULL;
    }

    return tewrapper_handle;
}

void tewrapper_destroy(tewrapper_t tewrapper_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, tewrapper_t, tewrapper_handle);
    tewrapper_context_t *tewrapper = tewrapper_t_get_context(tewrapper_handle);

    // Notify the transform engine thread to stop
    Lock(tewrapper->worker_lock);
    tewrapper->thread_stop = true;
    Condition_Post(tewrapper->worker_condition);
    Unlock(tewrapper->worker_lock);

    Lock(tewrapper->main_lock);
    THREAD_HANDLE thread = tewrapper->thread;
    tewrapper->thread = NULL;
    Unlock(tewrapper->main_lock);

    if (thread)
    {
        int thread_result;
        THREADAPI_RESULT tresult = ThreadAPI_Join(thread, &thread_result);
        (void)K4A_RESULT_FROM_BOOL(tresult == THREADAPI_OK); // Trace the issue, but we don't return a failure
    }

    if (tewrapper->main_condition)
    {
        Condition_Deinit(tewrapper->main_condition);
    }

    if (tewrapper->worker_condition)
    {
        Condition_Deinit(tewrapper->worker_condition);
    }

    if (tewrapper->main_lock)
    {
        Lock_Deinit(tewrapper->main_lock);
    }

    if (tewrapper->worker_lock)
    {
        Lock_Deinit(tewrapper->worker_lock);
    }

    tewrapper_t_destroy(tewrapper_handle);
}
