// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

// Module being tested
#include <k4ainternal/color.h>

#include <azure_c_shared_utility/tickcounter.h>
#ifdef _WIN32
#include "color_mock_mediafoundation.h"
#include "color_mock_windows.h"
#else
#include "color_mock_libuvc.h"
#endif // _WIN32

// Fake container ID
static const guid_t guid_FakeGoodContainerId = {
    { 0x4e, 0x66, 0x6a, 0xbb, 0x31, 0xe9, 0x44, 0x25, 0xaf, 0x9f, 0x11, 0x81, 0x2e, 0x64, 0x34, 0xde }
};
static const guid_t guid_FakeBadContainerId = {
    { 0xff, 0x66, 0x6a, 0xbb, 0x31, 0xe9, 0x44, 0x25, 0xaf, 0x9f, 0x11, 0x81, 0x2e, 0x64, 0x34, 0xde }
};

// Fake serial number
static const char *str_FakeGoodSerialNumber = "0123456789";
static const char *str_FakeBadSerialNumber = "9876543210";

using namespace testing;

class color_ut : public ::testing::Test
{
protected:
#ifdef _WIN32
    MockMediaFoundation m_mockMediaFoundation; // Mock for Media Foundation
    MockWindowsSystem m_mockWindows;           // Mock for dllexport Windows system calls
#else
    MockLibUVC m_mockLibUVC; // Mock for LibUVC
#endif

    void SetUp() override
    {
#ifdef _WIN32
        g_mockMediaFoundation = &m_mockMediaFoundation;
        SetWindowsSystemMock(&m_mockWindows);

        EXPECT_MFEnumDeviceSources(m_mockMediaFoundation);
        EXPECT_MFCreateSourceReaderFromMediaSource(m_mockMediaFoundation);
        EXPECT_CM_Get_Device_Interface_PropertyW(m_mockWindows);
        EXPECT_CM_Locate_DevNodeW(m_mockWindows);
        GUID *containerId = (GUID *)&guid_FakeGoodContainerId;
        EXPECT_CM_Get_DevNode_PropertyW(m_mockWindows, *containerId);
#else
        g_mockLibUVC = &m_mockLibUVC;

        EXPECT_uvc_init(m_mockLibUVC);
        EXPECT_uvc_find_device(m_mockLibUVC, str_FakeGoodSerialNumber);
        EXPECT_uvc_open(m_mockLibUVC);
        EXPECT_uvc_get_stream_ctrl_format_size(m_mockLibUVC);
        EXPECT_uvc_start_streaming(m_mockLibUVC);
        EXPECT_uvc_stop_streaming(m_mockLibUVC);
        EXPECT_uvc_close(m_mockLibUVC);
        EXPECT_uvc_unref_device(m_mockLibUVC);
        EXPECT_uvc_exit(m_mockLibUVC);

        EXPECT_uvc_get_ae_mode(m_mockLibUVC);
        EXPECT_uvc_get_exposure_abs(m_mockLibUVC);
        EXPECT_uvc_get_ae_priority(m_mockLibUVC);
        EXPECT_uvc_get_brightness(m_mockLibUVC);
        EXPECT_uvc_get_contrast(m_mockLibUVC);
        EXPECT_uvc_get_saturation(m_mockLibUVC);
        EXPECT_uvc_get_sharpness(m_mockLibUVC);
        EXPECT_uvc_get_white_balance_temperature_auto(m_mockLibUVC);
        EXPECT_uvc_get_white_balance_temperature(m_mockLibUVC);
        EXPECT_uvc_get_backlight_compensation(m_mockLibUVC);
        EXPECT_uvc_get_gain(m_mockLibUVC);
        EXPECT_uvc_get_power_line_frequency(m_mockLibUVC);

        EXPECT_uvc_set_ae_mode(m_mockLibUVC);
        EXPECT_uvc_set_exposure_abs(m_mockLibUVC);
        EXPECT_uvc_set_ae_priority(m_mockLibUVC);
        EXPECT_uvc_set_brightness(m_mockLibUVC);
        EXPECT_uvc_set_contrast(m_mockLibUVC);
        EXPECT_uvc_set_saturation(m_mockLibUVC);
        EXPECT_uvc_set_sharpness(m_mockLibUVC);
        EXPECT_uvc_set_white_balance_temperature_auto(m_mockLibUVC);
        EXPECT_uvc_set_white_balance_temperature(m_mockLibUVC);
        EXPECT_uvc_set_backlight_compensation(m_mockLibUVC);
        EXPECT_uvc_set_gain(m_mockLibUVC);
        EXPECT_uvc_set_power_line_frequency(m_mockLibUVC);

        EXPECT_uvc_strerror(m_mockLibUVC);
#endif
    }

