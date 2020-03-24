// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#ifdef _WIN32
#include <windows.h>
#endif

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
#include <deque>
#include <mutex>

#ifndef _WIN32
#include <time.h>
#endif

#define LLD(val) ((int64_t)(val))
#define STS_TO_MS(ts) (LLD((ts) / 1000000)) // System TS convertion to milliseconds

static bool g_skip_delay_off_color_validation = false;
static int32_t g_depth_delay_off_color_usec = 0;
static uint8_t g_device_index = K4A_DEVICE_DEFAULT;
static k4a_wired_sync_mode_t g_wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
static int g_capture_count = 50;
static bool g_synchronized_images_only = false;
static bool g_no_startup_flush = false;
static uint32_t g_subordinate_delay_off_master_usec = 0;
static bool g_manual_exposure = true;
static uint32_t g_exposure_setting = 31000; // will round up to nearest value
static bool g_power_line_50_hz = false;

using ::testing::ValuesIn;

typedef struct _sys_pts_time_t
{
    uint64_t pts;
    uint64_t system;
} sys_pts_time_t;

static std::mutex g_lock_mutex;
static std::deque<sys_pts_time_t> g_time_c; // Color image copy of data
static std::deque<sys_pts_time_t> g_time_i; // Ir image copy of data

struct latency_parameters
{
    int test_number;
    const char *test_name;
    k4a_fps_t fps;
    k4a_image_format_t color_format;
    k4a_color_resolution_t color_resolution;
    k4a_depth_mode_t depth_mode;

    friend std::ostream &operator<<(std::ostream &os, const latency_parameters &obj)
    {
        return os << "test index: (" << obj.test_name << ") " << (int)obj.test_number;
    }
};

struct thread_data
{
    volatile bool save_samples;
    volatile bool exit;
    volatile uint32_t imu_samples;
    k4a_device_t device;
};

class latency_perf : public ::testing::Test, public ::testing::WithParamInterface<latency_parameters>
{
public:
    virtual void SetUp()
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(g_device_index, &m_device)) << "Couldn't open device\n";
        ASSERT_NE(m_device, nullptr);
        EXPECT_NE((FILE *)NULL, (m_file_handle = fopen("latency_testResults.csv", "a")));
    }

    virtual void TearDown()
    {
        if (m_device != nullptr)
        {
            k4a_device_close(m_device);
            m_device = nullptr;
        }
        if (m_file_handle)
        {
            fclose(m_file_handle);
        }
    }

    void print_and_log(const char *message, const char *mode, int64_t ave, int64_t min, int64_t max);
    void process_image(k4a_capture_t capture,
                       uint64_t current_system_ts,
                       bool process_color,
                       bool *image_first_pass,
                       std::deque<uint64_t> *system_latency,
                       std::deque<uint64_t> *system_latency_from_pts,
                       uint64_t *system_ts_last,
                       uint64_t *system_ts_from_pts_last);

    k4a_device_t m_device = nullptr;
    FILE *m_file_handle;
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

static bool get_system_time(uint64_t *time_nsec)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
#ifdef _WIN32
    LARGE_INTEGER qpc = { 0 };
    static LARGE_INTEGER freq = { 0 };
    result = K4A_RESULT_FROM_BOOL(QueryPerformanceCounter(&qpc) != 0);
    if (K4A_FAILED(result))
    {
        return false;
    }
    if (freq.QuadPart == 0)
    {
        result = K4A_RESULT_FROM_BOOL(QueryPerformanceFrequency(&freq) != 0);
        if (K4A_FAILED(result))
        {
            return false;
        }
    }

    // Calculate seconds in such a way we minimize overflow.
    // Rollover happens, for a 1MHz Freq, when qpc.QuadPart > 0x003F FFFF FFFF FFFF; ~571 Years after boot.
    *time_nsec = qpc.QuadPart / freq.QuadPart * 1000000000;
    *time_nsec += qpc.QuadPart % freq.QuadPart * 1000000000 / freq.QuadPart;

