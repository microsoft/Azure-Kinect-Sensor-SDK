// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include <ut_calibration_data.h>

// Module being tested
#include <k4ainternal/common.h>
#include <k4ainternal/depth.h>
#include <k4ainternal/depth_mcu.h>
#include <k4ainternal/calibration.h>
#include <k4ainternal/dewrapper.h>

using namespace testing;

#define FAKE_MCU ((depthmcu_t)0xface000)

// Define a mock class for the public functions depthmcu
class MockDepthMcu
{
public:
    MOCK_CONST_METHOD3(depthmcu_get_serialnum,
                       k4a_buffer_result_t(depthmcu_t depthmcu_handle,
                                           char *serial_number,
                                           size_t *serial_number_size));

    MOCK_CONST_METHOD2(depthmcu_get_version,
                       k4a_result_t(depthmcu_t depthmcu_handle, depthmcu_firmware_versions_t *version));

    MOCK_CONST_METHOD2(depthmcu_depth_set_capture_mode,
                       k4a_result_t(depthmcu_t depthmcu_handle, k4a_depth_mode_t capture_mode));

    MOCK_CONST_METHOD2(depthmcu_depth_set_fps, k4a_result_t(depthmcu_t depthmcu_handle, k4a_fps_t capture_fps));

    MOCK_CONST_METHOD3(depthmcu_depth_start_streaming,
                       k4a_result_t(depthmcu_t depthmcu_handle,
                                    depthmcu_stream_cb_t *callback,
                                    void *callback_context));

    MOCK_CONST_METHOD2(depthmcu_depth_stop_streaming, void(depthmcu_t depthmcu_handle, bool quiet));

    MOCK_CONST_METHOD4(
        depthmcu_get_cal,
        k4a_result_t(depthmcu_t depthmcu_handle, uint8_t *calibration, size_t cal_size, size_t *bytes_read));

    MOCK_CONST_METHOD4(depthmcu_get_extrinsic_calibration,
                       k4a_result_t(depthmcu_t depthmcu_handle, char *json, size_t json_size, size_t *bytes_read));

    MOCK_CONST_METHOD1(depthmcu_wait_is_ready, bool(depthmcu_t depthmcu_handle));
};

extern "C" {

// Use a global singleton for the mock object.
static MockDepthMcu *g_MockDepthMcu;

// Define the symbols needed from the usb_cmd module.
// Only functions required to link the depth module are needed
k4a_buffer_result_t depthmcu_get_serialnum(depthmcu_t depthmcu_handle, char *serial_number, size_t *serial_number_size)
{
    return g_MockDepthMcu->depthmcu_get_serialnum(depthmcu_handle, serial_number, serial_number_size);
}

k4a_result_t depthmcu_get_version(depthmcu_t depthmcu_handle, depthmcu_firmware_versions_t *version)
{
    return g_MockDepthMcu->depthmcu_get_version(depthmcu_handle, version);
}

k4a_result_t depthmcu_depth_set_capture_mode(depthmcu_t depthmcu_handle, k4a_depth_mode_t capture_mode)
{
    return g_MockDepthMcu->depthmcu_depth_set_capture_mode(depthmcu_handle, capture_mode);
}

k4a_result_t depthmcu_depth_set_fps(depthmcu_t depthmcu_handle, k4a_fps_t capture_fps)
{
    return g_MockDepthMcu->depthmcu_depth_set_fps(depthmcu_handle, capture_fps);
}

k4a_result_t depthmcu_depth_start_streaming(depthmcu_t depthmcu_handle,
                                            depthmcu_stream_cb_t *callback,
                                            void *callback_context)
{
    return g_MockDepthMcu->depthmcu_depth_start_streaming(depthmcu_handle, callback, callback_context);
}

void depthmcu_depth_stop_streaming(depthmcu_t depthmcu_handle, bool quiet)
{
    g_MockDepthMcu->depthmcu_depth_stop_streaming(depthmcu_handle, quiet);
}

k4a_result_t depthmcu_get_cal(depthmcu_t depthmcu_handle, uint8_t *calibration, size_t cal_size, size_t *bytes_read)
{
    return g_MockDepthMcu->depthmcu_get_cal(depthmcu_handle, calibration, cal_size, bytes_read);
}

k4a_result_t
depthmcu_get_extrinsic_calibration(depthmcu_t depthmcu_handle, char *json, size_t json_size, size_t *bytes_read)
{
    return g_MockDepthMcu->depthmcu_get_extrinsic_calibration(depthmcu_handle, json, json_size, bytes_read);
}

bool depthmcu_wait_is_ready(depthmcu_t depthmcu_handle)
{
    return g_MockDepthMcu->depthmcu_wait_is_ready(depthmcu_handle);
}
}

