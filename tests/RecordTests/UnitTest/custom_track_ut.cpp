// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "test_helpers.h"

#include <cstdio>
#include <k4arecord/record.h>
#include <k4arecord/playback.h>

using namespace testing;

struct BITMAPINFOHEADER
{
    uint32_t biSize = sizeof(BITMAPINFOHEADER);
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes = 1;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter = 0;
    uint32_t biYPelsPerMeter = 0;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
};

// Testing values
const uint32_t test_depth_width = 640;
const uint32_t test_depth_height = 576;
const uint32_t test_camera_fps = 30;
const uint32_t test_timestamp_delta_usec = 33333;
const size_t max_frame_index = 10;
const size_t custom_track_start_index = 2;

class CustomTrackRecordings : public ::testing::Environment
{
public:
    ~CustomTrackRecordings() override {}

protected:
    void SetUp() override
    {
        // Use custom track recording API to create a recording with Depth and IR tracks
        k4a_record_t handle = NULL;
        k4a_result_t result =
            k4a_record_create("record_test_custom_track.mkv", NULL, K4A_DEVICE_CONFIG_INIT_DISABLE_ALL, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        BITMAPINFOHEADER depth_codec_header;
        depth_codec_header.biWidth = test_depth_width;
        depth_codec_header.biHeight = test_depth_height;
        depth_codec_header.biBitCount = 16;
        depth_codec_header.biCompression = 0x32595559; // YUY2 little endian
        depth_codec_header.biSizeImage = sizeof(uint16_t) * test_depth_width * test_depth_height;

        k4a_record_video_settings_t depth_video_settings;
        depth_video_settings.width = test_depth_width;
        depth_video_settings.height = test_depth_height;
        depth_video_settings.frame_rate = test_camera_fps;

        // Add custom tracks to the k4a_record_t
        result = k4a_record_add_custom_video_track(handle,
                                                   "DEPTH",
                                                   "V_MS/VFW/FOURCC",
                                                   reinterpret_cast<uint8_t *>(&depth_codec_header),
                                                   sizeof(depth_codec_header),
                                                   &depth_video_settings);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_add_custom_video_track(handle,
                                                   "IR",
                                                   "V_MS/VFW/FOURCC",
                                                   reinterpret_cast<uint8_t *>(&depth_codec_header),
                                                   sizeof(depth_codec_header),
                                                   &depth_video_settings);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_add_tag(handle, "K4A_DEPTH_MODE", "NFOV_UNBINNED");
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result =
            k4a_record_add_custom_subtitle_track(handle, "CUSTOM_TRACK_1", "S_K4A/CUSTOM_TRACK_1", nullptr, 0, nullptr);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_add_tag(handle, "CUSTOM_TRACK_1_VERSION", "1.0.0");
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        ASSERT_EQ(k4a_record_write_header(handle), K4A_RESULT_SUCCEEDED);

        uint64_t timestamp_usec = 0;
        for (size_t i = 0; i < max_frame_index; i++)
        {
            k4a_image_t depth_image = create_test_image(timestamp_usec,
                                                        K4A_IMAGE_FORMAT_DEPTH16,
                                                        test_depth_width,
                                                        test_depth_height,
                                                        sizeof(uint16_t) * test_depth_width);

            k4a_image_t ir_image = create_test_image(timestamp_usec,
                                                     K4A_IMAGE_FORMAT_IR16,
                                                     test_depth_width,
                                                     test_depth_height,
                                                     sizeof(uint16_t) * test_depth_width);

            result = k4a_record_write_custom_track_data(handle,
                                                        "DEPTH",
                                                        k4a_image_get_timestamp_usec(depth_image),
                                                        k4a_image_get_buffer(depth_image),
                                                        static_cast<uint32_t>(k4a_image_get_size(depth_image)));
            ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

            result = k4a_record_write_custom_track_data(handle,
                                                        "IR",
                                                        k4a_image_get_timestamp_usec(ir_image),
                                                        k4a_image_get_buffer(ir_image),
                                                        static_cast<uint32_t>(k4a_image_get_size(ir_image)));
            ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

            // Test custom track to start from different index
            if (i >= custom_track_start_index)
            {
                std::vector<uint8_t> custom_track_block = create_test_custom_track_block(timestamp_usec);
                result = k4a_record_write_custom_track_data(handle,
                                                            "CUSTOM_TRACK_1",
                                                            timestamp_usec,
                                                            custom_track_block.data(),
                                                            custom_track_block.size());
                ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
            }

            k4a_image_release(depth_image);
            k4a_image_release(ir_image);

            timestamp_usec += test_timestamp_delta_usec;
        }

        result = k4a_record_flush(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_record_close(handle);
    }

    void TearDown() override
    {
        ASSERT_EQ(std::remove("record_test_custom_track.mkv"), 0);
    }
};

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
    ASSERT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_OFF);
    ASSERT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
    ASSERT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
    ASSERT_FALSE(config.color_track_enabled);
    ASSERT_TRUE(config.depth_track_enabled);
    ASSERT_TRUE(config.ir_track_enabled);
    ASSERT_FALSE(config.imu_track_enabled);