    void TearDown() override
    {
        // Verify all expectations and clear them before the next test
#ifdef _WIN32
        Mock::VerifyAndClear(&m_mockMediaFoundation);
        Mock::VerifyAndClear(&m_mockWindows);

        g_mockMediaFoundation = nullptr;
        SetWindowsSystemMock(nullptr);
#else
        Mock::VerifyAndClear(&m_mockLibUVC);
        g_mockLibUVC = nullptr;
#endif
    }
};

TEST_F(color_ut, create)
{
    // Create the color instance
    color_t color_handle1 = NULL;
    color_t color_handle2 = NULL;
    TICK_COUNTER_HANDLE tick;

    ASSERT_NE((TICK_COUNTER_HANDLE)0, (tick = tickcounter_create()));

    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, NULL, NULL, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, &guid_FakeBadContainerId, NULL, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, NULL, str_FakeBadSerialNumber, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, &guid_FakeBadContainerId, str_FakeBadSerialNumber, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, NULL, NULL, NULL, NULL, &color_handle1));
    ASSERT_EQ(color_handle1, (color_t)NULL);

    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, &guid_FakeBadContainerId, NULL, NULL, NULL, &color_handle1));
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, NULL, str_FakeBadSerialNumber, NULL, NULL, &color_handle1));
    ASSERT_EQ(color_handle1, (color_t)NULL);

    ASSERT_EQ(K4A_RESULT_FAILED, color_create(tick, &guid_FakeBadContainerId, NULL, NULL, NULL, &color_handle1));
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(tick, NULL, str_FakeBadSerialNumber, NULL, NULL, &color_handle1));
    ASSERT_EQ(color_handle1, (color_t)NULL);

    // Create an instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              color_create(tick, &guid_FakeGoodContainerId, str_FakeGoodSerialNumber, NULL, NULL, &color_handle1));
    ASSERT_NE(color_handle1, (color_t)NULL);

    // Create a second instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              color_create(tick, &guid_FakeGoodContainerId, str_FakeGoodSerialNumber, NULL, NULL, &color_handle2));
    ASSERT_NE(color_handle2, (color_t)NULL);

    // Verify the instances are unique
    EXPECT_NE(color_handle1, color_handle2);

    color_destroy(color_handle1);
    color_destroy(color_handle2);
    tickcounter_destroy(tick);
}

TEST_F(color_ut, streaming)
{
    color_t color_handle = NULL;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    TICK_COUNTER_HANDLE tick;

    ASSERT_NE((TICK_COUNTER_HANDLE)0, (tick = tickcounter_create()));

    // test color_create()
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              color_create(tick, &guid_FakeGoodContainerId, str_FakeGoodSerialNumber, NULL, NULL, &color_handle));
    ASSERT_NE(color_handle, (color_t)NULL);

    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_NV12;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_OFF;

    // test color_start()
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, color_start(color_handle, &config));

    color_destroy(color_handle);
    tickcounter_destroy(tick);
}

// color_ut exposure control testing
TEST_F(color_ut, exposure_control)
{
    color_t color_handle = NULL;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_color_control_mode_t control_mode;
    int32_t value;
    TICK_COUNTER_HANDLE tick;

    ASSERT_NE((TICK_COUNTER_HANDLE)0, (tick = tickcounter_create()));

    // test color_create()
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              color_create(tick, &guid_FakeGoodContainerId, str_FakeGoodSerialNumber, NULL, NULL, &color_handle));
    ASSERT_NE(color_handle, (color_t)NULL);

    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_NV12;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_OFF;

    // set exposure to 500 uSec
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              color_set_control(color_handle,
                                K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                K4A_COLOR_CONTROL_MODE_MANUAL,
                                500));

    // get exposure settings
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              color_get_control(color_handle, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &control_mode, &value));
    ASSERT_EQ(control_mode, K4A_COLOR_CONTROL_MODE_MANUAL);
    ASSERT_EQ(value, 500);

    // test color_start()
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, color_start(color_handle, &config));

    // test color_stop()
    color_stop(color_handle);
    color_destroy(color_handle);
    tickcounter_destroy(tick);
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
