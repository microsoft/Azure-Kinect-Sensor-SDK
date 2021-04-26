// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <k4a/k4a.h>
#include <k4ainternal/modes.h>
#include <gtest/gtest.h>
#include <utcommon.h>
#include <azure_c_shared_utility/tickcounter.h>

//**************Symbolic Constant Macros (defines)  *************
#define STREAM_RUN_TIME_SEC 4
#define ERROR_START_STREAM_TIME 10000
#define K4A_DEPTH_MODE_NFOV_2X2BINNED_EXPECTED_SIZE 184320 // 368640
#define K4A_DEPTH_MODE_NFOV_UNBINNED_EXPECTED_SIZE 737280  // 1474560
#define K4A_DEPTH_MODE_WFOV_2X2BINNED_EXPECTED_SIZE 524288 // 1048576
#define K4A_DEPTH_MODE_WFOV_UNBINNED_EXPECTED_SIZE 2097152 // 4194304
#define K4A_DEPTH_MODE_PASSIVE_IR_EXPECTED_SIZE 2097152    // 4194304
#define DEPTH_MODE_EXPECTED_FPS_30 30
#define DEPTH_MODE_EXPECTED_FPS_15 15
#define DEPTH_MODE_EXPECTED_FPS_5 5
#define MAX_BUFFER_SIZE 256
#define SERIAL_NUMBER_SIZE 12

//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

class depth_ft : public ::testing::Test
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

/**
 *  Functional test for verifying correct serial number format
 *
 *  @Test criteria
 *   Pass conditions;
 *       Serial number is 12 digits long
 *       The serial number shall only be comprised of ascii digits
 *       Digit 7 (1 based) is mod-7 Check Digit. (7-(sum(digits 1 - 6) % 7)
 *
 *
 */
TEST_F(depth_ft, depthSerialNumber)
{
    char char_buffer[MAX_BUFFER_SIZE] = { 0 };
    size_t serial_number_size = MAX_BUFFER_SIZE;
    uint16_t check;

    ASSERT_EQ(K4A_BUFFER_RESULT_SUCCEEDED, k4a_device_get_serialnum(m_device, char_buffer, &serial_number_size))
        << "Couldn't get serial number information\n";

    EXPECT_GE(serial_number_size, (size_t)SERIAL_NUMBER_SIZE) << "Serial Number Length invalid\n";
    if (serial_number_size > 0)
    {
        // Serial number is null terminated and serial_number_size include that NULL
        for (uint8_t i = 0; i < serial_number_size - 1; i++)
        {
            EXPECT_TRUE(isdigit(char_buffer[i]))
                << "Failed index " << (int)i << " of " << serial_number_size - 1 << " loop iteration\n";
        }
    }

    check = (uint8_t)char_buffer[0] + (uint8_t)char_buffer[1] + (uint8_t)char_buffer[2] + (uint8_t)char_buffer[3] +
            (uint8_t)char_buffer[4] + (uint8_t)char_buffer[5];
    check = 7 - (check % 7);
    EXPECT_GE(char_buffer[6], (char)check) << "Serial Number check value invalid\n";

    if (serial_number_size > 0)
    {
        // This might crash as we may have failed digit validation or may not be NULL terminated
        std::cout << "Serial Number read: " << char_buffer << "\n";
    }
}

/**
 *  Utility to configure the sensor and run the sensor at the configuration.
 *  Includes all of the pass / fail conditions as determined by the calling
 *  function
 *
 *  Caller must check HasFatalFailure() since this function may use ASSERT_ failures
 *
 *  @param depth_fps
 *   Frame per second configuration
 *
 *  @param depth_mode
 *   Depth operating mode configuration
 *
 *  @param expected_depth_capture_size
 *   Expected size of frame
 *
 */
