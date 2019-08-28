// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/queue.h>

// Dependent libraries
#include <k4ainternal/allocator.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/condition.h>
#include <azure_c_shared_utility/threadapi.h>

// System dependencies
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct _queue_entry_t
{
    k4a_capture_t capture;
} queue_entry_t;

typedef struct _queue_context_t
{
    bool enabled;
    bool stopped;
    uint32_t queue_pop_blocked; // number of waiting threads for queue_pop so complete
    uint32_t read_location;     // current location to read frokm
    uint32_t write_location;    // current location to write to
    queue_entry_t *queue;       // the queue array
    uint32_t depth;             // 1 element larger than the max elements the queue can hold.
    const char *name;           // Queue name in logger
    uint32_t dropped_count;     // Count of the dropped captures

    LOCK_HANDLE lock;
    COND_HANDLE condition;
} queue_context_t;

K4A_DECLARE_CONTEXT(queue_t, queue_context_t);

// This queue is empty if read and write pointers index the same location. The queue is full when the write pointer is
// one away from pointing at the read location. This means that while we allocate a size of N, we can only hold N-1
// elements. This allows us to maintain state through the read and write pointers only.
#define inc_read_write_location(queue, location) (((location) + 1) % (queue)->depth)
#define is_queue_empty(queue) ((queue)->write_location == (queue)->read_location)
#define is_queue_full(queue) (inc_read_write_location((queue), (queue)->write_location) == (queue)->read_location)

k4a_result_t queue_create(uint32_t queue_depth, const char *queue_name, queue_t *queue_handle)
{
    k4a_result_t result;
    queue_context_t *queue = queue_t_create(queue_handle);

    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, queue_depth == 0);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, queue_depth > 10000); // Sanity Check
    queue->depth = queue_depth + 1;                              // Adding one; see comment on inc_read_write_location()
    queue->name = queue_name;
    if (queue->name == NULL)
    {
        queue->name = "Unknown queue";
    }

    queue->queue = malloc(sizeof(queue_entry_t) * queue->depth);
    result = K4A_RESULT_FROM_BOOL(queue->queue != NULL);

    if (K4A_SUCCEEDED(result))
    {
        queue->lock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(queue->lock != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        queue->condition = Condition_Init();
    }
    else
    {
        if (queue)
        {
            queue_destroy(*queue_handle);
            *queue_handle = NULL;
        }
    }
    return result;
}

static k4a_capture_t queue_pop_internal_locked(queue_context_t *queue)
{
    if (is_queue_empty(queue) == false)
    {
        queue_entry_t *entry = &queue->queue[queue->read_location];

        queue->read_location = inc_read_write_location(queue, queue->read_location);

        return entry->capture;
    }
    return NULL;
}

k4a_wait_result_t queue_pop(queue_t queue_handle, int32_t wait_in_ms, k4a_capture_t *out_capture)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_WAIT_RESULT_FAILED, queue_t, queue_handle);
    RETURN_VALUE_IF_ARG(K4A_WAIT_RESULT_FAILED, out_capture == NULL);

    queue_context_t *queue = queue_t_get_context(queue_handle);
    k4a_capture_t capture = NULL;
    k4a_wait_result_t wresult = K4A_WAIT_RESULT_SUCCEEDED;

    Lock(queue->lock);

    if (queue->enabled != true)
    {
        LOG_ERROR("Queue \"%s\" was popped in a disabled state.", queue->name);
        wresult = K4A_WAIT_RESULT_FAILED;
    }

    if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
    {
        wresult = K4A_WAIT_RESULT_TIMEOUT;
        queue->queue_pop_blocked++;

        capture = queue_pop_internal_locked(queue);
        if (capture != NULL)
        {
            wresult = K4A_WAIT_RESULT_SUCCEEDED;
        }
        else if (wait_in_ms != 0)
        {
            // Anything less than 0 is a wait forever condition in the lower level calls.
            // K4A_WAIT_INFINITE (-1) is defined for the user for this purpose
            if (wait_in_ms < 0)
            {
                wait_in_ms = 0; // infinite to Condition_wait
            }

            // NOTE: The event used by Condition_Wait only gets set when a thread is actively
            // waiting. There is a bit of a race condition between Condition_Post, Condition_Wait
            // and how waiting_thread_count is used. So the queue may have data in it with no event set.
            COND_RESULT cond_result = Condition_Wait(queue->condition, queue->lock, wait_in_ms);
            if (cond_result == COND_OK)
            {
                capture = queue_pop_internal_locked(queue);
                wresult = K4A_WAIT_RESULT_SUCCEEDED;

                // Condition_Wait should only return COND_OK if there is data or if we are shutting down.
                assert(capture != NULL || queue->enabled == false);
            }
            else if (cond_result != COND_TIMEOUT)
            {
                K4A_RESULT_FROM_BOOL(cond_result != COND_ERROR);
                assert(cond_result == COND_TIMEOUT || cond_result == COND_OK);
                wresult = K4A_WAIT_RESULT_FAILED;
            }
        }
        queue->queue_pop_blocked--;
    }

    if (queue->enabled == false)
    {
        wresult = K4A_WAIT_RESULT_FAILED;
        if (capture)
        {
            // drop the capture
            capture_dec_ref(capture);
            capture = NULL;
        }
    }

    if (queue->dropped_count != 0)
    {
        LOG_INFO("Queue \"%s\" dropped oldest %d captures from queue.", queue->name, queue->dropped_count);
        queue->dropped_count = 0;
    }

    Unlock(queue->lock);

    // We are transfering the ref we had to the caller.
    // capture_dec_ref(capture);
    *out_capture = capture;

    return wresult;
}

