// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

// Module being tested
#include <k4ainternal/depth_mcu.h>
#include <../src/depth_mcu/depthcommands.h>

// Module being mocked
#include <k4ainternal/usbcommand.h>

using namespace testing;

#define USB_INDEX (0)
#define FAKE_USB ((usbcmd_t)0xface000)

// Define a mock class for the public functions usb_cmd
class MockUsbCmd
{
public:
    MOCK_CONST_METHOD4(usb_cmd_create,
                       k4a_result_t(usb_command_device_type_t device_type,
                                    uint32_t device_index,
                                    const guid_t *container_id,
                                    usbcmd_t *p_command_handle));

    MOCK_CONST_METHOD1(usb_cmd_destroy, void(usbcmd_t p_command_handle));

    MOCK_CONST_METHOD7(usb_cmd_read,
                       k4a_result_t(usbcmd_t p_handle,
                                    uint32_t cmd,
                                    uint8_t *p_cmd_data, /* changed void* to uint8_t */
                                    size_t cmd_data_size,
                                    uint8_t *p_data, /* changed void* to uint8_t */
                                    size_t data_size,
                                    size_t *bytes_read));

    MOCK_CONST_METHOD8(usb_cmd_read_with_status,
                       k4a_result_t(usbcmd_t p_handle,
                                    uint32_t cmd,
                                    uint8_t *p_cmd_data, /* changed void* to uint8_t */
                                    size_t cmd_data_size,
                                    uint8_t *p_data, /* changed void* to uint8_t */
                                    size_t data_size,
                                    size_t *bytes_read,
                                    uint32_t *cmd_status));

    MOCK_CONST_METHOD6(usb_cmd_write,
                       k4a_result_t(usbcmd_t p_handle,
                                    uint32_t cmd,
                                    uint8_t *p_cmd_data, /* changed void* to uint8_t */
                                    size_t cmd_data_size,
                                    uint8_t *p_data, /* changed void* to uint8_t */
                                    size_t data_size));

    MOCK_CONST_METHOD7(usb_cmd_write_with_status,
                       k4a_result_t(usbcmd_t p_handle,
                                    uint32_t cmd,
                                    uint8_t *p_cmd_data, /* changed void* to uint8_t */
                                    size_t cmd_data_size,
                                    uint8_t *p_data, /* changed void* to uint8_t */
                                    size_t data_size,
                                    uint32_t *cmd_status));

    MOCK_CONST_METHOD3(usb_cmd_stream_register_cb,
                       k4a_result_t(usbcmd_t p_command_handle, usb_cmd_stream_cb_t *frame_ready_cb, void *context));

    MOCK_CONST_METHOD2(usb_cmd_stream_start, k4a_result_t(usbcmd_t p_command_handle, size_t payload_size));

    MOCK_CONST_METHOD1(usb_cmd_stream_stop, k4a_result_t(usbcmd_t p_command_handle));
};