static void RunStreamConfig(k4a_device_t device,
                            k4a_fps_t depth_fps,
                            k4a_depth_mode_t depth_mode,
                            size_t expected_depth_capture_size,
                            uint32_t expected_fps)
{
    uint32_t stream_count;
    int timeout_ms;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_capture_t capture;
    k4a_image_t image;
    uint8_t *ir16;
    uint8_t *depth16;
    TICK_COUNTER_HANDLE tick_count;
    tickcounter_ms_t start_ms;
    tickcounter_ms_t end_ms;
    tickcounter_ms_t delta_ms;
    uint32_t error_tolerance;

    bool depth16_present = (depth_mode == K4A_DEPTH_MODE_NFOV_2X2BINNED || depth_mode == K4A_DEPTH_MODE_NFOV_UNBINNED ||
                            depth_mode == K4A_DEPTH_MODE_WFOV_2X2BINNED || depth_mode == K4A_DEPTH_MODE_WFOV_UNBINNED);

    stream_count = STREAM_RUN_TIME_SEC * expected_fps;
    tick_count = tickcounter_create();

    // Configure the stream
    config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config.color_mode_id = K4A_COLOR_RESOLUTION_OFF;
    config.depth_mode_id = depth_mode;
    config.fps_mode_id = depth_fps;

    // start streaming.
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(device, &config));

    // allow stream start time
    timeout_ms = ERROR_START_STREAM_TIME;
    ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(device, &capture, timeout_ms));
    ASSERT_NE(image = k4a_capture_get_ir_image(capture), (k4a_image_t)NULL);
    ASSERT_NE(ir16 = k4a_image_get_buffer(image), (uint8_t *)NULL);
    ASSERT_EQ(k4a_image_get_size(image), expected_depth_capture_size);
    k4a_image_release(image);

    if (depth16_present)
    {
        ASSERT_NE(image = k4a_capture_get_depth_image(capture), (k4a_image_t)NULL);
        ASSERT_NE(depth16 = k4a_image_get_buffer(image), (uint8_t *)NULL);
        ASSERT_NE(depth16, ir16);
        ASSERT_EQ(k4a_image_get_size(image), expected_depth_capture_size);
        k4a_image_release(image);
    }
    k4a_capture_release(capture);

    // Start clock on getting frames
    tickcounter_get_current_ms(tick_count, &start_ms);
    timeout_ms = 2000;

    while (stream_count > 0)
    {
        // get captures as available
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(device, &capture, timeout_ms))
            << "Failed to receive capture.  Timeout = " << timeout_ms << "msec, capture #"
            << ((STREAM_RUN_TIME_SEC * expected_fps) - stream_count) << "\n";

        stream_count--;

        ASSERT_NE(image = k4a_capture_get_ir_image(capture), (k4a_image_t)NULL);
        ASSERT_NE((uint8_t *)NULL, ir16 = k4a_image_get_buffer(image));

        // Verify the image size for this mode
        ASSERT_EQ(expected_depth_capture_size, k4a_image_get_size(image));
        k4a_image_release(image);

        if (depth16_present)
        {
            ASSERT_NE(image = k4a_capture_get_depth_image(capture), (k4a_image_t)NULL);
            ASSERT_NE((uint8_t *)NULL, depth16 = k4a_image_get_buffer(image));
            ASSERT_NE(ir16, depth16);

            // Verify the image size for this mode
            ASSERT_EQ(expected_depth_capture_size, k4a_image_get_size(image));
            k4a_image_release(image);
        }

        k4a_capture_release(capture);
    };

    // Check if this was the correct capture rate (+/- 10%)
    tickcounter_get_current_ms(tick_count, &end_ms);
    delta_ms = end_ms - start_ms;
    k4a_device_stop_cameras(device);

    error_tolerance = STREAM_RUN_TIME_SEC * 100; // 10%
#if 0
    Bug 1170246
    EXPECT_LT(delta_ms, (STREAM_RUN_TIME_SEC * 1000) + error_tolerance)
        << "Frame rate too slow, " << (1000 * (STREAM_RUN_TIME_SEC * expected_fps)) / delta_ms << "fps\n";
    EXPECT_GT(delta_ms, (STREAM_RUN_TIME_SEC * 1000) - error_tolerance)
        << "Frame rate too fast, " << (1000 * (STREAM_RUN_TIME_SEC * expected_fps)) / delta_ms << "fps\n";
#else
    if (delta_ms > ((STREAM_RUN_TIME_SEC * 1000) + error_tolerance))
    {
        std::cout << "Frame rate too slow, " << (1000 * (STREAM_RUN_TIME_SEC * expected_fps)) / delta_ms << "fps\n";
    }
    if (delta_ms < ((STREAM_RUN_TIME_SEC * 1000) - error_tolerance))
    {
        std::cout << "Frame rate too fast, " << (1000 * (STREAM_RUN_TIME_SEC * expected_fps)) / delta_ms << "fps\n";
    }
#endif
    tickcounter_destroy(tick_count);
}

/**
 *  Functional test for verifying 30 FPS depth, Narrow FOV, Unbinned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 1474560 bytes
 *   Frames shall not be dropped
 *   FPS   shall be 30 FPS +/- 10%
 *
 */
