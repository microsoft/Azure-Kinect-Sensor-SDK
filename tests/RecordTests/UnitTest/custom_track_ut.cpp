// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "test_helpers.h"

#include <k4ainternal/matroska_common.h>

#include <cstdio>
#include <k4arecord/record.h>
#include <k4arecord/playback.h>

using namespace testing;

class custom_track_ut : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(custom_track_ut, open_custom_track_file)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_custom_track.mkv", &handle);
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
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)1000000);

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t timestamps_usec[3] = { 1000000, 1000000, 1000000 };
    for (size_t i = 0; i < test_frame_count; i++)
    {
        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps_usec,
                                          config.color_format,
                                          K4A_COLOR_RESOLUTION_OFF,
                                          config.depth_mode));
        k4a_capture_release(capture);
        timestamps_usec[0] += test_timestamp_delta_usec;
        timestamps_usec[1] += test_timestamp_delta_usec;
        timestamps_usec[2] += test_timestamp_delta_usec;
    }
    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    k4a_playback_close(handle);
}

TEST_F(custom_track_ut, list_available_tracks)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_custom_track.mkv", &handle);
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
    ASSERT_EQ(config.start_timestamp_offset_usec, (uint32_t)1000000);

    // Loop through all tracks and validate the name and codec id
    static const char *track_names[] = { "CUSTOM_TRACK", "CUSTOM_TRACK_HIGH_FREQ", "DEPTH", "IR" };
    static const char *track_codecs[] = { "S_K4A/CUSTOM_TRACK",
                                          "S_K4A/CUSTOM_TRACK",
                                          "V_MS/VFW/FOURCC",
                                          "V_MS/VFW/FOURCC" };
    static const bool track_builtin[] = { false, false, true, true };
    size_t track_count = k4a_playback_get_track_count(handle);

    ASSERT_EQ(k4a_playback_track_is_builtin(handle, "DEPTH"), true);
    ASSERT_EQ(k4a_playback_track_is_builtin(handle, "IR"), true);
    ASSERT_EQ(k4a_playback_track_is_builtin(handle, "CUSTOM_TRACK"), false);
    ASSERT_EQ(k4a_playback_track_is_builtin(handle, "CUSTOM_TRACK_HIGH_FREQ"), false);
    ASSERT_EQ(track_count, COUNTOF(track_names));

    for (size_t i = 0; i < track_count; i++)
    {
        char name[256];
        size_t name_len = 256;
        k4a_buffer_result_t buffer_result = k4a_playback_get_track_name(handle, i, name, &name_len);
        ASSERT_EQ(buffer_result, K4A_BUFFER_RESULT_SUCCEEDED);
        ASSERT_STREQ(name, track_names[i]);

        ASSERT_EQ(k4a_playback_track_is_builtin(handle, name), track_builtin[i]);

        char codec_id[256];
        size_t codec_id_len = 256;
        buffer_result = k4a_playback_track_get_codec_id(handle, name, codec_id, &codec_id_len);
        ASSERT_EQ(buffer_result, K4A_BUFFER_RESULT_SUCCEEDED);
        ASSERT_STREQ(codec_id, track_codecs[i]);
    }

    ASSERT_EQ(k4a_playback_track_is_builtin(handle, "DOES_NOT_EXIST"), false);

    k4a_playback_close(handle);
}

