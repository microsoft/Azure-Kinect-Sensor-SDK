// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <k4a/k4a.h>
#include <k4ainternal/common.h>
#include <k4ainternal/../../src/color/color_priv.h>
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
typedef enum _power_line_t
{
    K4A_POWER_LINE_50HZ = 1,
    K4A_POWER_LINE_60HZ = 2
} power_line_t;

//************ Declarations (Statics and globals) ***************
#define EXPOSURE_TIME_ABSOLUTE_CONTROL_DEFAULT_60_HZ_VALUE 33330 // 60Hz
#define EXPOSURE_TIME_ABSOLUTE_CONTROL_DEFAULT_50_HZ_VALUE 30000 // 50Hz

#define SET_POWER_LINE_FREQ(val)                                                                                       \
    {                                                                                                                  \
        int32_t value;                                                                                                 \
        const k4a_color_control_command_t power_cmd = K4A_COLOR_CONTROL_POWERLINE_FREQUENCY;                           \
        k4a_color_control_mode_t mode = K4A_COLOR_CONTROL_MODE_MANUAL;                                                 \
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_set_color_control(m_device, power_cmd, mode, val));                 \
                                                                                                                       \
        /*Verify Results */                                                                                            \
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, power_cmd, &mode, &value));             \
        ASSERT_EQ(mode, K4A_COLOR_CONTROL_MODE_MANUAL);                                                                \
        ASSERT_EQ(value, val);                                                                                         \
    }
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

    uint64_t ts_d = 0;
    uint64_t ts_s = 0;
    bool ts_init = false;

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
            ASSERT_EQ(0, k4a_image_get_stride_bytes(image));
            break;
        case K4A_IMAGE_FORMAT_COLOR_NV12:
            ASSERT_EQ(k4a_image_get_width_pixels(image), k4a_image_get_stride_bytes(image));
            break;
        case K4A_IMAGE_FORMAT_COLOR_YUY2:
            ASSERT_EQ(k4a_image_get_width_pixels(image) * 2, k4a_image_get_stride_bytes(image));
            break;
        case K4A_IMAGE_FORMAT_COLOR_BGRA32:
            ASSERT_EQ(k4a_image_get_width_pixels(image) * 4, k4a_image_get_stride_bytes(image));
            break;
        default:
            break;
        }

        if (ts_init)
        {
            // Ensure the device and system time stamps are increasing, image might get dropped, which is ok for this
            // portion of the test.
            uint64_t ts;
            ts = k4a_image_get_device_timestamp_usec(image);
            ASSERT_GT(ts, ts_d); // We don't test upper max, because we expect the next sample to be larger, this also
                                 // allows frames to be dropped and not error out.
            ts_d = ts;

            ts = k4a_image_get_system_timestamp_nsec(image);
            ASSERT_GT(ts, ts_s); // We don't test upper max, because we expect the next sample to be larger, this also
                                 // allows frames to be dropped and not error out.
            ts_s = ts;
        }
        else
        {
            ts_d = k4a_image_get_device_timestamp_usec(image);
            ts_s = k4a_image_get_system_timestamp_nsec(image);
            ts_init = true;
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
    ASSERT_EQ(config_expected_image_size, k4a_image_get_size(image)) << "Failed due to invalid frame size\n";

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
    ASSERT_EQ(config2_expected_image_size, k4a_image_get_size(image)) << "Failed due to invalid frame size\n";

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
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_set_color_control(m_device,
                                           K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                           K4A_COLOR_CONTROL_MODE_MANUAL,
                                           15625));

    // Exposure get test
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_get_color_control(m_device, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &control_mode, &value));
    std::cout << "control_mode = " << (control_mode == K4A_COLOR_CONTROL_MODE_AUTO ? "auto" : "manual")
              << ", value = " << value << " uSec\n";

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));

    ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &capture, timeout_ms));

    // Verify exposure metadata
    ASSERT_NE(image = k4a_capture_get_color_image(capture), (k4a_image_t)NULL);
    ASSERT_GT(exposure_time = k4a_image_get_exposure_usec(image), 0);
    ASSERT_LT(exposure_time, 33333); // At a min, this should be smaller than the frame rate

    std::cout << "exposure_time applied = " << exposure_time << " uSec\n";

    k4a_image_release(image);
    k4a_capture_release(capture);

    // Reset exposure time to default
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_get_color_control_capabilities(m_device,
                                                        K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                                        &supports_auto,
                                                        &min_value,
                                                        &max_value,
                                                        &step_value,
                                                        &default_value,
                                                        &default_mode));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_set_color_control(m_device,
                                           K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                           K4A_COLOR_CONTROL_MODE_MANUAL,
                                           default_value));

    // If default mode is not manual, recover color control mode as well
    if (default_mode != K4A_COLOR_CONTROL_MODE_MANUAL)
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
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

    void control_test_worker(const k4a_color_control_command_t command,
                             const k4a_color_control_mode_t default_mode,
                             const int32_t default_value);

    k4a_device_t m_device = nullptr;