extern "C" {

// Use a global singleton for the mock object.
static MockUsbCmd *g_MockUsbCmd;

// Define the symbols needed from the usb_cmd module.
// Only functions required to link the depth module are needed
k4a_result_t usb_cmd_create(usb_command_device_type_t device_type,
                            uint32_t device_index,
                            const guid_t *container_id,
                            usbcmd_t *p_command_handle)
{
    return g_MockUsbCmd->usb_cmd_create(device_type, device_index, container_id, p_command_handle);
}

void usb_cmd_destroy(usbcmd_t p_command_handle)
{
    g_MockUsbCmd->usb_cmd_destroy(p_command_handle);
}

k4a_result_t usb_cmd_read(usbcmd_t p_handle,
                          uint32_t cmd,
                          uint8_t *p_cmd_data,
                          size_t cmd_data_size,
                          uint8_t *p_data,
                          size_t data_size,
                          size_t *bytes_read)
{
    // Direct the call to the mock class
    return g_MockUsbCmd->usb_cmd_read(p_handle, cmd, p_cmd_data, cmd_data_size, p_data, data_size, bytes_read);
}

k4a_result_t usb_cmd_read_with_status(usbcmd_t p_handle,
                                      uint32_t cmd,
                                      uint8_t *p_cmd_data,
                                      size_t cmd_data_size,
                                      uint8_t *p_data,
                                      size_t data_size,
                                      size_t *bytes_read,
                                      uint32_t *cmd_status)
{
    // Direct the call to the mock class
    return g_MockUsbCmd
        ->usb_cmd_read_with_status(p_handle, cmd, p_cmd_data, cmd_data_size, p_data, data_size, bytes_read, cmd_status);
}

k4a_result_t usb_cmd_write(usbcmd_t p_handle,
                           uint32_t cmd,
                           uint8_t *p_cmd_data,
                           size_t cmd_data_size,
                           uint8_t *p_data,
                           size_t data_size)
{
    // Direct the call to the mock class
    return g_MockUsbCmd->usb_cmd_write(p_handle, cmd, p_cmd_data, cmd_data_size, p_data, data_size);
}

k4a_result_t usb_cmd_write_with_status(usbcmd_t p_handle,
                                       uint32_t cmd,
                                       uint8_t *p_cmd_data,
                                       size_t cmd_data_size,
                                       uint8_t *p_data,
                                       size_t data_size,
                                       uint32_t *cmd_status)
{
    // Direct the call to the mock class
    return g_MockUsbCmd
        ->usb_cmd_write_with_status(p_handle, cmd, p_cmd_data, cmd_data_size, p_data, data_size, cmd_status);
}

k4a_result_t usb_cmd_stream_register_cb(usbcmd_t p_command_handle, usb_cmd_stream_cb_t *frame_ready_cb, void *context)
{
    // Direct the call to the mock class
    return g_MockUsbCmd->usb_cmd_stream_register_cb(p_command_handle, frame_ready_cb, context);
}

k4a_result_t usb_cmd_stream_start(usbcmd_t p_command_handle, size_t payload_size)
{
    return g_MockUsbCmd->usb_cmd_stream_start(p_command_handle, payload_size);
}

k4a_result_t usb_cmd_stream_stop(usbcmd_t p_command_handle)
{
    return g_MockUsbCmd->usb_cmd_stream_stop(p_command_handle);
}
}

// Set an expectation on the mock object for a serial number USB request which will succeed
static void EXPECT_SerialNumberCall(MockUsbCmd &usbcmd, char *mockSerialNumber, size_t mockSerialNumberBytes)
{
    EXPECT_CALL(usbcmd,
                usb_cmd_read(
                    /* usbcmd_t p_handle */ FAKE_USB,
                    /* uint32_t cmd */ DEV_CMD_DEPTH_READ_PRODUCT_SN,
                    /* uint8_t* p_cmd_data */ IsNull(),
                    /* size_t cmd_data_size */ 0,
                    /* uint8_t* p_data */ NotNull(),
                    /* size_t data_size */ _,
                    /* size_t *bytes_read */ NotNull()))
        .WillRepeatedly(Invoke([mockSerialNumber, mockSerialNumberBytes](Unused /* usbcmd_t p_handle */,
                                                                         Unused /* uint32_t cmd */,
                                                                         Unused /* uint8_t* p_cmd_data */,
                                                                         Unused /* uint32_t cmd_data_size */,
                                                                         uint8_t *p_data,
                                                                         size_t data_size,
                                                                         size_t *bytes_read) {
            if (data_size >= mockSerialNumberBytes)
            {
                memcpy(p_data, mockSerialNumber, mockSerialNumberBytes);
                *bytes_read = mockSerialNumberBytes;
                return K4A_RESULT_SUCCEEDED;
            }
            return K4A_RESULT_FAILED;
        }));
}

// Set an expectation on the mock object for a serial number USB request which will fail
static void EXPECT_SerialNumberCallFail(MockUsbCmd &usbcmd, k4a_result_t failure)
{
    EXPECT_CALL(usbcmd,
                usb_cmd_read(
                    /* usbcmd_t p_handle */ FAKE_USB,
                    /* uint32_t cmd */ DEV_CMD_DEPTH_READ_PRODUCT_SN,
                    /* void* p_cmd_data */ IsNull(),
                    /* size_t cmd_data_size */ 0,
                    /* void* p_data */ NotNull(),
                    /* size_t data_size */ _,
                    /* size_t *bytes_read */ NotNull()))
        .WillRepeatedly(Invoke([failure](Unused /* usbcmd_t p_handle */,
                                         Unused /* uint32_t cmd */,
                                         Unused /* void* p_cmd_data */,
                                         Unused /* size_t cmd_data_size */,
                                         Unused /* void* p_data */,
                                         Unused /* size_t data_size */,
                                         Unused /* size_t *bytes_read */) { return failure; }));
}

