// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define _CRT_NONSTDC_NO_DEPRECATE

#include "firmware_helper.h"

#include <utcommon.h>
#include <ConnEx.h>

#include <k4ainternal/logging.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

struct firmware_interrupt_parameters
{
    firmware_operation_component_t component;
    firmware_operation_interruption_t interruption;
};

class firmware_interrupt_fw : public ::testing::Test,
                              public ::testing::WithParamInterface<firmware_interrupt_parameters>
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

        if (depthmcu_handle != nullptr)
        {
            depthmcu_destroy(depthmcu_handle);
            depthmcu_handle = nullptr;
        }
    }

    void open_firmware_device();
    void interrupt_operation(firmware_operation_interruption_t interruption);
    void reset_device();
    void interrupt_device_at_update_stage(firmware_operation_component_t component,
                                          firmware_operation_interruption_t interruption,
                                          firmware_status_summary_t *finalStatus,
                                          bool verbose_logging);
    void perform_device_update(uint8_t *firmware_buffer,
                               size_t firmware_size,
                               firmware_package_info_t firmware_package_info,
                               bool verbose_logging);

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

    depthmcu_t depthmcu_handle = nullptr;
    firmware_t firmware_handle = nullptr;

    depthmcu_firmware_versions_t current_version = { 0 };
};

int firmware_interrupt_fw::k4a_port_number = -1;
connection_exerciser *firmware_interrupt_fw::connEx = nullptr;

uint8_t *firmware_interrupt_fw::test_firmware_buffer = nullptr;
size_t firmware_interrupt_fw::test_firmware_size = 0;
firmware_package_info_t firmware_interrupt_fw::test_firmware_package_info = { 0 };

uint8_t *firmware_interrupt_fw::candidate_firmware_buffer = nullptr;
size_t firmware_interrupt_fw::candidate_firmware_size = 0;
firmware_package_info_t firmware_interrupt_fw::candidate_firmware_package_info = { 0 };

uint8_t *firmware_interrupt_fw::lkg_firmware_buffer = nullptr;
size_t firmware_interrupt_fw::lkg_firmware_size = 0;
firmware_package_info_t firmware_interrupt_fw::lkg_firmware_package_info = { 0 };

uint8_t *firmware_interrupt_fw::factory_firmware_buffer = nullptr;
size_t firmware_interrupt_fw::factory_firmware_size = 0;
firmware_package_info_t firmware_interrupt_fw::factory_firmware_package_info = { 0 };