// k4a_result_t depthmcu_get_serialnum(depthmcu_t depthmcu_handle, char* serial_number, size_t*
// serial_number_size)
static void EXPECT_depthmcu_get_serialnum(MockDepthMcu &mockMcu)
{
    EXPECT_CALL(mockMcu,
                depthmcu_get_serialnum(
                    /* depthmcu_t depthmcu_handle */ FAKE_MCU,
                    /* char* serial_number */ _,
                    /* size_t* serial_number_size */ _))
        .WillRepeatedly(Invoke([](Unused, // depthmcu_t depthmcu_handle,
                                  char *serial_number,
                                  size_t *serial_number_size) {
            const char serial_num[] = "1234567890";
            if (serial_number_size == NULL)
            {
                return K4A_BUFFER_RESULT_FAILED;
            }
            else if (*serial_number_size < sizeof(serial_num) || serial_number == NULL)
            {
                return K4A_BUFFER_RESULT_TOO_SMALL;
            }
            memcpy(serial_number, serial_num, sizeof(serial_num));
            return K4A_BUFFER_RESULT_SUCCEEDED;
        }));
}

// k4a_result_t depthmcu_get_version(depthmcu_t depthmcu_handle, depthmcu_firmware_versions_t *version)
static void EXPECT_depthmcu_get_version(MockDepthMcu &mockMcu)
{
    EXPECT_CALL(mockMcu,
                depthmcu_get_version(
                    /* depthmcu_t depthmcu_handle */ FAKE_MCU,
                    /* depthmcu_firmware_versions_t *version */ _))
        .WillRepeatedly(Invoke([](Unused, // depthmcu_t depthmcu_handle,
                                  depthmcu_firmware_versions_t *version) {
            if (version == NULL)
            {
                return K4A_RESULT_FAILED;
            }
            memset(version, 0xFF, sizeof(depthmcu_firmware_versions_t));

            return K4A_RESULT_SUCCEEDED;
        }));
}

// k4a_result_t depthmcu_get_extrinsic_calibration(depthmcu_t depthmcu_handle, char *json, size_t json_size,
// size_t *bytes_read)
static void EXPECT_depthmcu_get_extrinsic_calibration(MockDepthMcu &mockMcu)
{
    EXPECT_CALL(mockMcu,
                depthmcu_get_extrinsic_calibration(
                    /* depthmcu_t depthmcu_handle */ FAKE_MCU,
                    /* char* json */ _,
                    /* size_t json_size */ _,
                    /* size_t* */ _))
        .WillRepeatedly(Invoke([](Unused, // depthmcu_t depthmcu_handle,
                                  char *json,
                                  size_t json_size,
                                  size_t *bytes_read) {
            if (json_size < sizeof(g_test_json))
            {
                return K4A_RESULT_FAILED;
            }
            memcpy(json, g_test_json, sizeof(g_test_json));
            *bytes_read = sizeof(g_test_json);
            return K4A_RESULT_SUCCEEDED;
        }));
}

// bool depthmcu_wait_is_ready(depthmcu_t depthmcu_handle)
static void EXPECT_depthmcu_wait_is_ready(MockDepthMcu &mockMcu)
{
    EXPECT_CALL(mockMcu,
                depthmcu_wait_is_ready(
                    /* depthmcu_t depthmcu_handle */ FAKE_MCU))
        .WillRepeatedly(Invoke([](Unused) { // depthmcu_t depthmcu_handle,
            return true;
        }));
}

dewrapper_t dewrapper_create(k4a_calibration_camera_t *calibration,
                             dewrapper_streaming_capture_cb_t *capture_ready,
                             void *capture_ready_context)
{
    (void)calibration;
    (void)capture_ready;
    (void)capture_ready_context;
    return (dewrapper_t)1;
}
void dewrapper_destroy(dewrapper_t dewrapper_handle)
{
    (void)dewrapper_handle;
}
k4a_result_t dewrapper_start(dewrapper_t dewrapper_handle,
                             const k4a_device_configuration_t *config,
                             uint8_t *calibration_memory,
                             size_t calibration_memory_size)
{
    (void)dewrapper_handle;
    (void)config;
    (void)calibration_memory;
    (void)calibration_memory_size;
    return K4A_RESULT_SUCCEEDED;
}
void dewrapper_stop(dewrapper_t dewrapper_handle)
{
    (void)dewrapper_handle;
}
void dewrapper_post_capture(k4a_result_t cb_result, k4a_capture_t capture_raw, void *context)
{
    (void)cb_result;
    (void)capture_raw;
    (void)context;
}

class depth_ut : public ::testing::Test
{
protected:
    MockDepthMcu m_MockDepthMcu;

    void SetUp() override
    {
        g_MockDepthMcu = &m_MockDepthMcu;
        EXPECT_depthmcu_get_version(m_MockDepthMcu);
        EXPECT_depthmcu_get_serialnum(m_MockDepthMcu);
        EXPECT_depthmcu_get_extrinsic_calibration(m_MockDepthMcu);
        EXPECT_depthmcu_wait_is_ready(m_MockDepthMcu);
    }

    void TearDown() override
    {
        // Verify all expectations and clear them before the next test
        Mock::VerifyAndClear(&m_MockDepthMcu);
        g_MockDepthMcu = NULL;
    }
};