TEST_F(custom_track_ut, read_track_information)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_custom_track.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    // Read video track information
    k4a_record_video_settings_t video_settings;
    result = k4a_playback_track_get_video_settings(handle, "DEPTH", &video_settings);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(video_settings.width, test_depth_width);
    ASSERT_EQ(video_settings.height, test_depth_height);
    ASSERT_EQ(video_settings.frame_rate, test_camera_fps);

    result = k4a_playback_track_get_video_settings(handle, "IR", &video_settings);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(video_settings.width, test_depth_width);
    ASSERT_EQ(video_settings.height, test_depth_height);
    ASSERT_EQ(video_settings.frame_rate, test_camera_fps);

    result = k4a_playback_track_get_video_settings(handle, "CUSTOM_TRACK", &video_settings);
    ASSERT_EQ(result, K4A_RESULT_FAILED);

    // Read codec id
    size_t data_size = 0;
    std::vector<char> codec_id;
    ASSERT_EQ(k4a_playback_track_get_codec_id(handle, "DEPTH", nullptr, &data_size), K4A_BUFFER_RESULT_TOO_SMALL);
    codec_id.resize(data_size);
    ASSERT_EQ(k4a_playback_track_get_codec_id(handle, "DEPTH", codec_id.data(), &data_size),
              K4A_BUFFER_RESULT_SUCCEEDED);
    ASSERT_STREQ(codec_id.data(), "V_MS/VFW/FOURCC");

    ASSERT_EQ(k4a_playback_track_get_codec_id(handle, "CUSTOM_TRACK", nullptr, &data_size),
              K4A_BUFFER_RESULT_TOO_SMALL);
    codec_id.resize(data_size);
    ASSERT_EQ(k4a_playback_track_get_codec_id(handle, "CUSTOM_TRACK", codec_id.data(), &data_size),
              K4A_BUFFER_RESULT_SUCCEEDED);
    ASSERT_STREQ(codec_id.data(), "S_K4A/CUSTOM_TRACK");

    // Read private codec
    std::vector<uint8_t> codec_context;
    ASSERT_EQ(k4a_playback_track_get_codec_context(handle, "DEPTH", nullptr, &data_size), K4A_BUFFER_RESULT_TOO_SMALL);
    codec_context.resize(data_size);
    ASSERT_EQ(k4a_playback_track_get_codec_context(handle, "DEPTH", codec_context.data(), &data_size),
              K4A_BUFFER_RESULT_SUCCEEDED);

    const k4arecord::BITMAPINFOHEADER *depth_codec_header = reinterpret_cast<const k4arecord::BITMAPINFOHEADER *>(
        codec_context.data());
    ASSERT_EQ(depth_codec_header->biWidth, test_depth_width);
    ASSERT_EQ(depth_codec_header->biHeight, test_depth_height);
    ASSERT_EQ(depth_codec_header->biBitCount, (uint64_t)16);
    ASSERT_EQ(depth_codec_header->biCompression, static_cast<uint32_t>(0x32595559)); // YUY2 FOURCC
    ASSERT_EQ(depth_codec_header->biSizeImage, sizeof(uint16_t) * test_depth_width * test_depth_height);

    ASSERT_EQ(k4a_playback_track_get_codec_context(handle, "CUSTOM_TRACK", nullptr, &data_size),
              K4A_BUFFER_RESULT_TOO_SMALL);
    ASSERT_EQ(data_size, (size_t)0);

    k4a_playback_close(handle);
}

