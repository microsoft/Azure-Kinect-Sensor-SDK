// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library Transform Engine Wrapper
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
    LOCK_HANDLE lock;
    COND_HANDLE condition;
    // TODO: Check
    LOCK_HANDLE processingLock;
    COND_HANDLE processingCondition;
    volatile bool thread_stop;
    k4a_result_t thread_start_result;
    k4a_result_t processing_result;

    k4a_transform_engine_type_t type;
    void *depth_image_data;
    size_t depth_image_size;
    void *color_image_data;
    size_t color_image_size;
    void *transformed_image_data;
    size_t transformed_image_size;

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
    Lock(tewrapper->lock);
    tewrapper->thread_start_result = result;
    Condition_Post(tewrapper->condition);
    Unlock(tewrapper->lock);

    while (result != K4A_RESULT_FAILED && tewrapper->thread_stop == false)
    {
        Lock(tewrapper->processingLock);
        int infinite_timeout = 0;
        COND_RESULT cond_result = Condition_Wait(tewrapper->processingCondition,
                                                 tewrapper->processingLock,
                                                 infinite_timeout);
        result = K4A_RESULT_FROM_BOOL(cond_result == COND_OK);
        if (tewrapper->thread_stop)
            break;

        if (K4A_SUCCEEDED(result))
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
        if (K4A_SUCCEEDED(result))
        {
            k4a_depth_engine_result_code_t teresult =
                deloader_transform_engine_process_frame(tewrapper->transform_engine,
                                                        tewrapper->type,
                                                        tewrapper->depth_image_data,
                                                        tewrapper->depth_image_size,
                                                        tewrapper->color_image_data,
                                                        tewrapper->color_image_size,
                                                        tewrapper->transformed_image_data,
                                                        tewrapper->transformed_image_size);
            if (teresult != K4A_DEPTH_ENGINE_RESULT_SUCCEEDED)
            {
                LOG_ERROR("Transform engine process frame failed with error code: %d.", teresult);
                result = K4A_RESULT_FAILED;
            }
        }
        Unlock(tewrapper->processingLock);

        // Notify the main thread that this thread completed a frame processing
        Lock(tewrapper->lock);
        tewrapper->processing_result = result;
        Condition_Post(tewrapper->condition);
        Unlock(tewrapper->lock);
    }
    transform_engine_stop_helper(tewrapper);

    return (int)result;
}

k4a_result_t tewrapper_process(tewrapper_t tewrapper_handle,
                               k4a_transform_engine_type_t type,
                               const void *depth_frame,
                               size_t depth_frame_size,
                               const void *color_frame,
                               size_t color_frame_size,
                               void *output_frame,
                               size_t output_frame_size)
{
    tewrapper_context_t *tewrapper = tewrapper_t_get_context(tewrapper_handle);

    Lock(tewrapper->processingLock);
    tewrapper->type = type;
    tewrapper->depth_image_data = (void *)depth_frame;
    tewrapper->depth_image_size = depth_frame_size;
    tewrapper->color_image_data = (void *)color_frame;
    tewrapper->color_image_size = color_frame_size;
    tewrapper->transformed_image_data = output_frame;
    tewrapper->transformed_image_size = output_frame_size;
    Condition_Post(tewrapper->processingCondition);
    Unlock(tewrapper->processingLock);

    // Waiting the transform engine thread to finish processing
    Lock(tewrapper->lock);
    int infinite_timeout = 0;
    COND_RESULT cond_result = Condition_Wait(tewrapper->condition, tewrapper->lock, infinite_timeout);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(cond_result == COND_OK);

    if (K4A_SUCCEEDED(result) && K4A_FAILED(tewrapper->processing_result))
    {
        LOG_ERROR("Transform Engine thread failed to process", 0);
        result = tewrapper->processing_result;
    }
    Unlock(tewrapper->lock);

    return result;
}

