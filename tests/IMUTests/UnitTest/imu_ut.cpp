// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include <ut_calibration_data.h>

// Module being tested
#include <k4ainternal/imu.h>
#include <k4ainternal/capture.h>
#include <k4ainternal/color_mcu.h>
#include <k4ainternal/depth_mcu.h>
#include <k4ainternal/calibration.h>

using namespace testing;

#define FAKE_COLOR_MCU ((colormcu_t)0xface100)
#define FAKE_DEPTH_MCU ((depthmcu_t)0xface200)

// Define a mock class for the public functions depthmcu
class MockDepthMcu
{
public:
    MOCK_CONST_METHOD4(depthmcu_get_extrinsic_calibration,
                       k4a_result_t(depthmcu_t depthmcu_handle, char *json, size_t json_size, size_t *bytes_read));
};

extern "C" {

// Use a global singleton for the mock object.
static MockDepthMcu *g_MockDepthMcu;

// Define the symbols needed from the usb_cmd module.
// Only functions required to link the depth module are needed
k4a_result_t
depthmcu_get_extrinsic_calibration(depthmcu_t depthmcu_handle, char *json, size_t json_size, size_t *bytes_read)
{
    return g_MockDepthMcu->depthmcu_get_extrinsic_calibration(depthmcu_handle, json, json_size, bytes_read);
}
} // extern "C"