TEST_F(custom_track_ut, read_custom_track_data)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_custom_track.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t expected_timestamp_usec = (uint64_t)config.start_timestamp_offset_usec;
    for (size_t i = 0; i < test_frame_count; i++)
    {
        k4a_playback_data_block_t data_block = NULL;
        stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK", &data_block);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);

        uint64_t timestamp_usec = k4a_playback_data_block_get_device_timestamp_usec(data_block);
        ASSERT_EQ(timestamp_usec, expected_timestamp_usec);

        size_t block_size = k4a_playback_data_block_get_buffer_size(data_block);
        uint8_t *block_buffer = k4a_playback_data_block_get_buffer(data_block);

        ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, expected_timestamp_usec));

        k4a_playback_data_block_release(data_block);

        stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK_HIGH_FREQ", &data_block);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);

        timestamp_usec = k4a_playback_data_block_get_device_timestamp_usec(data_block);
        ASSERT_EQ(timestamp_usec, expected_timestamp_usec);

        block_size = k4a_playback_data_block_get_buffer_size(data_block);
        block_buffer = k4a_playback_data_block_get_buffer(data_block);

        ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, expected_timestamp_usec));

        k4a_playback_data_block_release(data_block);

        for (uint64_t j = 1; j < 10; j++)
        {
            uint64_t expected_timestamp_usec_high_freq = expected_timestamp_usec + j * test_timestamp_delta_usec / 10;
            stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK_HIGH_FREQ", &data_block);
            ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);

            timestamp_usec = k4a_playback_data_block_get_device_timestamp_usec(data_block);
            // This timestamp is estimated, allow for +/- 1 usec for rounding errors
            ASSERT_TRUE(timestamp_usec + 1 >= expected_timestamp_usec_high_freq &&
                        timestamp_usec <= expected_timestamp_usec_high_freq + 1);

            block_size = k4a_playback_data_block_get_buffer_size(data_block);
            block_buffer = k4a_playback_data_block_get_buffer(data_block);

            ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, expected_timestamp_usec_high_freq));

            k4a_playback_data_block_release(data_block);
        }

        expected_timestamp_usec += test_timestamp_delta_usec;
    }
    k4a_playback_data_block_t data_block = NULL;
    stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK", &data_block);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);

    stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK_HIGH_FREQ", &data_block);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);

    // After getting K4A_STREAM_RESULT_EOF, calling k4a_playback_get_previous_data_block is expected to get the last
    // frame
    stream_result = k4a_playback_get_previous_data_block(handle, "CUSTOM_TRACK", &data_block);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    uint64_t timestamp_usec = k4a_playback_data_block_get_device_timestamp_usec(data_block);
    ASSERT_EQ(timestamp_usec,
              (uint64_t)config.start_timestamp_offset_usec + test_timestamp_delta_usec * (test_frame_count - 1));
    size_t block_size = k4a_playback_data_block_get_buffer_size(data_block);
    uint8_t *block_buffer = k4a_playback_data_block_get_buffer(data_block);
    ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, timestamp_usec));
    k4a_playback_data_block_release(data_block);

    stream_result = k4a_playback_get_previous_data_block(handle, "CUSTOM_TRACK_HIGH_FREQ", &data_block);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    timestamp_usec = k4a_playback_data_block_get_device_timestamp_usec(data_block);
    ASSERT_EQ(timestamp_usec,
              (uint64_t)config.start_timestamp_offset_usec + test_timestamp_delta_usec * test_frame_count -
                  (test_timestamp_delta_usec / 10) - 1);
    block_size = k4a_playback_data_block_get_buffer_size(data_block);
    block_buffer = k4a_playback_data_block_get_buffer(data_block);
    ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, timestamp_usec));
    k4a_playback_data_block_release(data_block);

    k4a_playback_close(handle);
}

TEST_F(custom_track_ut, seek_custom_track_frame)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_custom_track.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    std::vector<size_t> seek_frame_indices = { 200, 2, 4, 7, 1, 10, 0, 200, 3 };
    std::vector<uint64_t> seek_timestamps_usec(seek_frame_indices.size());
    for (size_t i = 0; i < seek_frame_indices.size(); i++)
    {
        seek_timestamps_usec[i] = (uint64_t)config.start_timestamp_offset_usec +
                                  seek_frame_indices[i] * test_timestamp_delta_usec;
    }
    uint64_t max_seek_timestamp = k4a_playback_get_recording_length_usec(handle) +
                                  (uint64_t)config.start_timestamp_offset_usec;

    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    for (size_t i = 0; i < seek_timestamps_usec.size(); i++)
    {
        result = k4a_playback_seek_timestamp(handle, (int64_t)seek_timestamps_usec[i], K4A_PLAYBACK_SEEK_DEVICE_TIME);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_playback_data_block_t data_block = NULL;
        stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK", &data_block);

        if (seek_timestamps_usec[i] > max_seek_timestamp)
        {
            ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
        }
        else
        {
            ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);

            uint64_t timestamp_usec = k4a_playback_data_block_get_device_timestamp_usec(data_block);
            ASSERT_EQ(timestamp_usec, seek_timestamps_usec[i]);

            size_t block_size = k4a_playback_data_block_get_buffer_size(data_block);
            uint8_t *block_buffer = k4a_playback_data_block_get_buffer(data_block);

            ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, timestamp_usec));

            k4a_playback_data_block_release(data_block);
        }
    }

    k4a_playback_close(handle);
}

