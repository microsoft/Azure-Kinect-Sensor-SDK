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

    k4a_device_configuration_t record_config_color_only = record_config_full;
    record_config_color_only.depth_mode = K4A_DEPTH_MODE_OFF;

    k4a_device_configuration_t record_config_depth_only = record_config_full;
    record_config_depth_only.color_resolution = K4A_COLOR_RESOLUTION_OFF;

    k4a_device_configuration_t record_config_bgra_color = record_config_full;
    record_config_bgra_color.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    record_config_bgra_color.depth_mode = K4A_DEPTH_MODE_OFF;

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
        uint32_t timestamp_delta = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(record_config_full.camera_fps));
        k4a_capture_t capture = NULL;
        for (size_t i = 0; i < test_frame_count; i++)
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
        uint32_t timestamp_delta = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(record_config_delay.camera_fps));
        k4a_capture_t capture = NULL;
        for (size_t i = 0; i < test_frame_count; i++)
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
    { // Create a recording file with a subordinate delay off master
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_sub.mkv", NULL, record_config_sub, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { record_config_sub.subordinate_delay_off_master_usec,
                                   record_config_sub.subordinate_delay_off_master_usec,
                                   record_config_sub.subordinate_delay_off_master_usec };
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
        uint32_t timestamp_delta = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(record_config_full.camera_fps));
        for (size_t i = 0; i < test_frame_count; i++)
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
    { // Create a recording file with a start offset and all tracks enabled
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_offset.mkv", NULL, record_config_full, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_add_imu_track(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { 1000000, 1000000, 1000000 };
        uint64_t imu_timestamp = 1001150;
        uint32_t timestamp_delta = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(record_config_delay.camera_fps));
        k4a_capture_t capture = NULL;
        for (size_t i = 0; i < test_frame_count; i++)
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
    { // Create a recording file with only the color camera enabled
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_color_only.mkv", NULL, record_config_color_only, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { 0, 0, 0 };
        k4a_capture_t capture = create_test_capture(timestamps,
                                                    record_config_color_only.color_format,
                                                    record_config_color_only.color_resolution,
                                                    record_config_color_only.depth_mode);
        result = k4a_record_write_capture(handle, capture);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
        k4a_capture_release(capture);

        result = k4a_record_flush(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_record_close(handle);
    }
    { // Create a recording file with only the depth camera enabled
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_depth_only.mkv", NULL, record_config_depth_only, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { 0, 0, 0 };
        k4a_capture_t capture = create_test_capture(timestamps,
                                                    record_config_depth_only.color_format,
                                                    record_config_depth_only.color_resolution,
                                                    record_config_depth_only.depth_mode);
        result = k4a_record_write_capture(handle, capture);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
        k4a_capture_release(capture);

        result = k4a_record_flush(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_record_close(handle);
    }
    { // Create a recording file with BGRA color
        k4a_record_t handle = NULL;
        k4a_result_t result = k4a_record_create("record_test_bgra_color.mkv", NULL, record_config_bgra_color, &handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_header(handle);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint64_t timestamps[3] = { 0, 0, 0 };
        k4a_capture_t capture = create_test_capture(timestamps,
                                                    record_config_bgra_color.color_format,
                                                    record_config_bgra_color.color_resolution,
                                                    record_config_bgra_color.depth_mode);
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
    ASSERT_EQ(std::remove("record_test_empty.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_full.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_delay.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_skips.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_sub.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_offset.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_color_only.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_depth_only.mkv"), 0);
    ASSERT_EQ(std::remove("record_test_bgra_color.mkv"), 0);
}

void CustomTrackRecordings::SetUp()
{
    // Use custom track recording API to create a recording with Depth and IR tracks
    k4a_record_t handle = NULL;
    k4a_result_t result =
        k4a_record_create("record_test_custom_track.mkv", NULL, K4A_DEVICE_CONFIG_INIT_DISABLE_ALL, &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4arecord::BITMAPINFOHEADER depth_codec_header;
    depth_codec_header.biWidth = test_depth_width;
    depth_codec_header.biHeight = test_depth_height;
    depth_codec_header.biBitCount = 16;
    // YUY2 FOURCC (no longer used for recording, but still expected to be supported).
    depth_codec_header.biCompression = 0x32595559;
    depth_codec_header.biSizeImage = sizeof(uint16_t) * test_depth_width * test_depth_height;

    k4a_record_video_settings_t depth_video_settings;
    depth_video_settings.width = test_depth_width;
    depth_video_settings.height = test_depth_height;
    depth_video_settings.frame_rate = test_camera_fps;

    // Create the normally built-in DEPTH and IR tracks using the custom track API
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

    // Add two subtitle tracks with and without the high-frequency data flag
    k4a_record_subtitle_settings_t subtitle_settings;

    subtitle_settings.high_freq_data = false;
    result = k4a_record_add_custom_subtitle_track(handle,
                                                  "CUSTOM_TRACK",
                                                  "S_K4A/CUSTOM_TRACK",
                                                  nullptr,
                                                  0,
                                                  &subtitle_settings);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    subtitle_settings.high_freq_data = true;
    result = k4a_record_add_custom_subtitle_track(handle,
                                                  "CUSTOM_TRACK_HIGH_FREQ",
                                                  "S_K4A/CUSTOM_TRACK",
                                                  nullptr,
                                                  0,
                                                  &subtitle_settings);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    result = k4a_record_add_tag(handle, "CUSTOM_TRACK_VERSION", "1.0.0");
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    result = k4a_record_add_tag(handle, "CUSTOM_TRACK_HIGH_FREQ_VERSION", "1.1.0");
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    ASSERT_EQ(k4a_record_write_header(handle), K4A_RESULT_SUCCEEDED);

    uint64_t timestamp_usec = 1000000;
    for (size_t i = 0; i < test_frame_count; i++)
    {
        // Write Depth + IR tracks with custom data
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
                                                    k4a_image_get_device_timestamp_usec(depth_image),
                                                    k4a_image_get_buffer(depth_image),
                                                    static_cast<uint32_t>(k4a_image_get_size(depth_image)));
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        result = k4a_record_write_custom_track_data(handle,
                                                    "IR",
                                                    k4a_image_get_device_timestamp_usec(ir_image),
                                                    k4a_image_get_buffer(ir_image),
                                                    static_cast<uint32_t>(k4a_image_get_size(ir_image)));
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        // Write data to the custom subtitle tracks
        std::vector<uint8_t> custom_track_block = create_test_custom_track_block(timestamp_usec);
        result = k4a_record_write_custom_track_data(handle,
                                                    "CUSTOM_TRACK",
                                                    timestamp_usec,
                                                    custom_track_block.data(),
                                                    custom_track_block.size());
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        // Write the high frequency track at 10x the rate of the regular track.
        for (uint64_t j = 0; j < 10; j++)
        {
            uint64_t timestamp_usec_high_freq = timestamp_usec + j * test_timestamp_delta_usec / 10;
            custom_track_block = create_test_custom_track_block(timestamp_usec_high_freq);
            result = k4a_record_write_custom_track_data(handle,
                                                        "CUSTOM_TRACK_HIGH_FREQ",
                                                        timestamp_usec_high_freq,
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

void CustomTrackRecordings::TearDown()
{
    ASSERT_EQ(std::remove("record_test_custom_track.mkv"), 0);
}
