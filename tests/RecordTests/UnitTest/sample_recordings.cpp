// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "test_helpers.h"

#include <cstdio>
#include <k4arecord/record.h>
#include <k4ainternal/common.h>
#include <k4ainternal/matroska_write.h>

using namespace testing;

void SampleRecordings::SetUp()
{
    k4a_device_configuration_t record_config_empty = {};
    record_config_empty.color_resolution = K4A_COLOR_RESOLUTION_OFF;
    record_config_empty.depth_mode = K4A_DEPTH_MODE_OFF;

    k4a_device_configuration_t record_config_full = {};
    record_config_full.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    record_config_full.color_resolution = K4A_COLOR_RESOLUTION_1080P;
    record_config_full.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    record_config_full.camera_fps = K4A_FRAMES_PER_SECOND_30;

    k4a_device_configuration_t record_config_delay = record_config_full;
    record_config_delay.depth_delay_off_color_usec = 10000; // 10ms

    k4a_device_configuration_t record_config_sub = record_config_full;
    record_config_sub.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
    record_config_sub.subordinate_delay_off_master_usec = 10000; // 10ms

    {
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_empty.mkv", NULL, record_config_empty, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_flush(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_record_close(handle);
    }
    { // Create a fully populated, regular recording file
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_full.mkv", NULL, record_config_full, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_add_imu_track(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { 0, 1000, 1000 }; // Offset the Depth and IR tracks by 1ms to test
        uint64_t imu_timestamp = 1150;
        uint32_t timestamp_delta = 1000000 / k4a_convert_fps_to_uint(record_config_full.camera_fps);
        k4a_capture_t capture = NULL;
        for (int i = 0; i < 100; i++)
        {
            capture = create_test_capture(timestamps,
                                          record_config_full.color_format,
                                          record_config_full.color_resolution,
                                          record_config_full.depth_mode);
            result = k4a_record_write_capture(handle, capture);
            ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
            k4a_capture_release(capture);

            timestamps[0] += timestamp_delta;
            timestamps[1] += timestamp_delta;
            timestamps[2] += timestamp_delta;

            while (imu_timestamp < timestamps[0])
            {
                k4a_imu_sample_t imu_sample = create_test_imu_sample(imu_timestamp);
                result = k4a_record_write_imu_sample(handle, imu_sample);
                ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

                // Write IMU samples at ~1000 samples per second (this is an arbitrary rate for testing)
                imu_timestamp += 1000; // 1ms
            }
        }

        result = k4a_record_flush(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_record_close(handle);
    }
    { // Create a recording file with a depth delay offset
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_delay.mkv", NULL, record_config_delay, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { 0,
                                   (uint64_t)record_config_delay.depth_delay_off_color_usec,
                                   (uint64_t)record_config_delay.depth_delay_off_color_usec };
        uint32_t timestamp_delta = 1000000 / k4a_convert_fps_to_uint(record_config_delay.camera_fps);
        k4a_capture_t capture = NULL;
        for (int i = 0; i < 100; i++)
        {
            capture = create_test_capture(timestamps,
                                          record_config_delay.color_format,
                                          record_config_delay.color_resolution,
                                          record_config_delay.depth_mode);
            result = k4a_record_write_capture(handle, capture);
            ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
            k4a_capture_release(capture);

            timestamps[0] += timestamp_delta;
            timestamps[1] += timestamp_delta;
            timestamps[2] += timestamp_delta;
        }

        result = k4a_record_flush(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_record_close(handle);
    }
    {
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_sub.mkv", NULL, record_config_sub, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { 0, 0, 0 };
        k4a_capture_t capture = create_test_capture(timestamps,
                                                    record_config_sub.color_format,
                                                    record_config_sub.color_resolution,
                                                    record_config_sub.depth_mode);
        result = k4a_record_write_capture(handle, capture);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
        k4a_capture_release(capture);

        result = k4a_record_flush(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_record_close(handle);
    }
    { // Create a recording file with time skips and missing frames
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_skips.mkv", NULL, record_config_full, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        { // Force the timestamp offset so the recording starts at a non-zero timestamp.
            k4arecord::k4a_record_context_t *context = &((k4arecord::k4a_record_t_wrapper__cpp *)handle)->context;
            context->first_cluster_written = true;
            context->start_timestamp_offset = 1_ms;
        }

        uint64_t timestamps[3] = { 1000000, 1001000, 1001000 }; // Start recording at 1s
        uint32_t timestamp_delta = 1000000 / k4a_convert_fps_to_uint(record_config_full.camera_fps);
        for (int i = 0; i < 100; i++)
        {
            // Create a known pattern of dropped / missing frames that can be tested against
            // The pattern is repeated every 4 captures until the end of the file.
            k4a_capture_t capture = NULL;
            switch (i % 4)
            {
            case 0: // Depth Only
                capture = create_test_capture(timestamps,
                                              record_config_full.color_format,
                                              K4A_COLOR_RESOLUTION_OFF,
                                              record_config_full.depth_mode);
                break;
            case 1: // Color Only
                capture = create_test_capture(timestamps,
                                              record_config_full.color_format,
                                              record_config_full.color_resolution,
                                              K4A_DEPTH_MODE_OFF);
                break;
            case 2: // No frames
                break;
            case 3: // Both Depth + Color
                capture = create_test_capture(timestamps,
                                              record_config_full.color_format,
                                              record_config_full.color_resolution,
                                              record_config_full.depth_mode);
                break;
            }
            if (capture)
            {
                result = k4a_record_write_capture(handle, capture);
                ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
                k4a_capture_release(capture);
            }
            // Flush the file every 7 captures to test flushing at multiple points in the recording.
            // 7 is prime, so all flush points should be covered in the above 4 capture sequence.
            if (i % 7 == 0)
            {
                result = k4a_record_flush(handle);
                ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
            }

            timestamps[0] += timestamp_delta;
            timestamps[1] += timestamp_delta;
            timestamps[2] += timestamp_delta;
        }

        result = k4a_record_flush(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_record_close(handle);
    }
}

void SampleRecordings::TearDown()
{
    ASSERT_EQ(std::remove("record_test_empty.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_full.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_delay.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_skips.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_sub.mkv"), 0);
}
