// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "firmware_helper.h"

#include <utcommon.h>
#include <ConnEx.h>

#include <k4ainternal/logging.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

class firmware_fw : public ::testing::Test
{
public:
    static void SetUpTestCase()
    {
        int port;
        float voltage;
        float current;

        ASSERT_TRUE(g_candidate_firmware_path != nullptr)
            << "The firmware path setting is required and wasn't supplied.\n\n";

        k4a_port_number = -1;
        connEx = new connection_exerciser();

        LOG_INFO("Searching for Connection Exerciser...", 0);
        ASSERT_TRUE(K4A_SUCCEEDED(connEx->find_connection_exerciser()));
        LOG_INFO("Clearing port...");
        ASSERT_TRUE(K4A_SUCCEEDED(connEx->set_usb_port(0)));

        LOG_INFO("Searching for device...");

        for (int i = 0; i < CONN_EX_MAX_NUM_PORTS; ++i)
        {
            ASSERT_TRUE(K4A_SUCCEEDED(connEx->set_usb_port(i)));
            port = connEx->get_usb_port();
            ASSERT_TRUE(port == i);

            voltage = connEx->get_voltage_reading();
            ASSERT_FALSE(voltage == -1);
            current = connEx->get_current_reading();
            ASSERT_FALSE(current == -1);

            if (current < 0)
            {
                current = current * -1;
            }

            if (voltage > 4.5 && voltage < 5.5 && current > 0.1)
            {
                ASSERT_TRUE(k4a_port_number == -1) << "More than one device was detected on the connection exerciser.";
                k4a_port_number = port;
            }

            printf("On port #%d: %4.2fV %4.2fA\n", port, voltage, current);
        }

        ASSERT_FALSE(k4a_port_number == -1)
            << "The Kinect for Azure was not detected on a port of the connection exerciser.";
        connEx->set_usb_port(port);

        std::cout << "Loading Test firmware package: " << K4A_TEST_FIRMWARE_PATH << std::endl;
        load_firmware_files(K4A_TEST_FIRMWARE_PATH, &test_firmware_buffer, &test_firmware_size);
        parse_firmware_package(test_firmware_buffer, test_firmware_size, &test_firmware_package_info);

        std::cout << "Loading Release Candidate firmware package: " << g_candidate_firmware_path << std::endl;
        load_firmware_files(g_candidate_firmware_path, &candidate_firmware_buffer, &candidate_firmware_size);
        parse_firmware_package(candidate_firmware_buffer, candidate_firmware_size, &candidate_firmware_package_info);

        std::cout << "Loading LKG firmware package: " << K4A_LKG_FIRMWARE_PATH << std::endl;
        load_firmware_files(K4A_LKG_FIRMWARE_PATH, &lkg_firmware_buffer, &lkg_firmware_size);
        parse_firmware_package(lkg_firmware_buffer, lkg_firmware_size, &lkg_firmware_package_info);

        std::cout << "Loading Factory firmware package: " << K4A_FACTORY_FIRMWARE_PATH << std::endl;
        load_firmware_files(K4A_FACTORY_FIRMWARE_PATH, &factory_firmware_buffer, &factory_firmware_size);
        parse_firmware_package(factory_firmware_buffer, factory_firmware_size, &factory_firmware_package_info);
    }

