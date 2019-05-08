// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <k4a/k4a.h>
#include <utcommon.h>
#include <gtest/gtest.h>
#include <azure_c_shared_utility/tickcounter.h>
#include <math.h>

//************** Symbolic Constant Macros (defines) *************
#define STREAM_RUN_TIME_SEC 4
#define ERROR_START_STREAM_TIME 10000
#define K4A_COLOR_MODE_NV12_720P_EXPECTED_SIZE (1280 * 720 * 3 / 2) //  1,382,400 bytes
#define K4A_COLOR_MODE_YUY2_720P_EXPECTED_SIZE (1280 * 720 * 2)     //  1,843,200 bytes
#define K4A_COLOR_MODE_MJPG_EXPECTED_SIZE 0                         // Unknown, not static size
#define K4A_COLOR_MODE_RGB_720P_EXPECTED_SIZE (1280 * 720 * 4)
#define K4A_COLOR_MODE_RGB_1080P_EXPECTED_SIZE (1920 * 1080 * 4)
#define K4A_COLOR_MODE_RGB_1440P_EXPECTED_SIZE (2560 * 1440 * 4)
#define K4A_COLOR_MODE_RGB_1536P_EXPECTED_SIZE (2048 * 1536 * 4)
#define K4A_COLOR_MODE_RGB_2160P_EXPECTED_SIZE (3840 * 2160 * 4)
#define K4A_COLOR_MODE_RGB_3072P_EXPECTED_SIZE (4096 * 3072 * 4)
#define K4A_COLOR_MODE_EXPECTED_FPS_30 30
#define K4A_COLOR_MODE_EXPECTED_FPS_15 15
#define K4A_COLOR_MODE_EXPECTED_FPS_5 5

//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

struct color_mode_parameter
{
    uint32_t test_index;
    k4a_image_format_t color_format;
    k4a_color_resolution_t color_resolution;
    k4a_fps_t color_rate;
    size_t expected_image_size;
    uint32_t expected_fps;

    friend std::ostream &operator<<(std::ostream &os, const color_mode_parameter &obj)
    {
        return os << "test index: " << (int)obj.test_index;
    }
};

class color_functional_test : public ::testing::Test, public ::testing::WithParamInterface<color_mode_parameter>
{
public:
    // SetUp() will be called before each test iteration
    // Constructor may be used for same purpose, but Setup() allows to check failure on it.
    void SetUp()
    {
        m_tick_count = tickcounter_create();
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_open(K4A_DEVICE_DEFAULT, &m_device)) << "Couldn't open device\n";
        ASSERT_NE(m_device, nullptr);
    }

    // TearDown() will be called after each test iteration
    // Destructor can be used to clean up, but because k4a_device_open() is called in Setup(),
    // matching TearDown() would be better place to close handles.
    void TearDown()
    {
        if (m_device != nullptr)
        {
            k4a_device_close(m_device);
            m_device = nullptr;
        }

        if (m_tick_count != nullptr)
        {
            tickcounter_destroy(m_tick_count);
            m_tick_count = nullptr;
        }
    }

protected:
    k4a_device_t m_device = nullptr;
    TICK_COUNTER_HANDLE m_tick_count = nullptr;
};