// k4a_result_t depthmcu_get_extrinsic_calibration(depthmcu_t depthmcu_handle, char *json, size_t json_size,
// size_t *bytes_read)
static void EXPECT_depthmcu_get_extrinsic_calibration(MockDepthMcu &mockMcu)
{
    EXPECT_CALL(mockMcu,
                depthmcu_get_extrinsic_calibration(
                    /* depthmcu_t depthmcu_handle */ FAKE_DEPTH_MCU,
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

// Define a mock class for the public functions color_mcu and queue
class MockColorMcu
{
public:
    MOCK_CONST_METHOD1(colormcu_imu_start_streaming, k4a_result_t(colormcu_t color_handle));

    MOCK_CONST_METHOD3(colormcu_imu_register_stream_cb,
                       k4a_result_t(colormcu_t color_handle, usb_cmd_stream_cb_t *frame_ready_cb, void *context));

    MOCK_CONST_METHOD1(colormcu_imu_stop_streaming, void(colormcu_t color_handle));

    usb_cmd_stream_cb_t *frame_ready_cb;
    void *cb_context;
};

extern "C" {

// Use a global singleton for the mock object.
static MockColorMcu *g_MockColorMcu;

// Only functions required to link the color module are needed
k4a_result_t colormcu_imu_start_streaming(colormcu_t color_handle)
{
    return g_MockColorMcu->colormcu_imu_start_streaming(color_handle);
}

k4a_result_t colormcu_imu_register_stream_cb(colormcu_t color_handle,
                                             usb_cmd_stream_cb_t *frame_ready_cb,
                                             void *context)
{
    return g_MockColorMcu->colormcu_imu_register_stream_cb(color_handle, frame_ready_cb, context);
}

void colormcu_imu_stop_streaming(colormcu_t color_handle)
{
    return g_MockColorMcu->colormcu_imu_stop_streaming(color_handle);
}
}

// k4a_result_t colormcu_imu_start_streaming(colormcu_t color_handle)
static void EXPECT_colormcu_imu_start_streaming(MockColorMcu &mockMcu)
{
    EXPECT_CALL(mockMcu,
                colormcu_imu_start_streaming(
                    /* colormcu_t color_handle */ FAKE_COLOR_MCU))
        .WillRepeatedly(Invoke([](Unused // colormcu_t color_handle
                               ) { return K4A_RESULT_SUCCEEDED; }));
}

// k4a_result_t colormcu_imu_register_stream_cb(colormcu_t color_handle, usb_cmd_stream_cb_t* frame_ready_cb,void
// *context)
static void EXPECT_colormcu_imu_register_stream_cb(MockColorMcu &mockMcu)
{
    EXPECT_CALL(mockMcu,
                colormcu_imu_register_stream_cb(
                    /* colormcu_t color_handle */ FAKE_COLOR_MCU,
                    /* usb_cmd_stream_cb_t* frame_ready_cb */ NotNull(),
                    /* void *context */ _))
        .WillRepeatedly(Invoke([](Unused, // color_handle,
                                  usb_cmd_stream_cb_t *frame_ready_cb,
                                  void *context) {
            if (frame_ready_cb == NULL)
            {
                return K4A_RESULT_FAILED;
            }
            g_MockColorMcu->frame_ready_cb = frame_ready_cb;
            g_MockColorMcu->cb_context = context;
            return K4A_RESULT_SUCCEEDED;
        }));
}

// void colormcu_imu_stop_streaming(colormcu_t color_handle)
static void EXPECT_colormcu_imu_stop_streaming(MockColorMcu &mockMcu)
{
    EXPECT_CALL(mockMcu,
                colormcu_imu_stop_streaming(
                    /* colormcu_t color_handle */ FAKE_COLOR_MCU))
        .WillRepeatedly(Return());
}

class imu_ut : public ::testing::Test
{
protected:
    MockColorMcu m_MockColorMcu;
    MockDepthMcu m_MockDepthMcu;

    void SetUp() override
    {
        g_MockColorMcu = &m_MockColorMcu;
        g_MockDepthMcu = &m_MockDepthMcu;

        EXPECT_colormcu_imu_register_stream_cb(m_MockColorMcu);
        EXPECT_colormcu_imu_start_streaming(m_MockColorMcu);
        EXPECT_colormcu_imu_stop_streaming(m_MockColorMcu);
        EXPECT_depthmcu_get_extrinsic_calibration(m_MockDepthMcu);
    }

    void TearDown() override
    {
        // Verify all expectations and clear them before the next test
        Mock::VerifyAndClear(&m_MockColorMcu);
        Mock::VerifyAndClear(&m_MockDepthMcu);

        g_MockColorMcu = NULL;
        g_MockDepthMcu = NULL;
    }
};

static k4a_capture_t capture_manufacture(size_t size)
{
    k4a_result_t result;
    k4a_capture_t capture = NULL;
    k4a_image_t image = NULL;

    result = TRACE_CALL(capture_create(&capture));
    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(image_create_empty_internal(ALLOCATION_SOURCE_IMU, size, &image));
    }

    if (K4A_SUCCEEDED(result))
    {
        capture_set_imu_image(capture, image);
    }

    if (K4A_FAILED(result))
    {
        capture_dec_ref(capture);
        capture = NULL;
    }

    if (image)
    {
        image_dec_ref(image);
        image = NULL;
    }
    return capture;
}

TEST_F(imu_ut, create)
{
    // Create the instance
    imu_t imu_handle1 = NULL;
    imu_t imu_handle2 = NULL;
    TICK_COUNTER_HANDLE tick;

    ASSERT_NE((TICK_COUNTER_HANDLE)0, (tick = tickcounter_create()));

    // Sanity check success
    calibration_t calibration_handle;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, calibration_create(FAKE_DEPTH_MCU, &calibration_handle));

    // validate input checking
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(0, NULL, NULL, NULL));

    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(0, FAKE_COLOR_MCU, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(0, NULL, calibration_handle, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(0, NULL, NULL, &imu_handle1));
    ASSERT_EQ(imu_handle1, (imu_t)NULL);
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(0, FAKE_COLOR_MCU, calibration_handle, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(0, FAKE_COLOR_MCU, NULL, &imu_handle1));
    ASSERT_EQ(imu_handle1, (imu_t)NULL);
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(0, NULL, calibration_handle, &imu_handle1));
    ASSERT_EQ(imu_handle1, (imu_t)NULL);
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(0, FAKE_COLOR_MCU, calibration_handle, &imu_handle1));
    ASSERT_EQ(imu_handle1, (imu_t)NULL);

    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(tick, NULL, NULL, NULL));

    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(tick, FAKE_COLOR_MCU, NULL, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(tick, NULL, calibration_handle, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(tick, NULL, NULL, &imu_handle1));
    ASSERT_EQ(imu_handle1, (imu_t)NULL);
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(tick, FAKE_COLOR_MCU, calibration_handle, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(tick, FAKE_COLOR_MCU, NULL, &imu_handle1));
    ASSERT_EQ(imu_handle1, (imu_t)NULL);
    ASSERT_EQ(K4A_RESULT_FAILED, imu_create(tick, NULL, calibration_handle, &imu_handle1));
    ASSERT_EQ(imu_handle1, (imu_t)NULL);

    // Create an instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, imu_create(tick, FAKE_COLOR_MCU, calibration_handle, &imu_handle1));
    ASSERT_NE(imu_handle1, (imu_t)NULL);

    // Create a second instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, imu_create(tick, FAKE_COLOR_MCU, calibration_handle, &imu_handle2));
    ASSERT_NE(imu_handle2, (imu_t)NULL);

    // Verify the instances are unique
    EXPECT_NE(imu_handle1, imu_handle2);

    // Destroy the instances
    imu_destroy(imu_handle1);
    imu_destroy(imu_handle2);
    tickcounter_destroy(tick);
    calibration_destroy(calibration_handle);
}