private:
    int32_t map_manual_exposure(int32_t value, bool sixty_hertz);
    int32_t limit_exposure_to_fps_setting(int32_t value, bool sixty_hertz, k4a_fps_t fps);
    bool validate_image_exposure_setting(int test_value, bool b_sixty_hertz, k4a_fps_t fps);
};

int32_t color_control_test::map_manual_exposure(int32_t value, bool sixty_hertz)
{

    for (uint32_t x = 0; x < COUNTOF(device_exposure_mapping); x++)
    {
        int32_t exposure = device_exposure_mapping[x].exposure_mapped_50Hz_usec;
        if (sixty_hertz)
        {
            exposure = device_exposure_mapping[x].exposure_mapped_60Hz_usec;
        }

        if (value <= exposure)
        {
            return exposure;
        }
    }

    return MAX_EXPOSURE(sixty_hertz);
}

// Limit exposure setting based on FPS setting.
int32_t color_control_test::limit_exposure_to_fps_setting(int32_t value, bool sixty_hertz, k4a_fps_t fps)
{
    int fps_usec = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(fps));
    int last_exposure;

    if (value < fps_usec)
    {
        // No work to do
        return value;
    }

    last_exposure = sixty_hertz ? device_exposure_mapping[0].exposure_mapped_60Hz_usec :
                                  device_exposure_mapping[0].exposure_mapped_50Hz_usec;
    for (uint32_t x = 1; x < COUNTOF(device_exposure_mapping); x++)
    {
        int mapped_exposure = sixty_hertz ? device_exposure_mapping[x].exposure_mapped_60Hz_usec :
                                            device_exposure_mapping[x].exposure_mapped_50Hz_usec;

        if (mapped_exposure > fps_usec)
        {
            return last_exposure;
        }
        last_exposure = mapped_exposure;
    }
    EXPECT_FALSE(1); // we should not get here
    return last_exposure;
}

// returns true if the exposure setting on the read image has been updated.
bool color_control_test::validate_image_exposure_setting(int test_value, bool sixty_hertz, k4a_fps_t fps)
{

    test_value = limit_exposure_to_fps_setting(test_value, sixty_hertz, fps);

    // Validate the exposure value on the image is correct
    uint64_t img_exposure_setting = 0;
    int tries = 0;
    for (tries = 0; tries < 10 && (int32_t)img_exposure_setting != test_value; tries++)
    {
        k4a_capture_t capture;
        EXPECT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &capture, 1000));
        k4a_image_t img = k4a_capture_get_color_image(capture);
        EXPECT_NE((k4a_image_t)NULL, img);
        img_exposure_setting = k4a_image_get_exposure_usec(img);
        k4a_image_release(img);
        k4a_capture_release(capture);
    }
    EXPECT_EQ(img_exposure_setting, test_value);
    return (int32_t)img_exposure_setting == test_value;
}