static void EXPECT_UsbCmdCreateSuccess(MockUsbCmd &usbcmd)
{
    EXPECT_CALL(usbcmd,
                usb_cmd_create(
                    /* usb_command_device_type_t device_type  */ USB_DEVICE_DEPTH_PROCESSOR,
                    /* uint32_t device_index                  */ 0,
                    /* const guid_t *container_id             */ NULL,
                    /* usbcmd_t *p_command_handle */ NotNull()))
        .WillRepeatedly(Invoke([](Unused, // usb_command_device_type_t device_type
                                  Unused, // uint8_t device_index
                                  Unused, // const guid_t* container_id
                                  usbcmd_t *p_command_handle) {
            *p_command_handle = FAKE_USB;
            return K4A_RESULT_SUCCEEDED;
        }));
}

// Validate the contract of errors returned from usb_cmd_create and how
// depthmcu_create responds to that.
static void EXPECT_UsbCmdCreateFailure3(MockUsbCmd &usbcmd)
{
    EXPECT_CALL(usbcmd,
                usb_cmd_create(
                    /* usb_command_device_type_t device_type  */ USB_DEVICE_DEPTH_PROCESSOR,
                    /* uint32_t device_index                  */ 0,
                    /* const guid_t *container_id             */ NULL,
                    /* usbcmd_t *p_command_handle */ NotNull()))
        .WillOnce(Return(K4A_RESULT_FAILED))
        .WillOnce(Return(K4A_RESULT_FAILED))
        .WillOnce(Return(K4A_RESULT_FAILED));
}

static void EXPECT_UsbCmdDestroyNone(MockUsbCmd &usbcmd)
{
    EXPECT_CALL(usbcmd,
                usb_cmd_destroy(
                    /* usbcmd_t p_command_handle */ NotNull()))
        .Times(0);
}

static void EXPECT_UsbCmdDestroy(MockUsbCmd &usbcmd)
{
    EXPECT_CALL(usbcmd,
                usb_cmd_destroy(
                    /* usbcmd_t p_command_handle */ NotNull()))
        .WillRepeatedly(Return());
}

static void EXPECT_UsbCmdRegisterCallback(MockUsbCmd &usbcmd)
{
    EXPECT_CALL(usbcmd,
                usb_cmd_stream_register_cb(
                    /* usbcmd_t p_command_handle, */ FAKE_USB,
                    /* usb_cmd_stream_cb_t* frame_ready_cb */ NotNull(),
                    /* void *context*/ _))
        .WillRepeatedly(Return(K4A_RESULT_SUCCEEDED));
}

const guid_t *usb_cmd_get_container_id(usbcmd_t usbcmd_handle)
{
    (void)usbcmd_handle;
    return NULL;
}

class depthmcu_ut : public ::testing::Test
{
protected:
    MockUsbCmd m_MockUsb;

    void SetUp() override
    {
        g_MockUsbCmd = &m_MockUsb;
        EXPECT_UsbCmdCreateSuccess(m_MockUsb);
        EXPECT_UsbCmdDestroy(m_MockUsb);
        EXPECT_UsbCmdRegisterCallback(m_MockUsb);
    }

    void TearDown() override
    {
        // Verify all expectations and clear them before the next test
        Mock::VerifyAndClear(&m_MockUsb);
        g_MockUsbCmd = NULL;
    }
};