TEST_P(color_functional_test, color_streaming_test)
{
    auto as = GetParam();
    uint32_t stream_count;
    int32_t timeout_ms;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_capture_t capture;
    k4a_image_t image;
    uint8_t *buffer = nullptr;
    tickcounter_ms_t start_ms;
    tickcounter_ms_t end_ms;
    tickcounter_ms_t delta_ms;
    uint32_t error_tolerance;

    stream_count = STREAM_RUN_TIME_SEC * as.expected_fps;

    // Configure the stream
    config.camera_fps = as.color_rate;
    config.color_format = as.color_format;
    config.color_resolution = as.color_resolution;
    config.depth_mode = K4A_DEPTH_MODE_OFF;

    // start streaming.
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));

    // allow stream start time
    timeout_ms = ERROR_START_STREAM_TIME;
    ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &capture, timeout_ms));
    k4a_capture_release(capture);

    // Start clock on getting frames
    tickcounter_get_current_ms(m_tick_count, &start_ms);
    timeout_ms = 2000;

    while (stream_count > 0)
    {
        // get frames as available
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &capture, timeout_ms))
            << "Failed to receive frame.  Timeout = " << timeout_ms << "msec, frame #"
            << ((STREAM_RUN_TIME_SEC * as.expected_fps) - stream_count) << "\n";

        stream_count--;
        size_t image_size;
        image = k4a_capture_get_color_image(capture);
        ASSERT_NE(image, (k4a_image_t)NULL);
        buffer = k4a_image_get_buffer(image);
        image_size = k4a_image_get_size(image);
        ASSERT_NE((uint8_t *)NULL, buffer);
        ASSERT_NE((size_t)0, image_size);

        // Verify the frame size for this mode
        if (as.expected_image_size == 0)
        {
            ASSERT_GT(image_size, as.expected_image_size) << "Failed due to invalid frame size\n";
        }
        else
        {
            ASSERT_EQ(as.expected_image_size, image_size) << "Failed due to invalid frame size\n";
        }

        // Check image stride
        switch (as.color_format)
        {
        case K4A_IMAGE_FORMAT_COLOR_MJPG:
            EXPECT_EQ(0, k4a_image_get_stride_bytes(image));
            break;
        case K4A_IMAGE_FORMAT_COLOR_NV12:
            EXPECT_EQ(k4a_image_get_width_pixels(image), k4a_image_get_stride_bytes(image));
            break;
        case K4A_IMAGE_FORMAT_COLOR_YUY2:
            EXPECT_EQ(k4a_image_get_width_pixels(image) * 2, k4a_image_get_stride_bytes(image));
            break;
        case K4A_IMAGE_FORMAT_COLOR_BGRA32:
            EXPECT_EQ(k4a_image_get_width_pixels(image) * 4, k4a_image_get_stride_bytes(image));
            break;
        default:
            break;
        }

        k4a_image_release(image);
        k4a_capture_release(capture);
    };

    // Check if this was the correct frame rate (+/- 10%)
    tickcounter_get_current_ms(m_tick_count, &end_ms);
    delta_ms = end_ms - start_ms;
    k4a_device_stop_cameras(m_device);

    error_tolerance = STREAM_RUN_TIME_SEC * 100; // 10%
    if (delta_ms > ((STREAM_RUN_TIME_SEC * 1000) + error_tolerance))
    {
        std::cout << "Frame rate too slow, " << (1000 * (STREAM_RUN_TIME_SEC * as.expected_fps)) / delta_ms << "fps\n";
    }
    if (delta_ms < ((STREAM_RUN_TIME_SEC * 1000) - error_tolerance))
    {
        std::cout << "Frame rate too fast, " << (1000 * (STREAM_RUN_TIME_SEC * as.expected_fps)) / delta_ms << "fps\n";
    }
}

INSTANTIATE_TEST_CASE_P(color_streaming,
                        color_functional_test,
                        ::testing::Values(
                            // 30 fps tests
                            color_mode_parameter{ 0,
                                                  K4A_IMAGE_FORMAT_COLOR_NV12,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_NV12_720P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 1,
                                                  K4A_IMAGE_FORMAT_COLOR_YUY2,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_YUY2_720P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 2,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_2160P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 3,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_1440P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 4,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_1080P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 5,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 6,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_1536P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 7,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_2160P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_RGB_2160P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 8,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_1440P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_RGB_1440P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 9,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_1080P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_RGB_1080P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 10,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_RGB_720P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },
                            color_mode_parameter{ 11,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_1536P,
                                                  K4A_FRAMES_PER_SECOND_30,
                                                  K4A_COLOR_MODE_RGB_1536P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_30 },

                            // 15 fps tests
                            color_mode_parameter{ 12,
                                                  K4A_IMAGE_FORMAT_COLOR_NV12,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_NV12_720P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 13,
                                                  K4A_IMAGE_FORMAT_COLOR_YUY2,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_YUY2_720P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 14,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_2160P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 15,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_1440P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 16,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_1080P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 17,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 18,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_3072P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 19,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_1536P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 20,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_2160P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_RGB_2160P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 21,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_1440P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_RGB_1440P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 22,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_1080P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_RGB_1080P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 23,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_RGB_720P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 24,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_3072P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_RGB_3072P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },
                            color_mode_parameter{ 25,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_1536P,
                                                  K4A_FRAMES_PER_SECOND_15,
                                                  K4A_COLOR_MODE_RGB_1536P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_15 },

                            // 5 fps tests
                            color_mode_parameter{ 26,
                                                  K4A_IMAGE_FORMAT_COLOR_NV12,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_NV12_720P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 27,
                                                  K4A_IMAGE_FORMAT_COLOR_YUY2,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_YUY2_720P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 28,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_2160P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 29,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_1440P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 30,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_1080P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 31,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 32,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_3072P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 33,
                                                  K4A_IMAGE_FORMAT_COLOR_MJPG,
                                                  K4A_COLOR_RESOLUTION_1536P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_MJPG_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 34,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_2160P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_RGB_2160P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 35,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_1440P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_RGB_1440P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 36,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_1080P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_RGB_1080P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 37,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_720P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_RGB_720P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 38,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_3072P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_RGB_3072P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 },
                            color_mode_parameter{ 39,
                                                  K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  K4A_COLOR_RESOLUTION_1536P,
                                                  K4A_FRAMES_PER_SECOND_5,
                                                  K4A_COLOR_MODE_RGB_1536P_EXPECTED_SIZE,
                                                  K4A_COLOR_MODE_EXPECTED_FPS_5 }));