TEST_F(depth_ft, depthStream30FPS_NFOV_UNBINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_30,
                    K4A_DEPTH_MODE_NFOV_UNBINNED,
                    K4A_DEPTH_MODE_NFOV_UNBINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_30);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 15 FPS depth, Narrow FOV, Unbinned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 1474560 bytes
 *   Frames shall not be dropped
 *   FPS   shall be 15 FPS +/- 10%*
 */
TEST_F(depth_ft, depthStream15FPS_NFOV_UNBINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_15,
                    K4A_DEPTH_MODE_NFOV_UNBINNED,
                    K4A_DEPTH_MODE_NFOV_UNBINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_15);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 5 FPS depth, Narrow FOV, Unbinned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 1474560 bytes
 *   Frames shall not be dropped
 *   FPS shall be 5 FPS +/- 10%*
 */
TEST_F(depth_ft, depthStream5FPS_NFOV_UNBINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_5,
                    K4A_DEPTH_MODE_NFOV_UNBINNED,
                    K4A_DEPTH_MODE_NFOV_UNBINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_5);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 30 FPS depth, Narrow FOV, 2x2 binned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 368640 bytes
 *   Frames shall not be dropped
 *   FPS   shall be 30 FPS +/- 10%
 *
 */
TEST_F(depth_ft, depthStream30FPS_NFOV_2X2BINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_30,
                    K4A_DEPTH_MODE_NFOV_2X2BINNED,
                    K4A_DEPTH_MODE_NFOV_2X2BINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_30);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 15 FPS depth, Narrow FOV, 2x2 binned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 368640 bytes
 *   Frames shall not be dropped
 *   FPS   shall be 15 FPS +/- 10%*
 */
TEST_F(depth_ft, depthStream15FPS_NFOV_2X2BINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_15,
                    K4A_DEPTH_MODE_NFOV_2X2BINNED,
                    K4A_DEPTH_MODE_NFOV_2X2BINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_15);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 5 FPS depth, Narrow FOV, 2x2 binned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 368640 bytes
 *   Frames shall not be dropped
 *   FPS shall be 5 FPS +/- 10%*
 */
TEST_F(depth_ft, depthStream5FPS_NFOV_2X2BINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_5,
                    K4A_DEPTH_MODE_NFOV_2X2BINNED,
                    K4A_DEPTH_MODE_NFOV_2X2BINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_5);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 30 FPS depth, Wide FOV, 2x2 binned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 1048576 bytes
 *   Frames shall not be dropped
 *   FPS shall be 30 FPS +/- 10%*
 */
TEST_F(depth_ft, depthStream30FPS_WFOV_2x2BINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_30,
                    K4A_DEPTH_MODE_WFOV_2X2BINNED,
                    K4A_DEPTH_MODE_WFOV_2X2BINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_30);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 15 FPS depth, Wide FOV, 2x2 binned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 1048576 bytes
 *   Frames shall not be dropped
 *   FPS shall be 15 FPS +/- 10%*
 */
TEST_F(depth_ft, depthStream15FPS_WFOV_2x2BINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_15,
                    K4A_DEPTH_MODE_WFOV_2X2BINNED,
                    K4A_DEPTH_MODE_WFOV_2X2BINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_15);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 5 FPS depth, Wide FOV, 2x2 binned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 1048576 bytes
 *   Frames shall not be dropped
 *   FPS shall be 5 FPS +/- 10%*
 */
TEST_F(depth_ft, depthStream5FPS_WFOV_2x2BINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_5,
                    K4A_DEPTH_MODE_WFOV_2X2BINNED,
                    K4A_DEPTH_MODE_WFOV_2X2BINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_5);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 15 FPS depth, Wide FOV, Unbinned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 4194304 bytes
 *   Frames shall not be dropped
 *   FPS shall be 15 FPS +/- 10%*
 */
TEST_F(depth_ft, depthStream15FPS_WFOV_UNBINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_15,
                    K4A_DEPTH_MODE_WFOV_UNBINNED,
                    K4A_DEPTH_MODE_WFOV_UNBINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_15);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 5 FPS depth, Wide FOV, Unbinned
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 4194304 bytes
 *   Frames shall not be dropped
 *   FPS shall be 5 FPS +/- 10%*
 */
TEST_F(depth_ft, depthStream5FPS_WFOV_UNBINNED)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_5,
                    K4A_DEPTH_MODE_WFOV_UNBINNED,
                    K4A_DEPTH_MODE_WFOV_UNBINNED_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_5);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying 30 FPS depth, Passive IR
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be 4194304 bytes
 *   Frames shall not be dropped
 *
 */