TEST_F(depthmcu_ut, create)
{
    // Create the depth instance
    depthmcu_t depthmcu_handle1 = NULL;
    depthmcu_t depthmcu_handle2 = NULL;

    // no call to usb_cmd_create should be made
    ASSERT_EQ(K4A_RESULT_FAILED, depthmcu_create(USB_INDEX, NULL));
    ASSERT_EQ(K4A_RESULT_FAILED, depthmcu_create(255, &depthmcu_handle1));

    EXPECT_UsbCmdCreateFailure3(m_MockUsb);
    EXPECT_UsbCmdDestroyNone(m_MockUsb);
    ASSERT_EQ(K4A_RESULT_FAILED, depthmcu_create(USB_INDEX, &depthmcu_handle1));
    ASSERT_EQ(K4A_RESULT_FAILED, depthmcu_create(USB_INDEX, &depthmcu_handle1));
    ASSERT_EQ(K4A_RESULT_FAILED, depthmcu_create(USB_INDEX, &depthmcu_handle1));

    EXPECT_UsbCmdCreateSuccess(m_MockUsb);
    EXPECT_UsbCmdDestroy(m_MockUsb);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depthmcu_create(USB_INDEX, &depthmcu_handle1));
    ASSERT_NE(depthmcu_handle1, (depthmcu_t)NULL);

    // Create a second instance
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depthmcu_create(USB_INDEX, &depthmcu_handle2));
    ASSERT_NE(depthmcu_handle2, (depthmcu_t)NULL);

    // Verify the instances are unique
    EXPECT_NE(depthmcu_handle1, depthmcu_handle2);

    // Destroy the depth instances
    depthmcu_destroy(depthmcu_handle1);
    depthmcu_destroy(depthmcu_handle2);

    // No calls to the usb_cmd layer should have been made. It is assumed that the
    // create call should be fast and not conduct any I/O

    // By not defining any expectations, we should fail when we leave this test if any have been
}

#define MAX_USER_ALLOCATED_BUFFER 128
static void TestSerialNumInputConditions(depthmcu_t depthmcu_handle, char *expectedMockSerialNumber)
{
    char serialno_initvalue[MAX_USER_ALLOCATED_BUFFER] = { 0 };
    char serialno[MAX_USER_ALLOCATED_BUFFER] = { 0 };

    // Use 0xab as the initial value of the buffer so we can see which bytes get modified
    memset(serialno_initvalue, 0xab, sizeof(serialno_initvalue));
    memcpy(serialno, serialno_initvalue, sizeof(serialno));
    ASSERT_EQ(sizeof(serialno_initvalue), sizeof(serialno));

    size_t serialnoSize = 0;
    // If the string parameter is null, we expect only serialnoSize to be set
    // Since serialnoSize is too small, we expect K4A_BUFFER_RESULT_TOO_SMALL
    EXPECT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, depthmcu_get_serialnum(depthmcu_handle, NULL, &serialnoSize));
    // serialnoSize should be the size of the serial number, plus the null terminator
    EXPECT_EQ(serialnoSize, strlen(expectedMockSerialNumber) + 1);

    serialnoSize = 1000;
    // If the string parameter is null, we expect only serialnoSize to be set
    // Since the string parameter is null, we expect K4A_BUFFER_RESULT_TOO_SMALL
    EXPECT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, depthmcu_get_serialnum(depthmcu_handle, NULL, &serialnoSize));
    // serialnoSize should be the size of the serial number, plus the null terminator
    EXPECT_EQ(serialnoSize, strlen(expectedMockSerialNumber) + 1);

    // Check the case where the input buffer is larger than needed

    serialnoSize = sizeof(serialno);
    memcpy(serialno, serialno_initvalue, sizeof(serialno));
    EXPECT_EQ(K4A_BUFFER_RESULT_SUCCEEDED, depthmcu_get_serialnum(depthmcu_handle, serialno, &serialnoSize));
    // serialnoSize should be the size of the serial number, plus the null terminator
    EXPECT_EQ(serialnoSize, strlen(expectedMockSerialNumber) + 1);

    // The string should have been properly set
    EXPECT_STREQ(expectedMockSerialNumber, serialno);

    // Check the case where the input buffer is exactly the correct size

    serialnoSize = strlen(expectedMockSerialNumber) + 1;
    memcpy(serialno, serialno_initvalue, sizeof(serialno));
    EXPECT_EQ(K4A_BUFFER_RESULT_SUCCEEDED, depthmcu_get_serialnum(depthmcu_handle, serialno, &serialnoSize));
    // serialnoSize should be the size of the serial number, plus the null terminator
    EXPECT_EQ(serialnoSize, strlen(expectedMockSerialNumber) + 1);
    // The string should have been properly set
    EXPECT_STREQ(expectedMockSerialNumber, serialno);

    // Check the case where the input buffer is too small

    serialnoSize = 5;
    ASSERT_LT(serialnoSize, strlen(expectedMockSerialNumber));

    memcpy(serialno, serialno_initvalue, sizeof(serialno));
    EXPECT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, depthmcu_get_serialnum(depthmcu_handle, serialno, &serialnoSize));
    // serialnoSize should be the size of the serial number, plus the null terminator
    EXPECT_EQ(serialnoSize, strlen(expectedMockSerialNumber) + 1);

    // Expect the buffer has been initialized to a safe value
    EXPECT_STREQ("", serialno);

    // Check the case where the input buffer is an empty string

    serialnoSize = 0;
    ASSERT_LT(serialnoSize, strlen(expectedMockSerialNumber));

    memcpy(serialno, serialno_initvalue, sizeof(serialno));
    EXPECT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, depthmcu_get_serialnum(depthmcu_handle, serialno, &serialnoSize));
    // serialnoSize should be the size of the serial number, plus the null terminator
    EXPECT_EQ(serialnoSize, strlen(expectedMockSerialNumber) + 1);

    // Expect the buffer has not been touched
    EXPECT_EQ(0, memcmp(serialno_initvalue, serialno, strlen(expectedMockSerialNumber)));
}

