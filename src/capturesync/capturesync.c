// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/capturesync.h>

// Dependent libraries
#include <k4ainternal/handle.h>
#include <k4ainternal/queue.h>
#include <k4ainternal/logging.h>
#include <k4ainternal/common.h>

#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/envvariable.h>

// System dependencies
#include <stdlib.h>
#include <stdbool.h>

typedef k4a_image_t(pfn_get_typed_image_t)(k4a_capture_t capture);
typedef struct _image_t
{
    pfn_get_typed_image_t *get_typed_image; // Accessor function to access the typed image
    bool color_capture;                     // type of the image and capture this struct represents; color / depth & ir
    queue_t queue;                          // The queue this data is stored in.

    k4a_capture_t capture; // Oldest capture received from the sensor
    k4a_image_t image;     // The image stored in the capture
    uint64_t ts;           // The Timestamp of the image
} frame_info_t;

typedef struct _capturesync_context_t
{
    queue_t sync_queue;    // Queue for storing synchronized captures in
    frame_info_t color;    // Oldest capture received from the color sensor
    frame_info_t depth_ir; // Timestamp in us of the oldest depth capture

    uint64_t fps_period;           // The slowest sample period in micro seconds
    uint64_t fps_1_quarter_period; // fps_period / 4
    bool sync_captures;            // enables depth and color captures to be synchronized.
    bool synchronized_images_only; // Only send captures to the user if they contain both color and depth images

    bool waiting_for_clean_depth_ts; // Flag to indicate the TS on depth captures has been reset.
    int depth_captures_dropped;      // Count of dropped captures waiting for waiting_for_clean_depth_ts.

    bool disable_sync;      // Disables synchronizing depth and color captures. Instead releases them as they arrive.
    bool enable_ts_logging; // Write capture timestamps and type to the logger to analysis
    int32_t depth_delay_off_color_usec; // Timing between color and depth image timestamps
    volatile bool running;              // We have received start and should be processing data when true.
    LOCK_HANDLE lock;

} capturesync_context_t;

K4A_DECLARE_CONTEXT(capturesync_t, capturesync_context_t);

#define TS_SUBTRACT(TS, val) ((TS <= val) ? 0 : TS - val)
#define TS_ADD(TS, val) ((TS + val <= 0) ? 0 : TS + val)

#define DEPTH_CAPTURE (false)
#define COLOR_CAPTURE (true)

/**
 * This function is responsible for updating the information in either capturesync_context_t->depth_ir or in
 * capturesync_context_t->color. capturesync_context_t holds the capture, image, and ts for the sample we are currenly
 * tring to synchronize.
 *
 * This function is called when we want to refresh the data  depth_ir or color of frame_info_t. We do this by releasing
 * holds onthe memory we currently reference, and then poping a new value off the queue.
 */
static void
drop_sample(capturesync_context_t *sync, k4a_wait_result_t *wresult, bool color_capture, bool drop_into_queue)
{

    frame_info_t *frame_info = &sync->depth_ir;

    if (color_capture)
    {
        frame_info = &sync->color;
    }

    if (drop_into_queue)
    {
        // Log the capture being dropped on the floor
        LOG_INFO("capturesync_drop, Dropping sample TS:%10lld type:%s",
                 frame_info->ts,
                 color_capture ? "Color" : "Depth");

        // If drop_into_queue is provided, then that caller wants the capture to placed into the provided queue, if no
        // drop_into_queue is provided, then it is dropped on the floor
        if (!sync->synchronized_images_only)
        {
            queue_push(sync->sync_queue, frame_info->capture);
        }
    }

    capture_dec_ref(frame_info->capture);
    frame_info->capture = NULL;

    image_dec_ref(frame_info->image);
    frame_info->image = NULL;

    if (*wresult != K4A_WAIT_RESULT_FAILED)
    {
        *wresult = queue_pop(frame_info->queue, 0, &frame_info->capture);
        if (*wresult == K4A_WAIT_RESULT_SUCCEEDED)
        {
            frame_info->image = frame_info->get_typed_image(frame_info->capture);
            *wresult = K4A_WAIT_RESULT_SUCCEEDED;
            if (frame_info->image == NULL)
            {
                (void)K4A_RESULT_FROM_BOOL(frame_info->image == NULL);
                *wresult = K4A_WAIT_RESULT_FAILED;
            }
        }
        if (*wresult == K4A_WAIT_RESULT_SUCCEEDED)
        {
            frame_info->ts = image_get_device_timestamp_usec(frame_info->image);
        }
    }

    if (*wresult != K4A_WAIT_RESULT_SUCCEEDED)
    {
        if (frame_info->capture)
        {
            capture_dec_ref(frame_info->capture);
        }
        if (frame_info->image)
        {
            image_dec_ref(frame_info->image);
        }
        frame_info->capture = NULL;
        frame_info->image = NULL;
        frame_info->ts = 0;
    }
}

