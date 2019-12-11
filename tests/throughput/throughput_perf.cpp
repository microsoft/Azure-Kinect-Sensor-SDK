// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <k4a/k4a.h>
#include <k4ainternal/common.h>
#include <k4ainternal/logging.h>
#include <gtest/gtest.h>
#include <utcommon.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <k4a/k4a.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/envvariable.h>

#define LLD(val) ((int64_t)(val))
#define TS_TO_MS(ts) (LLD((ts) / 1)) // TS convertion to milliseconds

#define K4A_IMU_SAMPLE_RATE 1666 // +/- 2%

static bool g_skip_delay_off_color_validation = false;
static int32_t g_depth_delay_off_color_usec = 0;
static uint8_t g_device_index = K4A_DEVICE_DEFAULT;
static k4a_wired_sync_mode_t g_wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
static int g_capture_count = 100;
static bool g_synchronized_images_only = false;
static bool g_no_imu = false;
static bool g_no_startup_flush = false;
static uint32_t g_subordinate_delay_off_master_usec = 0;
static bool g_manual_exposure = false;
static uint32_t g_exposure_setting = 0;
static bool g_power_line_50_hz = false;

using ::testing::ValuesIn;

struct throughput_parameters
{
    int test_number;
    const char *test_name;
    k4a_fps_t fps;
    k4a_image_format_t color_format;
    k4a_color_resolution_t color_resolution;
    k4a_depth_mode_t depth_mode;

    friend std::ostream &operator<<(std::ostream &os, const throughput_parameters &obj)
    {
        return os << "test index: (" << obj.test_name << ") " << (int)obj.test_number;
    }
};

struct thread_data
{
    volatile bool enable_counting;
    volatile bool exit;
    volatile uint32_t imu_samples;
    k4a_device_t device;
};

class throughput_perf : public ::testing::Test, public ::testing::WithParamInterface<throughput_parameters>
{
public:
    virtual void SetUp()
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(g_device_index, &m_device)) << "Couldn't open device\n";
        ASSERT_NE(m_device, nullptr);
    }

    virtual void TearDown()
    {
        if (m_device != nullptr)
        {
            k4a_device_close(m_device);
            m_device = nullptr;
        }
    }

    k4a_device_t m_device = nullptr;
};

static const char *get_string_from_color_format(k4a_image_format_t format)
{
    switch (format)
    {
    case K4A_IMAGE_FORMAT_COLOR_NV12:
        return "K4A_IMAGE_FORMAT_COLOR_NV12";
        break;
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
        return "K4A_IMAGE_FORMAT_COLOR_YUY2";
        break;
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
        return "K4A_IMAGE_FORMAT_COLOR_MJPG";
        break;
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        return "K4A_IMAGE_FORMAT_COLOR_BGRA32";
        break;
    case K4A_IMAGE_FORMAT_DEPTH16:
        return "K4A_IMAGE_FORMAT_DEPTH16";
        break;
    case K4A_IMAGE_FORMAT_IR16:
        return "K4A_IMAGE_FORMAT_IR16";
        break;
    case K4A_IMAGE_FORMAT_CUSTOM8:
        return "K4A_IMAGE_FORMAT_CUSTOM8";
        break;
    case K4A_IMAGE_FORMAT_CUSTOM16:
        return "K4A_IMAGE_FORMAT_CUSTOM16";
        break;
    case K4A_IMAGE_FORMAT_CUSTOM:
        return "K4A_IMAGE_FORMAT_CUSTOM";
        break;
    }
    assert(0);
    return "K4A_IMAGE_FORMAT_UNKNOWN";
}

static const char *get_string_from_color_resolution(k4a_color_resolution_t resolution)
{
    switch (resolution)
    {
    case K4A_COLOR_RESOLUTION_OFF:
        return "OFF";
        break;
    case K4A_COLOR_RESOLUTION_720P:
        return "1280 * 720  16:9";
        break;
    case K4A_COLOR_RESOLUTION_1080P:
        return "1920 * 1080 16:9";
        break;
    case K4A_COLOR_RESOLUTION_1440P:
        return "2560 * 1440  16:9";
        break;
    case K4A_COLOR_RESOLUTION_1536P:
        return "2048 * 1536 4:3";
        break;
    case K4A_COLOR_RESOLUTION_2160P:
        return "3840 * 2160 16:9";
        break;
    case K4A_COLOR_RESOLUTION_3072P:
        return "4096 * 3072 4:3";
        break;
    }
    assert(0);
    return "Unknown resolution";
}