TEST_F(depthmcu_ut, depthmcu_get_serialnum_base)
{
    EXPECT_UsbCmdCreateSuccess(m_MockUsb);
    EXPECT_UsbCmdDestroy(m_MockUsb);

    // Create the depth instance
    depthmcu_t depthmcu_handle = NULL;
    ASSERT_EQ(K4A_BUFFER_RESULT_SUCCEEDED, depthmcu_create(USB_INDEX, &depthmcu_handle));
    ASSERT_NE(depthmcu_handle, (depthmcu_t)NULL);

    // serial_number_size is a required parameter
    char serialno[10];
    EXPECT_EQ(K4A_BUFFER_RESULT_FAILED, depthmcu_get_serialnum(depthmcu_handle, NULL, NULL));
    EXPECT_EQ(K4A_BUFFER_RESULT_FAILED, depthmcu_get_serialnum(depthmcu_handle, serialno, NULL));

    depthmcu_destroy(depthmcu_handle);
}

TEST_F(depthmcu_ut, depthmcu_get_serialnum_null_terminated)
{
    // Create the depth instance
    depthmcu_t depthmcu_handle = NULL;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depthmcu_create(USB_INDEX, &depthmcu_handle));
    ASSERT_NE(depthmcu_handle, (depthmcu_t)NULL);

    char mockSerialNumber[] = "1234567890";
    char expectedSerialNumber[] = "1234567890";

    // If the implementation caches the result, only a single call may be made. Otherwise it may be called multiple
    // times
    EXPECT_SerialNumberCall(m_MockUsb, mockSerialNumber, sizeof(mockSerialNumber));

    // Test the various input edge cases
    TestSerialNumInputConditions(depthmcu_handle, expectedSerialNumber);

    depthmcu_destroy(depthmcu_handle);
}

TEST_F(depthmcu_ut, depthmcu_get_serialnum_not_terminated)
{
    // Create the depth instance
    depthmcu_t depthmcu_handle = NULL;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depthmcu_create(USB_INDEX, &depthmcu_handle));
    ASSERT_NE(depthmcu_handle, (depthmcu_t)NULL);

    // Create a serial number with no NULL termination
    char mockSerialNumber[10];
    memcpy(mockSerialNumber, "1234567890", 10);
    char expectedSerialNumber[] = "1234567890";

    // If the implementation caches the result, only a single call may be made. Otherwise it may be called multiple
    // times
    EXPECT_SerialNumberCall(m_MockUsb, mockSerialNumber, sizeof(mockSerialNumber));

    // Test the various input edge cases
    TestSerialNumInputConditions(depthmcu_handle, expectedSerialNumber);

    depthmcu_destroy(depthmcu_handle);
}

