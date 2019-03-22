// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "firmware_helper.h"

#include <utcommon.h>
#include <k4ainternal/logging.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

class firmware_fw : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, TRACE_CALL(setup_common_test()));
        const ::testing::TestInfo *const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        LOG_INFO("Test %s requires a connection exerciser.", test_info->name());
        LOG_INFO("Disconnecting the device", 0);
        g_connection_exerciser->set_usb_port(0);
        ThreadAPI_Sleep(500);

        // Make sure that all of the firmwares have loaded correctly.
        ASSERT_TRUE(g_test_firmware_buffer != nullptr);
        ASSERT_TRUE(g_test_firmware_size > 0);
        ASSERT_TRUE(g_candidate_firmware_buffer != nullptr);
        ASSERT_TRUE(g_candidate_firmware_size > 0);
        ASSERT_TRUE(g_lkg_firmware_buffer != nullptr);
        ASSERT_TRUE(g_lkg_firmware_size > 0);
        ASSERT_TRUE(g_factory_firmware_buffer != nullptr);
        ASSERT_TRUE(g_factory_firmware_size > 0);

        // Make sure that the Test firmware has all components with a different version when compared to the Release
        // Candidate firmware.
        // Depth Sensor isn't expect to change.
        ASSERT_FALSE(compare_version(g_test_firmware_package_info.audio, g_candidate_firmware_package_info.audio));
        ASSERT_FALSE(compare_version(g_test_firmware_package_info.depth, g_candidate_firmware_package_info.depth));
        ASSERT_FALSE(compare_version(g_test_firmware_package_info.rgb, g_candidate_firmware_package_info.rgb));

        // There should be no other devices.
        uint32_t device_count = 0;
        usb_cmd_get_device_count(&device_count);
        ASSERT_EQ((uint32_t)0, device_count);
    }

    void TearDown() override
    {
        if (firmware_handle != nullptr)
        {
            firmware_destroy(firmware_handle);
            firmware_handle = nullptr;
        }

        if (serial_number != nullptr)
        {
            free(serial_number);
            serial_number = nullptr;
            serial_number_length = 0;
        }
    }

    firmware_t firmware_handle = nullptr;
    char *serial_number = nullptr;
    size_t serial_number_length = 0;
};

TEST_F(firmware_fw, DISABLED_update_timing)
{
    LOG_INFO("Beginning the manual test to get update timings.", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, g_connection_exerciser->set_usb_port(g_k4a_port_number));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    ASSERT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, firmware_get_device_serialnum(firmware_handle, NULL, &serial_number_length));

    serial_number = (char *)malloc(serial_number_length);
    ASSERT_NE(nullptr, serial_number);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              firmware_get_device_serialnum(firmware_handle, serial_number, &serial_number_length));

    LOG_INFO("Updating the device to the Candidate firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_candidate_firmware_buffer,
                                    g_candidate_firmware_size,
                                    g_candidate_firmware_package_info,
                                    true));

    ASSERT_TRUE(compare_device_serial_number(firmware_handle, serial_number));

    LOG_INFO("Updating the device to the Test firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_test_firmware_buffer,
                                    g_test_firmware_size,
                                    g_test_firmware_package_info,
                                    true));

    ASSERT_TRUE(compare_device_serial_number(firmware_handle, serial_number));
}

TEST_F(firmware_fw, simple_update_from_lkg)
{
    LOG_INFO("Beginning the basic update test from the LKG firmware.", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, g_connection_exerciser->set_usb_port(g_k4a_port_number));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    ASSERT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, firmware_get_device_serialnum(firmware_handle, NULL, &serial_number_length));

    serial_number = (char *)malloc(serial_number_length);
    ASSERT_NE(nullptr, serial_number);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              firmware_get_device_serialnum(firmware_handle, serial_number, &serial_number_length));

    LOG_INFO("Updating the device to the LKG firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_lkg_firmware_buffer,
                                    g_lkg_firmware_size,
                                    g_lkg_firmware_package_info,
                                    false));

    ASSERT_TRUE(compare_device_serial_number(firmware_handle, serial_number));

    LOG_INFO("Updating the device to the Candidate firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_candidate_firmware_buffer,
                                    g_candidate_firmware_size,
                                    g_candidate_firmware_package_info,
                                    false));

    ASSERT_TRUE(compare_device_serial_number(firmware_handle, serial_number));

    LOG_INFO("Updating the device to the Test firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_test_firmware_buffer,
                                    g_test_firmware_size,
                                    g_test_firmware_package_info,
                                    false));

    ASSERT_TRUE(compare_device_serial_number(firmware_handle, serial_number));
}

TEST_F(firmware_fw, simple_update_from_factory)
{
    LOG_INFO("Beginning the basic update test from the Factory firmware.", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, g_connection_exerciser->set_usb_port(g_k4a_port_number));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    ASSERT_EQ(K4A_BUFFER_RESULT_TOO_SMALL, firmware_get_device_serialnum(firmware_handle, NULL, &serial_number_length));

    serial_number = (char *)malloc(serial_number_length);
    ASSERT_NE(nullptr, serial_number);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              firmware_get_device_serialnum(firmware_handle, serial_number, &serial_number_length));

    LOG_INFO("Updating the device to the Factory firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_factory_firmware_buffer,
                                    g_factory_firmware_size,
                                    g_factory_firmware_package_info,
                                    false));

    ASSERT_TRUE(compare_device_serial_number(firmware_handle, serial_number));

    LOG_INFO("Updating the device to the Candidate firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_candidate_firmware_buffer,
                                    g_candidate_firmware_size,
                                    g_candidate_firmware_package_info,
                                    false));

    ASSERT_TRUE(compare_device_serial_number(firmware_handle, serial_number));

    LOG_INFO("Updating the device to the Test firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_test_firmware_buffer,
                                    g_test_firmware_size,
                                    g_test_firmware_package_info,
                                    false));

    ASSERT_TRUE(compare_device_serial_number(firmware_handle, serial_number));
}
