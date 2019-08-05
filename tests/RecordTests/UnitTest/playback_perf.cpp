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

static std::string g_test_file_name;

class playback_perf : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(playback_perf, test_open)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = K4A_RESULT_FAILED;
    {
        Timer t("File open: " + g_test_file_name);
        result = k4a_playback_open(g_test_file_name.c_str(), &handle);
    }
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    std::cout << "Record config:" << std::endl;
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

    std::pair<k4a_image_t, std::string> images[] = { { NULL, "Color" }, { NULL, "Depth" }, { NULL, "IR" } };
    while (images[0].first == NULL || images[1].first == NULL || images[2].first == NULL)
    {
        k4a_capture_t capture = NULL;
        k4a_stream_result_t playback_result = k4a_playback_get_next_capture(handle, &capture);
        ASSERT_NE(playback_result, K4A_STREAM_RESULT_FAILED);
        if (playback_result == K4A_STREAM_RESULT_EOF)
        {
            break;
        }
        else
        {
            ASSERT_NE(capture, nullptr);
            if (images[0].first == NULL)
            {
                images[0].first = k4a_capture_get_color_image(capture);
            }
            if (images[1].first == NULL)
            {
                images[1].first = k4a_capture_get_depth_image(capture);
            }
            if (images[2].first == NULL)
            {
                images[2].first = k4a_capture_get_ir_image(capture);
            }
            k4a_capture_release(capture);
        }
    }
    if (images[0].first == NULL && images[1].first == NULL && images[2].first == NULL)
    {
        std::cout << std::endl;
        std::cout << "Input file has no captures." << std::endl;
    }
    else
    {
        for (size_t i = 0; i < arraysize(images); i++)
        {
            if (images[i].first)
            {
                std::cout << std::endl;
                std::cout << "First " << images[i].second << " image:" << std::endl;
                std::cout << "    Timestamp: " << k4a_image_get_device_timestamp_usec(images[i].first) << " usec"
                          << std::endl;
                std::cout << "    Image format: " << format_names[k4a_image_get_format(images[i].first)] << std::endl;
                std::cout << "    Resolution: " << k4a_image_get_width_pixels(images[i].first) << "x"
                          << k4a_image_get_height_pixels(images[i].first) << std::endl;
                std::cout << "    Buffer size: " << k4a_image_get_size(images[i].first)
                          << " (stride: " << k4a_image_get_stride_bytes(images[i].first) << " bytes)" << std::endl;
                k4a_image_release(images[i].first);
            }
        }
    }

    if (config.imu_track_enabled)
    {
        k4a_imu_sample_t imu_sample = { 0 };
        k4a_stream_result_t playback_result = k4a_playback_get_next_imu_sample(handle, &imu_sample);
        ASSERT_NE(playback_result, K4A_STREAM_RESULT_FAILED);
        if (playback_result == K4A_STREAM_RESULT_EOF)
        {
            std::cout << "No IMU data in recording." << std::endl;
        }
        else
        {
            std::cout << std::endl;
            std::cout << "First IMU sample:" << std::endl;
            std::cout << "    Accel Timestamp: " << imu_sample.acc_timestamp_usec << " usec" << std::endl;
            std::cout << "    Accel Data: (" << imu_sample.acc_sample.xyz.x << ", " << imu_sample.acc_sample.xyz.y
                      << ", " << imu_sample.acc_sample.xyz.z << ")" << std::endl;
            std::cout << "    Gyro Timestamp: " << imu_sample.gyro_timestamp_usec << " usec" << std::endl;
            std::cout << "    Gyro Data: (" << imu_sample.gyro_sample.xyz.x << ", " << imu_sample.gyro_sample.xyz.y
                      << ", " << imu_sample.gyro_sample.xyz.z << ")" << std::endl;
        }
    }

    k4a_playback_close(handle);
}