/**
 *  Functional test for verifying that changing modes actually causes data to be returned in the right mode
 *
 *  @Test criteria
 *   Frames shall be received within 600 mSec of starting the color stream
 *   Frames shall be of the correct size for the mode the device is configured with
 *
 */
TEST_F(color_functional_test, colorModeChange)
{
    int32_t timeout_ms = ERROR_START_STREAM_TIME;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_device_configuration_t config2 = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_capture_t capture;
    const size_t config_expected_image_size = K4A_COLOR_MODE_NV12_720P_EXPECTED_SIZE;
    const size_t config2_expected_image_size = K4A_COLOR_MODE_YUY2_720P_EXPECTED_SIZE;

    static_assert(config_expected_image_size != config2_expected_image_size,
                  "Test modes should have different-sized payloads");

    // Create two valid configs that are expected to yield different-sized color payloads
    //
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_NV12;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_OFF;

    config2.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config2.color_format = K4A_IMAGE_FORMAT_COLOR_YUY2;
    config2.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config2.depth_mode = K4A_DEPTH_MODE_OFF;

    // Start device in first mode and check frame size
    //
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));

    ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &capture, timeout_ms));
    k4a_image_t image = k4a_capture_get_color_image(capture);
    ASSERT_NE(image, (k4a_image_t)NULL);
    ASSERT_NE((uint8_t *)NULL, k4a_image_get_buffer(image));
    EXPECT_EQ(config_expected_image_size, k4a_image_get_size(image)) << "Failed due to invalid frame size\n";

    k4a_image_release(image);
    k4a_capture_release(capture);

    k4a_device_stop_cameras(m_device);

    // Start device in second mode and check frame size
    //
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config2));

    ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &capture, timeout_ms));
    image = k4a_capture_get_color_image(capture);
    ASSERT_NE(image, (k4a_image_t)NULL);
    ASSERT_NE((uint8_t *)NULL, k4a_image_get_buffer(image));
    EXPECT_EQ(config2_expected_image_size, k4a_image_get_size(image)) << "Failed due to invalid frame size\n";

    k4a_image_release(image);
    k4a_capture_release(capture);

    k4a_device_stop_cameras(m_device);
}

/**
 *  Functional test for verifying that changing exposure time actually applied to the frame
 *
 *  @Test criteria
 *   Exposure setting shall be succeeded
 *   Getting exposure value shall have same value of setting
 *   Exposure time setting shall be applied to frame payload
 *
 */
TEST_F(color_functional_test, colorExposureTest)
{
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    int32_t timeout_ms = ERROR_START_STREAM_TIME;
    k4a_capture_t capture;
    k4a_image_t image;
    k4a_color_control_mode_t control_mode;
    int32_t value;
    uint64_t exposure_time;
    int32_t min_value;
    int32_t max_value;
    int32_t step_value;
    int32_t default_value;
    bool supports_auto;
    k4a_color_control_mode_t default_mode = K4A_COLOR_CONTROL_MODE_AUTO;

    // Create two valid configs that are expected to yield different-sized color payloads
    //
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_NV12;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_OFF;

    // Exposure set test
    EXPECT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_set_color_control(m_device,
                                           K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                           K4A_COLOR_CONTROL_MODE_MANUAL,
                                           15625));

    // Exposure get test
    EXPECT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_get_color_control(m_device, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &control_mode, &value));
    std::cout << "control_mode = " << (control_mode == K4A_COLOR_CONTROL_MODE_AUTO ? "auto" : "manual")
              << ", value = " << value << " uSec\n";

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));

    ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &capture, timeout_ms));

    // Verify exposure metadata
    ASSERT_NE(image = k4a_capture_get_color_image(capture), (k4a_image_t)NULL);
    EXPECT_GT(exposure_time = k4a_image_get_exposure_usec(image), 0);
    EXPECT_LT(exposure_time, 33333); // At a min, this should be smaller than the frame rate

    std::cout << "exposure_time applied = " << exposure_time << " uSec\n";

    k4a_image_release(image);
    k4a_capture_release(capture);

    // Reset exposure time to default
    EXPECT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_get_color_control_capabilities(m_device,
                                                        K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                                        &supports_auto,
                                                        &min_value,
                                                        &max_value,
                                                        &step_value,
                                                        &default_value,
                                                        &default_mode));
    EXPECT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_set_color_control(m_device,
                                           K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                           K4A_COLOR_CONTROL_MODE_MANUAL,
                                           default_value));

    // If default mode is not manual, recover color control mode as well
    if (default_mode != K4A_COLOR_CONTROL_MODE_MANUAL)
    {
        EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_set_color_control(m_device,
                                               K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                               default_mode,
                                               default_value));
    }

    k4a_device_stop_cameras(m_device);
}