static void replace_sample(capturesync_context_t *sync, k4a_capture_t capture_new, frame_info_t *frame_info)
{
    // Log the capture being dropped
    LOG_ERROR("capturesync_drop, releasing capture early due to full queue TS:%10lld type:%s",
              frame_info->ts,
              frame_info->color_capture ? "Color" : "Depth");

    if (!sync->synchronized_images_only)
    {
        queue_push(sync->sync_queue, frame_info->capture);
    }
    capture_dec_ref(frame_info->capture);
    image_dec_ref(frame_info->image);

    if (frame_info->capture)
    {
        frame_info->capture = capture_new;
        frame_info->image = frame_info->get_typed_image(capture_new);
        frame_info->ts = image_get_device_timestamp_usec(frame_info->image);
    }
    else
    {
        frame_info->capture = NULL;
        frame_info->image = NULL;
        frame_info->ts = 0;
    }
}

/* Callers of this function still ned to call capture_dec_ref on depth and color captures when they are done using the
 * captures */
static k4a_capture_t merge_captures(k4a_capture_t depth, k4a_capture_t color)
{
    // We merge color into depth because its slightly more efficient to link 1 color image than it is for a depth and IR
    // images.
    k4a_image_t image = capture_get_color_image(color);
    (void)K4A_RESULT_FROM_BOOL(image != NULL);
    capture_set_color_image(depth, image);
    image_dec_ref(image);
    return depth;
}

