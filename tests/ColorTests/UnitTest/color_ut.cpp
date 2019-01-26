#include <utcommon.h>

// Module being tested
#include <k4ainternal/color.h>

#include <azure_c_shared_utility/tickcounter.h>
#ifdef _WIN32
#include "color_mock_mediafoundation.h"
#include "color_mock_windows.h"

// Fake container ID
static const GUID guid_FakeContainerId = { 0x4e666abb,
                                           0x31e9,
                                           0x4425,
                                           { 0xaf, 0x9f, 0x11, 0x81, 0x2e, 0x64, 0x34, 0xde } };
#endif // _WIN32

#define FAKE_COLOR_MCU ((colormcu_t)0xface100)
#define FAKE_DEPTH_MCU ((depthmcu_t)0xface200)

using namespace testing;

class color_ut : public ::testing::Test
{
protected:
#ifdef _WIN32
    MockMediaFoundation m_mockMediaFoundation; // Mock for Media Foundation
    MockWindowsSystem m_mockWindows;           // Mock for dllexport Windows system calls
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
        EXPECT_CM_Get_DevNode_PropertyW(m_mockWindows, guid_FakeContainerId);
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
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, FAKE_COLOR_MCU, NULL, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, NULL, FAKE_DEPTH_MCU, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, FAKE_COLOR_MCU, FAKE_DEPTH_MCU, NULL, NULL, NULL));

    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, NULL, NULL, NULL, NULL, &color_handle1));
    ASSERT_EQ(color_handle1, (color_t)NULL);
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, FAKE_COLOR_MCU, NULL, NULL, NULL, &color_handle1));
    ASSERT_EQ(color_handle1, (color_t)NULL);
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, NULL, FAKE_DEPTH_MCU, NULL, NULL, &color_handle1));
    ASSERT_EQ(color_handle1, (color_t)NULL);
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, NULL, FAKE_DEPTH_MCU, NULL, NULL, &color_handle1));
    ASSERT_EQ(color_handle1, (color_t)NULL);
    ASSERT_EQ(K4A_RESULT_FAILED, color_create(0, FAKE_COLOR_MCU, FAKE_DEPTH_MCU, NULL, NULL, &color_handle1));
    ASSERT_EQ(color_handle1, (color_t)NULL);

    // Create an instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, color_create(tick, FAKE_COLOR_MCU, FAKE_DEPTH_MCU, NULL, NULL, &color_handle1));
    ASSERT_NE(color_handle1, (color_t)NULL);

    // Create a second instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, color_create(tick, FAKE_COLOR_MCU, FAKE_DEPTH_MCU, NULL, NULL, &color_handle2));
    ASSERT_NE(color_handle2, (color_t)NULL);

    // Verify the instances are unique
    EXPECT_NE(color_handle1, color_handle2);

    color_destroy(color_handle1);
    color_destroy(color_handle2);
    tickcounter_destroy(tick);
}

#ifdef _WIN32
// color_ut streaming test is only available for Windows
TEST_F(color_ut, streaming)
{
    color_t color_handle = NULL;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    TICK_COUNTER_HANDLE tick;

    ASSERT_NE((TICK_COUNTER_HANDLE)0, (tick = tickcounter_create()));

    // test color_create()
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, color_create(tick, FAKE_COLOR_MCU, FAKE_DEPTH_MCU, NULL, NULL, &color_handle));
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
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, color_create(tick, FAKE_COLOR_MCU, FAKE_DEPTH_MCU, NULL, NULL, &color_handle));
    ASSERT_NE(color_handle, (color_t)NULL);

    config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    config.color_format = K4A_IMAGE_FORMAT_COLOR_NV12;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_OFF;

    // set exposure to 488 uSec
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              color_set_control(color_handle,
                                K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                K4A_COLOR_CONTROL_MODE_MANUAL,
                                488));

    // get exposure settings
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              color_get_control(color_handle, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &control_mode, &value));
    ASSERT_EQ(control_mode, K4A_COLOR_CONTROL_MODE_MANUAL);
    ASSERT_EQ(value, 488);

    // test color_start()
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, color_start(color_handle, &config));

    // test color_stop()
    color_stop(color_handle);
    color_destroy(color_handle);
    tickcounter_destroy(tick);
}
#endif

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}