struct color_control_parameter
{
    k4a_color_control_command_t command;
    k4a_color_control_mode_t default_mode;
    int32_t default_value;

    friend std::ostream &operator<<(std::ostream &os, const color_control_parameter &obj)
    {
        switch (obj.command)
        {
        case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE:
            return os << "command : Exposure Time absolute";
        case K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY:
            return os << "command : Auto exposure priority";
        case K4A_COLOR_CONTROL_BRIGHTNESS:
            return os << "command : Brightness";
        case K4A_COLOR_CONTROL_CONTRAST:
            return os << "command : Contrast";
        case K4A_COLOR_CONTROL_SATURATION:
            return os << "command : Saturation";
        case K4A_COLOR_CONTROL_SHARPNESS:
            return os << "command : Sharpness";
        case K4A_COLOR_CONTROL_WHITEBALANCE:
            return os << "command : White balance";
        case K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION:
            return os << "command : Backlight compensation";
        case K4A_COLOR_CONTROL_GAIN:
            return os << "command : Gain";
        case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY:
            return os << "command : Powerline frequency";
        }
        return os << "command: Unkown";
    }
};

class color_control_test : public ::testing::Test, public ::testing::WithParamInterface<color_control_parameter>
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

TEST_P(color_control_test, control_test)
{
    auto as = GetParam();
    k4a_color_control_mode_t control_mode = K4A_COLOR_CONTROL_MODE_AUTO;
    int32_t min_value;
    int32_t max_value;
    int32_t step_value;
    int32_t default_value;
    k4a_color_control_mode_t default_mode = K4A_COLOR_CONTROL_MODE_AUTO;
    int32_t current_value;
    k4a_color_control_mode_t current_mode = K4A_COLOR_CONTROL_MODE_MANUAL;
    bool supports_auto;
    int32_t value = 0;

    // Invalid device handle test
    EXPECT_EQ(K4A_RESULT_FAILED, k4a_device_get_color_control(nullptr, as.command, &control_mode, &value));
    EXPECT_EQ(K4A_RESULT_FAILED, k4a_device_set_color_control(nullptr, as.command, control_mode, value));

    // Read control capabilities
    EXPECT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_get_color_control_capabilities(m_device,
                                                        as.command,
                                                        &supports_auto,
                                                        &min_value,
                                                        &max_value,
                                                        &step_value,
                                                        &default_value,
                                                        &default_mode));

    EXPECT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, as.command, &current_mode, &current_value));

    // Verify default values
    EXPECT_EQ(default_mode, as.default_mode);
    EXPECT_EQ(default_value, as.default_value);

    if (current_mode != default_mode)
    {
        std::cout << "Current mode (" << current_mode << ") is not equal to default mode (" << default_mode
                  << "), please reset device before running test.\n";
    }
    if (current_value != default_value)
    {
        std::cout << "Current value (" << current_value << ") is not equal to default value (" << default_value
                  << "), please reset device before running test.\n";
    }

    if (supports_auto)
    {
        EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_set_color_control(m_device, as.command, K4A_COLOR_CONTROL_MODE_AUTO, 0));
    }
    else
    {
        EXPECT_EQ(K4A_RESULT_FAILED,
                  k4a_device_set_color_control(m_device, as.command, K4A_COLOR_CONTROL_MODE_AUTO, 0));
    }

    if (as.command == K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE)
    {
#ifdef _WIN32
        // Windows only support exposure time value as log base 2 seconds.
        // Convert micro sec unit of min, max and step to Windows based values and test
        min_value = (int32_t)log2f((float)min_value * 0.000001f);
        max_value = (int32_t)log2f((float)max_value * 0.000001f);
        step_value = 1;

        // Test valid range
        for (int32_t testValue = min_value; testValue <= max_value; testValue += step_value)
        {
            // Set test value
            EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                      k4a_device_set_color_control(m_device,
                                                   as.command,
                                                   K4A_COLOR_CONTROL_MODE_MANUAL,
                                                   (int32_t)(exp2f((float)testValue) * 1000000.0f))); // Convert Windows
                                                                                                      // based parameter
                                                                                                      // to micro sec.

            // Get current value
            EXPECT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, as.command, &control_mode, &value));

            EXPECT_EQ(control_mode, K4A_COLOR_CONTROL_MODE_MANUAL);
            // Compare applied value in micro sec unit.
            EXPECT_EQ(value, (int32_t)(exp2f((float)testValue) * 1000000.0f));
        }