TEST_F(playback_perf, test_1000_reads_forward)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = K4A_RESULT_FAILED;
    {
        Timer t("File open: " + g_test_file_name);
        result = k4a_playback_open(g_test_file_name.c_str(), &handle);
    }
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    {
        k4a_capture_t capture = NULL;
        k4a_stream_result_t playback_result = K4A_STREAM_RESULT_FAILED;
        Timer t("Next capture x1000");
        for (int i = 0; i < 1000; i++)
        {
            playback_result = k4a_playback_get_next_capture(handle, &capture);
            ASSERT_NE(playback_result, K4A_STREAM_RESULT_FAILED);
            if (playback_result == K4A_STREAM_RESULT_EOF)
            {
                std::cout << "    Warning: Input file is too short, only read " << i << " captures." << std::endl;
                break;
            }
            ASSERT_NE(capture, nullptr);
            k4a_capture_release(capture);
        }
    }

    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    if (config.imu_track_enabled)
    {
        k4a_imu_sample_t sample = { 0 };
        k4a_stream_result_t playback_result = K4A_STREAM_RESULT_FAILED;
        Timer t("Next imu smaple x10000");
        for (int i = 0; i < 10000; i++)
        {
            playback_result = k4a_playback_get_next_imu_sample(handle, &sample);
            ASSERT_NE(playback_result, K4A_STREAM_RESULT_FAILED);
            if (playback_result == K4A_STREAM_RESULT_EOF)
            {
                std::cout << "    Warning: Input file is too short, only read " << i << " imu_samples." << std::endl;
                break;
            }
        }
    }

    k4a_playback_close(handle);
}

TEST_F(playback_perf, test_1000_reads_backward)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = K4A_RESULT_FAILED;
    {
        Timer t("File open: " + g_test_file_name);
        result = k4a_playback_open(g_test_file_name.c_str(), &handle);
    }
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    {
        Timer t("Seek to end");
        result = k4a_playback_seek_timestamp(handle, 0, K4A_PLAYBACK_SEEK_END);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    }

    {
        k4a_capture_t capture = NULL;
        k4a_stream_result_t playback_result = K4A_STREAM_RESULT_FAILED;
        Timer t("Previous capture x1000");
        for (int i = 0; i < 1000; i++)
        {
            playback_result = k4a_playback_get_previous_capture(handle, &capture);
            ASSERT_NE(playback_result, K4A_STREAM_RESULT_FAILED);
            if (playback_result == K4A_STREAM_RESULT_EOF)
            {
                std::cout << "    Warning: Input file is too short, only read " << i << " captures." << std::endl;
                break;
            }
            ASSERT_NE(capture, nullptr);
            k4a_capture_release(capture);
        }
    }

    k4a_record_configuration_t config;
    result = k4a_playback_get_record_configuration(handle, &config);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    if (config.imu_track_enabled)
    {
        k4a_imu_sample_t sample = { 0 };
        k4a_stream_result_t playback_result = K4A_STREAM_RESULT_FAILED;
        Timer t("Previous imu smaple x10000");
        for (int i = 0; i < 10000; i++)
        {
            playback_result = k4a_playback_get_previous_imu_sample(handle, &sample);
            ASSERT_NE(playback_result, K4A_STREAM_RESULT_FAILED);
            if (playback_result == K4A_STREAM_RESULT_EOF)
            {
                std::cout << "    Warning: Input file is too short, only read " << i << " imu_samples." << std::endl;
                break;
            }
        }
    }

    k4a_playback_close(handle);
}