TEST_F(depthmcu_ut, depthmcu_get_serialnum_extra_padding)
{
    // Create the depth instance
    depthmcu_t depthmcu_handle = NULL;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depthmcu_create(USB_INDEX, &depthmcu_handle));
    ASSERT_NE(depthmcu_handle, (depthmcu_t)NULL);

    // Create a serial number with extra characters after the NULL
    char mockSerialNumber[20];
    memcpy(mockSerialNumber, "1234567890\0WXYZ", sizeof("1234567890\0WXYZ"));
    char expectedSerialNumber[] = "1234567890";

    // If the implementation caches the result, only a single call may be made. Otherwise it may be called multiple
    // times
    EXPECT_SerialNumberCall(m_MockUsb, mockSerialNumber, sizeof(mockSerialNumber));

    // Test the various input edge cases
    TestSerialNumInputConditions(depthmcu_handle, expectedSerialNumber);

    depthmcu_destroy(depthmcu_handle);
}

TEST_F(depthmcu_ut, depthmcu_get_serialnum_non_ascii)
{
    // Create the depth instance
    depthmcu_t depthmcu_handle = NULL;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depthmcu_create(USB_INDEX, &depthmcu_handle));
    ASSERT_NE(depthmcu_handle, (depthmcu_t)NULL);

    // Create a serial number with non-ACII characters
    char mockSerialNumber[20];
    memcpy(mockSerialNumber, "12\145\t67890\0WXYZ", sizeof("12\145\t67890\0WXYZ"));

    // If the implementation caches the result, only a single call may be made. Otherwise it may be called multiple
    // times
    EXPECT_SerialNumberCall(m_MockUsb, mockSerialNumber, sizeof(mockSerialNumber));

    char serialno[MAX_USER_ALLOCATED_BUFFER];
    size_t serialnoSize = sizeof(serialno);
    EXPECT_EQ(K4A_BUFFER_RESULT_FAILED, depthmcu_get_serialnum(depthmcu_handle, serialno, &serialnoSize));

    depthmcu_destroy(depthmcu_handle);
}

TEST_F(depthmcu_ut, depthmcu_get_serialnum_devicefailure)
{
    // Create the depth instance
    depthmcu_t depthmcu_handle = NULL;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depthmcu_create(USB_INDEX, &depthmcu_handle));
    ASSERT_NE(depthmcu_handle, (depthmcu_t)NULL);

    // Cause the mock to return a failure
    EXPECT_SerialNumberCallFail(m_MockUsb, K4A_RESULT_FAILED);

    char serialno[MAX_USER_ALLOCATED_BUFFER];
    size_t serialnoSize = sizeof(serialno);
    EXPECT_EQ(K4A_BUFFER_RESULT_FAILED, depthmcu_get_serialnum(depthmcu_handle, serialno, &serialnoSize));

    depthmcu_destroy(depthmcu_handle);
}

TEST_F(depthmcu_ut, depthmcu_get_serialnum_extra_long)
{
    // Create the depth instance
    depthmcu_t depthmcu_handle = NULL;
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, depthmcu_create(USB_INDEX, &depthmcu_handle));
    ASSERT_NE(depthmcu_handle, (depthmcu_t)NULL);

    // Construct a large null string
    size_t largeSize = 1024 * 1024;
    char *bigbuffer = (char *)malloc(largeSize);
    memset(bigbuffer, 'a', largeSize);
    bigbuffer[largeSize - 1] = '\0';

    // Cause the mock to return the large buffer
    EXPECT_SerialNumberCall(m_MockUsb, bigbuffer, largeSize);

    // Hardware returning excessively large buffers is considered an error
    char serialno[MAX_USER_ALLOCATED_BUFFER];
    size_t serialnoSize = sizeof(serialno);
    EXPECT_EQ(K4A_BUFFER_RESULT_FAILED, depthmcu_get_serialnum(depthmcu_handle, serialno, &serialnoSize));

    free(bigbuffer);
    depthmcu_destroy(depthmcu_handle);
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
