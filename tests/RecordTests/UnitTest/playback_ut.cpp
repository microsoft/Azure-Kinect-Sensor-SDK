// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include <k4a/k4a.h>
#include <k4ainternal/common.h>
#include <k4ainternal/matroska_common.h>

#include "test_helpers.h"
#include <fstream>
#include <thread>
#include <chrono>

// Module being tested
#include <k4arecord/playback.h>

using namespace testing;

class playback_ut : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(playback_ut, open_empty_file)
{
    { // Check to make sure the test recording exists and is readable.
        std::ifstream test_file("record_test_empty.mkv");
        ASSERT_TRUE(test_file.good());
    }
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_empty.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_FAILED);
}

TEST_F(playback_ut, read_playback_tags)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_full.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Get tag with exact buffer size
    size_t tag_value_size = 0;
    k4a_buffer_result_t buffer_result = k4a_playback_get_tag(handle, "K4A_COLOR_MODE", NULL, &tag_value_size);
    ASSERT_EQ(buffer_result, K4A_BUFFER_RESULT_TOO_SMALL);
    ASSERT_EQ(tag_value_size, (size_t)11);
    std::vector<char> tag_value = std::vector<char>(tag_value_size);
    buffer_result = k4a_playback_get_tag(handle, "K4A_COLOR_MODE", tag_value.data(), &tag_value_size);
    ASSERT_EQ(buffer_result, K4A_BUFFER_RESULT_SUCCEEDED);
    ASSERT_EQ(tag_value_size, (size_t)11);
    ASSERT_STREQ(tag_value.data(), "MJPG_1080P");

    // Get tag with oversize buffer
    tag_value.resize(256);
    tag_value_size = tag_value.size();
    buffer_result = k4a_playback_get_tag(handle, "K4A_DEPTH_MODE", tag_value.data(), &tag_value_size);
    ASSERT_EQ(buffer_result, K4A_BUFFER_RESULT_SUCCEEDED);
    ASSERT_EQ(tag_value_size, (size_t)14);
    ASSERT_STREQ(tag_value.data(), "NFOV_UNBINNED");

    // Missing Tag
    buffer_result = k4a_playback_get_tag(handle, "FOO", NULL, &tag_value_size);
    ASSERT_EQ(buffer_result, K4A_BUFFER_RESULT_FAILED);

    k4a_playback_close(handle);
}

TEST_F(playback_ut, open_large_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_full.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_MJPG);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_TRUE(config.color_track_enabled);
    ASSERT_TRUE(config.depth_track_enabled);
    ASSERT_TRUE(config.ir_track_enabled);
    ASSERT_TRUE(config.imu_track_enabled);
    ASSERT_EQ(config.depth_delay_off_color_usec, 0);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_STANDALONE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)0);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)0);

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t timestamps[3] = { 0, 1000, 1000 };
    uint64_t timestamp_delta = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config.camera_fps));
    size_t i = 0;
    for (; i < 50; i++)
    {
        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);
        timestamps[0] += timestamp_delta;
        timestamps[1] += timestamp_delta;
        timestamps[2] += timestamp_delta;
    }
    { // Try reading backwards for a couple captures.
        timestamps[0] -= timestamp_delta;
        timestamps[1] -= timestamp_delta;
        timestamps[2] -= timestamp_delta;
        for (; i > 40; i--)
        {
            timestamps[0] -= timestamp_delta;
            timestamps[1] -= timestamp_delta;
            timestamps[2] -= timestamp_delta;
            stream_result = k4a_playback_get_previous_capture(handle, &capture);
            ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
            ASSERT_TRUE(validate_test_capture(capture,
                                              timestamps,
                                              config.color_format,
                                              config.color_resolution,
                                              config.depth_mode));
            k4a_capture_release(capture);
        }
        timestamps[0] += timestamp_delta;
        timestamps[1] += timestamp_delta;
        timestamps[2] += timestamp_delta;
    }
    for (; i < test_frame_count; i++)
    {
        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);
        timestamps[0] += timestamp_delta;
        timestamps[1] += timestamp_delta;
        timestamps[2] += timestamp_delta;
    }
    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    k4a_playback_close(handle);
}