void capturesync_add_capture(capturesync_t capturesync_handle,
                             k4a_result_t capture_result,
                             k4a_capture_t capture_raw,
                             bool color_capture)
{
    capturesync_context_t *sync = NULL;
    k4a_wait_result_t wresult = K4A_WAIT_RESULT_SUCCEEDED;
    k4a_result_t result;
    bool locked = false;
    uint64_t ts_raw_capture = 0;

    result = K4A_RESULT_FROM_BOOL(capturesync_handle != NULL);
    if (K4A_SUCCEEDED(result))
    {
        sync = capturesync_t_get_context(capturesync_handle);
        result = K4A_RESULT_FROM_BOOL(sync != NULL);
    }

    if (K4A_FAILED(capture_result))
    {
        if (sync->running)
        {
            LOG_WARNING("Capture Error Detected, %s", color_capture ? "Color " : "Depth ");
        }
        // Stop queues
        queue_stop(sync->sync_queue);
        queue_stop(sync->depth_ir.queue);
        queue_stop(sync->color.queue);

        // Reflect the low level error in the current result
        result = capture_result;
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(capture_raw != NULL);
    }

    // Read the timestamp of the raw sample
    if (K4A_SUCCEEDED(result))
    {
        k4a_image_t image = NULL;
        if (color_capture)
        {
            image = capture_get_color_image(capture_raw);
        }
        else
        {
            image = capture_get_ir_image(capture_raw);
        }
        result = K4A_RESULT_FROM_BOOL(image != NULL);
        if (K4A_SUCCEEDED(result))
        {
            ts_raw_capture = image_get_device_timestamp_usec(image);
            image_dec_ref(image);
            image = NULL;
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        Lock(sync->lock);
        locked = true;
        result = sync->running == true ? K4A_RESULT_SUCCEEDED : K4A_RESULT_FAILED;
    }

    if (K4A_SUCCEEDED(result))
    {
        if (sync->enable_ts_logging)
        {
            LOG_INFO("capturesync_ts, Arriving capture, TS:%10lld, %s, Color TS:%10lld, Depth TS:%10lld",
                     ts_raw_capture,
                     color_capture ? "Color " : "Depth ",
                     sync->color.ts,
                     sync->depth_ir.ts);
        }

        if (sync->sync_captures == false || sync->disable_sync == true)
        {
            // we are not synchronizing samples, just copy to the queue
            queue_push(sync->sync_queue, capture_raw);
            result = K4A_RESULT_FAILED; // Not an error, just a graceful exit
        }
        else if (!color_capture && sync->waiting_for_clean_depth_ts)
        {
            // Timestamps at the start of streaming are tricky, they will get reset to zero when the color camera is
            // started. This code protects against the depth timestamps from being reported before the reset happens.
            if (ts_raw_capture / sync->fps_period > 10)
            {
                sync->depth_captures_dropped++;
                result = K4A_RESULT_FAILED; // Not an error, just a graceful exit
            }
            else
            {
                // Once we get a good TS we are going to always get a good TS
                sync->waiting_for_clean_depth_ts = false;
                if (sync->depth_captures_dropped)
                {
                    LOG_INFO("Dropped %d depth captures waiting for time stamps to stabilize",
                             sync->depth_captures_dropped);
                }
            }
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        frame_info_t *frame_info = &sync->depth_ir;
        if (color_capture)
        {
            frame_info = &sync->color;
        }

        if (frame_info->capture == NULL)
        {
            assert(frame_info->image == 0); // Both capture and image should be NULL
            frame_info->image = frame_info->get_typed_image(capture_raw);
            frame_info->ts = image_get_device_timestamp_usec(frame_info->image);
            frame_info->capture = capture_raw;
            capture_inc_ref(capture_raw);
            capture_raw = NULL;
        }
        else
        {
            // Store the capture in our internal queue
            k4a_capture_t dropped_sample = NULL;
            queue_push_w_dropped(frame_info->queue, capture_raw, &dropped_sample);
            if (dropped_sample)
            {
                // If the internal queue is full, then we publish the oldest frame as we can no longer store it.
                // The user will interpret this a capture that is either depth or color, but not both
                replace_sample(sync, dropped_sample, frame_info);
            }
        }

        while (wresult == K4A_WAIT_RESULT_SUCCEEDED && sync->depth_ir.capture != NULL && sync->color.capture != NULL)
        {
            if (sync->depth_delay_off_color_usec < 0)
            {
                // We are using the depth capture TS as the window to find its matching color capture. Its TS
                // won't be perfectly aligned so we give it a slush margine of 25% of the FPS
                uint64_t begin_sync_window = TS_SUBTRACT(sync->depth_ir.ts, sync->fps_1_quarter_period);

                // Offset sync window by the time we have programmed the hardware, in this case
                // depth_delay_off_color_usec is negative so we subtract.
                begin_sync_window = (uint64_t)TS_SUBTRACT((int64_t)begin_sync_window, sync->depth_delay_off_color_usec);

                uint64_t end_sync_window = begin_sync_window + sync->fps_period;
                if (sync->color.ts > end_sync_window)
                {
                    // Drop depth_cap because color is beyond 1 period away
                    drop_sample(sync, &wresult, DEPTH_CAPTURE, true);
                    continue;
                }
                else if (sync->color.ts < begin_sync_window)
                {
                    // Drop color sample because it happened before this frame window
                    drop_sample(sync, &wresult, COLOR_CAPTURE, true);
                    continue;
                }
            }
            else
            {
                // We are using the color capture TS as the window to find its matching depth capture. Its TS
                // won't be perfectly aligned so we give it a slush margine of 25% of the FPS
                uint64_t begin_sync_window = TS_SUBTRACT(sync->color.ts, sync->fps_1_quarter_period);

                // Offset sync window by the time we have programmed the hardware, in this case
                // depth_delay_off_color_usec is positive.
                begin_sync_window = (uint64_t)TS_ADD((int64_t)begin_sync_window, sync->depth_delay_off_color_usec);

                uint64_t end_sync_window = begin_sync_window + sync->fps_period;
                if (sync->depth_ir.ts > end_sync_window)
                {
                    // Drop color_cap because depth is beyond 1 period away
                    drop_sample(sync, &wresult, COLOR_CAPTURE, true);
                    continue;
                }
                else if (sync->depth_ir.ts < begin_sync_window)
                {
                    // Drop depth sample because it happened before this frame window
                    drop_sample(sync, &wresult, DEPTH_CAPTURE, true);
                    continue;
                }
            }

            if (sync->color.capture != NULL && sync->depth_ir.capture != NULL)
            {
                if (sync->enable_ts_logging)
                {
                    LOG_INFO("capturesync_link,TS_Color, %10lld, TS_Depth, %10lld,", sync->color.ts, sync->depth_ir.ts);
                }

                k4a_capture_t merged = merge_captures(sync->depth_ir.capture, sync->color.capture);
                queue_push(sync->sync_queue, merged);
                merged = NULL; // No need to call capture_dec_ref() here.

                // Use drop symantic to get another sample from the queue if present. Synchronized sample is
                // already in output queue and has its own ref
                drop_sample(sync, &wresult, COLOR_CAPTURE, false);
                drop_sample(sync, &wresult, DEPTH_CAPTURE, false);
                continue;
            }

            if (wresult == K4A_WAIT_RESULT_FAILED)
            {
                sync->running = false;
                LOG_ERROR("CaptureSync, error encountered access queue", 0);
            }
        }
    }

    if (locked)
    {
        Unlock(sync->lock);
        locked = false;
    }
}

k4a_result_t capturesync_create(capturesync_t *capturesync_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, capturesync_handle == NULL);

    capturesync_context_t *sync = capturesync_t_create(capturesync_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(sync != NULL);

    sync->color.color_capture = true;
    sync->color.get_typed_image = capture_get_color_image;

    // In this module we can either use depth or color, we are only after the timestamp which is the same on both.
    sync->depth_ir.color_capture = false;
    sync->depth_ir.get_typed_image = capture_get_ir_image;

    if (K4A_SUCCEEDED(result))
    {
        sync->lock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(sync->lock != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(queue_create(QUEUE_DEFAULT_SIZE, "Queue_depth", &sync->depth_ir.queue));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(queue_create(QUEUE_DEFAULT_SIZE, "Queue_color", &sync->color.queue));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(queue_create(QUEUE_DEFAULT_SIZE / 2, "Queue_capture", &sync->sync_queue));
    }

    if (K4A_SUCCEEDED(result))
    {
        queue_disable(sync->color.queue);
        queue_disable(sync->depth_ir.queue);
        queue_disable(sync->sync_queue);
    }

    const char *disable_sync = environment_get_variable("K4A_DISABLE_SYNCHRONIZATION");
    if (disable_sync != NULL && disable_sync[0] != '\0' && disable_sync[0] != '0')
    {
        sync->disable_sync = true;
    }

    const char *enable_ts_logging = environment_get_variable("K4A_ENABLE_TS_LOGGING");
    if (enable_ts_logging != NULL && enable_ts_logging[0] != '\0' && enable_ts_logging[0] != '0')
    {
        sync->enable_ts_logging = true;
    }

    if (K4A_FAILED(result))
    {
        capturesync_destroy(*capturesync_handle);
        *capturesync_handle = NULL;
    }

    return result;
}

void capturesync_destroy(capturesync_t capturesync_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, capturesync_t, capturesync_handle);
    capturesync_context_t *sync = capturesync_t_get_context(capturesync_handle);

    capturesync_stop(capturesync_handle);

    if (sync->depth_ir.queue)
    {
        queue_destroy(sync->depth_ir.queue);
    }
    if (sync->color.queue)
    {
        queue_destroy(sync->color.queue);
    }
    if (sync->sync_queue)
    {
        queue_destroy(sync->sync_queue);
    }

    Lock_Deinit(sync->lock);
    capturesync_t_destroy(capturesync_handle);
}

k4a_result_t capturesync_start(capturesync_t capturesync_handle, const k4a_device_configuration_t *config)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, capturesync_t, capturesync_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);
    capturesync_context_t *sync = capturesync_t_get_context(capturesync_handle);

    // Reset frames to drop
    sync->waiting_for_clean_depth_ts = true;
    sync->synchronized_images_only = config->synchronized_images_only;

    sync->fps_period = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config->camera_fps));
    sync->fps_1_quarter_period = sync->fps_period / 4;
    sync->depth_delay_off_color_usec = config->depth_delay_off_color_usec;
    sync->sync_captures = true;
    sync->depth_captures_dropped = 0;

    if (config->color_resolution == K4A_COLOR_RESOLUTION_OFF || config->depth_mode == K4A_DEPTH_MODE_OFF)
    {
        // Only 1 sensor is running, disable synchronization
        sync->sync_captures = false;
    }

    queue_enable(sync->color.queue);
    queue_enable(sync->depth_ir.queue);
    queue_enable(sync->sync_queue);

    // Not taking the lock as we don't need to synchronize this on start
    sync->running = true;

    return K4A_RESULT_SUCCEEDED;
}