#else
        // Test valid range
        step_value *= 3; // skip test to do not too many test cases
        for (int32_t testValue = min_value; testValue <= max_value; testValue += step_value)
        {
            // Set test value
            EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                      k4a_device_set_color_control(m_device, as.command, K4A_COLOR_CONTROL_MODE_MANUAL, testValue));

            // Get current value
            EXPECT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, as.command, &control_mode, &value));

            EXPECT_EQ(control_mode, K4A_COLOR_CONTROL_MODE_MANUAL);

            // LibUVC exposure time camera control has 0.0001 sec precision
            EXPECT_EQ(value, testValue);
        }
#endif
    }
    else
    {
        // Test invalid range
        EXPECT_EQ(K4A_RESULT_FAILED,
                  k4a_device_set_color_control(m_device,
                                               as.command,
                                               K4A_COLOR_CONTROL_MODE_MANUAL,
                                               min_value - step_value));
        EXPECT_EQ(K4A_RESULT_FAILED,
                  k4a_device_set_color_control(m_device,
                                               as.command,
                                               K4A_COLOR_CONTROL_MODE_MANUAL,
                                               max_value + step_value));

        // Test valid range
        for (int32_t testValue = min_value; testValue <= max_value; testValue += step_value)
        {
            // Set test value
            EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                      k4a_device_set_color_control(m_device, as.command, K4A_COLOR_CONTROL_MODE_MANUAL, testValue));

            // Get current value
            EXPECT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, as.command, &control_mode, &value));

            EXPECT_EQ(control_mode, K4A_COLOR_CONTROL_MODE_MANUAL);
            EXPECT_EQ(value, testValue);
        }
    }

    // Recover to initial value
    EXPECT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_set_color_control(m_device, as.command, K4A_COLOR_CONTROL_MODE_MANUAL, default_value));

    // If default mode is not manual, recover color control mode as well
    if (default_mode != K4A_COLOR_CONTROL_MODE_MANUAL)
    {
        EXPECT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_set_color_control(m_device, as.command, default_mode, default_value));
    }
}

#ifdef _WIN32
#define EXPOSURE_TIME_ABSOLUTE_CONTROL_DEFAULT_VALUE 15625
#else
#define EXPOSURE_TIME_ABSOLUTE_CONTROL_DEFAULT_VALUE 12700
#endif

INSTANTIATE_TEST_CASE_P(
    color_control,
    color_control_test,
    ::testing::Values(
        color_control_parameter{ K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                 K4A_COLOR_CONTROL_MODE_AUTO,
                                 EXPOSURE_TIME_ABSOLUTE_CONTROL_DEFAULT_VALUE },
        color_control_parameter{ K4A_COLOR_CONTROL_BRIGHTNESS, K4A_COLOR_CONTROL_MODE_MANUAL, 128 },
        color_control_parameter{ K4A_COLOR_CONTROL_CONTRAST, K4A_COLOR_CONTROL_MODE_MANUAL, 5 },
        color_control_parameter{ K4A_COLOR_CONTROL_SATURATION, K4A_COLOR_CONTROL_MODE_MANUAL, 32 },
        color_control_parameter{ K4A_COLOR_CONTROL_SHARPNESS, K4A_COLOR_CONTROL_MODE_MANUAL, 2 },
        color_control_parameter{ K4A_COLOR_CONTROL_WHITEBALANCE, K4A_COLOR_CONTROL_MODE_AUTO, 4500 },
        color_control_parameter{ K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION, K4A_COLOR_CONTROL_MODE_MANUAL, 0 },
        color_control_parameter{ K4A_COLOR_CONTROL_GAIN, K4A_COLOR_CONTROL_MODE_MANUAL, 0 },
        color_control_parameter{ K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, K4A_COLOR_CONTROL_MODE_MANUAL, 2 }));

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}