TEST_F(playback_perf, test_read_latency_30fps)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = K4A_RESULT_FAILED;
    {
        Timer t("File open: " + g_test_file_name);
        result = k4a_playback_open(g_test_file_name.c_str(), &handle);
    }
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    std::vector<int64_t> deltas;

    {
        k4a_capture_t capture = NULL;
        k4a_stream_result_t playback_result = K4A_STREAM_RESULT_FAILED;
        Timer t("Next capture x1000");
        for (int i = 0; i < 1000; i++)
        {

            auto start = std::chrono::high_resolution_clock::now();
            playback_result = k4a_playback_get_next_capture(handle, &capture);
            auto delta = std::chrono::high_resolution_clock::now() - start;

            ASSERT_NE(playback_result, K4A_STREAM_RESULT_FAILED);
            if (playback_result == K4A_STREAM_RESULT_EOF)
            {
                std::cout << "    Warning: Input file is too short, only read " << i << " captures." << std::endl;
                break;
            }
            ASSERT_NE(capture, nullptr);
            k4a_capture_release(capture);

            deltas.push_back(delta.count());
            std::this_thread::sleep_until(start + std::chrono::milliseconds(33));
        }
    }

    std::sort(deltas.begin(), deltas.end(), std::less<int64_t>());
    int64_t total_ns = 0;
    for (auto d : deltas)
    {
        total_ns += d;
    }
    std::cout << "    Avg latency: " << (total_ns / (int64_t)deltas.size() / 1000) << " usec" << std::endl;
    std::cout << "    P95 latency: " << (deltas[(size_t)((double)deltas.size() * 0.95) - 1] / 1000) << " usec"
              << std::endl;
    std::cout << "    P99 latency: " << (deltas[(size_t)((double)deltas.size() * 0.99) - 1] / 1000) << " usec"
              << std::endl;

    k4a_playback_close(handle);
}

TEST_F(playback_perf, test_read_latency_30fps_bgra_conversion)
{
    k4a_playback_t handle = NULL;
    k4a_result_t result = K4A_RESULT_FAILED;
    {
        Timer t("File open: " + g_test_file_name);
        result = k4a_playback_open(g_test_file_name.c_str(), &handle);
    }
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    result = k4a_playback_set_color_conversion(handle, K4A_IMAGE_FORMAT_COLOR_BGRA32);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    std::vector<int64_t> deltas;

    {
        k4a_capture_t capture = NULL;
        k4a_stream_result_t playback_result = K4A_STREAM_RESULT_FAILED;
        Timer t("Next capture x1000");
        for (int i = 0; i < 1000; i++)
        {

            auto start = std::chrono::high_resolution_clock::now();
            playback_result = k4a_playback_get_next_capture(handle, &capture);
            auto delta = std::chrono::high_resolution_clock::now() - start;

            if (playback_result == K4A_STREAM_RESULT_EOF)
            {
                std::cout << "    Warning: Input file is too short, only read " << i << " captures." << std::endl;
                break;
            }
            ASSERT_NE(capture, nullptr);
            k4a_capture_release(capture);

            deltas.push_back(delta.count());
            std::this_thread::sleep_until(start + std::chrono::milliseconds(33));
        }
    }

    std::sort(deltas.begin(), deltas.end(), std::less<int64_t>());
    int64_t total_ns = 0;
    for (auto d : deltas)
    {
        total_ns += d;
    }
    std::cout << "    Avg latency: " << (total_ns / (int64_t)deltas.size() / 1000) << " usec" << std::endl;
    std::cout << "    P95 latency: " << (deltas[(size_t)((double)deltas.size() * 0.95) - 1] / 1000) << " usec"
              << std::endl;
    std::cout << "    P99 latency: " << (deltas[(size_t)((double)deltas.size() * 0.99) - 1] / 1000) << " usec"
              << std::endl;

    k4a_playback_close(handle);
}

int main(int argc, char **argv)
{
    k4a_unittest_init();

    ::testing::InitGoogleTest(&argc, argv);

    if (argc < 2)
    {
        std::cout << "Usage: playback_perf <options> <testfile.mkv>" << std::endl;
        return 1;
    }
    g_test_file_name = std::string(argv[1]);

    int results = RUN_ALL_TESTS();
    k4a_unittest_deinit();
    return results;
}