    k4a_capture_t capture = NULL;
    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t timestamps_usec[3] = { 0, 0, 0 };
    for (size_t i = 0; i < max_frame_index; i++)
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

    result = k4a_playback_track_get_video_settings(handle, "CUSTOM_TRACK_1", &video_settings);
    ASSERT_EQ(result, K4A_RESULT_FAILED);

    // Read codec id
    size_t data_size = 0;
    std::vector<char> codec_id;
    ASSERT_EQ(k4a_playback_track_get_codec_id(handle, "DEPTH", nullptr, &data_size), K4A_BUFFER_RESULT_TOO_SMALL);
    codec_id.resize(data_size);
    ASSERT_EQ(k4a_playback_track_get_codec_id(handle, "DEPTH", codec_id.data(), &data_size),
              K4A_BUFFER_RESULT_SUCCEEDED);
    ASSERT_STREQ(reinterpret_cast<char *>(codec_id.data()), "V_MS/VFW/FOURCC");

    ASSERT_EQ(k4a_playback_track_get_codec_id(handle, "CUSTOM_TRACK_1", nullptr, &data_size),
              K4A_BUFFER_RESULT_TOO_SMALL);
    codec_id.resize(data_size);
    ASSERT_EQ(k4a_playback_track_get_codec_id(handle, "CUSTOM_TRACK_1", codec_id.data(), &data_size),
              K4A_BUFFER_RESULT_SUCCEEDED);
    ASSERT_STREQ(reinterpret_cast<char *>(codec_id.data()), "S_K4A/CUSTOM_TRACK_1");

    // Read private codec
    std::vector<uint8_t> codec_context;
    ASSERT_EQ(k4a_playback_track_get_codec_context(handle, "DEPTH", nullptr, &data_size), K4A_BUFFER_RESULT_TOO_SMALL);
    codec_context.resize(data_size);
    ASSERT_EQ(k4a_playback_track_get_codec_context(handle, "DEPTH", codec_context.data(), &data_size),
              K4A_BUFFER_RESULT_SUCCEEDED);

    const BITMAPINFOHEADER *depth_codec_header = reinterpret_cast<const BITMAPINFOHEADER *>(codec_context.data());
    ASSERT_EQ(depth_codec_header->biWidth, test_depth_width);
    ASSERT_EQ(depth_codec_header->biHeight, test_depth_height);
    ASSERT_EQ(depth_codec_header->biBitCount, (uint64_t)16);
    ASSERT_EQ(depth_codec_header->biCompression, static_cast<uint32_t>(0x32595559)); // YUY2 little endian
    ASSERT_EQ(depth_codec_header->biSizeImage, sizeof(uint16_t) * test_depth_width * test_depth_height);