#else
    struct timespec ts_time;
    result = K4A_RESULT_FROM_BOOL(clock_gettime(CLOCK_MONOTONIC, &ts_time) == 0);
    if (K4A_FAILED(result))
    {
        return false;
    }
    // Rollover happens about ~136 years after boot.
    *time_nsec = (uint64_t)ts_time.tv_sec * 1000000000 + (uint64_t)ts_time.tv_nsec;
#endif
    return true;
}

static int _latency_imu_thread(void *param)
{
    struct thread_data *data = (struct thread_data *)param;
    k4a_result_t result;
    k4a_imu_sample_t imu;

    result = k4a_device_start_imu(data->device);
    if (K4A_FAILED(result))
    {
        printf("Failed to start imu\n");
        return result;
    }

    g_time_c.clear();
    g_time_i.clear();

    while (data->exit == false)
    {
        k4a_wait_result_t wresult = k4a_device_get_imu_sample(data->device, &imu, 10);
        if (wresult == K4A_WAIT_RESULT_FAILED)
        {
            printf("k4a_device_get_imu_sample failed\n");
            result = K4A_RESULT_FAILED;
            break;
        }
        else if ((wresult == K4A_WAIT_RESULT_SUCCEEDED) && (data->save_samples))
        {
            sys_pts_time_t time;
            time.pts = imu.acc_timestamp_usec;
            if (get_system_time(&time.system) == 0)
            {
                result = K4A_RESULT_FAILED;
                break;
            }

            // Save data to each of the queues
            g_lock_mutex.lock();
            g_time_c.push_back(time);
            g_time_i.push_back(time);
            g_lock_mutex.unlock();
        }
    };

    k4a_device_stop_imu(data->device);
    return result;
}

// Drop the lock and sleep for Xms. This is to allow the queue to fill again. Return if we yield too long.
#define YIELD_THREAD(lock_var, count, message)                                                                         \
    lock_var.unlock();                                                                                                 \
    printf("Lock dropped while %s\n", message);                                                                        \
    ThreadAPI_Sleep(2);                                                                                                \
    if (++count > 15)                                                                                                  \
    {                                                                                                                  \
        EXPECT_LT(count, 15);                                                                                          \
        return 0;                                                                                                      \
    }                                                                                                                  \
    lock_var.lock();

static uint64_t lookup_system_ts(uint64_t pts_ts, bool color)
{
    sys_pts_time_t last_time;
    uint64_t start_time_nsec;
    uint64_t current_time_nsec;
    int count = 0;

    bool found = false;

    std::deque<sys_pts_time_t> *time_queue = &g_time_i;
    if (color)
    {
        time_queue = &g_time_c;
    }

    g_lock_mutex.lock();

    // Record start time
    if (get_system_time(&start_time_nsec) == 0)
    {
        printf("ERROR getting system time\n");
        EXPECT_TRUE(0);
        g_lock_mutex.unlock();
        return 0;
    }

    int delay_count = 0;
    while (time_queue->empty())
    {
        // Drop lock, wait, retake lock - Exit if taking too long
        YIELD_THREAD(g_lock_mutex, delay_count, "Initializing")
    }

    last_time = time_queue->front();
    time_queue->pop_front();

    while (!found)
    {
        int x;
        for (x = 0; !time_queue->empty(); x++)
        {
            last_time = time_queue->front();
            if (pts_ts > last_time.pts)
            {
                // Hold onto last_time for 1 more loop
                last_time = time_queue->front();
                time_queue->pop_front();
            }
            else
            {
                // We just found the first system time that is beyond the one we are looking for.
                if ((pts_ts - last_time.pts) < (time_queue->front().pts - pts_ts))
                {
                    g_lock_mutex.unlock();
                    found = true;
                    return last_time.system;
                }
                uint64_t ret_time = time_queue->front().system;
                g_lock_mutex.unlock();

                found = true;
                return ret_time;
            }

            if (get_system_time(&current_time_nsec) == 0)
            {
                printf("ERROR getting system time\n");
                EXPECT_TRUE(0);
                g_lock_mutex.unlock();
                return 0;
            }

            if (STS_TO_MS(current_time_nsec - start_time_nsec) > 1000)
            {
                printf("Count for break is %d\n", count);
                break; // Don't hold lock too long, run YIELD_THREAD below
            }
        }

        // Queue is drained or we held the lock too long. We need to let the IMU thread catch up. Drop lock, wait,
        // retake lock - Exit if taking too long
        YIELD_THREAD(g_lock_mutex, delay_count, "walking list.");

        // Update start time after the thread yield
        if (get_system_time(&start_time_nsec) == 0)
        {
            printf("ERROR getting system time\n");
            EXPECT_TRUE(0);
            g_lock_mutex.unlock();
            return 0;
        }
    }

    // Should not happen
    EXPECT_FALSE(1);
    g_lock_mutex.unlock();
    return 0;
}