void capturesync_stop(capturesync_t capturesync_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, capturesync_t, capturesync_handle);
    capturesync_context_t *sync = capturesync_t_get_context(capturesync_handle);

    Lock(sync->lock);
    sync->running = false;

    if (sync->color.queue)
    {
        queue_disable(sync->color.queue);
    }
    if (sync->depth_ir.queue)
    {
        queue_disable(sync->depth_ir.queue);
    }
    if (sync->sync_queue)
    {
        queue_disable(sync->sync_queue);
    }

    if (sync->color.capture)
    {
        capture_dec_ref(sync->color.capture);
        sync->color.capture = NULL;
        sync->color.ts = UINT64_MAX;
    }

    if (sync->color.image)
    {
        image_dec_ref(sync->color.image);
        sync->color.image = NULL;
    }

    if (sync->depth_ir.capture)
    {
        capture_dec_ref(sync->depth_ir.capture);
        sync->depth_ir.capture = NULL;
        sync->depth_ir.ts = UINT64_MAX;
    }

    if (sync->depth_ir.image)
    {
        image_dec_ref(sync->depth_ir.image);
        sync->depth_ir.image = NULL;
    }
    Unlock(sync->lock);
}

k4a_wait_result_t capturesync_get_capture(capturesync_t capturesync_handle,
                                          k4a_capture_t *capture,
                                          int32_t timeout_in_ms)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_WAIT_RESULT_FAILED, capturesync_t, capturesync_handle);
    RETURN_VALUE_IF_ARG(K4A_WAIT_RESULT_FAILED, capture == NULL);

    capturesync_context_t *sync = capturesync_t_get_context(capturesync_handle);
    k4a_capture_t capture_handle;

    k4a_wait_result_t wresult = queue_pop(sync->sync_queue, timeout_in_ms, &capture_handle);
    if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
    {
        *capture = capture_handle;
    }
    return wresult;
}