TEST_F(playback_ut, open_delay_offset_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_delay.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_MJPG);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_TRUE(config.color_track_enabled);
    ASSERT_TRUE(config.depth_track_enabled);
    ASSERT_TRUE(config.ir_track_enabled);
    ASSERT_FALSE(config.imu_track_enabled);
    ASSERT_EQ(config.depth_delay_off_color_usec, 10000);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_STANDALONE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)0);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)0);

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t timestamps[3] = { 0, 10000, 10000 };
    uint64_t timestamp_delta = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config.camera_fps));

    // Read forward
    for (size_t i = 0; i < test_frame_count; i++)
    {
        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);
        timestamps[0] += timestamp_delta;
        timestamps[1] += timestamp_delta;
        timestamps[2] += timestamp_delta;
    }
    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    // Read backward
    for (size_t i = 0; i < test_frame_count; i++)
    {
        timestamps[0] -= timestamp_delta;
        timestamps[1] -= timestamp_delta;
        timestamps[2] -= timestamp_delta;
        stream_result = k4a_playback_get_previous_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);
    }
    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    k4a_playback_close(handle);
}

TEST_F(playback_ut, open_subordinate_delay_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_sub.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_MJPG);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_TRUE(config.color_track_enabled);
    ASSERT_TRUE(config.depth_track_enabled);
    ASSERT_TRUE(config.ir_track_enabled);
    ASSERT_FALSE(config.imu_track_enabled);
    ASSERT_EQ(config.depth_delay_off_color_usec, 0);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_SUBORDINATE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)10000);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)10000);

    uint64_t timestamps[3] = { 10000, 10000, 10000 };

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    k4a_playback_close(handle);
}

TEST_F(playback_ut, playback_seek_test)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_full.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_MJPG);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_TRUE(config.color_track_enabled);
    ASSERT_TRUE(config.depth_track_enabled);
    ASSERT_TRUE(config.ir_track_enabled);
    ASSERT_TRUE(config.imu_track_enabled);
    ASSERT_EQ(config.depth_delay_off_color_usec, 0);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_STANDALONE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)0);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)0);

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t timestamps[3] = { 0, 1000, 1000 };
    uint64_t timestamp_delta = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config.camera_fps));

    k4a_imu_sample_t imu_sample = { 0 };
    uint64_t imu_timestamp = 1150;

    // Test initial state
    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_TRUE(validate_null_imu_sample(imu_sample));

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

    int64_t recording_length = (int64_t)k4a_playback_get_recording_length_usec(handle) + 1;
    std::pair<int64_t, k4a_playback_seek_origin_t> start_seek_combinations[] = { // Beginning
                                                                                 { 0, K4A_PLAYBACK_SEEK_BEGIN },
                                                                                 { -recording_length,
                                                                                   K4A_PLAYBACK_SEEK_END },
                                                                                 // Past beginning
                                                                                 { -10, K4A_PLAYBACK_SEEK_BEGIN },
                                                                                 { -recording_length - 10,
                                                                                   K4A_PLAYBACK_SEEK_END }
    };

    std::pair<int64_t, k4a_playback_seek_origin_t> end_seek_combinations[] = { // End
                                                                               { 0, K4A_PLAYBACK_SEEK_END },
                                                                               { recording_length,
                                                                                 K4A_PLAYBACK_SEEK_BEGIN },
                                                                               // Past end
                                                                               { 10, K4A_PLAYBACK_SEEK_END },
                                                                               { recording_length + 10,
                                                                                 K4A_PLAYBACK_SEEK_BEGIN }
    };

    std::pair<int64_t, k4a_playback_seek_origin_t> middle_seek_combinations[] = {
        // Between captures, and between IMU samples
        { timestamp_delta * 50 - 250, K4A_PLAYBACK_SEEK_BEGIN },
        { -recording_length + (int64_t)(timestamp_delta * 50 - 250), K4A_PLAYBACK_SEEK_END },
        // Middle of capture, and exact IMU timestamp
        { timestamp_delta * 50 + 500, K4A_PLAYBACK_SEEK_BEGIN },
        { -recording_length + (int64_t)(timestamp_delta * 50 + 500), K4A_PLAYBACK_SEEK_END },
    };

    std::cerr << "[          ] Testing seek to start:" << std::endl;
    // Test seek combinations around beginning of recording
    for (auto seek : start_seek_combinations)
    {
        std::cerr << "[          ]     Seeking to " << seek.first << " from "
                  << (seek.second == K4A_PLAYBACK_SEEK_BEGIN ? "beginning" : "end") << std::endl;

        // Seek then read backward
        result = k4a_playback_seek_timestamp(handle, seek.first, seek.second);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_previous_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
        ASSERT_EQ(capture, (k4a_capture_t)NULL);

        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);

        stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
        ASSERT_TRUE(validate_null_imu_sample(imu_sample));

        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        // Seek then read forward
        result = k4a_playback_seek_timestamp(handle, seek.first, seek.second);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);

        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));
    }

    std::cerr << "[          ] Testing seek to end:" << std::endl;
    // Test seek combinations around end of recording
    timestamps[0] += timestamp_delta * 99;
    timestamps[1] += timestamp_delta * 99;
    timestamps[2] += timestamp_delta * 99;
    imu_timestamp = 3333150;
    for (auto seek : end_seek_combinations)
    {
        std::cerr << "[          ]     Seeking to " << seek.first << " from "
                  << (seek.second == K4A_PLAYBACK_SEEK_BEGIN ? "beginning" : "end") << std::endl;

        // Seek then read forward
        result = k4a_playback_seek_timestamp(handle, seek.first, seek.second);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
        ASSERT_EQ(capture, (k4a_capture_t)NULL);

        stream_result = k4a_playback_get_previous_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);

        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
        ASSERT_TRUE(validate_null_imu_sample(imu_sample));

        stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        // Seek then read backward
        result = k4a_playback_seek_timestamp(handle, seek.first, seek.second);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_previous_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);

        stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));
    }

    std::cerr << "[          ] Testing seek to middle:" << std::endl;
    // Test seek combinations around middle of recording
    timestamps[0] -= timestamp_delta * 49;
    timestamps[1] -= timestamp_delta * 49;
    timestamps[2] -= timestamp_delta * 49;
    imu_timestamp = 1667150;
    for (auto seek : middle_seek_combinations)
    {
        std::cerr << "[          ]     Seeking to " << seek.first << " from "
                  << (seek.second == K4A_PLAYBACK_SEEK_BEGIN ? "beginning" : "end") << std::endl;

        // Test next then previous capture
        result = k4a_playback_seek_timestamp(handle, seek.first, seek.second);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);

        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        timestamps[0] -= timestamp_delta;
        timestamps[1] -= timestamp_delta;
        timestamps[2] -= timestamp_delta;
        imu_timestamp -= 1000;

        stream_result = k4a_playback_get_previous_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);

        stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        // Test previous then next capture
        result = k4a_playback_seek_timestamp(handle, seek.first, seek.second);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_previous_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);

        stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        timestamps[0] += timestamp_delta;
        timestamps[1] += timestamp_delta;
        timestamps[2] += timestamp_delta;
        imu_timestamp += 1000;

        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);

        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));
    }

    k4a_playback_close(handle);
}