void latency_perf::print_and_log(const char *message, const char *mode, int64_t ave, int64_t min, int64_t max)
{
    printf("    %30s %30s: Ave=%" PRId64 " min=%" PRId64 " max=%" PRId64 "\n", message, mode, ave, min, max);

    if (m_file_handle)
    {
        char buffer[1024];
        snprintf(buffer,
                 sizeof(buffer),
                 "%s, %s (min ave max),%" PRId64 ",%" PRId64 ",%" PRId64 ",",
                 mode,
                 message,
                 min,
                 ave,
                 max);
        fputs(buffer, m_file_handle);
    }
}

void latency_perf::process_image(k4a_capture_t capture,
                                 uint64_t current_system_ts,
                                 bool process_color,
                                 bool *image_first_pass,
                                 std::deque<uint64_t> *system_latency,
                                 std::deque<uint64_t> *system_latency_from_pts,
                                 uint64_t *system_ts_last,
                                 uint64_t *system_ts_from_pts_last)
{
    k4a_image_t image;
    if (process_color)
    {
        image = k4a_capture_get_color_image(capture);
    }
    else
    {
        image = k4a_capture_get_ir_image(capture);
    }

    if (image)
    {
        uint64_t system_ts = k4a_image_get_system_timestamp_nsec(image);

        uint64_t system_ts_from_pts = lookup_system_ts(k4a_image_get_device_timestamp_usec(image), process_color);

        // Time from center of exposure until given to us from the SDK; based on Host system time.
        uint64_t system_ts_latency = current_system_ts - system_ts;

        // Time from center of exposure PTS time (converted to system time based on low latency IMU data) until we
        // read the frame; based on Host system time.
        uint64_t system_ts_latency_from_pts = current_system_ts - system_ts_from_pts;
        if (system_ts_from_pts > current_system_ts)
        {
            printf("Calculated %s pts system time %" PRId64 " is after our arrival system time %" PRId64
                   " a diff of %" PRId64 "\n",
                   process_color ? "color" : "IR",
                   STS_TO_MS(system_ts_from_pts),
                   STS_TO_MS(current_system_ts),
                   STS_TO_MS(system_ts_from_pts - current_system_ts));

            // Update values anyway
            *system_ts_last = system_ts;
            *system_ts_from_pts_last = system_ts_from_pts;
        }
        else
        {

            if (!*image_first_pass)
            {
                system_latency->push_back(current_system_ts - system_ts);
                system_latency_from_pts->push_back(system_ts_latency_from_pts);

                printf("| %9" PRId64 " [%5" PRId64 "] [%5" PRId64 "] ",
                       STS_TO_MS(system_ts),
                       STS_TO_MS(system_ts_latency),
                       STS_TO_MS(system_ts_latency_from_pts));

                // TS should increase
                EXPECT_GT(system_ts, *system_ts_last);
                EXPECT_GT(system_ts_from_pts, *system_ts_from_pts_last);
            }
            *system_ts_last = system_ts;
            *system_ts_from_pts_last = system_ts_from_pts;
            *image_first_pass = false;
        }

        k4a_image_release(image);
    }
    else
    {
        printf("|                           ");
    }
}