TEST_F(imu_ut, start)
{
    // Create the  instance
    imu_t imu_handle = NULL;
    TICK_COUNTER_HANDLE tick;

    ASSERT_NE((TICK_COUNTER_HANDLE)0, (tick = tickcounter_create()));

    calibration_t calibration_handle;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, calibration_create(FAKE_DEPTH_MCU, &calibration_handle));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, imu_create(tick, FAKE_COLOR_MCU, calibration_handle, &imu_handle));
    ASSERT_NE(imu_handle, (imu_t)NULL);

    ASSERT_EQ(K4A_RESULT_FAILED, imu_start(NULL, 0));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, imu_start(imu_handle, 0));

    // Destroy the instance
    imu_destroy(imu_handle);
    tickcounter_destroy(tick);
    calibration_destroy(calibration_handle);
}

TEST_F(imu_ut, get_sample)
{
    // Create the  instance
    imu_t imu_handle = NULL;
    calibration_t calibration_handle;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, calibration_create(FAKE_DEPTH_MCU, &calibration_handle));
    k4a_capture_t cb_capture;
    k4a_image_t image;
    k4a_imu_sample_t imu_sample;
    imu_payload_metadata_t *p_imu_packet;
    TICK_COUNTER_HANDLE tick;

    ASSERT_NE((TICK_COUNTER_HANDLE)0, (tick = tickcounter_create()));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, imu_create(tick, FAKE_COLOR_MCU, calibration_handle, &imu_handle));
    ASSERT_NE(imu_handle, (imu_t)NULL);

    // Fail if not started
    ASSERT_EQ(K4A_WAIT_RESULT_FAILED, imu_get_sample(imu_handle, &imu_sample, 10));

    // Start the imu
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, imu_start(imu_handle, 0));

    uint32_t test_sample_count_accel = 1;
    uint32_t test_sample_count_gyro = 1;
    uint32_t imu_alloc_size = sizeof(imu_payload_metadata_t) +
                              sizeof(xyz_vector_t) * (test_sample_count_accel + test_sample_count_gyro);

    // put something in the queue
    cb_capture = capture_manufacture(imu_alloc_size);
    image = capture_get_imu_image(cb_capture);
    p_imu_packet = (imu_payload_metadata_t *)image_get_buffer(image);
    // need to fill in the packet data because the callback uses the content
    p_imu_packet->gyro.sample_count = test_sample_count_gyro;
    p_imu_packet->accel.sample_count = test_sample_count_accel;
    g_MockColorMcu->frame_ready_cb(K4A_RESULT_SUCCEEDED, image, g_MockColorMcu->cb_context);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, imu_get_sample(imu_handle, &imu_sample, 0));
    image_dec_ref(image);
    capture_dec_ref(cb_capture);

    cb_capture = capture_manufacture(imu_alloc_size);
    image = capture_get_imu_image(cb_capture);
    p_imu_packet = (imu_payload_metadata_t *)image_get_buffer(image);
    p_imu_packet->gyro.sample_count = test_sample_count_gyro;
    p_imu_packet->accel.sample_count = test_sample_count_accel;
    g_MockColorMcu->frame_ready_cb(K4A_RESULT_SUCCEEDED, image, g_MockColorMcu->cb_context);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, imu_get_sample(imu_handle, &imu_sample, K4A_WAIT_INFINITE));
    capture_dec_ref(cb_capture);
    image_dec_ref(image);

    cb_capture = capture_manufacture(imu_alloc_size);
    image = capture_get_imu_image(cb_capture);
    p_imu_packet = (imu_payload_metadata_t *)image_get_buffer(image);
    p_imu_packet->gyro.sample_count = test_sample_count_gyro;
    p_imu_packet->accel.sample_count = test_sample_count_accel;
    g_MockColorMcu->frame_ready_cb(K4A_RESULT_FAILED, image, g_MockColorMcu->cb_context);
    ASSERT_EQ(K4A_WAIT_RESULT_FAILED, imu_get_sample(imu_handle, NULL, 0));        // bad parameter
    ASSERT_EQ(K4A_WAIT_RESULT_FAILED, imu_get_sample(imu_handle, &imu_sample, 0)); // error state
    capture_dec_ref(cb_capture);
    image_dec_ref(image);

    ASSERT_EQ(allocator_test_for_leaks(), 0);
    // Destroy the instance
    imu_destroy(imu_handle);
    tickcounter_destroy(tick);
    calibration_destroy(calibration_handle);
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
