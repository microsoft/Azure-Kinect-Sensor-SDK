// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <k4a/k4a.hpp>
#include <gtest/gtest.h>
#include <utcommon.h>

//**************Symbolic Constant Macros (defines)  *************
#define SERIAL_NUMBER_SIZE 12

//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

class cpp_projection_ft : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        m_device = k4a::device::open(K4A_DEVICE_DEFAULT);
    }

    virtual void TearDown()
    {
        m_device.close();
    }

    k4a::device m_device;
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
 */
TEST_F(cpp_projection_ft, depthSerialNumber)
{
    std::string serialnum = m_device.get_serialnum();

    ASSERT_GE(serialnum.size(), (size_t)SERIAL_NUMBER_SIZE) << "Serial Number Length invalid\n";
    if (serialnum.size() > 0)
    {
        for (size_t i = 0; i < serialnum.size(); i++)
        {
            EXPECT_TRUE(isdigit(serialnum[i]))
                << "Failed index " << (int)i << " of " << serialnum.size() << " loop iteration\n";
        }
    }

    uint16_t check = (uint8_t)serialnum[0] + (uint8_t)serialnum[1] + (uint8_t)serialnum[2] + (uint8_t)serialnum[3] +
                     (uint8_t)serialnum[4] + (uint8_t)serialnum[5];
    check = 7 - (check % 7);
    ASSERT_GE(serialnum[6], (char)check) << "Serial Number check value invalid\n";

    std::cout << "Serial Number read: " << serialnum << "\n";
}

enum class camera_type
{
    depth,
    color
};

void check_image(const k4a::image &image, const char *type, bool expected, size_t expected_size);
void check_image(const k4a::image &image, const char *type, bool expected, size_t expected_size)
{
    if (expected)
    {
        ASSERT_NE(image, nullptr) << type << " image missing!";
        ASSERT_NE(image.get_buffer(), nullptr) << type << " buffer missing!";
        ASSERT_EQ(image.get_size(), expected_size) << type << " image had unexpected size!";
    }
    else
    {
        ASSERT_EQ(image, nullptr) << type << " image unexpected!";
    }
}
/**
 *  Functional test for verifying basic C++ wrapper functionality
 *  Most of the heavy lifting is done by the existing C tests, (e.g. depth_ft, color_ft);
 *  this is just a sanity check.
 *
 *  @Test criteria
 *   All captures contain images of the specified type and no others
 *   Images are valid and correctly-sized for the mode they were started with
 *
 */
void test_camera(k4a::device *device, camera_type type); // Need prototype first to satisfy clang
void test_camera(k4a::device *device, camera_type type)
{
    constexpr int stream_run_time_sec = 4;
    constexpr int expected_fps = 15;
    constexpr int expected_captures = expected_fps * stream_run_time_sec;
    constexpr size_t nfov_2x2binned_expected_size = 184320;
    constexpr size_t bgra32_720p_expected_size = 1280 * 720 * 4 * sizeof(uint8_t);

    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    if (type == camera_type::color)
    {
        config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    }
    if (type == camera_type::depth)
    {
        config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    }
    config.camera_fps = K4A_FRAMES_PER_SECOND_15;

    device->start_cameras(&config);

    k4a::capture cap;

    // The first read takes a bit longer, so we want to wait until we've started the stream
    //
    ASSERT_TRUE(device->get_capture(&cap, std::chrono::milliseconds(10000))) << "Initial capture read timed out!";
    check_image(cap.get_color_image(), "Color", type == camera_type::color, bgra32_720p_expected_size);
    check_image(cap.get_depth_image(), "Depth", type == camera_type::depth, nfov_2x2binned_expected_size);
    check_image(cap.get_depth_image(), "IR", type == camera_type::depth, nfov_2x2binned_expected_size);
    cap.reset();

    for (int i = 0; i < expected_captures; ++i)
    {
        ASSERT_TRUE(device->get_capture(&cap, std::chrono::milliseconds(2000))) << "Capture read timed out!";

        check_image(cap.get_color_image(), "Color", type == camera_type::color, bgra32_720p_expected_size);
        check_image(cap.get_depth_image(), "Depth", type == camera_type::depth, nfov_2x2binned_expected_size);
        check_image(cap.get_depth_image(), "IR", type == camera_type::depth, nfov_2x2binned_expected_size);
    }

    device->stop_cameras();
}

#ifdef _WIN32 // Color not supported on Linux yet
TEST_F(cpp_projection_ft, test_color)
{
    test_camera(&m_device, camera_type::color);
}
#endif

TEST_F(cpp_projection_ft, test_depth)
{
    test_camera(&m_device, camera_type::depth);
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