void color_control_test::control_test_worker(const k4a_color_control_command_t command,
                                             const k4a_color_control_mode_t default_mode,
                                             const int32_t default_value)
{
    int32_t min_value;
    int32_t max_value;
    int32_t step_value;
    int32_t default_value_read;
    k4a_color_control_mode_t default_mode_read = K4A_COLOR_CONTROL_MODE_AUTO;
    int32_t current_value;
    k4a_color_control_mode_t current_mode = K4A_COLOR_CONTROL_MODE_MANUAL;
    bool supports_auto;
    int32_t value = 0;
    bool cameras_running = false;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

    // 50% of the time we should test with the camera running
    if ((rand() * 2 / RAND_MAX) >= 1)
    {
        config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        config.camera_fps = K4A_FRAMES_PER_SECOND_5;
        config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
        config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
        config.synchronized_images_only = true;
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_start_cameras(m_device, &config));
        std::cout << "control_test_worker: k4a_device_start_cameras called\n";
        cameras_running = true;

        // ensure the captures are flowing
        k4a_capture_t capture;
        ASSERT_EQ(K4A_WAIT_RESULT_SUCCEEDED, k4a_device_get_capture(m_device, &capture, 30000));
        k4a_capture_release(capture);
    }
    else
    {
        std::cout << "control_test_worker: k4a_device_start_cameras not called\n";
    }

    // Read control capabilities
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              k4a_device_get_color_control_capabilities(m_device,
                                                        command,
                                                        &supports_auto,
                                                        &min_value,
                                                        &max_value,
                                                        &step_value,
                                                        &default_value_read,
                                                        &default_mode_read));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, command, &current_mode, &current_value));

    // Verify default values
    ASSERT_EQ(default_mode_read, default_mode);
    if (default_mode == K4A_COLOR_CONTROL_MODE_MANUAL)
    {
        ASSERT_EQ(default_value_read, default_value);
    }

    if (supports_auto)
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  k4a_device_set_color_control(m_device, command, K4A_COLOR_CONTROL_MODE_AUTO, 0));
    }
    else
    {
        ASSERT_EQ(K4A_RESULT_FAILED, k4a_device_set_color_control(m_device, command, K4A_COLOR_CONTROL_MODE_AUTO, 0));
    }

    if (command == K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE)
    {
        bool b_sixty_hertz = (default_value == EXPOSURE_TIME_ABSOLUTE_CONTROL_DEFAULT_60_HZ_VALUE) ? true : false;
        std::cout << "K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE running at " << (b_sixty_hertz ? "60Hz" : "50Hz")
                  << "\n";

        k4a_color_control_mode_t manual = K4A_COLOR_CONTROL_MODE_MANUAL;

        for (uint32_t x = 0; x < COUNTOF(device_exposure_mapping); x++)
        {
            int32_t threshold = b_sixty_hertz ? device_exposure_mapping[x].exposure_mapped_60Hz_usec :
                                                device_exposure_mapping[x].exposure_mapped_50Hz_usec;

            // For this test we test the mapping value transitions; 1 below, 1 above, and the exact value.
            int32_t testValue = threshold - 1;
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_set_color_control(m_device, command, manual, testValue));
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, command, &current_mode, &value));
            ASSERT_EQ(current_mode, manual);
            ASSERT_EQ(value, map_manual_exposure(testValue, b_sixty_hertz)) << testValue << " was the value tested\n";
            if (cameras_running)
            {
                ASSERT_TRUE(validate_image_exposure_setting(value, b_sixty_hertz, config.camera_fps)) << "1";
            }

            testValue = threshold;
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_set_color_control(m_device, command, manual, testValue));
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, command, &current_mode, &value));
            ASSERT_EQ(current_mode, manual);
            ASSERT_EQ(value, map_manual_exposure(testValue, b_sixty_hertz)) << testValue << " was the value tested\n";
            if (cameras_running)
            {
                ASSERT_TRUE(validate_image_exposure_setting(value, b_sixty_hertz, config.camera_fps)) << "2";
            }

            testValue = threshold + 1;
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_set_color_control(m_device, command, manual, testValue));
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, command, &current_mode, &value));
            ASSERT_EQ(current_mode, manual);
            ASSERT_EQ(value, map_manual_exposure(testValue, b_sixty_hertz)) << testValue << " was the value tested\n";
            if (cameras_running)
            {
                ASSERT_TRUE(validate_image_exposure_setting(value, b_sixty_hertz, config.camera_fps)) << "3";
            }

            ASSERT_EQ(current_mode, manual);
            ASSERT_EQ(current_mode, K4A_COLOR_CONTROL_MODE_MANUAL);

            // LibUVC exposure time camera control has 0.0001 sec precision
        }
    }
    else
    {
        // Test invalid range
        ASSERT_EQ(K4A_RESULT_FAILED,
                  k4a_device_set_color_control(m_device,
                                               command,
                                               K4A_COLOR_CONTROL_MODE_MANUAL,
                                               min_value - step_value));
        ASSERT_EQ(K4A_RESULT_FAILED,
                  k4a_device_set_color_control(m_device,
                                               command,
                                               K4A_COLOR_CONTROL_MODE_MANUAL,
                                               max_value + step_value));

        // Test valid range
        for (int32_t testValue = min_value; testValue <= max_value; testValue += step_value)
        {
            // Set test value
            ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                      k4a_device_set_color_control(m_device, command, K4A_COLOR_CONTROL_MODE_MANUAL, testValue));
            ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, command, &current_mode, &value));
            ASSERT_EQ(current_mode, K4A_COLOR_CONTROL_MODE_MANUAL);
            ASSERT_EQ(value, testValue);
        }
    }

    if (cameras_running)
    {
        k4a_device_stop_cameras(m_device);
    }

    // Restore Defaults
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_set_color_control(m_device, command, default_mode, default_value));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_color_control(m_device, command, &current_mode, &current_value));
        ASSERT_EQ(current_mode, default_mode);
        if (default_mode == K4A_COLOR_CONTROL_MODE_MANUAL)
        {
            ASSERT_EQ(current_value, default_value);
        }
    }
}