TEST_P(latency_perf, testTest)
{
    auto as = GetParam();
    const int32_t TIMEOUT_IN_MS = 1000;
    k4a_capture_t capture = NULL;
    int capture_count = g_capture_count;
    bool failed = false;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    thread_data thread = { 0 };
    THREAD_HANDLE th1 = NULL;
    std::deque<uint64_t> color_system_latency;
    std::deque<uint64_t> color_system_latency_from_pts;
    std::deque<uint64_t> ir_system_latency;
    std::deque<uint64_t> ir_system_latency_from_pts;
    uint64_t current_system_ts = 0;
    uint64_t color_system_ts_last = 0, color_system_ts_from_pts_last = 0;
    uint64_t ir_system_ts_last = 0, ir_system_ts_from_pts_last = 0;
    int32_t read_exposure = 0;

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
        read_exposure = 0; // Clear this so we read it again after sensor is started.
    }
    else
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_set_color_control(m_device,
                                               K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                               K4A_COLOR_CONTROL_MODE_AUTO,
                                               0));
        printf("Auto Exposure\n");
        read_exposure = 0;
    }

    config.color_format = as.color_format;
    config.color_resolution = as.color_resolution;
    config.depth_mode = as.depth_mode;
    config.camera_fps = as.fps;
    config.depth_delay_off_color_usec = g_depth_delay_off_color_usec;
    config.wired_sync_mode = g_wired_sync_mode;
    config.synchronized_images_only = g_synchronized_images_only;
    config.subordinate_delay_off_master_usec = g_subordinate_delay_off_master_usec;

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

    thread.device = m_device;
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th1, _latency_imu_thread, &thread));

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
        capture = NULL;
    }

    printf("Sys lat: is this difference in the system time recorded on the image and the system time when the image "
           "was presented to the caller.\n");
    printf(
        "PTS lat: Similar to Sys lat, but instead of using the system time assigned to the image (which is recorded by "
        "the Host PC), the image PTS (which is center of exposure in single camera mode) is used to "
        "calculate a more accurate system time from when the same PTS arrived from the least latent sensor source, "
        "IMU. The IMU data received is turned into a list of PTS values and associated system ts's for when each "
        "sample arrived on system.\n");
    printf("+---------------------------+---------------------------+\n");
    printf("|         Color Info (ms)   |     IR 16 Info (ms)       |\n");
    printf("|   system  [ sys ] [ PTS ] |   system  [ sys ] [ PTS ] |\n");
    printf("|     ts    [ lat ] [ lat ] |     ts    [ lat ] [ lat ] |\n");
    printf("+---------------------------+---------------------------+\n");

    thread.save_samples = true; // start saving IMU samples
    bool color_first_pass = true;
    bool ir_first_pass = true;
    capture_count++; // to account for dropping the first sample
    while (capture_count-- > 0)
    {
        if (capture)
        {
            k4a_capture_release(capture);
        }

        // Get a depth frame
        k4a_wait_result_t wresult = k4a_device_get_capture(m_device, &capture, TIMEOUT_IN_MS);
        if (wresult != K4A_WAIT_RESULT_SUCCEEDED)
        {
            if (wresult == K4A_WAIT_RESULT_TIMEOUT)
            {
                printf("Timed out waiting for a capture\n");
            }
            else // wresult == K4A_WAIT_RESULT_FAILED:
            {
                printf("Failed to read a capture\n");
                capture_count = 0;
            }
            failed = true;
            continue;
        }

        if (get_system_time(&current_system_ts) == 0)
        {
            printf("Timed out waiting for a capture\n");
            failed = true;
            continue;
        }

        if (read_exposure == 0)
        {
            k4a_image_t image = k4a_capture_get_color_image(capture);
            if (image)
            {
                read_exposure = (int32_t)k4a_image_get_exposure_usec(image);
                k4a_image_release(image);
            }
        }

        process_image(capture,
                      current_system_ts,
                      true, // Color Image
                      &color_first_pass,
                      &color_system_latency,
                      &color_system_latency_from_pts,
                      &color_system_ts_last,
                      &color_system_ts_from_pts_last);
        process_image(capture,
                      current_system_ts,
                      false, // IR Image
                      &ir_first_pass,
                      &ir_system_latency,
                      &ir_system_latency_from_pts,
                      &ir_system_ts_last,
                      &ir_system_ts_from_pts_last);

        printf("|\n"); // End of line
    }                  // End capture loop

    thread.exit = true; // shut down IMU thread
    k4a_device_stop_cameras(m_device);
    if (capture)
    {
        k4a_capture_release(capture);
    }

    int thread_result;
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th1, &thread_result));
    ASSERT_EQ(thread_result, (int)K4A_RESULT_SUCCEEDED);

    printf("\nLatency Results:\n");

    {
        // init CSV line
        if (m_file_handle != 0)
        {
            std::time_t date_time = std::time(NULL);
            char buffer_date_time[100];
            std::strftime(buffer_date_time, sizeof(buffer_date_time), "%c", localtime(&date_time));

            const char *computer_name = environment_get_variable("COMPUTERNAME");
            const char *disable_synchronization = environment_get_variable("K4A_DISABLE_SYNCHRONIZATION");

            char buffer[1024];
            snprintf(buffer,
                     sizeof(buffer),
                     "%s, %s, %s, %s,%s, %s, fps, %d, %s, captures, %d, %d, %d,",
                     buffer_date_time,
                     computer_name ? computer_name : "computer name not set",
                     as.test_name,
                     disable_synchronization ? disable_synchronization : "0",
                     get_string_from_color_format(as.color_format),
                     get_string_from_color_resolution(as.color_resolution),
                     k4a_convert_fps_to_uint(as.fps),
                     get_string_from_depth_mode(as.depth_mode),
                     g_capture_count,
                     g_manual_exposure,
                     read_exposure);
            fputs(buffer, m_file_handle);
        }
    }
    {
        uint64_t color_system_latency_ave = 0;
        uint64_t min = (uint64_t)-1;
        uint64_t max = 0;
        for (size_t x = 0; x < color_system_latency.size(); x++)
        {
            color_system_latency_ave += color_system_latency[x];
            if (color_system_latency[x] < min)
            {
                min = color_system_latency[x];
            }
            if (color_system_latency[x] > max)
            {
                max = color_system_latency[x];
            }
        }
        color_system_latency_ave = color_system_latency_ave / color_system_latency.size();
        print_and_log("Color System Time Latency",
                      get_string_from_color_format(config.color_format),
                      STS_TO_MS(color_system_latency_ave),
                      STS_TO_MS(min),
                      STS_TO_MS(max));
    }
    {
        uint64_t color_system_latency_from_pts_ave = 0;
        uint64_t min = (uint64_t)-1;
        uint64_t max = 0;
        for (size_t x = 0; x < color_system_latency_from_pts.size(); x++)
        {
            color_system_latency_from_pts_ave += color_system_latency_from_pts[x];
            if (color_system_latency_from_pts[x] < min)
            {
                min = color_system_latency_from_pts[x];
            }
            if (color_system_latency_from_pts[x] > max)
            {
                max = color_system_latency_from_pts[x];
            }
        }
        color_system_latency_from_pts_ave = color_system_latency_from_pts_ave / color_system_latency_from_pts.size();
        print_and_log("Color System Time PTS Latency",
                      get_string_from_color_format(config.color_format),
                      STS_TO_MS(color_system_latency_from_pts_ave),
                      STS_TO_MS(min),
                      STS_TO_MS(max));
    }
    {
        uint64_t ir_system_latency_ave = 0;
        uint64_t min = (uint64_t)-1;
        uint64_t max = 0;
        for (size_t x = 0; x < ir_system_latency.size(); x++)
        {
            ir_system_latency_ave += ir_system_latency[x];
            if (ir_system_latency[x] < min)
            {
                min = ir_system_latency[x];
            }
            if (ir_system_latency[x] > max)
            {
                max = ir_system_latency[x];
            }
        }
        ir_system_latency_ave = ir_system_latency_ave / ir_system_latency.size();
        print_and_log("   IR System Time Latency",
                      get_string_from_depth_mode(config.depth_mode),
                      STS_TO_MS(ir_system_latency_ave),
                      STS_TO_MS(min),
                      STS_TO_MS(max));
    }
    {
        uint64_t ir_system_latency_from_pts_ave = 0;
        uint64_t min = (uint64_t)-1;
        uint64_t max = 0;
        for (size_t x = 0; x < ir_system_latency_from_pts.size(); x++)
        {
            ir_system_latency_from_pts_ave += ir_system_latency_from_pts[x];
            if (ir_system_latency_from_pts[x] < min)
            {
                min = ir_system_latency_from_pts[x];
            }
            if (ir_system_latency_from_pts[x] > max)
            {
                max = ir_system_latency_from_pts[x];
            }
        }
        ir_system_latency_from_pts_ave = ir_system_latency_from_pts_ave / ir_system_latency_from_pts.size();
        print_and_log("   IR System Time PTS",
                      get_string_from_depth_mode(config.depth_mode),
                      STS_TO_MS(ir_system_latency_from_pts_ave),
                      STS_TO_MS(min),
                      STS_TO_MS(max));
    }

    printf("\n");
    if (m_file_handle != 0)
    {
        // Terminate line
        fputs("\n", m_file_handle);
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
// PASSIVE_IR is fastest Depth Mode - YUY2 is fastest Color mode
static struct latency_parameters tests_30fps[] = {
    // All Color modes with fast Depth
    {  0, "FPS_30_MJPEG_2160P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,   K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  1, "FPS_30_MJPEG_1536P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,   K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  2, "FPS_30_MJPEG_1440P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,   K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  3, "FPS_30_MJPEG_1080P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,   K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  4, "FPS_30_MJPEG_0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_MJPG,   K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
    {  5, "FPS_30_NV12__0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_NV12,   K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
    {  6, "FPS_30_YUY2__0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_YUY2,   K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},
    {  7, "FPS_30_BGRA32_2160P_PASSIVE_IR",    K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_2160P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  8, "FPS_30_BGRA32_1536P_PASSIVE_IR",    K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_1536P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  9, "FPS_30_BGRA32_1440P_PASSIVE_IR",    K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_1440P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 10, "FPS_30_BGRA32_1080P_PASSIVE_IR",    K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_1080P, K4A_DEPTH_MODE_PASSIVE_IR},
    { 11, "FPS_30_BGRA32_0720P_PASSIVE_IR",    K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},

    // All Depth Modes with fastest Color
    { 12, "FPS_30_YUY2__0720P_NFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_YUY2,   K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_2X2BINNED},
    { 13, "FPS_30_YUY2__0720P_NFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_YUY2,   K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_NFOV_UNBINNED},
    { 14, "FPS_30_YUY2__0720P_WFOV_2X2BINNED", K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_YUY2,   K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_2X2BINNED},
    { 15, "FPS_30_YUY2__0720P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_30, K4A_IMAGE_FORMAT_COLOR_YUY2,   K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_PASSIVE_IR},

};

INSTANTIATE_TEST_CASE_P(30FPS_TESTS, latency_perf, ValuesIn(tests_30fps));

static struct latency_parameters tests_15fps[] = {
    // All Color modes with fast Depth
    {  0, "FPS_15_MJPEG_3072P_PASSIVE_IR",     K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_MJPG,   K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_PASSIVE_IR},
    {  1, "FPS_15_BGRA32_3072P_PASSIVE_IR",    K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_BGRA32, K4A_COLOR_RESOLUTION_3072P, K4A_DEPTH_MODE_PASSIVE_IR},

    // All Depth Modes with fastest Color
    {  2, "FPS_15_YUY2__0720P_WFOV_UNBINNED",  K4A_FRAMES_PER_SECOND_15, K4A_IMAGE_FORMAT_COLOR_YUY2,   K4A_COLOR_RESOLUTION_720P,  K4A_DEPTH_MODE_WFOV_UNBINNED},
};

INSTANTIATE_TEST_CASE_P(15FPS_TESTS, latency_perf, ValuesIn(tests_15fps));
// clang-format on

int main(int argc, char **argv)
{
    bool error = false;
    k4a_unittest_init();

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
                printf("g_exposure_setting = %d\n", g_exposure_setting);
                g_manual_exposure = true;
                i++;
            }
            else
            {
                printf("Error: index parameter missing\n");
                error = true;
            }
        }
        else if (strcmp(argument, "--auto") == 0)
        {
            g_manual_exposure = false;
            printf("Auto Exposure Enabled\n");
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
        printf("      Deault is manual exposure with an exposure of 33,333us. This will test with the manual exposure "
               "setting\n");
        printf("      that is passed in.\n");
        printf("  --auto\n");
        printf("      By default the test uses manual exposure. This will test with auto exposure.\n");
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
