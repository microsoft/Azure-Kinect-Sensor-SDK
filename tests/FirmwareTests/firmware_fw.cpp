// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "firmware_helper.h"

#include <utcommon.h>
#include <k4ainternal/logging.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

class firmware_fw : public ::testing::Test
{
public:
    static void SetUpTestCase()
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, TRACE_CALL(setup_common_test()));
    }

    static void TearDownTestCase() {}

protected:
    void SetUp() override
    {
        const ::testing::TestInfo *const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        LOG_INFO("Test %s requires a connection exerciser.", test_info->name());
        LOG_INFO("Disconnecting the device", 0);
        g_connection_exerciser->set_usb_port(0);
        ThreadAPI_Sleep(500);

        // Make sure that all of the firmwares have loaded correctly.
        ASSERT_TRUE(test_firmware_buffer != nullptr);
        ASSERT_TRUE(test_firmware_size > 0);
        ASSERT_TRUE(candidate_firmware_buffer != nullptr);
        ASSERT_TRUE(candidate_firmware_size > 0);
        ASSERT_TRUE(lkg_firmware_buffer != nullptr);
        ASSERT_TRUE(lkg_firmware_size > 0);
        ASSERT_TRUE(factory_firmware_buffer != nullptr);
        ASSERT_TRUE(factory_firmware_size > 0);

        // Make sure that the Test firmware has all components with a different version when compared to the Release
        // Candidate firmware.
        // Depth Config isn't expect to change.
        ASSERT_FALSE(compare_version(test_firmware_package_info.audio, candidate_firmware_package_info.audio));
        ASSERT_FALSE(compare_version(test_firmware_package_info.depth, candidate_firmware_package_info.depth));
        ASSERT_FALSE(compare_version(test_firmware_package_info.rgb, candidate_firmware_package_info.rgb));

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
    }

    firmware_t firmware_handle = nullptr;
};

TEST_F(firmware_fw, DISABLED_update_timing)
{
    LOG_INFO("Beginning the manual test to get update timings.", 0);
    g_connection_exerciser->set_usb_port(g_k4a_port_number);

    open_firmware_device(&firmware_handle);

    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(&firmware_handle,
                          candidate_firmware_buffer,
                          candidate_firmware_size,
                          candidate_firmware_package_info,
                          true);

    LOG_INFO("Updating the device to the Test firmware.");
    perform_device_update(&firmware_handle, test_firmware_buffer, test_firmware_size, test_firmware_package_info, true);
}

TEST_F(firmware_fw, simple_update_from_lkg)
{
    LOG_INFO("Beginning the basic update test from the LKG firmware.", 0);
    g_connection_exerciser->set_usb_port(g_k4a_port_number);

    open_firmware_device(&firmware_handle);

    LOG_INFO("Updating the device to the LKG firmware.");
    perform_device_update(&firmware_handle, lkg_firmware_buffer, lkg_firmware_size, lkg_firmware_package_info, false);

    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(&firmware_handle,
                          candidate_firmware_buffer,
                          candidate_firmware_size,
                          candidate_firmware_package_info,
                          false);

    LOG_INFO("Updating the device to the Test firmware.");
    perform_device_update(&firmware_handle,
                          test_firmware_buffer,
                          test_firmware_size,
                          test_firmware_package_info,
                          false);
}

TEST_F(firmware_fw, simple_update_from_factory)
{
    LOG_INFO("Beginning the basic update test from the Factory firmware.", 0);
    g_connection_exerciser->set_usb_port(g_k4a_port_number);

    open_firmware_device(&firmware_handle);

    LOG_INFO("Updating the device to the Factory firmware.");
    perform_device_update(&firmware_handle,
                          factory_firmware_buffer,
                          factory_firmware_size,
                          factory_firmware_package_info,
                          false);

    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(&firmware_handle,
                          candidate_firmware_buffer,
                          candidate_firmware_size,
                          candidate_firmware_package_info,
                          false);

    LOG_INFO("Updating the device to the Test firmware.");
    perform_device_update(&firmware_handle,
                          test_firmware_buffer,
                          test_firmware_size,
                          test_firmware_package_info,
                          false);
}