TEST_F(playback_ut, open_skipped_frames_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_skips.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_MJPG);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_EQ(config.depth_delay_off_color_usec, 0);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_STANDALONE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)0);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)1000);

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t timestamps[3] = { 1000000, 1001000, 1001000 };
    uint64_t timestamp_delta = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config.camera_fps));

    // Test initial state
    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // According to the generated sample sequence, the first capture is missing a Color image
    // i == 0 in generation loop (see sample_recordings.cpp)
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, K4A_COLOR_RESOLUTION_OFF, config.depth_mode));
    k4a_capture_release(capture);

    // Test seek to beginning
    result = k4a_playback_seek_timestamp(handle, 0, K4A_PLAYBACK_SEEK_BEGIN);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // i == 0, Color image is dropped
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, K4A_COLOR_RESOLUTION_OFF, config.depth_mode));
    k4a_capture_release(capture);

    // Test seek past beginning
    result = k4a_playback_seek_timestamp(handle,
                                         -(int64_t)k4a_playback_get_recording_length_usec(handle) - 10,
                                         K4A_PLAYBACK_SEEK_END);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // i == 0, Color image is dropped
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, K4A_COLOR_RESOLUTION_OFF, config.depth_mode));
    k4a_capture_release(capture);

    // Test seek to end
    result = k4a_playback_seek_timestamp(handle, 0, K4A_PLAYBACK_SEEK_END);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    timestamps[0] += timestamp_delta * 99;
    timestamps[1] += timestamp_delta * 99;
    timestamps[2] += timestamp_delta * 99;

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // i == 99, No images are dropped
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    // Test seek to end, relative to start
    result = k4a_playback_seek_timestamp(handle,
                                         (int64_t)k4a_playback_get_recording_length_usec(handle) + 1,
                                         K4A_PLAYBACK_SEEK_BEGIN);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // i == 99, No images are dropped
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    // Test seek to middle of the recording, then read forward
    timestamps[0] -= timestamp_delta * 50;
    timestamps[1] -= timestamp_delta * 50;
    timestamps[2] -= timestamp_delta * 50;
    result = k4a_playback_seek_timestamp(handle, (int64_t)timestamps[0], K4A_PLAYBACK_SEEK_DEVICE_TIME);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // i == 49, Depth image is dropped
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, K4A_DEPTH_MODE_OFF));
    k4a_capture_release(capture);

    // Test seek to middle of the recording, then read backward
    result = k4a_playback_seek_timestamp(handle, (int64_t)timestamps[0], K4A_PLAYBACK_SEEK_DEVICE_TIME);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    timestamps[0] -= timestamp_delta;
    timestamps[1] -= timestamp_delta;
    timestamps[2] -= timestamp_delta;

    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // i == 48, Color image is dropped
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, K4A_COLOR_RESOLUTION_OFF, config.depth_mode));
    k4a_capture_release(capture);

    // Read the rest of the file
    for (size_t i = 49; i < test_frame_count; i++)
    {
        timestamps[0] += timestamp_delta;
        timestamps[1] += timestamp_delta;
        timestamps[2] += timestamp_delta;

        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        // Check each capture against the same sequence generated in sample_recordings.cpp
        switch (i % 4)
        {
        case 0: // Depth Only
            ASSERT_TRUE(validate_test_capture(capture,
                                              timestamps,
                                              config.color_format,
                                              K4A_COLOR_RESOLUTION_OFF,
                                              config.depth_mode));
            break;
        case 1: // Color Only
            ASSERT_TRUE(validate_test_capture(capture,
                                              timestamps,
                                              config.color_format,
                                              config.color_resolution,
                                              K4A_DEPTH_MODE_OFF));
            break;
        case 2: // No frames, advance timestamp and read as next index.
            timestamps[0] += timestamp_delta;
            timestamps[1] += timestamp_delta;
            timestamps[2] += timestamp_delta;
            i++;

            // Both Color + Depth
            ASSERT_TRUE(validate_test_capture(capture,
                                              timestamps,
                                              config.color_format,
                                              config.color_resolution,
                                              config.depth_mode));
            break;
        }
        k4a_capture_release(capture);
    }

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    k4a_playback_close(handle);
}