static const char *get_string_from_depth_mode(k4a_depth_mode_t mode)
{
    switch (mode)
    {
    case K4A_DEPTH_MODE_OFF:
        return "K4A_DEPTH_MODE_OFF";
        break;
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
        return "K4A_DEPTH_MODE_NFOV_2X2BINNED";
        break;
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        return "K4A_DEPTH_MODE_NFOV_UNBINNED";
        break;
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        return "K4A_DEPTH_MODE_WFOV_2X2BINNED";
        break;
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        return "K4A_DEPTH_MODE_WFOV_UNBINNED";
        break;
    case K4A_DEPTH_MODE_PASSIVE_IR:
        return "K4A_DEPTH_MODE_PASSIVE_IR";
        break;
    }
    assert(0);
    return "Unknown Depth";
}

static int _throughput_imu_thread(void *param)
{
    struct thread_data *data = (struct thread_data *)param;
    k4a_result_t result;
    k4a_imu_sample_t imu;

    // Validate that time stamp is always changing and increasing
    uint64_t acc_ts = 0;
    uint64_t gyro_ts = 0;

    result = k4a_device_start_imu(data->device);
    if (K4A_FAILED(result))
    {
        printf("Failed to start imu\n");
        return result;
    }

    while (data->exit == false)
    {
        k4a_wait_result_t wresult = k4a_device_get_imu_sample(data->device, &imu, 1);
        if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
        {
            if (data->enable_counting == true)
            {
                data->imu_samples++;

                EXPECT_LT(acc_ts, imu.acc_timestamp_usec);
                EXPECT_LT(gyro_ts, imu.gyro_timestamp_usec);

                if (acc_ts != 0)
                {
                    EXPECT_LT(imu.acc_timestamp_usec, acc_ts + 900);
                    EXPECT_LT(imu.gyro_timestamp_usec, gyro_ts + 900);
                }

                acc_ts = imu.acc_timestamp_usec;
                gyro_ts = imu.gyro_timestamp_usec;
            }
        }
        else if (wresult == K4A_WAIT_RESULT_FAILED)
        {
            printf("k4a_device_get_imu_sample failed\n");
            result = K4A_RESULT_FAILED;
            break;
        }
    };

    k4a_device_stop_imu(data->device);
    return result;
}