TEST_F(depth_ft, depthStream30FPS_PASSIVE_IR)
{
    RunStreamConfig(m_device,
                    K4A_FRAMES_PER_SECOND_30,
                    K4A_DEPTH_MODE_PASSIVE_IR,
                    K4A_DEPTH_MODE_PASSIVE_IR_EXPECTED_SIZE,
                    DEPTH_MODE_EXPECTED_FPS_30);
    if (HasFatalFailure())
        return;
}

/**
 *  Functional test for verifying that changing modes actually causes data to be returned in the right mode
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the depth stream
 *   Frames shall be of the correct size for the mode the device is configured with
 *
 */
TEST_F(depth_ft, depthModeChange)
{
    int32_t timeout_ms = ERROR_START_STREAM_TIME;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_device_configuration_t config2 = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_capture_t depth_capture;
    k4a_image_t image;
    const size_t config_expected_capture_size = K4A_DEPTH_MODE_NFOV_UNBINNED_EXPECTED_SIZE;
    const size_t config2_expected_capture_size = K4A_DEPTH_MODE_NFOV_2X2BINNED_EXPECTED_SIZE;

    static_assert(config_expected_capture_size != config2_expected_capture_size,
                  "Test modes should have different-sized payloads");

    // Create two valid configs that are expected to yield different-sized depth payloads
    //
    config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config.color_mode_id = K4A_COLOR_RESOLUTION_OFF;
    config.depth_mode_id = K4A_DEPTH_MODE_NFOV_UNBINNED;
    config.fps_mode_id = K4A_FRAMES_PER_SECOND_15;

    config2.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    config2.color_mode_id = K4A_COLOR_RESOLUTION_OFF;
    config2.depth_mode_id = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    config2.fps_mode_id = K4A_FRAMES_PER_SECOND_15;

    // Start device in first mode and check frame size
    //
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));

    ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &depth_capture, timeout_ms));

    if (config.depth_mode_id != K4A_DEPTH_MODE_PASSIVE_IR)
    {
        image = k4a_capture_get_depth_image(depth_capture);
        ASSERT_NE(k4a_image_get_buffer(image), (uint8_t *)NULL);
        EXPECT_EQ(config_expected_capture_size, k4a_image_get_size(image));
        k4a_image_release(image);
    }

    ASSERT_NE(image = k4a_capture_get_ir_image(depth_capture), (k4a_image_t)NULL);
    ASSERT_NE(k4a_image_get_buffer(image), (uint8_t *)NULL);
    EXPECT_EQ(config_expected_capture_size, k4a_image_get_size(image));
    k4a_image_release(image);

    k4a_capture_release(depth_capture);

    k4a_device_stop_cameras(m_device);

    // Start device in second mode and check frame size
    //
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config2));

    ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &depth_capture, timeout_ms));

    if (config2.depth_mode_id != K4A_DEPTH_MODE_PASSIVE_IR)
    {
        image = k4a_capture_get_depth_image(depth_capture);
        ASSERT_NE(k4a_image_get_buffer(image), (uint8_t *)NULL);
        EXPECT_EQ(config2_expected_capture_size, k4a_image_get_size(image));
        k4a_image_release(image);
    }

    ASSERT_NE(image = k4a_capture_get_ir_image(depth_capture), (k4a_image_t)NULL);
    ASSERT_NE(k4a_image_get_buffer(image), (uint8_t *)NULL);
    EXPECT_EQ(config2_expected_capture_size, k4a_image_get_size(image));
    k4a_image_release(image);

    (void)k4a_capture_release(depth_capture);

    k4a_device_stop_cameras(m_device);
}

/**
 *  Functional test for verifying correct device info.
 *
 *  @Test criteria
 *   Pass conditions;
 *       Device info has non-zero VID, PID, and capabilities.
 *       Capabilities shall only be composed of binary bitmap allowed values.
 *
 */