    static void TearDownTestCase()
    {
        if (connEx != nullptr)
        {
            delete connEx;
            connEx = nullptr;
        }

        if (test_firmware_buffer != nullptr)
        {
            free(test_firmware_buffer);
            test_firmware_buffer = nullptr;
        }

        if (candidate_firmware_buffer != nullptr)
        {
            free(candidate_firmware_buffer);
            candidate_firmware_buffer = nullptr;
        }

        if (lkg_firmware_buffer != nullptr)
        {
            free(lkg_firmware_buffer);
            lkg_firmware_buffer = nullptr;
        }

        if (factory_firmware_buffer != nullptr)
        {
            free(factory_firmware_buffer);
            factory_firmware_buffer = nullptr;
        }
    }

protected:
    void SetUp() override
    {
        const ::testing::TestInfo *const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        LOG_INFO("Test %s requires a connection exerciser.", test_info->name());
        LOG_INFO("Disconnecting the device", 0);
        connEx->set_usb_port(0);
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

    static int k4a_port_number;
    static connection_exerciser *connEx;

    static uint8_t *test_firmware_buffer;
    static size_t test_firmware_size;
    static firmware_package_info_t test_firmware_package_info;

    static uint8_t *candidate_firmware_buffer;
    static size_t candidate_firmware_size;
    static firmware_package_info_t candidate_firmware_package_info;

    static uint8_t *lkg_firmware_buffer;
    static size_t lkg_firmware_size;
    static firmware_package_info_t lkg_firmware_package_info;

    static uint8_t *factory_firmware_buffer;
    static size_t factory_firmware_size;
    static firmware_package_info_t factory_firmware_package_info;

    firmware_t firmware_handle = nullptr;

    k4a_hardware_version_t current_version = { 0 };
};

int firmware_fw::k4a_port_number = -1;
connection_exerciser *firmware_fw::connEx = nullptr;

uint8_t *firmware_fw::test_firmware_buffer = nullptr;
size_t firmware_fw::test_firmware_size = 0;
firmware_package_info_t firmware_fw::test_firmware_package_info = { 0 };

uint8_t *firmware_fw::candidate_firmware_buffer = nullptr;
size_t firmware_fw::candidate_firmware_size = 0;
firmware_package_info_t firmware_fw::candidate_firmware_package_info = { 0 };

uint8_t *firmware_fw::lkg_firmware_buffer = nullptr;
size_t firmware_fw::lkg_firmware_size = 0;
firmware_package_info_t firmware_fw::lkg_firmware_package_info = { 0 };

uint8_t *firmware_fw::factory_firmware_buffer = nullptr;
size_t firmware_fw::factory_firmware_size = 0;
firmware_package_info_t firmware_fw::factory_firmware_package_info = { 0 };

TEST_F(firmware_fw, DISABLED_update_timing)
{
    LOG_INFO("Beginning the manual test to get update timings.", 0);
    connEx->set_usb_port(k4a_port_number);

    open_firmware_device(&firmware_handle);

    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(&firmware_handle,
                          candidate_firmware_buffer,
                          candidate_firmware_size,
                          candidate_firmware_package_info,
                          &current_version,
                          true);

    LOG_INFO("Updating the device to the Test firmware.");
    perform_device_update(&firmware_handle,
                          test_firmware_buffer,
                          test_firmware_size,
                          test_firmware_package_info,
                          &current_version,
                          true);
}

TEST_F(firmware_fw, simple_update_from_lkg)
{
    LOG_INFO("Beginning the basic update test from the LKG firmware.", 0);
    connEx->set_usb_port(k4a_port_number);

    open_firmware_device(&firmware_handle);

    LOG_INFO("Updating the device to the LKG firmware.");
    perform_device_update(&firmware_handle,
                          lkg_firmware_buffer,
                          lkg_firmware_size,
                          lkg_firmware_package_info,
                          &current_version,
                          false);

    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(&firmware_handle,
                          candidate_firmware_buffer,
                          candidate_firmware_size,
                          candidate_firmware_package_info,
                          &current_version,
                          false);

    LOG_INFO("Updating the device to the Test firmware.");
    perform_device_update(&firmware_handle,
                          test_firmware_buffer,
                          test_firmware_size,
                          test_firmware_package_info,
                          &current_version,
                          false);
}

TEST_F(firmware_fw, simple_update_from_factory)
{
    LOG_INFO("Beginning the basic update test from the Factory firmware.", 0);
    connEx->set_usb_port(k4a_port_number);

    open_firmware_device(&firmware_handle);

    LOG_INFO("Updating the device to the Factory firmware.");
    perform_device_update(&firmware_handle,
                          factory_firmware_buffer,
                          factory_firmware_size,
                          factory_firmware_package_info,
                          &current_version,
                          false);

    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(&firmware_handle,
                          candidate_firmware_buffer,
                          candidate_firmware_size,
                          candidate_firmware_package_info,
                          &current_version,
                          false);

    LOG_INFO("Updating the device to the Test firmware.");
    perform_device_update(&firmware_handle,
                          test_firmware_buffer,
                          test_firmware_size,
                          test_firmware_package_info,
                          &current_version,
                          false);
}