TEST_P(throughput_perf, testTest)
{
    auto as = GetParam();
    const int32_t TIMEOUT_IN_MS = 1000;
    k4a_capture_t capture = NULL;
    int capture_count = g_capture_count;
    int both_count = 0;
    int depth_count = 0;
    int color_count = 0;
    int missed_count = 0;
    int not_synchronized_count = 0;
    uint64_t last_ts = UINT64_MAX;
    int32_t fps_in_usec = 0;
    uint64_t last_color_ts = 0;
    uint64_t last_depth16_ts = 0;
    uint64_t last_ir16_ts = 0;
    int failure_threshold_percent = 5;
    int failure_threshold_count = g_capture_count * failure_threshold_percent / 100; // 5%
    bool failed = false;
    FILE *file_handle = NULL;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    thread_data thread = { 0 };
    THREAD_HANDLE th1 = NULL;
    int64_t max_sync_delay = 0;

    printf("Capturing %d frames for test: %s\n", g_capture_count, as.test_name);

    {
        int32_t power_line_setting = g_power_line_50_hz ? 1 : 2;
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_set_color_control(m_device,
                                               K4A_COLOR_CONTROL_POWERLINE_FREQUENCY,
                                               K4A_COLOR_CONTROL_MODE_MANUAL,
                                               power_line_setting));
        printf("Power line mode set to manual and %s.\n", power_line_setting == 1 ? "50Hz" : "60Hz");
    }

    if (g_manual_exposure)
    {
        k4a_color_control_mode_t read_mode;
        int32_t read_exposure;
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_set_color_control(m_device,
                                               K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                               K4A_COLOR_CONTROL_MODE_MANUAL,
                                               (int32_t)g_exposure_setting));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_get_color_control(m_device,
                                               K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                               &read_mode,
                                               &read_exposure));
        printf(
            "Setting exposure to manual mode, exposure target is: %d.   Actual mode is: %s.   Actual value is: %d.\n",
            g_exposure_setting,
            read_mode == K4A_COLOR_CONTROL_MODE_AUTO ? "auto" : "manual",
            read_exposure);
    }
    else
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_set_color_control(m_device,
                                               K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                               K4A_COLOR_CONTROL_MODE_AUTO,
                                               0));
        printf("Auto Exposure\n");
    }

    fps_in_usec = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(as.fps));

    config.color_format = as.color_format;
    config.color_resolution = as.color_resolution;
    config.depth_mode = as.depth_mode;
    config.camera_fps = as.fps;
    config.depth_delay_off_color_usec = g_depth_delay_off_color_usec;
    config.wired_sync_mode = g_wired_sync_mode;
    config.synchronized_images_only = g_synchronized_images_only;
    config.subordinate_delay_off_master_usec = g_subordinate_delay_off_master_usec;
    if (g_depth_delay_off_color_usec == 0)
    {
        // Create delay that can be +fps to -fps
        config.depth_delay_off_color_usec = (int32_t)RAND_VALUE(-fps_in_usec, fps_in_usec);
    }

    max_sync_delay = k4a_unittest_get_max_sync_delay_ms(as.fps);

    printf("Config being used is:\n");
    printf("    color_format:%d\n", config.color_format);
    printf("    color_resolution:%d\n", config.color_resolution);
    printf("    depth_mode:%d\n", config.depth_mode);
    printf("    camera_fps:%d\n", config.camera_fps);
    printf("    synchronized_images_only:%d\n", config.synchronized_images_only);
    printf("    depth_delay_off_color_usec:%d\n", config.depth_delay_off_color_usec);
    printf("    wired_sync_mode:%d\n", config.wired_sync_mode);
    printf("    subordinate_delay_off_master_usec:%d\n", config.subordinate_delay_off_master_usec);
    printf("    disable_streaming_indicator:%d\n", config.disable_streaming_indicator);
    printf("\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));

    if (!g_no_imu)
    {
        thread.device = m_device;
        ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th1, _throughput_imu_thread, &thread));
    }

    if (!g_no_startup_flush)
    {
        //
        // Wait for streams to start and then purge the data collected
        //
        if (as.fps == K4A_FRAMES_PER_SECOND_30)
        {
            printf("Flushing first 2s of data\n");
            ThreadAPI_Sleep(2000);
        }
        else if (as.fps == K4A_FRAMES_PER_SECOND_15)
        {
            printf("Flushing first 3s of data\n");
            ThreadAPI_Sleep(3000);
        }
        else
        {
            printf("Flushing first 4s of data\n");
            ThreadAPI_Sleep(4000);
        }
        while (K4A_WAIT_RESULT_SUCCEEDED == k4a_device_get_capture(m_device, &capture, 0))
        {
            // Drain the queue
            k4a_capture_release(capture);
        };
    }
    else
    {
        printf("Flushing no start of stream data\n");
    }

    // For consistent IMU timing, block entering the while loop until we get 1 sample
    if (K4A_WAIT_RESULT_SUCCEEDED == k4a_device_get_capture(m_device, &capture, 1000))
    {
        k4a_capture_release(capture);
    }

    printf("All times in us\n");
    printf("+---------------------------+-------------------+-------------------+--------+\n");
    printf("|         Color Info        |     IR 16 Info    |   Depth16 Info    | TS Del |\n");
    printf("|  TS [Delta TS][Exposure]  |   TS [Delta TS]   |   TS [Delta TS]   | (C-D)  |\n");
    printf("+---------------------------+-------------------+-------------------+--------+\n");

    thread.enable_counting = true; // start counting IMU samples
    while (capture_count-- > 0)
    {
        uint64_t adjusted_max_ts = 0;
        uint64_t ts;
        bool color = false;
        bool depth = false;

        // Get a depth frame
        k4a_wait_result_t wresult = k4a_device_get_capture(m_device, &capture, TIMEOUT_IN_MS);

        if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
        {
            k4a_image_t image;

            printf("|");

            // Probe for a color image
            image = k4a_capture_get_color_image(capture);
            if (image)
            {
                color = true;
                ts = k4a_image_get_device_timestamp_usec(image);
                adjusted_max_ts = std::max(ts, adjusted_max_ts);
                static_assert(sizeof(ts) == 8, "this should not be wrong");
                printf(" %9" PRId64 "[%6" PRId64 "][%6" PRId64 "]",
                       TS_TO_MS(ts),
                       TS_TO_MS(ts - last_color_ts),
                       LLD(k4a_image_get_exposure_usec(image)));

                // TS should increase
                EXPECT_GT(ts, last_color_ts);
                last_color_ts = ts;

                k4a_image_release(image);
            }
            else
            {
                printf(" Color None               ");
            }

            // probe for a IR16 image
            image = k4a_capture_get_ir_image(capture);
            if (image)
            {
                depth = true;
                ts = k4a_image_get_device_timestamp_usec(image);
                adjusted_max_ts = std::max(ts - (uint64_t)config.depth_delay_off_color_usec, adjusted_max_ts);
                printf(" | %9" PRId64 "[%6" PRId64 "]", TS_TO_MS(ts), TS_TO_MS(ts - last_ir16_ts));

                // TS should increase
                EXPECT_GT(ts, last_ir16_ts);
                last_ir16_ts = ts;

                k4a_image_release(image);
            }
            else
            {
                printf(" |                  ");
            }

            // Probe for a depth16 image
            image = k4a_capture_get_depth_image(capture);
            if (image)
            {
                ts = k4a_image_get_device_timestamp_usec(image);
                adjusted_max_ts = std::max(ts - (uint64_t)config.depth_delay_off_color_usec, adjusted_max_ts);
                printf(" | %9" PRId64 "[%6" PRId64 "]", TS_TO_MS(ts), TS_TO_MS(ts - last_depth16_ts));

                // TS should increase
                EXPECT_GT(ts, last_depth16_ts);
                last_depth16_ts = ts;

                k4a_image_release(image);
            }
            else
            {
                printf(" |                  ");
            }
        }
        else if (wresult == K4A_WAIT_RESULT_TIMEOUT)
        {
            printf("Timed out waiting for a capture\n");
        }
        else // wresult == K4A_WAIT_RESULT_FAILED:
        {
            printf("Failed to read a capture\n");
            capture_count = 0;
        }

        if (depth && color)
        {
            both_count++;

            int64_t delta = (int64_t)(last_ir16_ts - last_color_ts);
            printf(" | %6" PRId64, delta);

            delta -= config.depth_delay_off_color_usec;
            delta = std::abs(delta);

            if (delta > max_sync_delay)
            {
                not_synchronized_count++;
            }
        }
        else if (depth)
        {
            printf(" | ------");
            depth_count++;
        }
        else if (color)
        {
            printf(" | ------");
            color_count++;
        }

        printf(" |\n");

        EXPECT_NE(adjusted_max_ts, 0);
        if (last_ts == UINT64_MAX)
        {
            last_ts = adjusted_max_ts;
        }
        else if (last_ts > adjusted_max_ts)
        {
            // This happens when one queue gets saturated and must drop samples early; i.e. the depth queue is full, but
            // the color image is delayed due to perf issues. When this happens we just ignore the sample because our
            // time stamp logic has already moved beyond the time this sample was supposed to arrive at.
        }
        else if ((adjusted_max_ts - last_ts) >= ((unsigned)(fps_in_usec * 15 / 10)))
        {
            // Calc how many captures we didn't get. If the delta between the last two time stamps is more than 1.5
            // * fps_in_usec then we count
            int32_t missed_this_period = (int32_t)(adjusted_max_ts - last_ts) / fps_in_usec;
            missed_this_period--; // We got a new time stamp to do this math, so this count has 1 too many, remove
                                  // it
            if (((adjusted_max_ts - last_ts) % ((unsigned)fps_in_usec)) > ((unsigned)fps_in_usec) / 2)
            {
                missed_this_period++;
            }
            printf("Missed %d captures before previous capture %" PRId64 " %" PRId64 "\n",
                   missed_this_period,
                   LLD(adjusted_max_ts),
                   LLD(last_ts));
            if (missed_this_period > capture_count)
            {
                missed_count += capture_count;
                capture_count = 0;
            }
            else
            {
                missed_count += missed_this_period;
                capture_count -= missed_this_period;
            }
        }
        last_ts = std::max(last_ts, adjusted_max_ts);

        // release capture
        if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
        {
            k4a_capture_release(capture);
        }
    }
    thread.enable_counting = false; // stop counting IMU samples
    thread.exit = true;             // shut down IMU thread
    k4a_device_stop_cameras(m_device);

    if (!g_no_imu)
    {
        int thread_result;
        ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th1, &thread_result));
        ASSERT_EQ(thread_result, (int)K4A_RESULT_SUCCEEDED);
    }

    int imu_samples_per_sec_usec = 1000000 / K4A_IMU_SAMPLE_RATE;
    int target_imu_samples = (g_capture_count * (int)fps_in_usec) / imu_samples_per_sec_usec;
    float imu_percent = ((float)thread.imu_samples - (float)target_imu_samples) / (float)target_imu_samples;
    imu_percent *= 100;

    failed = false;
    printf("\nRESULTS Captures\n");

    {
        bool criteria_failed = false;
        if (abs(both_count - g_capture_count) > failure_threshold_count)
        {
            failed = true;
            criteria_failed = true;
        }
        printf("    Synchronized:%d %s\n", both_count, criteria_failed ? "FAILED" : "PASSED");
    }

    {
        bool criteria_failed = false;
        if (abs(depth_count) > failure_threshold_count)
        {
            failed = true;
            criteria_failed = true;
        }
        printf("      Depth Only:%d %s\n", depth_count, criteria_failed ? "FAILED" : "PASSED");
    }
    {
        bool criteria_failed = false;
        if (abs(color_count) > failure_threshold_count)
        {
            failed = true;
            criteria_failed = true;
        }
        printf("      Color Only:%d %s\n", color_count, criteria_failed ? "FAILED" : "PASSED");
    }
    {
        bool criteria_failed = false;
        if (abs(missed_count) > failure_threshold_count)
        {
            failed = true;
            criteria_failed = true;
        }
        printf(" Missed Captures:%d %s\n", missed_count, criteria_failed ? "FAILED" : "PASSED");
    }
    {
        bool criteria_failed = false;
        if (!g_no_imu)
        {
            if (imu_percent > failure_threshold_percent || imu_percent < -failure_threshold_percent)
            {
                failed = true;
                criteria_failed = true;
            }
        }
        printf("     Imu Samples:%d %0.01f%% of target(%d) %s\n",
               thread.imu_samples,
               (double)imu_percent,
               target_imu_samples,
               g_no_imu ? "Disabled" : (criteria_failed ? "FAILED" : "PASSED"));
    }
    {
        bool criteria_failed = false;
        if (not_synchronized_count > failure_threshold_count)
        {
            if (!g_skip_delay_off_color_validation)
            {
                failed = true;
            }
            criteria_failed = true;
        }
        printf("   TS not sync'd:%d %s\n", not_synchronized_count, criteria_failed ? "FAILED" : "PASSED");
    }
    printf("  Total captures:%d\n\n", both_count + depth_count + color_count + missed_count);

    file_handle = fopen("testResults.csv", "a");
    if (file_handle != 0)
    {
        std::time_t date_time = std::time(NULL);
        char buffer_date_time[100];
        std::strftime(buffer_date_time, sizeof(buffer_date_time), "%c", localtime(&date_time));

        const char *user_name = environment_get_variable("USERNAME");
        const char *computer_name = environment_get_variable("COMPUTERNAME");

        char buffer[1024];
        snprintf(buffer,
                 sizeof(buffer),
                 "%s, %s, %s, %s, %s, %s, %s, fps, %d, %s, captures, %d, syncd captures, %d, depth only, %d, color "
                 "only, %d, missing capture periods, %d, imu %%, %0.01f, not_synchronized, %d, %d\n",
                 buffer_date_time,
                 failed ? "FAILED" : "PASSED",
                 computer_name ? computer_name : "computer name not set",
                 user_name ? user_name : "user name not set",
                 as.test_name,
                 get_string_from_color_format(as.color_format),
                 get_string_from_color_resolution(as.color_resolution),
                 k4a_convert_fps_to_uint(as.fps),
                 get_string_from_depth_mode(as.depth_mode),
                 g_capture_count,
                 both_count,
                 depth_count,
                 color_count,
                 missed_count,
                 (double)imu_percent,
                 not_synchronized_count,
                 config.depth_delay_off_color_usec);

        fputs(buffer, file_handle);
        fclose(file_handle);
    }

    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_set_color_control(m_device,
                                           K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                           K4A_COLOR_CONTROL_MODE_AUTO,
                                           0));

    ASSERT_EQ(failed, false);
    return;
}