tewrapper_t tewrapper_create(k4a_transform_engine_calibration_t *transform_engine_calibration)
{
    RETURN_VALUE_IF_ARG(NULL, transform_engine_calibration == NULL);

    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    tewrapper_t tewrapper_handle;
    tewrapper_context_t *tewrapper = tewrapper_t_create(&tewrapper_handle);

    tewrapper->transform_engine_calibration = transform_engine_calibration;
    tewrapper->thread_start_result = K4A_RESULT_FAILED;
    if (K4A_SUCCEEDED(result))
    {
        tewrapper->lock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(tewrapper->lock != NULL);

        tewrapper->processingLock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(tewrapper->processingLock != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        tewrapper->condition = Condition_Init();
        tewrapper->processingCondition = Condition_Init();
    }

    if (K4A_FAILED(result))
    {
        tewrapper_t_destroy(tewrapper_handle);
        tewrapper_handle = NULL;
    }

    return tewrapper_handle;
}

void tewrapper_destroy(tewrapper_t tewrapper_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, tewrapper_t, tewrapper_handle);
    tewrapper_context_t *tewrapper = tewrapper_t_get_context(tewrapper_handle);

    tewrapper_stop(tewrapper_handle);

    if (tewrapper->condition)
    {
        Condition_Deinit(tewrapper->condition);
    }

    if (tewrapper->lock)
    {
        Lock_Deinit(tewrapper->lock);
    }

    if (tewrapper->processingCondition)
    {
        Condition_Deinit(tewrapper->processingCondition);
    }

    if (tewrapper->processingLock)
    {
        Lock_Deinit(tewrapper->processingLock);
    }

    tewrapper_t_destroy(tewrapper_handle);
}

k4a_result_t tewrapper_start(tewrapper_t tewrapper_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, tewrapper_t, tewrapper_handle);
    tewrapper_context_t *tewrapper = tewrapper_t_get_context(tewrapper_handle);

    tewrapper->thread_start_result = K4A_RESULT_FAILED;

    k4a_result_t result = K4A_RESULT_FROM_BOOL(tewrapper->thread == NULL);

    if (K4A_SUCCEEDED(result))
    {
        bool locked = false;
        tewrapper->thread_stop = false;

        THREADAPI_RESULT tresult = ThreadAPI_Create(&tewrapper->thread, transform_engine_thread, tewrapper);
        result = K4A_RESULT_FROM_BOOL(tresult == THREADAPI_OK);

        if (K4A_SUCCEEDED(result))
        {
            Lock(tewrapper->lock);
            locked = true;
            int infinite_timeout = 0;
            COND_RESULT cond_result = Condition_Wait(tewrapper->condition, tewrapper->lock, infinite_timeout);
            result = K4A_RESULT_FROM_BOOL(cond_result == COND_OK);
        }

        if (K4A_SUCCEEDED(result) && K4A_FAILED(tewrapper->thread_start_result))
        {
            LOG_ERROR("Transform Engine thread failed to start", 0);
            result = tewrapper->thread_start_result;
        }

        if (locked)
        {
            Unlock(tewrapper->lock);
            locked = false;
        }
    }

    if (K4A_FAILED(result))
    {
        tewrapper_stop(tewrapper_handle);
    }
    return result;
}

void tewrapper_stop(tewrapper_t tewrapper_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, tewrapper_t, tewrapper_handle);
    tewrapper_context_t *tewrapper = tewrapper_t_get_context(tewrapper_handle);

    // TODO: Check
    tewrapper->thread_stop = true;
    Lock(tewrapper->processingLock);
    Condition_Post(tewrapper->processingCondition);
    Unlock(tewrapper->processingLock);

    Lock(tewrapper->lock);
    THREAD_HANDLE thread = tewrapper->thread;
    tewrapper->thread = NULL;
    Unlock(tewrapper->lock);

    if (thread)
    {
        int thread_result;
        THREADAPI_RESULT tresult = ThreadAPI_Join(thread, &thread_result);
        (void)K4A_RESULT_FROM_BOOL(tresult == THREADAPI_OK); // Trace the issue, but we don't return a failure
    }
}
