#include "test_helpers.h"

#include <cstdio>
#include <k4arecord/record.h>
#include <k4ainternal/common.h>

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
    {
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_full.mkv", NULL, record_config_full, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_add_imu_track(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { 0, 1000, 1000 }; // Offset the Depth and IR tracks by 1ms to test
        uint64_t imu_timestamp = 0;
        uint32_t timestamp_delta = 1000000 / k4a_convert_fps_to_uint(record_config_full.camera_fps);
        k4a_capture_t capture;
        for (int i = 0; i < 100; i++)
        {
            capture = create_test_capture(timestamps,
                                          record_config_full.color_format,
                                          record_config_full.color_resolution,
                                          record_config_full.depth_mode);
            result = k4a_record_write_capture(handle, capture);
            ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
            k4a_capture_release(capture);

            while (imu_timestamp < timestamps[0])
            {
                k4a_imu_sample_t imu_sample = create_test_imu_sample(imu_timestamp);
                result = k4a_record_write_imu_sample(handle, imu_sample);
                ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

                // Write IMU samples at ~1000 samples per second (this is an arbitrary for testing)
                imu_timestamp += 1000; // 1ms
            }

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
        k4a_result_t result = k4a_record_create("record_test_delay.mkv", NULL, record_config_delay, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { 0,
                                   (uint64_t)record_config_delay.depth_delay_off_color_usec,
                                   (uint64_t)record_config_delay.depth_delay_off_color_usec };
        uint32_t timestamp_delta = 1000000 / k4a_convert_fps_to_uint(record_config_delay.camera_fps);
        k4a_capture_t capture;
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
}

void SampleRecordings::TearDown()
{
    std::remove("record_test_empty.mkv");
    std::remove("record_test_full.mkv");
    std::remove("record_test_delay.mkv");
    std::remove("record_test_sub.mkv");
}