    ASSERT_EQ(k4a_playback_track_get_codec_context(handle, "CUSTOM_TRACK_1", nullptr, &data_size),
              K4A_BUFFER_RESULT_TOO_SMALL);
    ASSERT_EQ(data_size, (size_t)0);

    k4a_playback_close(handle);
}

TEST_F(custom_track_ut, read_custom_track_data)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_custom_track.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    uint64_t expected_timestamp_usec = 0;
    for (size_t i = 0; i < max_frame_index; i++)
    {
        if (i >= custom_track_start_index)
        {
            k4a_playback_data_block_t data_block = NULL;
            stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK_1", &data_block);
            ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);

            uint64_t timestamp_usec = k4a_playback_data_block_get_timestamp_usec(data_block);
            ASSERT_EQ(timestamp_usec, expected_timestamp_usec);

            size_t block_size = k4a_playback_data_block_get_buffer_size(data_block);
            uint8_t *block_buffer = k4a_playback_data_block_get_buffer(data_block);

            ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, timestamp_usec));

            k4a_playback_data_block_release(data_block);
        }

        expected_timestamp_usec += test_timestamp_delta_usec;
    }
    k4a_playback_data_block_t data_block = NULL;
    stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK_1", &data_block);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);

    // After getting K4A_STREAM_RESULT_EOF, calling k4a_playback_get_previous_data_block is expected to get the last
    // frame
    stream_result = k4a_playback_get_previous_data_block(handle, "CUSTOM_TRACK_1", &data_block);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
    uint64_t last_frame_expected_timestamp_usec = test_timestamp_delta_usec * (max_frame_index - 1);
    uint64_t timestamp_usec = k4a_playback_data_block_get_timestamp_usec(data_block);
    ASSERT_EQ(timestamp_usec, last_frame_expected_timestamp_usec);
    size_t block_size = k4a_playback_data_block_get_buffer_size(data_block);
    uint8_t *block_buffer = k4a_playback_data_block_get_buffer(data_block);
    ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, timestamp_usec));
    k4a_playback_data_block_release(data_block);

    k4a_playback_close(handle);
}

TEST_F(custom_track_ut, seek_custom_track_frame)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = k4a_playback_open("record_test_custom_track.mkv", &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    std::vector<size_t> seek_frame_indices = { 20, 2, 4, 7, 1, 10, 0, 20, 3 };
    std::vector<uint64_t> seek_timestamps_usec(seek_frame_indices.size());
    for (size_t i = 0; i < seek_frame_indices.size(); i++)
    {
        seek_timestamps_usec[i] = (custom_track_start_index + seek_frame_indices[i]) * test_timestamp_delta_usec;
    }
    uint64_t max_seek_timestamp = k4a_playback_get_last_timestamp_usec(handle);

    k4a_stream_result_t stream_result = K4A_STREAM_RESULT_FAILED;
    for (size_t i = 0; i < seek_timestamps_usec.size(); i++)
    {
        result = k4a_playback_seek_timestamp(handle, (int64_t)seek_timestamps_usec[i], K4A_PLAYBACK_SEEK_BEGIN);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_playback_data_block_t data_block = NULL;
        stream_result = k4a_playback_get_next_data_block(handle, "CUSTOM_TRACK_1", &data_block);

        if (seek_timestamps_usec[i] > max_seek_timestamp)
        {
            ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
            continue;
        }
        else
        {
            ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        }

        uint64_t timestamp_usec = k4a_playback_data_block_get_timestamp_usec(data_block);
        ASSERT_EQ(timestamp_usec, seek_timestamps_usec[i]);

        size_t block_size = k4a_playback_data_block_get_buffer_size(data_block);
        uint8_t *block_buffer = k4a_playback_data_block_get_buffer(data_block);

        ASSERT_TRUE(validate_custom_track_block(block_buffer, block_size, timestamp_usec));

        k4a_playback_data_block_release(data_block);
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
