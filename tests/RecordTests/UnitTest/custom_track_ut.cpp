// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "test_helpers.h"

#include <cstdio>
#include <k4arecord/record.h>
#include <k4arecord/playback.h>
#include <k4aexperiment/record_experiment.h>

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
const uint32_t test_timestamp_delta = 33333;

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

        k4a_record_video_info_t depth_video_info;
        depth_video_info.width = test_depth_width;
        depth_video_info.height = test_depth_height;
        depth_video_info.frame_rate = test_camera_fps;

        // Add custom tracks to the k4a_record_t
        result = k4a_record_add_custom_track(handle,
                                             "DEPTH",
                                             K4A_RECORD_TRACK_TYPE_VIDEO,
                                             "V_MS/VFW/FOURCC",
                                             reinterpret_cast<uint8_t *>(&depth_codec_header),
                                             sizeof(depth_codec_header));
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_set_custom_track_info_video(handle, "DEPTH", &depth_video_info);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_add_custom_track(handle,
                                             "IR",
                                             K4A_RECORD_TRACK_TYPE_VIDEO,
                                             "V_MS/VFW/FOURCC",
                                             reinterpret_cast<uint8_t *>(&depth_codec_header),
                                             sizeof(depth_codec_header));
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
        result = k4a_record_set_custom_track_info_video(handle, "IR", &depth_video_info);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_add_custom_track_tag(handle, "DEPTH", "K4A_DEPTH_MODE", "NFOV_UNBINNED");
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        ASSERT_EQ(k4a_record_write_header(handle), K4A_RESULT_SUCCEEDED);

        uint64_t timestamp = 0;
        for (int i = 0; i < 10; i++)
        {
            k4a_image_t depth_image = create_test_image(timestamp,
                                                        K4A_IMAGE_FORMAT_DEPTH16,
                                                        test_depth_width,
                                                        test_depth_height,
                                                        sizeof(uint16_t) * test_depth_width);

            k4a_image_t ir_image = create_test_image(timestamp,
                                                     K4A_IMAGE_FORMAT_IR16,
                                                     test_depth_width,
                                                     test_depth_height,
                                                     sizeof(uint16_t) * test_depth_width);

            result = k4a_record_write_custom_track_data(handle,
                                                        "DEPTH",
                                                        k4a_image_get_timestamp_usec(depth_image) * 1000,
                                                        k4a_image_get_buffer(depth_image),
                                                        static_cast<uint32_t>(k4a_image_get_size(depth_image)),
                                                        true);
            ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

            result = k4a_record_write_custom_track_data(handle,
                                                        "IR",
                                                        k4a_image_get_timestamp_usec(ir_image) * 1000,
                                                        k4a_image_get_buffer(ir_image),
                                                        static_cast<uint32_t>(k4a_image_get_size(ir_image)),
                                                        true);
            ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

            k4a_image_release(depth_image);
            k4a_image_release(ir_image);

            timestamp += test_timestamp_delta;
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
    uint64_t timestamps[3] = { 0, 0, 0 };
    for (int i = 0; i < 10; i++)
    {
        stream_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_EQ(stream_result, K4A_STREAM_RESULT_SUCCEEDED);
        ASSERT_TRUE(validate_test_capture(capture,
                                          timestamps,
                                          config.color_format,
                                          K4A_COLOR_RESOLUTION_OFF,
                                          config.depth_mode));
        k4a_capture_release(capture);
        timestamps[0] += test_timestamp_delta;
        timestamps[1] += test_timestamp_delta;
        timestamps[2] += test_timestamp_delta;
    }
    stream_result = k4a_playback_get_next_capture(handle, &capture);
    ASSERT_EQ(stream_result, K4A_STREAM_RESULT_EOF);
    ASSERT_EQ(capture, (k4a_capture_t)NULL);

    k4a_playback_close(handle);
}

int main(int argc, char **argv)
{
    k4a_unittest_init();

    ::testing::AddGlobalTestEnvironment(new CustomTrackRecordings());
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