TEST_F(depth_ut, create)
{
    // Create the depth instance
    depth_t depth_handle1 = NULL;
    depth_t depth_handle2 = NULL;

    // Sanity check success
    calibration_t calibration_handle;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, calibration_create(FAKE_MCU, &calibration_handle));

    // Validate input checking
    ASSERT_EQ(K4A_RESULT_FAILED, depth_create(FAKE_MCU, NULL, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, depth_create(NULL, NULL, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, depth_create(NULL, NULL, NULL, NULL, &depth_handle1));
    ASSERT_EQ(K4A_RESULT_FAILED, depth_create(FAKE_MCU, calibration_handle, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, depth_create(NULL, calibration_handle, NULL, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, depth_create(NULL, calibration_handle, NULL, NULL, &depth_handle1));
    ASSERT_EQ(K4A_RESULT_FAILED, depth_create(FAKE_MCU, NULL, NULL, NULL, &depth_handle1));
    ASSERT_EQ(depth_handle1, (depth_t)NULL);

    // Create an instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depth_create(FAKE_MCU, calibration_handle, NULL, NULL, &depth_handle1));
    ASSERT_NE(depth_handle1, (depth_t)NULL);

    // Create a second instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depth_create(FAKE_MCU, calibration_handle, NULL, NULL, &depth_handle2));
    ASSERT_NE(depth_handle2, (depth_t)NULL);

    // Verify the instances are unique
    EXPECT_NE(depth_handle1, depth_handle2);

    calibration_destroy(calibration_handle);
    depth_destroy(depth_handle1);
    depth_destroy(depth_handle2);

    // No calls to the depthmcu layer should have been made. It is assumed that the
    // create call should be fast and not conduct any I/O

    // By not defining any expectations, we should fail when we leave this test if any have been
}

TEST_F(depth_ut, serialnum)
{
    // Create the depth instance
    depth_t depth_handle = NULL;
    char serial_num[128];
    size_t serial_num_sz = sizeof(serial_num);

    calibration_t calibration_handle;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, calibration_create(FAKE_MCU, &calibration_handle));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depth_create(FAKE_MCU, calibration_handle, NULL, NULL, &depth_handle));
    ASSERT_NE(depth_handle, (depth_t)NULL);

    ASSERT_EQ(K4A_BUFFER_RESULT_FAILED, depth_get_device_serialnum(NULL, serial_num, &serial_num_sz));
    ASSERT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, depth_get_device_serialnum(depth_handle, NULL, &serial_num_sz));
    ASSERT_EQ(K4A_BUFFER_RESULT_FAILED, depth_get_device_serialnum(depth_handle, serial_num, NULL));

    serial_num_sz = 0;
    ASSERT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, depth_get_device_serialnum(depth_handle, serial_num, &serial_num_sz));

    serial_num_sz = sizeof(serial_num);
    ASSERT_EQ(K4A_BUFFER_RESULT_SUCCEEDED, depth_get_device_serialnum(depth_handle, serial_num, &serial_num_sz));
    ASSERT_LE(serial_num_sz, sizeof(serial_num));

    serial_num_sz = 2;
    ASSERT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, depth_get_device_serialnum(depth_handle, serial_num, &serial_num_sz));
    ASSERT_LE(serial_num_sz, sizeof(serial_num));

    calibration_destroy(calibration_handle);
    depth_destroy(depth_handle);
}

// Function prototype for the Depth module API we want to test.
extern "C" bool is_fw_version_compatable(const char *fw_type, k4a_version_t *fw_version, k4a_version_t *fw_min_version);

TEST_F(depth_ut, version)
{
    // Create the depth instance
    depth_t depth_handle = NULL;
    k4a_hardware_version_t version;

    calibration_t calibration_handle;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, calibration_create(FAKE_MCU, &calibration_handle));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depth_create(FAKE_MCU, calibration_handle, NULL, NULL, &depth_handle));
    ASSERT_NE(depth_handle, (depth_t)NULL);

    ASSERT_EQ(K4A_RESULT_FAILED, depth_get_device_version(NULL, &version));
    ASSERT_EQ(K4A_RESULT_FAILED, depth_get_device_version(depth_handle, NULL));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depth_get_device_version(depth_handle, &version));

    calibration_destroy(calibration_handle);
    depth_destroy(depth_handle);

    k4a_version_t min = { 2, 2, 2 };
    k4a_version_t ver_bad[] = { { 1, 1, 1 }, { 2, 2, 1 }, { 2, 1, 2 }, { 1, 2, 2 } };
    k4a_version_t ver_good[] = { { 2, 2, 2 }, { 2, 2, 3 }, { 2, 3, 0 }, { 3, 0, 0 } };

    for (int x = 0; x < (int)COUNTOF(ver_bad); x++)
    {
        ASSERT_FALSE(is_fw_version_compatable("test fw", &ver_bad[x], &min)) << " Test Index is " << x << "\n";
    }

    for (int x = 0; x < (int)COUNTOF(ver_good); x++)
    {
        ASSERT_TRUE(is_fw_version_compatable("test fw", &ver_good[x], &min)) << " Test Index is " << x << "\n";
    }
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