TEST_F(playback_ut, open_imu_playback_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_full.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_MJPG);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_TRUE(config.color_track_enabled);
    ASSERT_TRUE(config.depth_track_enabled);
    ASSERT_TRUE(config.ir_track_enabled);
    ASSERT_TRUE(config.imu_track_enabled);
    ASSERT_EQ(config.depth_delay_off_color_usec, 0);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_STANDALONE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)0);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)0);

    k4a_imu_sample_t imu_sample = { 0 };
    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t imu_timestamp = 1150;
    uint64_t recording_length = k4a_playback_get_recording_length_usec(handle);
    ASSERT_EQ(recording_length, 3333150);

    // Read forward
    while (imu_timestamp <= recording_length)
    {
        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));
        imu_timestamp += 1000;
    }
    stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_TRUE(validate_null_imu_sample(imu_sample));

    // Read backward
    while (imu_timestamp > 1150)
    {
        imu_timestamp -= 1000;
        stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));
    }
    stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_TRUE(validate_null_imu_sample(imu_sample));

    // Test seeking to first 100 samples (covers edge cases around block boundaries)
    for (size_t i = 0; i < test_frame_count; i++)
    {
        // Seek to before sample
        result = k4a_playback_seek_timestamp(handle, (int64_t)imu_timestamp - 100, K4A_PLAYBACK_SEEK_BEGIN);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        // Seek exactly to sample
        result = k4a_playback_seek_timestamp(handle, (int64_t)imu_timestamp, K4A_PLAYBACK_SEEK_BEGIN);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        // Seek to after sample
        result = k4a_playback_seek_timestamp(handle, (int64_t)imu_timestamp + 100, K4A_PLAYBACK_SEEK_BEGIN);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        imu_timestamp += 1000;
    }

    k4a_playback_close(handle);
}