// K4A_DEPTH_MODE_WFOV_UNBINNED is the most demanding depth mode, only runs at 15FPS or less

// clang-format off
static struct throughput_parameters tests_30fps[] = {
    {  0, "FPS_30_MJPEG_2160P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    {  1, "FPS_30_MJPEG_2160P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    {  2, "FPS_30_MJPEG_2160P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    {  3, "FPS_30_MJPEG_2160P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  4, "FPS_30_MJPEG_1536P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    {  5, "FPS_30_MJPEG_1536P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    {  6, "FPS_30_MJPEG_1536P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    {  7, "FPS_30_MJPEG_1536P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  8, "FPS_30_MJPEG_1440P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    {  9, "FPS_30_MJPEG_1440P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 10, "FPS_30_MJPEG_1440P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 11, "FPS_30_MJPEG_1440P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 12, "FPS_30_MJPEG_1080P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 13, "FPS_30_MJPEG_1080P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 14, "FPS_30_MJPEG_1080P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 15, "FPS_30_MJPEG_1080P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 16, "FPS_30_MJPEG_0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 17, "FPS_30_MJPEG_0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 18, "FPS_30_MJPEG_0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 19, "FPS_30_MJPEG_0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
    { 20, "FPS_30_NV12__0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_NV12,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 21, "FPS_30_NV12__0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_NV12,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 22, "FPS_30_NV12__0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_NV12,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 23, "FPS_30_NV12__0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_NV12,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
    { 24, "FPS_30_YUY2__0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_YUY2,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 25, "FPS_30_YUY2__0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_YUY2,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 26, "FPS_30_YUY2__0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_YUY2,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 27, "FPS_30_YUY2__0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_YUY2,  K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},

    // RGB modes use one of the above modes and performs a conversion, so we don't test EVERY combination
    { 28, "FPS_30_BGRA32_2160P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 29, "FPS_30_BGRA32_1536P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 30, "FPS_30_BGRA32_1440P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 31, "FPS_30_BGRA32_1080P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 32, "FPS_30_BGRA32_0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
};

INSTANTIATE_TEST_CASE_P(30FPS_TESTS, throughput_perf, ValuesIn(tests_30fps));

static struct throughput_parameters tests_15fps[] = {
    {  0, "FPS_15_MJPEG_3072P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    {  1, "FPS_15_MJPEG_3072P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    {  2, "FPS_15_MJPEG_3072P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    {  3, "FPS_15_MJPEG_3072P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    {  4, "FPS_15_MJPEG_3072P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  5, "FPS_15_MJPEG_2160P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    {  6, "FPS_15_MJPEG_2160P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    {  7, "FPS_15_MJPEG_2160P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    {  8, "FPS_15_MJPEG_2160P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    {  9, "FPS_15_MJPEG_2160P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 10, "FPS_15_MJPEG_1536P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 11, "FPS_15_MJPEG_1536P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 12, "FPS_15_MJPEG_1536P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 13, "FPS_15_MJPEG_1536P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 14, "FPS_15_MJPEG_1536P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 15, "FPS_15_MJPEG_1440P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 16, "FPS_15_MJPEG_1440P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 17, "FPS_15_MJPEG_1440P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 18, "FPS_15_MJPEG_1440P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 19, "FPS_15_MJPEG_1440P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 20, "FPS_15_MJPEG_1080P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 21, "FPS_15_MJPEG_1080P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 22, "FPS_15_MJPEG_1080P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 23, "FPS_15_MJPEG_1080P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 24, "FPS_15_MJPEG_1080P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 25, "FPS_15_MJPEG_0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 26, "FPS_15_MJPEG_0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 27, "FPS_15_MJPEG_0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 28, "FPS_15_MJPEG_0720P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 29, "FPS_15_MJPEG_0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
    { 30, "FPS_15_NV12__0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 31, "FPS_15_NV12__0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 32, "FPS_15_NV12__0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 33, "FPS_15_NV12__0720P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 34, "FPS_15_NV12__0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
    { 35, "FPS_15_YUY2__0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 36, "FPS_15_YUY2__0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 37, "FPS_15_YUY2__0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 38, "FPS_15_YUY2__0720P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 39, "FPS_15_YUY2__0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
};

INSTANTIATE_TEST_CASE_P(15FPS_TESTS, throughput_perf, ValuesIn(tests_15fps));

static struct throughput_parameters tests_5fps[] = {
    {  0, "FPS_05_MJPEG_3072P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    {  1, "FPS_05_MJPEG_3072P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    {  2, "FPS_05_MJPEG_3072P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    {  3, "FPS_05_MJPEG_3072P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    {  4, "FPS_05_MJPEG_3072P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  5, "FPS_05_MJPEG_2160P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    {  6, "FPS_05_MJPEG_2160P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    {  7, "FPS_05_MJPEG_2160P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    {  8, "FPS_05_MJPEG_2160P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    {  9, "FPS_05_MJPEG_2160P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 10, "FPS_05_MJPEG_1536P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 11, "FPS_05_MJPEG_1536P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 12, "FPS_05_MJPEG_1536P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 13, "FPS_05_MJPEG_1536P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 14, "FPS_05_MJPEG_1536P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 15, "FPS_05_MJPEG_1440P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 16, "FPS_05_MJPEG_1440P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 17, "FPS_05_MJPEG_1440P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 18, "FPS_05_MJPEG_1440P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 19, "FPS_05_MJPEG_1440P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 20, "FPS_05_MJPEG_1080P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 21, "FPS_05_MJPEG_1080P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 22, "FPS_05_MJPEG_1080P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 23, "FPS_05_MJPEG_1080P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 24, "FPS_05_MJPEG_1080P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 25, "FPS_05_MJPEG_0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 26, "FPS_05_MJPEG_0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 27, "FPS_05_MJPEG_0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 28, "FPS_05_MJPEG_0720P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 29, "FPS_05_MJPEG_0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_MJPG, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
    { 30, "FPS_05_NV12__0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 31, "FPS_05_NV12__0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 32, "FPS_05_NV12__0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 33, "FPS_05_NV12__0720P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 34, "FPS_05_NV12__0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_NV12, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
    { 35, "FPS_05_YUY2__0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 36, "FPS_05_YUY2__0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 37, "FPS_05_YUY2__0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 38, "FPS_05_YUY2__0720P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_UNBINNED},
    { 39, "FPS_05_YUY2__0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_5,  K4A_IMAGE_FORMAT_COLOR_YUY2, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
};
// clang-format on

INSTANTIATE_TEST_CASE_P(5FPS_TESTS, throughput_perf, ValuesIn(tests_5fps));

int main(int argc, char **argv)
{
    bool error = false;
    k4a_unittest_init();

    srand((unsigned int)time(0)); // use current time as seed for random generator

    ::testing::InitGoogleTest(&argc, argv);

    for (int i = 1; i < argc; ++i)
    {
        char *argument = argv[i];
        for (int j = 0; argument[j]; j++)
        {
            argument[j] = (char)tolower(argument[j]);
        }
        if (strcmp(argument, "--depth_delay_off_color") == 0)
        {
            if (i + 1 <= argc)
            {

                g_depth_delay_off_color_usec = (int32_t)strtol(argv[i + 1], NULL, 10);
                printf("Setting g_depth_delay_off_color_usec = %d\n", g_depth_delay_off_color_usec);
                i++;
            }
            else
            {
                printf("Error: depth_delay_off_color parameter missing\n");
                error = true;
            }
        }
        else if (strcmp(argument, "--skip_delay_off_color_validation") == 0)
        {
            g_skip_delay_off_color_validation = true;
        }
        else if (strcmp(argument, "--no_imu") == 0)
        {
            g_no_imu = true;
        }
        else if (strcmp(argument, "--master") == 0)
        {
            g_wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
            printf("Setting g_wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER\n");
        }
        else if (strcmp(argument, "--subordinate") == 0)
        {
            g_wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
            printf("Setting g_wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE\n");
        }
        else if (strcmp(argument, "--synchronized_images_only") == 0)
        {
            g_synchronized_images_only = true;
            printf("g_synchronized_images_only = true\n");
        }
        else if (strcmp(argument, "--no_startup_flush") == 0)
        {
            g_no_startup_flush = true;
            printf("g_no_startup_flush = true\n");
        }
        else if (strcmp(argument, "--60hz") == 0)
        {
            g_power_line_50_hz = false;
            printf("g_power_line_50_hz = false\n");
        }
        else if (strcmp(argument, "--50hz") == 0)
        {
            g_power_line_50_hz = true;
            printf("g_power_line_50_hz = true\n");
        }
        else if (strcmp(argument, "--index") == 0)
        {
            if (i + 1 <= argc)
            {
                g_device_index = (uint8_t)strtol(argv[i + 1], NULL, 10);
                printf("setting g_device_index = %d\n", g_device_index);
                i++;
            }
            else
            {
                printf("Error: index parameter missing\n");
                error = true;
            }
        }
        else if (strcmp(argument, "--subordinate_delay_off_master_usec") == 0)
        {
            if (i + 1 <= argc)
            {
                g_subordinate_delay_off_master_usec = (uint32_t)strtol(argv[i + 1], NULL, 10);
                printf("g_subordinate_delay_off_master_usec = %d\n", g_subordinate_delay_off_master_usec);
                i++;
            }
            else
            {
                printf("Error: index parameter missing\n");
                error = true;
            }
        }
        else if (strcmp(argument, "--capture_count") == 0)
        {
            if (i + 1 <= argc)
            {
                g_capture_count = (int)strtol(argv[i + 1], NULL, 10);
                printf("g_capture_count g_device_index = %d\n", g_capture_count);
                i++;
            }
            else
            {
                printf("Error: index parameter missing\n");
                error = true;
            }
        }
        else if (strcmp(argument, "--exposure") == 0)
        {
            if (i + 1 <= argc)
            {
                g_exposure_setting = (uint32_t)strtol(argv[i + 1], NULL, 10);
                printf("g_exposure_setting = %d\n", g_capture_count);
                g_manual_exposure = true;
                i++;
            }
            else
            {
                printf("Error: index parameter missing\n");
                error = true;
            }
        }

        if ((strcmp(argument, "-h") == 0) || (strcmp(argument, "/h") == 0) || (strcmp(argument, "-?") == 0) ||
            (strcmp(argument, "/?") == 0))
        {
            error = true;
        }
    }

    if (error)
    {
        printf("\n\nOptional Custom Test Settings:\n");
        printf("  --depth_delay_off_color <+/- microseconds>\n");
        printf("      This is the time delay the depth image capture is delayed off the color.\n");
        printf("      valid ranges for this are -1 frame time to +1 frame time. The percentage\n");
        printf("      needs to be multiplied by 100 to achieve correct behavior; 10000 is \n");
        printf("      100.00%%, 100 is 1.00%%.\n");
        printf("  --skip_delay_off_color_validation\n");
        printf("      Set this when don't want the results of color to depth timestamp \n"
               "      measurements to allow your test run to fail. They will still be logged\n"
               "      to output and the CSV file.\n");
        printf("  --master\n");
        printf("      Run device in master mode\n");
        printf("  --subordinate\n");
        printf("      Run device in subordinate mode\n");
        printf("  --index\n");
        printf("      The device index to target when calling k4a_device_open()\n");
        printf("  --no_imu\n");
        printf("      Disables IMU in the test.\n");
        printf("  --capture_count\n");
        printf("      The number of captures the test should read; default is 100\n");
        printf("  --synchronized_images_only\n");
        printf("      By default this setting is false, enabling this will for the test to wait for\n");
        printf("      both and depth images to be available.\n");
        printf("  --subordinate_delay_off_master_usec <+ microseconds>\n");
        printf("      This is the time delay the device captures off the master devices capture sync\n");
        printf("      pulse. This value needs to be less than one image sample period, i.e for 30FPS \n");
        printf("      this needs to be less than 33333us.\n");
        printf("  --no_startup_flush\n");
        printf("      By default the test will wait for streams to run for X seconds to stabilize. This\n");
        printf("      disables that.\n");
        printf("  --exposure <exposure in usec>\n");
        printf("      By default the test uses auto exposure. This will test with the manual exposure setting\n");
        printf("      that is passed in.\n");
        printf("  --60hz\n");
        printf("      <default> Sets the power line compensation frequency to 60Hz\n");
        printf("  --50hz\n");
        printf("      Sets the power line compensation frequency to 50Hz\n");

        return 1; // Indicates an error or warning
    }
    int results = RUN_ALL_TESTS();
    k4a_unittest_deinit();
    return results;
}