TEST_F(depth_ft, deviceInfo)
{
    K4A_INIT_STRUCT(k4a_device_info_t, deviceInfo)

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_info(NULL, &deviceInfo))
        << "Unexpected return value for invalid device handle.";

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_info(m_device, NULL))
        << "Unexpected return value for invalid device info pointer.";

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_info(m_device, &deviceInfo)) << "Couldn't get device information\n";

    EXPECT_EQ(deviceInfo.struct_version, static_cast<uint32_t>(K4A_ABI_VERSION))
        << "Device info struct version invalid\n";
    EXPECT_EQ(deviceInfo.struct_size, static_cast<uint32_t>(sizeof(k4a_device_info_t)))
        << "Device info struct size invalid\n";
    EXPECT_NE(deviceInfo.vendor_id, static_cast<uint32_t>(0)) << "Device info vendor id invalid\n";
    EXPECT_NE(deviceInfo.device_id, static_cast<uint32_t>(0)) << "Device info device id invalid\n";
    EXPECT_GT(deviceInfo.capabilities.value, static_cast<uint32_t>(0))
        << "Device info capabilities must be greater than 0.\n";
}

/**
 *  Functional test for verifying depth modes.
 *
 *  @Test criteria
 *   Pass conditions;
 *       Calling k4a_device_get_depth_mode_count() and k4a_device_get_depth_mode() has proper return values.
 *
 */
TEST_F(depth_ft, deviceModeInfo)
{
    K4A_INIT_STRUCT(k4a_depth_mode_info_t, depthModeInfo)
    uint32_t depthModeCount = 0;

    // Test invalid arguments.
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_depth_mode_count(NULL, &depthModeCount))
        << "Unexpected return value for invalid device handle.\n";

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_depth_mode_count(m_device, NULL))
        << "Unexpected return value for invalid depth mode count pointer.\n";

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_depth_mode(NULL, 0, &depthModeInfo))
        << "Unexpected return value for invalid device handle.\n";

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_depth_mode(m_device, 0, NULL))
        << "Unexpected return value for invalid device handle.\n";

    // Get the depth mode count.
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_depth_mode_count(m_device, &depthModeCount))
        << "Couldn't get depth mode count.\n";

    // Test invalid depth mode index.
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_depth_mode(m_device, depthModeCount, &depthModeInfo))
        << "Unexpected return value for depth mode index greater than number of depth modes.\n";

    // Get depth mode info.
    for (uint32_t d = 0; d < depthModeCount; ++d)
    {
        depthModeInfo = { depthModeInfo.struct_size, depthModeInfo.struct_version, 0 };

        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_depth_mode(m_device, d, &depthModeInfo))
            << "Couldn't get depth mode info.\n";

        EXPECT_EQ(depthModeInfo.struct_version, static_cast<uint32_t>(K4A_ABI_VERSION))
            << "Depth mode struct version invalid\n";

        EXPECT_EQ(depthModeInfo.struct_size, static_cast<uint32_t>(sizeof(k4a_depth_mode_info_t)))
            << "Depth mode struct size invalid\n";
    }
}

/**
 *  Functional test for verifying fps modes.
 *
 *  @Test criteria
 *   Pass conditions;
 *       Calling k4a_device_get_fps_mode_count() and k4a_device_get_fps_mode() has proper return values.
 *
 */
TEST_F(depth_ft, fpsModeInfo)
{
    K4A_INIT_STRUCT(k4a_fps_mode_info_t, fpsModeInfo)
    uint32_t fpsModeCount = 0;

    // Test invalid arguments.
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_fps_mode_count(NULL, &fpsModeCount))
        << "Unexpected return value for invalid device handle.\n";

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_fps_mode_count(m_device, NULL))
        << "Unexpected return value for invalid depth mode count pointer.\n";

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_fps_mode(NULL, 0, &fpsModeInfo))
        << "Unexpected return value for invalid device handle.\n";

    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_fps_mode(m_device, 0, NULL))
        << "Unexpected return value for invalid device handle.\n";

    // Get the depth mode count.
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_fps_mode_count(m_device, &fpsModeCount))
        << "Couldn't get fps mode count.\n";

    // Test invalid depth mode index.
    ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_get_fps_mode(m_device, fpsModeCount, &fpsModeInfo))
        << "Unexpected return value for fps mode index greater than number of depth modes.\n";

    // Get fps mode info.
    for (uint32_t d = 0; d < fpsModeCount; ++d)
    {
        fpsModeInfo = { fpsModeInfo.struct_size, fpsModeInfo.struct_version, 0 };

        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_fps_mode(m_device, d, &fpsModeInfo))
            << "Couldn't get fps mode info.\n";

        EXPECT_EQ(fpsModeInfo.struct_version, static_cast<uint32_t>(K4A_ABI_VERSION))
            << "FPS mode struct version invalid\n";

        EXPECT_EQ(fpsModeInfo.struct_size, static_cast<uint32_t>(sizeof(k4a_fps_mode_info_t)))
            << "FPS mode struct size invalid\n";
    }
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
