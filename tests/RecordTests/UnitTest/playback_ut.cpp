// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include <k4a/k4a.h>
#include <k4ainternal/common.h>
#include <k4ainternal/matroska_common.h>

#include "test_helpers.h"
#include <fstream>

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
    uint64_t timestamp_delta = 1000000 / k4a_convert_fps_to_uint(config.camera_fps);
    int i = 0;
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
    for (; i < 100; i++)
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
    uint64_t timestamp_delta = 1000000 / k4a_convert_fps_to_uint(config.camera_fps);

    // Read forward
    for (int i = 0; i < 100; i++)
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
    for (int i = 0; i < 100; i++)
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
    uint64_t timestamp_delta = 1000000 / k4a_convert_fps_to_uint(config.camera_fps);

    // Test initial state
    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    // Test seek to beginning
    result = k4a_playback_seek_timestamp(handle, 0, K4A_PLAYBACK_SEEK_BEGIN);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    // Test seek past beginning
    result = k4a_playback_seek_timestamp(handle,
                                         -(int64_t)k4a_playback_get_last_timestamp_usec(handle) - 10,
                                         K4A_PLAYBACK_SEEK_END);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
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
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    // Test seek to end, relative to start
    result = k4a_playback_seek_timestamp(handle,
                                         (int64_t)k4a_playback_get_last_timestamp_usec(handle) + 1,
                                         K4A_PLAYBACK_SEEK_BEGIN);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, config.depth_mode));
    k4a_capture_release(capture);

    // Test seeking to the middle of a capture, then read forward
    timestamps[0] -= timestamp_delta * 50;
    timestamps[1] -= timestamp_delta * 50;
    timestamps[2] -= timestamp_delta * 50;
    result = k4a_playback_seek_timestamp(handle, (int64_t)timestamps[0] + 500, K4A_PLAYBACK_SEEK_BEGIN);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // Color image is before the timestamp and will be missing.
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, K4A_COLOR_RESOLUTION_OFF, config.depth_mode));
    k4a_capture_release(capture);

    // Test seeking to the middle of a capture, then read backward
    result = k4a_playback_seek_timestamp(handle, (int64_t)timestamps[0] + 500, K4A_PLAYBACK_SEEK_BEGIN);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // Depth and IR images are before the timestamp and will be missing.
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, K4A_DEPTH_MODE_OFF));
    k4a_capture_release(capture);

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
    uint64_t timestamps[3] = { 1000000 - config.start_timestamp_offset_usec,
                               1001000 - config.start_timestamp_offset_usec,
                               1001000 - config.start_timestamp_offset_usec };
    uint64_t timestamp_delta = 1000000 / k4a_convert_fps_to_uint(config.camera_fps);

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
                                         -(int64_t)k4a_playback_get_last_timestamp_usec(handle) - 10,
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
                                         (int64_t)k4a_playback_get_last_timestamp_usec(handle) + 1,
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
    result = k4a_playback_seek_timestamp(handle, (int64_t)timestamps[0], K4A_PLAYBACK_SEEK_BEGIN);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    // i == 49, Depth image is dropped
    ASSERT_TRUE(
        validate_test_capture(capture, timestamps, config.color_format, config.color_resolution, K4A_DEPTH_MODE_OFF));
    k4a_capture_release(capture);

    // Test seek to middle of the recording, then read backward
    result = k4a_playback_seek_timestamp(handle, (int64_t)timestamps[0], K4A_PLAYBACK_SEEK_BEGIN);
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
    for (int i = 49; i < 100; i++)
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

TEST_F(playback_ut, DISABLED_open_test_file)
{
    k4a_playback_t handle;
    k4a_result_t result = k4a_playback_open("F:/test.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    uint8_t buffer[8096];
    size_t buffer_size = 8096;
    k4a_buffer_result_t buffer_result = k4a_playback_get_raw_calibration(handle, &buffer[0], &buffer_size);
    ASSERT_EQ(buffer_result, K4A_BUFFER_RESULT_SUCCEEDED);

    k4a_calibration_t calibration;
    result = k4a_playback_get_calibration(handle, &calibration);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    result = k4a_playback_get_calibration(handle, &calibration);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    std::cout << "Previous capture" << std::endl;
    k4a_capture_t capture = NULL;
    k4a_stream_result_t playback_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(playback_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, nullptr);

    std::cout << "Next capture x1000" << std::endl;
    for (int i = 0; i < 1000; i++)
    {
        playback_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(playback_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_NE(capture, nullptr);
        k4a_capture_release(capture);
    }
    std::cout << "Previous capture x1000" << std::endl;
    for (int i = 0; i < 999; i++)
    {
        playback_result = k4a_playback_get_previous_capture(handle, &capture);
        ASSERT_EQ(playback_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_NE(capture, nullptr);
        k4a_capture_release(capture);
    }
    playback_result = k4a_playback_get_previous_capture(handle, &capture);
    ASSERT_EQ(playback_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, nullptr);

    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    std::cout << "Config:" << std::endl;
    std::cout << "    Tracks enabled:";
    static const std::pair<bool *, std::string> tracks[] = { { &config.color_track_enabled, "Color" },
                                                             { &config.depth_track_enabled, "Depth" },
                                                             { &config.ir_track_enabled, "IR" },
                                                             { &config.imu_track_enabled, "IMU" } };
    for (int i = 0; i < 4; i++)
    {
        if (*tracks[i].first)
        {
            std::cout << " " << tracks[i].second;
        }
    }
    std::cout << std::endl;
    std::cout << "    Color format: " << format_names[config.color_format] << std::endl;
    std::cout << "    Color resolution: " << resolution_names[config.color_resolution] << std::endl;
    std::cout << "    Depth mode: " << depth_names[config.depth_mode] << std::endl;
    std::cout << "    Frame rate: " << fps_names[config.camera_fps] << std::endl;
    std::cout << "    Depth delay: " << config.depth_delay_off_color_usec << " usec" << std::endl;
    std::cout << "    Start offset: " << config.start_timestamp_offset_usec << " usec" << std::endl;

    k4a_playback_close(handle);
}

int main(int argc, char **argv)
{
    k4a_unittest_init();

    ::testing::AddGlobalTestEnvironment(new SampleRecordings());
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