TEST_F(custom_track_ut, seek_custom_track_high_frequency)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_custom_track.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    uint64_t seek_timestamp_usec = (uint64_t)config.start_timestamp_offset_usec;
    uint64_t last_timestamp_usec_high_freq = 0;
    uint64_t max_seek_timestamp = k4a_playback_get_recording_length_usec(handle) +
                                  (uint64_t)config.start_timestamp_offset_usec;

    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    for (size_t i = 0; i < test_frame_count; i++)
    {
        for (size_t j = 0; j < 10; j++)
        {
            uint64_t expected_timestamp_usec_high_freq = seek_timestamp_usec + j * test_timestamp_delta_usec / 10;

            // Seek + read forward
            result = k4a_playback_seek_timestamp(handle,
                                                 (int64_t)expected_timestamp_usec_high_freq - 10,
                                                 K4A_PLAYBACK_SEEK_DEVICE_TIME);
            ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

            k4a_playback_data_block_t data_block = NULL;
            stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK_HIGH_FREQ", &data_block);

            if ((int64_t)expected_timestamp_usec_high_freq - 10 > (int64_t)max_seek_timestamp)
            {
                ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
            }
            else
            {
                ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);

                uint64_t timestamp_usec = k4a_playback_data_block_get_device_timestamp_usec(data_block);
                // This timestamp is estimated, allow for +/- 1 usec for rounding errors
                ASSERT_TRUE(timestamp_usec + 1 >= expected_timestamp_usec_high_freq &&
                            timestamp_usec <= expected_timestamp_usec_high_freq + 1);

                size_t block_size = k4a_playback_data_block_get_buffer_size(data_block);
                uint8_t *block_buffer = k4a_playback_data_block_get_buffer(data_block);

                ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, expected_timestamp_usec_high_freq));

                k4a_playback_data_block_release(data_block);
            }

            // Seek + read backward
            result = k4a_playback_seek_timestamp(handle,
                                                 (int64_t)expected_timestamp_usec_high_freq - 10,
                                                 K4A_PLAYBACK_SEEK_DEVICE_TIME);
            ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

            stream_result = k4a_playback_get_previous_data_block(handle, "CUSTOM_TRACK_HIGH_FREQ", &data_block);

            if ((int64_t)expected_timestamp_usec_high_freq - 10 <= (int64_t)config.start_timestamp_offset_usec)
            {
                ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
            }
            else
            {
                ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);

                uint64_t timestamp_usec = k4a_playback_data_block_get_device_timestamp_usec(data_block);
                // This timestamp is estimated, allow for +/- 1 usec for rounding errors
                ASSERT_TRUE(timestamp_usec + 1 >= last_timestamp_usec_high_freq &&
                            timestamp_usec <= last_timestamp_usec_high_freq + 1);

                size_t block_size = k4a_playback_data_block_get_buffer_size(data_block);
                uint8_t *block_buffer = k4a_playback_data_block_get_buffer(data_block);

                ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, last_timestamp_usec_high_freq));

                k4a_playback_data_block_release(data_block);
            }

            last_timestamp_usec_high_freq = expected_timestamp_usec_high_freq;
        }

        seek_timestamp_usec += test_timestamp_delta_usec;
    }

    k4a_playback_close(handle);
}

int main(int argc, char **argv)
{
    k4a_unittest_init();

    ::testing::AddGlobalTestEnvironment(new CustomTrackRecordings());
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