TEST_F(playback_ut, open_start_offset_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_offset.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_MJPG);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_TRUE(config.color_track_enabled);
    ASSERT_TRUE(config.depth_track_enabled);
    ASSERT_TRUE(config.ir_track_enabled);
    ASSERT_TRUE(config.imu_track_enabled);
    ASSERT_EQ(config.depth_delay_off_color_usec, 0);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_STANDALONE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)0);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)1000000);

    k4a_capture_t capture = NULL;
    k4a_imu_sample_t imu_sample = { 0 };
    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t timestamps[3] = { 1000000, 1000000, 1000000 };
    uint64_t imu_timestamp = 1001150;
    uint64_t timestamp_delta = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config.camera_fps));
    uint64_t last_timestamp = k4a_playback_get_recording_length_usec(handle) +
                              (uint64_t)config.start_timestamp_offset_usec;
    ASSERT_EQ(last_timestamp, (uint64_t)config.start_timestamp_offset_usec + 3333150);

    // Read capture forward
    for (size_t i = 0; i < test_frame_count; i++)
    {
        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);
        timestamps[0] += timestamp_delta;
        timestamps[1] += timestamp_delta;
        timestamps[2] += timestamp_delta;
    }
    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    // Read capture backward
    for (size_t i = 0; i < test_frame_count; i++)
    {
        timestamps[0] -= timestamp_delta;
        timestamps[1] -= timestamp_delta;
        timestamps[2] -= timestamp_delta;
        stream_result = k4a_playback_get_previous_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          config.color_resolution,
                                          config.depth_mode));
        k4a_capture_release(capture);
    }
    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    // Read IMU forward
    while (imu_timestamp <= last_timestamp)
    {
        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));
        imu_timestamp += 1000;
    }
    stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_TRUE(validate_null_imu_sample(imu_sample));

    // Read IMU backward
    while (imu_timestamp > 1001150)
    {
        imu_timestamp -= 1000;
        stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));
    }
    stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_TRUE(validate_null_imu_sample(imu_sample));

    // Test seeking to first 100 samples (covers edge cases around block boundaries)
    for (size_t i = 0; i < test_frame_count; i++)
    {
        // Seek to before sample
        result = k4a_playback_seek_timestamp(handle, (int64_t)imu_timestamp - 100, K4A_PLAYBACK_SEEK_DEVICE_TIME);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        // Seek exactly to sample
        result = k4a_playback_seek_timestamp(handle, (int64_t)imu_timestamp, K4A_PLAYBACK_SEEK_DEVICE_TIME);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        // Seek to after sample
        result = k4a_playback_seek_timestamp(handle, (int64_t)imu_timestamp + 100, K4A_PLAYBACK_SEEK_DEVICE_TIME);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        stream_result = k4a_playback_get_previous_imu_sample(handle, &imu_sample);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_imu_sample(imu_sample, imu_timestamp));

        imu_timestamp += 1000;
    }

    k4a_playback_close(handle);
}

TEST_F(playback_ut, open_color_only_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_color_only.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_MJPG);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_OFF);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_TRUE(config.color_track_enabled);
    ASSERT_FALSE(config.depth_track_enabled);
    ASSERT_FALSE(config.ir_track_enabled);
    ASSERT_FALSE(config.imu_track_enabled);
    ASSERT_EQ(config.depth_delay_off_color_usec, 0);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_STANDALONE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)0);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)0);

    uint64_t timestamps[3] = { 0, 0, 0 };

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    k4a_playback_close(handle);
}

TEST_F(playback_ut, open_depth_only_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_depth_only.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_CUSTOM);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_OFF);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_FALSE(config.color_track_enabled);
    ASSERT_TRUE(config.depth_track_enabled);
    ASSERT_TRUE(config.ir_track_enabled);
    ASSERT_FALSE(config.imu_track_enabled);
    ASSERT_EQ(config.depth_delay_off_color_usec, 0);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_STANDALONE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)0);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)0);

    uint64_t timestamps[3] = { 0, 0, 0 };

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    k4a_playback_close(handle);
}

TEST_F(playback_ut, open_bgra_color_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_bgra_color.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read recording configuration
    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_BGRA32);
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_OFF);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_TRUE(config.color_track_enabled);
    ASSERT_FALSE(config.depth_track_enabled);
    ASSERT_FALSE(config.ir_track_enabled);
    ASSERT_FALSE(config.imu_track_enabled);
    ASSERT_EQ(config.depth_delay_off_color_usec, 0);
    ASSERT_EQ(config.wired_sync_mode, K4A_WIRED_SYNC_MODE_STANDALONE);
    ASSERT_EQ(config.subordinate_delay_off_master_usec, (uint32_t)0);
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)0);

    uint64_t timestamps[3] = { 0, 0, 0 };

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    k4a_playback_close(handle);
}

int main(int argc, char **argv)
{
    k4a_unittest_init();

    ::testing::AddGlobalTestEnvironment(new SampleRecordings());
    ::testing::InitGoogleTest(&argc, argv);
    int results = RUN_ALL_TESTS();
    k4a_unittest_deinit();
    return results;
}