TEST_P(color_control_test, control_test)
{
    auto as = GetParam();
    if (as.command == K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE)
    {
        SET_POWER_LINE_FREQ(K4A_POWER_LINE_60HZ);
        control_test_worker(as.command, as.default_mode, EXPOSURE_TIME_ABSOLUTE_CONTROL_DEFAULT_60_HZ_VALUE);

        SET_POWER_LINE_FREQ(K4A_POWER_LINE_50HZ);
        control_test_worker(as.command, as.default_mode, EXPOSURE_TIME_ABSOLUTE_CONTROL_DEFAULT_50_HZ_VALUE);
    }
    else if (as.command == K4A_COLOR_CONTROL_GAIN)
    {
        k4a_hardware_version_t version;
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, k4a_device_get_version(m_device, &version));
        k4a_version_t new_gain_default = { 1, 6, 107 };
        if (k4a_is_version_greater_or_equal(&version.rgb, &new_gain_default))
        {
            control_test_worker(as.command, as.default_mode, 128);
        }
        else
        {
            control_test_worker(as.command, as.default_mode, 0);
        }
    }
    else
    {
        control_test_worker(as.command, as.default_mode, as.default_value);
    }
}

INSTANTIATE_TEST_CASE_P(
    color_control,
    color_control_test,
    ::testing::Values(
        color_control_parameter{ K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                 K4A_COLOR_CONTROL_MODE_AUTO,
                                 0 } /* default is overwritten for this test case */,
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
    srand((unsigned int)time(0)); // use current time as seed for random generator
    return k4a_test_common_main(argc, argv);
}
