// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

#include <gtest/gtest.h>

#include <k4ainternal/capturesync.h>
#include <k4ainternal/capture.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/condition.h>

// This wait is effectively an infinite wait, setting to 5 min will prevent the test from blocking indefinately in the
// event the test regresses.
#define WAIT_TEST_INFINITE (5 * 60 * 1000)

#define FPS_30_IN_US (1000000 / 30)

// Generates and FPS time based on the capture number passed in and then adds noise to the time based on percentage
// passed in.
#define FPS_30_US(captureNum, percent) (FPS_30_IN_US * captureNum) + (percent * FPS_30_IN_US / 100)
#define NO_CAPTURE 1

#define COLOR_FIRST 1
#define DEPTH_FIRST 0

#define COLOR_CAPTURE (true)
#define DEPTH_CAPTURE (false)

#define END_TEST_DATA                                                                                                  \
    {                                                                                                                  \
        UINT64_MAX, COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE                                                              \
    }

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}

TEST(capturesync_ut, capturesync)
{
    k4a_capture_t capture;
    capturesync_t sync;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

    config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_5;

    ASSERT_EQ(capturesync_create(&sync), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(capturesync_start(NULL, NULL), K4A_RESULT_FAILED);
    ASSERT_EQ(capturesync_start(sync, NULL), K4A_RESULT_FAILED);
    ASSERT_EQ(capturesync_start(NULL, &config), K4A_RESULT_FAILED);
    ASSERT_EQ(capturesync_start(sync, &config), K4A_RESULT_SUCCEEDED);
    // 2nd time should pass - public API does not allow start to be called twice, but internally we don't need to push
    // that requirement on each sub module
    ASSERT_EQ(capturesync_start(sync, &config), K4A_RESULT_SUCCEEDED);

    capturesync_stop(NULL);
    capturesync_stop(sync);
    // 2nd time should not crash or fail
    capturesync_stop(sync);

    // This should fail because we are in a stopped state.
    ASSERT_EQ(capturesync_get_capture(sync, &capture, 0), K4A_WAIT_RESULT_FAILED);

    ASSERT_EQ(capturesync_start(sync, &config), K4A_RESULT_SUCCEEDED);
    // This should timeout because we are in a running state and there is no data
    ASSERT_EQ(capturesync_get_capture(sync, &capture, 0), K4A_WAIT_RESULT_TIMEOUT);

    capturesync_destroy(sync);
    capturesync_destroy(NULL);
}

typedef struct _capturesync_test_timing_t
{
    uint64_t timestamp_usec;
    bool color_capture;
    int color_result;
    int depth_result;
} capturesync_test_timing_t;

static capturesync_test_timing_t Drop1Sample[] = {
    { FPS_30_US(0, 10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, 25), DEPTH_CAPTURE, -1, 0 },
    END_TEST_DATA,
};

static capturesync_test_timing_t Drop3SamplesSampleTsNearEndPeriod[] = {
    { FPS_30_US(0, 10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(2, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(3, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(3, 40), DEPTH_CAPTURE, -1, 0 },
    END_TEST_DATA,
};

static capturesync_test_timing_t Drop3SamplesSampleIsNearBeginPeriod[] = {
    { FPS_30_US(0, 10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(2, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(3, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(3, -40), DEPTH_CAPTURE, -2, 0 },
    END_TEST_DATA,
};

static capturesync_test_timing_t Drop3SamplesThen3More[] = {
    { FPS_30_US(0, 10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(2, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(3, -10), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(4, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    // Test where we are nearly the 1/4 FPS faster than depth - we are using 24% ahead of depth and expecting a capture
    // to be generated.
    { FPS_30_US(5, 10), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(5, 34), COLOR_CAPTURE, 0, -1 },

    // Test where we are just pass the 1/4 FPS faster than depth - we are using 26% ahead of depth here and expect there
    // not to be a capture - a failure will result in an extra capture waiting at the end of the test.
    { FPS_30_US(6, 10), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(6, 36), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(8, 00), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(8, 00), COLOR_CAPTURE, 0, -1 },
    END_TEST_DATA,
};

static capturesync_test_timing_t TwoToChooseFrom[] = {
    { FPS_30_US(0, 10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, 10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(2, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(2, 000), DEPTH_CAPTURE, -1, 0 },

    END_TEST_DATA,
};

static capturesync_test_timing_t OneSensorIsMultipleFramesAhead[] = {
    { FPS_30_US(0, 10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, 10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(2, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(3, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(0, 010), DEPTH_CAPTURE, -4, 0 },
    { FPS_30_US(4, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(1, 000), DEPTH_CAPTURE, -5, 0 },
    { FPS_30_US(5, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(2, 000), DEPTH_CAPTURE, -6, 0 },
    { FPS_30_US(6, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(3, 000), DEPTH_CAPTURE, -7, 0 },
    { FPS_30_US(7, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(10, 10), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(11, 10), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(12, -10), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(13, -10), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(10, 000), COLOR_CAPTURE, 0, -4 },
    { FPS_30_US(14, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(11, 000), COLOR_CAPTURE, 0, -5 },
    { FPS_30_US(15, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(12, 000), COLOR_CAPTURE, 0, -6 },
    { FPS_30_US(16, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(13, 000), COLOR_CAPTURE, 0, -7 },
    { FPS_30_US(17, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    END_TEST_DATA,
};

static capturesync_test_timing_t Drop2Captures1PeriodSamplesThen3More[] = {
    { FPS_30_US(0, 001), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(0, 001), DEPTH_CAPTURE, -1, 0 },
    { FPS_30_US(1, 010), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, -10), COLOR_CAPTURE, 0, -1 },
    { FPS_30_US(2, 010), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(2, -10), COLOR_CAPTURE, 0, -1 },
    { FPS_30_US(3, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(3, 000), DEPTH_CAPTURE, -1, 0 },

    // drop 3 then 3 more dup of Drop3SamplesThen3More
    { FPS_30_US(4, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(5, 010), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(6, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(7, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(8, 010), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(9, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    // 2 color captures in 1 period is not supported in this algorythm
    //{ FPS_30_US(10, -25), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    //{ FPS_30_US(10, 025), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    //{ FPS_30_US(10, 040), DEPTH_CAPTURE, -2, 0 },

    // we should recover and drop the extra frame that came in above
    { FPS_30_US(11, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(11, 0), DEPTH_CAPTURE, -1, 0 },
    END_TEST_DATA,
};

static capturesync_test_timing_t RandomTimingAndIssues[] = {
    { FPS_30_US(0, 10), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, -10), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(1, 000), COLOR_CAPTURE, 0, -1 },

    { FPS_30_US(2, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(2, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(3, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(3, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(4, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(4, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(5, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(5, -10), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(6, -10), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(6, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(7, -20), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(7, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(8, -30), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(8, 000), DEPTH_CAPTURE, -1, 0 }, // 16

    { FPS_30_US(9, -40), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(9, 000), DEPTH_CAPTURE, -1, 0 }, // 18

    { FPS_30_US(10, -35), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(10, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(11, -35), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(11, -35), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(12, -35), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(12, -25), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(13, -35), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(13, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(14, -35), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(14, 25), DEPTH_CAPTURE, -1, 0 }, // 28

    { FPS_30_US(15, -35), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(15, 35), DEPTH_CAPTURE, -1, 0 }, // 30

    { FPS_30_US(16, -35), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(16, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(17, -35), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(17, -10), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(18, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    // no depth frame

    { FPS_30_US(19, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    // no depth frame

    { FPS_30_US(20, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    // no depth frame

    { FPS_30_US(21, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    // no depth frame

    { FPS_30_US(22, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(22, 000), DEPTH_CAPTURE, -1, 0 }, // 40

    // no color depth
    { FPS_30_US(23, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    // no color depth
    { FPS_30_US(24, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    // no color depth
    { FPS_30_US(25, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    // no color depth
    { FPS_30_US(26, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(27, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(27, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(28, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    // no depth frame

    { FPS_30_US(29, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    // no depth frame

    // no color depth
    { FPS_30_US(30, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    // no color depth
    { FPS_30_US(31, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE }, // 50

    // no color depth
    { FPS_30_US(32, 000), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(33, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(33, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(34, 012), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(34, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(35, -12), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(35, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(36, -12), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(36, -12), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(37, -12), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE }, // 60
    { FPS_30_US(37, 012), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(38, 012), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(38, -12), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(40, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(40, 000), DEPTH_CAPTURE, -1, 0 },

    END_TEST_DATA,
};

// This test validates the implementation of synchronized_images_only. This test should provide enough data to over flow
// and queue and confirm that it is not sending captures to the user when synchronized_images_only=true,
static capturesync_test_timing_t DropIndividualSamplesToCaller[] = {
    { FPS_30_US(0, 1), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(0, 1), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(1, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(2, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(3, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(4, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(5, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(6, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(7, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(8, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(9, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(10, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(11, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(12, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(13, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(14, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(15, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(16, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(17, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(18, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(19, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(20, 0), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(21, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(21, 000), DEPTH_CAPTURE, -1, 0 },

    { FPS_30_US(22, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(23, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(24, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(25, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(26, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(27, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(28, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(29, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(30, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(31, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(32, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(33, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(34, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(35, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(36, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(37, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(38, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(39, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(40, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(41, 0), DEPTH_CAPTURE, NO_CAPTURE, NO_CAPTURE },

    { FPS_30_US(42, 000), COLOR_CAPTURE, NO_CAPTURE, NO_CAPTURE },
    { FPS_30_US(42, 000), DEPTH_CAPTURE, -1, 0 },

    END_TEST_DATA,
};

typedef struct _capturesync_test_t
{
    capturesync_test_timing_t *timing;
    bool color_capture;
    LOCK_HANDLE lock;
    capturesync_t sync;
    COND_HANDLE condition;
    bool thread_waiting;
} capturesync_test_t;

static k4a_result_t
capturesync_push_single_capture(k4a_result_t status, capturesync_t sync, bool color_capture, uint64_t timestamp)
{
    k4a_capture_t capture;
    k4a_image_t image = NULL;
    k4a_result_t result;
    const char *zone = color_capture ? "cs_color_test_thread" : "cs_depth_test_thread";

    result = TRACE_CALL(capture_create(&capture));
    if (K4A_SUCCEEDED(result))
    {
        if (color_capture)
        {
            result = TRACE_CALL(image_create_empty_internal(ALLOCATION_SOURCE_COLOR, 10, &image));
        }
        else
        {
            result = TRACE_CALL(image_create_empty_internal(ALLOCATION_SOURCE_DEPTH, 10, &image));
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        image_set_timestamp_usec(image, timestamp);
        if (color_capture)
        {
            capture_set_color_image(capture, image);
            image_dec_ref(image);
            image = NULL;
        }
        else
        {
            capture_set_ir_image(capture, image);
            capture_set_depth_image(capture, image);
            image_dec_ref(image);
            image = NULL;
        }
        LOG_INFO("%s: Pushing a capture", zone);
        capturesync_add_capture(sync, status, capture, color_capture);
        capture_dec_ref(capture);
    }
    return result;
}

// Simulate either a streaming depth thread or streaming color thread
static void capturesync_thread_generate_sample_ready(capturesync_test_t *test, k4a_result_t *result)
{
    capturesync_test_timing_t *timing;
    uint32_t i = 0;
    bool color_capture;

    timing = test->timing;
    color_capture = test->color_capture;
    *result = K4A_RESULT_SUCCEEDED;

    const char *zone = color_capture ? "cs_color_test_thread" : "cs_depth_test_thread";

    // Synchronize with test start
    Lock(test->lock);
    Unlock(test->lock);

    do
    {
        if (timing[i].color_capture == color_capture)
        {
            *result = capturesync_push_single_capture(K4A_RESULT_SUCCEEDED,
                                                      test->sync,
                                                      timing[i].color_capture,
                                                      timing[i].timestamp_usec);
        }

        // Send data without delay until we know we should get a sync'd capture, then wait - this keeps the internal
        // queue's from becomming saturated.
        if (timing[i].color_result != NO_CAPTURE && timing[i].depth_result != NO_CAPTURE)
        {
            Lock(test->lock);
            test->thread_waiting = true;

            COND_RESULT cond_result = Condition_Wait(test->condition, test->lock, 300000);
            Unlock(test->lock);
            ASSERT_EQ(cond_result, COND_OK) << "waiting on entry " << i << "\n";
        }

        i++;

    } while ((timing[i].timestamp_usec != UINT64_MAX) && (*result == K4A_RESULT_SUCCEEDED));

    LOG_INFO("%s: Thread exiting", zone);
}

// Simulate either a streaming depth thread or streaming color thread
static int _capturesync_thread_generate_sample_ready(void *param)
{
    capturesync_test_t *test;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    test = (capturesync_test_t *)param;

    capturesync_thread_generate_sample_ready(test, &result);
    return result;
}

static void capturesync_validate_synchronization(capturesync_test_timing_t *test_data,
                                                 bool color_first,
                                                 bool synchd_images_only = false)
{
    capturesync_test_t depth_test;
    capturesync_test_t color_test;
    THREAD_HANDLE th1, th2;
    k4a_capture_t capture = NULL;
    capturesync_t sync;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_result_t result;

    ASSERT_EQ(capturesync_create(&sync), K4A_RESULT_SUCCEEDED);

    depth_test.timing = color_test.timing = test_data;
    depth_test.sync = color_test.sync = sync;
    depth_test.color_capture = DEPTH_CAPTURE;
    color_test.color_capture = COLOR_CAPTURE;

    depth_test.thread_waiting = false;
    color_test.thread_waiting = false;

    depth_test.lock = Lock_Init();
    color_test.lock = depth_test.lock;
    ASSERT_NE(depth_test.lock, (LOCK_HANDLE)NULL);

    ASSERT_NE(color_test.condition = Condition_Init(), (COND_HANDLE)NULL);
    ASSERT_NE(depth_test.condition = Condition_Init(), (COND_HANDLE)NULL);

    config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.synchronized_images_only = synchd_images_only;
    if (color_first)
    {
        config.depth_delay_off_color_usec = 1;
    }
    else
    {
        config.depth_delay_off_color_usec = -1;
    }

    ASSERT_EQ(capturesync_start(sync, &config), K4A_RESULT_SUCCEEDED);

    // prevent the threads from running yet
    Lock(depth_test.lock);

    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th1, _capturesync_thread_generate_sample_ready, &depth_test));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th2, _capturesync_thread_generate_sample_ready, &color_test));

    Unlock(depth_test.lock);

    int32_t successfull_captures = 0;
    int i = 0;
    do
    {
        LOG_INFO("TS: %10lld C:%lld.%lld type: %s",
                 test_data[i].timestamp_usec,
                 test_data[i].timestamp_usec / FPS_30_IN_US,                     // frame period
                 test_data[i].timestamp_usec % FPS_30_IN_US * 10 / FPS_30_IN_US, // tenths of a frame period
                 (test_data[i].color_capture ? "color" : "depth"));

        if (test_data[i].color_result != NO_CAPTURE && test_data[i].depth_result != NO_CAPTURE)
        {
            uint64_t ts_color = 0;
            uint64_t ts_depth = 0;
            uint64_t ts_ir16 = 0;

            LOG_INFO("Waiting for capture upto %dms", WAIT_TEST_INFINITE);

            while (ts_color == 0 || ts_depth == 0 || ts_ir16 == 0)
            {
                if (capture)
                {
                    capture_dec_ref(capture);
                    capture = NULL;
                }
                // we will get unsynchronized captures, we have other tests for those. So skip a capture that doesn't
                // have both a color and depth/ir16 image
                ASSERT_EQ(capturesync_get_capture(sync, &capture, WAIT_TEST_INFINITE), (int)K4A_WAIT_RESULT_SUCCEEDED)
                    << "Test iteration is:" << i;

                for (int ts_loop = 0; ts_loop < 3; ts_loop++)
                {
                    k4a_image_t image;
                    uint64_t *ts;
                    switch (ts_loop)
                    {
                    case 0:
                        ts = &ts_color;
                        image = capture_get_color_image(capture);
                        break;
                    case 1:
                        ts = &ts_depth;
                        image = capture_get_depth_image(capture);
                        break;
                    default:
                        ts = &ts_ir16;
                        image = capture_get_ir_image(capture);
                        break;
                    }
                    *ts = 0;
                    if (image)
                    {
                        *ts = image_get_timestamp_usec(image);
                        image_dec_ref(image);
                    }
                }

                if (synchd_images_only)
                {
                    // When synchd_images_only is true, we should not recieve depth only or color only images.
                    ASSERT_NE(ts_color, 0);
                    ASSERT_NE(ts_depth, 0);
                    ASSERT_NE(ts_ir16, 0);
                }
            }

            // Validate the color, depth, and IR16 capture timestamps
            ASSERT_EQ(ts_color, test_data[i + test_data[i].color_result].timestamp_usec) << "Test iteration is:" << i;
            ASSERT_EQ(ts_depth, test_data[i + test_data[i].depth_result].timestamp_usec) << "Test iteration is:" << i;
            ASSERT_EQ(ts_ir16, test_data[i + test_data[i].depth_result].timestamp_usec) << "Test iteration is:" << i;

            capture_dec_ref(capture);
            capture = NULL;
            successfull_captures++;

            // Synchronize with the worker threads, we don't want them to put so much data into the queues that data has
            // to be dropped
            int delay = 0;
            Lock(depth_test.lock);
            while (color_test.thread_waiting == false || depth_test.thread_waiting == false)
            {
                Unlock(depth_test.lock);
                ThreadAPI_Sleep(10); // yield while the worker threads get to a parked state
                Lock(depth_test.lock);
                ASSERT_LT(delay += 10, 60000);
            };

            Condition_Post(color_test.condition); // Allow publishing threads to run
            Condition_Post(depth_test.condition); // Allow publishing threads to run

            color_test.thread_waiting = false;
            depth_test.thread_waiting = false;
            Unlock(depth_test.lock);
        }
        i++;
    } while (test_data[i].timestamp_usec != UINT64_MAX);

    // verify we didn't unexpectedly leave a capture in the queue
    ASSERT_EQ(capturesync_get_capture(sync, &capture, 0), (int)K4A_WAIT_RESULT_TIMEOUT);

    ASSERT_GT(successfull_captures, 0);
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th1, (int *)&result));
    ASSERT_EQ(result, (int)K4A_RESULT_SUCCEEDED);

    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th2, (int *)&result));
    ASSERT_EQ(result, (int)K4A_RESULT_SUCCEEDED);

    // inject error in data stream and verify this results in error with API
    if (color_first)
    {
        ASSERT_EQ((int)K4A_RESULT_SUCCEEDED,
                  capturesync_push_single_capture(K4A_RESULT_SUCCEEDED, sync, DEPTH_CAPTURE, 0));
        ASSERT_EQ((int)K4A_RESULT_SUCCEEDED,
                  capturesync_push_single_capture(K4A_RESULT_FAILED, sync, COLOR_CAPTURE, 0));
    }
    else
    {
        ASSERT_EQ((int)K4A_RESULT_SUCCEEDED,
                  capturesync_push_single_capture(K4A_RESULT_FAILED, sync, DEPTH_CAPTURE, 0));
        ASSERT_EQ((int)K4A_RESULT_SUCCEEDED,
                  capturesync_push_single_capture(K4A_RESULT_SUCCEEDED, sync, COLOR_CAPTURE, 0));
    }
    ASSERT_EQ(capturesync_get_capture(sync, &capture, WAIT_TEST_INFINITE), (int)K4A_WAIT_RESULT_FAILED)
        << "Sync capture failed to detect stream error";

    // capturesync_stop(sync);
    capturesync_destroy(sync);

    ASSERT_EQ(0, allocator_test_for_leaks());

    Condition_Deinit(color_test.condition);
    Condition_Deinit(depth_test.condition);
}

static capturesync_test_timing_t *invert_test_data_for_depth_first(capturesync_test_timing_t *test_data,
                                                                   size_t size_in_bytes)
{
    capturesync_test_timing_t *copy = (capturesync_test_timing_t *)malloc(size_in_bytes);
    if (copy)
    {
        for (unsigned int i = 0; i < size_in_bytes / sizeof(capturesync_test_timing_t); i++)
        {
            copy[i] = test_data[i];

            // Swaps types for running depth first based tests
            copy[i].color_capture = !test_data[i].color_capture;

            // Swap result locations
            int result = copy[i].color_result;
            copy[i].color_result = test_data[i].depth_result;
            copy[i].depth_result = result;
        }
    }
    return copy;
}

TEST(capturesync_ut, test_c_Drop1Sample)
{
    capturesync_validate_synchronization(Drop1Sample, COLOR_FIRST);
}

TEST(capturesync_ut, test_d_Drop1Sample)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(Drop1Sample, sizeof(Drop1Sample));
    capturesync_validate_synchronization(copy, DEPTH_FIRST);
    free(copy);
}

TEST(capturesync_ut, test_c_Drop3SamplesSampleTsNearEndPeriod)
{
    capturesync_validate_synchronization(Drop3SamplesSampleTsNearEndPeriod, COLOR_FIRST);
}

TEST(capturesync_ut, test_d_Drop3SamplesSampleTsNearEndPeriod)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(Drop3SamplesSampleTsNearEndPeriod,
                                                                       sizeof(Drop3SamplesSampleTsNearEndPeriod));
    capturesync_validate_synchronization(copy, DEPTH_FIRST);
    free(copy);
}

TEST(capturesync_ut, test_c_Drop3SamplesSampleIsNearBeginPeriod)
{
    capturesync_validate_synchronization(Drop3SamplesSampleIsNearBeginPeriod, COLOR_FIRST);
}

TEST(capturesync_ut, test_d_Drop3SamplesSampleIsNearBeginPeriod)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(Drop3SamplesSampleIsNearBeginPeriod,
                                                                       sizeof(Drop3SamplesSampleIsNearBeginPeriod));
    capturesync_validate_synchronization(copy, DEPTH_FIRST);
    free(copy);
}

TEST(capturesync_ut, test_c_Drop3SamplesThen3More)
{
    capturesync_validate_synchronization(Drop3SamplesThen3More, COLOR_FIRST);
}

TEST(capturesync_ut, test_d_Drop3SamplesThen3More)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(Drop3SamplesThen3More,
                                                                       sizeof(Drop3SamplesThen3More));
    capturesync_validate_synchronization(copy, DEPTH_FIRST);
    free(copy);
}

TEST(capturesync_ut, test_c_Drop2Captures1PeriodSamplesThen3More)
{
    capturesync_validate_synchronization(Drop2Captures1PeriodSamplesThen3More, COLOR_FIRST);
}

TEST(capturesync_ut, test_d_Drop2Captures1PeriodSamplesThen3More)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(Drop2Captures1PeriodSamplesThen3More,
                                                                       sizeof(Drop2Captures1PeriodSamplesThen3More));
    capturesync_validate_synchronization(copy, DEPTH_FIRST);
    free(copy);
}

TEST(capturesync_ut, test_c_RandomTimingAndIssues)
{
    capturesync_validate_synchronization(RandomTimingAndIssues, COLOR_FIRST);
}

TEST(capturesync_ut, test_d_RandomTimingAndIssues)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(RandomTimingAndIssues,
                                                                       sizeof(RandomTimingAndIssues));
    capturesync_validate_synchronization(copy, DEPTH_FIRST);
    free(copy);
}

TEST(capturesync_ut, test_c_TwoToChooseFrom)
{
    capturesync_validate_synchronization(TwoToChooseFrom, COLOR_FIRST);
}

TEST(capturesync_ut, test_d_TwoToChooseFrom)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(TwoToChooseFrom, sizeof(TwoToChooseFrom));
    capturesync_validate_synchronization(copy, DEPTH_FIRST);
    free(copy);
}

TEST(capturesync_ut, test_c_OneSensorIsMultipleFramesAhead)
{
    capturesync_validate_synchronization(OneSensorIsMultipleFramesAhead, COLOR_FIRST);
}

TEST(capturesync_ut, test_d_OneSensorIsMultipleFramesAhead)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(OneSensorIsMultipleFramesAhead,
                                                                       sizeof(OneSensorIsMultipleFramesAhead));
    capturesync_validate_synchronization(copy, DEPTH_FIRST);
    free(copy);
}

TEST(capturesync_ut, test_c_DropIndividualSamplesToCaller)
{
    capturesync_validate_synchronization(DropIndividualSamplesToCaller, COLOR_FIRST);
}

TEST(capturesync_ut, test_d_DropIndividualSamplesToCaller)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(DropIndividualSamplesToCaller,
                                                                       sizeof(DropIndividualSamplesToCaller));
    capturesync_validate_synchronization(copy, DEPTH_FIRST);
    free(copy);
}

TEST(capturesync_ut, test_c_DropIndividualSamplesToCaller_v2)
{
    capturesync_validate_synchronization(DropIndividualSamplesToCaller, COLOR_FIRST, true);
}

TEST(capturesync_ut, test_d_DropIndividualSamplesToCaller_v2)
{
    capturesync_test_timing_t *copy = invert_test_data_for_depth_first(DropIndividualSamplesToCaller,
                                                                       sizeof(DropIndividualSamplesToCaller));
    capturesync_validate_synchronization(copy, DEPTH_FIRST, true);
    free(copy);
}
