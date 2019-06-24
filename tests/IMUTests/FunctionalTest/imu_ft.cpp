// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <k4a/k4a.h>
#include <k4ainternal/common.h>
#include <utcommon.h>
#include <gtest/gtest.h>
#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

//**************Symbolic Constant Macros (defines)  *************
#define STREAM_RUN_TIME_SEC 4
#define ERROR_START_STREAM_TIME 10000

// Total ACC range is +/- 147.15 m/s^2.
#define MIN_ACC_READING -15.0f
#define MAX_ACC_READING 15.0f

// Total Gyro range is +/- 20 rad/s
#define MIN_GYRO_READING -0.1f
#define MAX_GYRO_READING 0.1f
//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

class imu_ft : public ::testing::Test

{
public:
    virtual void SetUp()
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(K4A_DEVICE_DEFAULT, &m_device)) << "Couldn't open device\n";
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

static bool is_float_in_range(float value, float min, float max, const char *description)
{
    if (min < value && value < max)
    {
        return true;
    }
    printf("%s is out of range value:%4.4f min:%4.4f max:%4.4f\n",
           description,
           (double)value,
           (double)min,
           (double)max);
    return false;
}

/**
 *  Utility to configure the sensor and run the sensor at the configuration.
 *  Includes all of the pass / fail conditions as determined by the calling
 *  function
 *
 *  Caller must check HasFatalFailure() since this function may use ASSERT_ failures
 *
 *  @param expected_fps
 *   Expected size of frame
 *
 */
static void RunStreamConfig(k4a_device_t device, uint32_t expected_fps)
{
    uint32_t stream_count;
    int32_t timeout_ms;
    k4a_imu_sample_t imu_sample;
    TICK_COUNTER_HANDLE tick_count;
    tickcounter_ms_t start_ms;
    tickcounter_ms_t end_ms;
    tickcounter_ms_t delta_ms;
    uint32_t error_tolerance;

    stream_count = STREAM_RUN_TIME_SEC * expected_fps;
    tick_count = tickcounter_create();

    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

    config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(device, &config));

    // start streaming.
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(device));

    // allow stream start time
    timeout_ms = ERROR_START_STREAM_TIME;
    ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_imu_sample(device, &imu_sample, timeout_ms));

    // Start clock on getting frames
    tickcounter_get_current_ms(tick_count, &start_ms);
    timeout_ms = 2000;

    uint64_t last_gyro_dev_ts = 0;
    uint64_t last_acc_dev_ts = 0;
    while (stream_count > 0)
    {
        // get frames as available
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_imu_sample(device, &imu_sample, timeout_ms));

        ASSERT_GT(imu_sample.acc_timestamp_usec, last_acc_dev_ts);
        last_acc_dev_ts = imu_sample.acc_timestamp_usec;
        ASSERT_GT(imu_sample.gyro_timestamp_usec, last_gyro_dev_ts);
        last_gyro_dev_ts = imu_sample.gyro_timestamp_usec;

        ASSERT_NE(imu_sample.temperature, 0);
        ASSERT_EQ(true, is_float_in_range(imu_sample.acc_sample.xyz.x, MIN_ACC_READING, MAX_ACC_READING, "ACC_X"));
        ASSERT_EQ(true, is_float_in_range(imu_sample.acc_sample.xyz.y, MIN_ACC_READING, MAX_ACC_READING, "ACC_Y"));
        ASSERT_EQ(true, is_float_in_range(imu_sample.acc_sample.xyz.z, MIN_ACC_READING, MAX_ACC_READING, "ACC_Z"));
        ASSERT_EQ(true, is_float_in_range(imu_sample.gyro_sample.xyz.x, MIN_GYRO_READING, MAX_GYRO_READING, "GYRO_X"));
        ASSERT_EQ(true, is_float_in_range(imu_sample.gyro_sample.xyz.y, MIN_GYRO_READING, MAX_GYRO_READING, "GYRO_Y"));
        ASSERT_EQ(true, is_float_in_range(imu_sample.gyro_sample.xyz.z, MIN_GYRO_READING, MAX_GYRO_READING, "GYRO_Z"));

        stream_count--;
    };

    // Check if this was the correct frame rate (+/- 10%)
    tickcounter_get_current_ms(tick_count, &end_ms);
    delta_ms = end_ms - start_ms;
    k4a_device_stop_imu(device);

    error_tolerance = STREAM_RUN_TIME_SEC * 100; // 10%
    if (delta_ms > ((STREAM_RUN_TIME_SEC * 1000) + error_tolerance))
    {
        std::cout << "Frame rate too slow, " << (1000 * (STREAM_RUN_TIME_SEC * expected_fps)) / delta_ms << "fps\n";
    }
    if (delta_ms < ((STREAM_RUN_TIME_SEC * 1000) - error_tolerance))
    {
        std::cout << "Frame rate too fast, " << (1000 * (STREAM_RUN_TIME_SEC * expected_fps)) / delta_ms << "fps\n";
    }
    k4a_device_stop_cameras(device);
    tickcounter_destroy(tick_count);
}

/**
 *  Functional test for verifying IMU
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the stream
 *   Frames shall be 1474560 bytes
 *   Frames shall not be dropped
 *   FPS   shall be 1500 FPS +/- 10%
 *
 */
TEST_F(imu_ft, imuStreamFull)
{
    RunStreamConfig(m_device, K4A_IMU_SAMPLE_RATE);
    if (HasFatalFailure())
        return;
}

TEST_F(imu_ft, imu_start)
{
    //    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_device_configuration_t config_all_running = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

    config_all_running.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config_all_running.color_resolution = K4A_COLOR_RESOLUTION_2160P;
    config_all_running.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config_all_running.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config = config_all_running;

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_start_imu(m_device)); // Sensor not running

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(m_device));
    k4a_device_stop_cameras(m_device);
    k4a_device_stop_imu(m_device);

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_start_imu(m_device)); // Sensor not running

    // IMU can start and stop as many times as it wants while color camera continues to run
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(m_device));
    k4a_device_stop_imu(m_device);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(m_device));
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_start_imu(m_device)); // already running
    k4a_device_stop_imu(m_device);
    k4a_device_stop_cameras(m_device);

    // color/depth camera can only start if IMU is not running - in this case it was left running
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(m_device));
    k4a_device_stop_cameras(m_device);
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_start_cameras(m_device, &config));
    k4a_device_stop_imu(m_device);

    // Sanity check last test didn't break us
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(m_device));
    k4a_device_stop_cameras(m_device);
    k4a_device_stop_imu(m_device);

    // Start only if running depth camera
    config = config_all_running;
    config.color_resolution = K4A_COLOR_RESOLUTION_OFF;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(m_device));
    k4a_device_stop_cameras(m_device);
    k4a_device_stop_imu(m_device);

    // Start only if running color camera
    config = config_all_running;
    config.depth_mode = K4A_DEPTH_MODE_OFF;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_imu(m_device));
    k4a_device_stop_cameras(m_device);
    k4a_device_stop_imu(m_device);
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