static void queue_push_internal_locked(queue_context_t *queue, k4a_capture_t capture)
{
    queue_entry_t *entry = &queue->queue[queue->write_location];
    entry->capture = capture;

    queue->write_location = inc_read_write_location(queue, queue->write_location);
}

void queue_push_w_dropped(queue_t queue_handle, k4a_capture_t capture, k4a_capture_t *dropped)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, queue_t, queue_handle);
    RETURN_VALUE_IF_ARG(VOID_VALUE, capture == NULL);

    queue_context_t *queue = queue_t_get_context(queue_handle);

    Lock(queue->lock);

    if (queue->enabled == false)
    {
        LOG_WARNING("Capture pushed into disabled queue.", queue->name);
    }
    else
    {
        if (is_queue_full(queue))
        {
            if (dropped == NULL)
            {
                queue->dropped_count++;
                capture_dec_ref(queue_pop_internal_locked(queue));
            }
            else
            {
                *dropped = queue_pop_internal_locked(queue);
            }
        }

        // We are accepting this into our queue, so add a ref to prevent it
        // from being freed
        capture_inc_ref(capture);

        queue_push_internal_locked(queue, capture);

        Condition_Post(queue->condition);
    }
    Unlock(queue->lock);
}

void queue_push(queue_t queue_handle, k4a_capture_t capture)
{
    queue_push_w_dropped(queue_handle, capture, NULL);
}

void queue_destroy(queue_t queue_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, queue_t, queue_handle);
    queue_context_t *queue = queue_t_get_context(queue_handle);

    queue_disable(queue_handle);

    if (queue->condition)
    {
        Condition_Deinit(queue->condition);
    }

    if (queue->queue)
    {
        free(queue->queue);
    }

    Lock_Deinit(queue->lock);

    queue_t_destroy(queue_handle);
}

void queue_enable(queue_t queue_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, queue_t, queue_handle);
    queue_context_t *queue = queue_t_get_context(queue_handle);
    Lock(queue->lock);
    queue->enabled = true;
    queue->stopped = false;
    Unlock(queue->lock);
}

void queue_disable(queue_t queue_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, queue_t, queue_handle);
    queue_context_t *queue = queue_t_get_context(queue_handle);

    Lock(queue->lock);

    queue->enabled = false;

    while (queue->queue_pop_blocked != 0)
    {
        LOG_INFO("Queue \"%s\" waiting for blocking call to complete.", queue->name);
        Condition_Post(queue->condition);
        Unlock(queue->lock);
        ThreadAPI_Sleep(25);
        Lock(queue->lock);
    }

    while (is_queue_empty(queue) == false)
    {
        capture_dec_ref(queue_pop_internal_locked(queue));
    }
    Unlock(queue->lock);
}

void queue_stop(queue_t queue_handle)
{
    queue_context_t *queue = queue_t_get_context(queue_handle);

    Lock(queue->lock);
    queue->stopped = true;
    Unlock(queue->lock);

    LOG_INFO("Queue \"%s\" stopped, shutting down and notifying consumers.", queue->name);
    queue_disable(queue_handle);
}
