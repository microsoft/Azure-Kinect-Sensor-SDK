// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define _CRT_NONSTDC_NO_DEPRECATE

#include "firmware_helper.h"

#include <utcommon.h>
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
    static void SetUpTestCase() {}

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
    k4a_hardware_version_t current_version = { 0 };
};

TEST_F(firmware_interrupt_fw, interrupt_update_reboot)
{
    firmware_status_summary_t finalStatus;
    LOG_INFO("Beginning the update test with reboots interrupting the update", 0);

    LOG_INFO("Powering on the device...", 0);
    g_connection_exerciser->set_usb_port(g_k4a_port_number);

    open_firmware_device(&firmware_handle);

    // Update to the Candidate firmware
    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(&firmware_handle,
                          candidate_firmware_buffer,
                          candidate_firmware_size,
                          candidate_firmware_package_info,
                          false);

    // Update to the Test firmware, but interrupt at the very beginning...
    LOG_INFO("Interrupting at the beginning of the update");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, test_firmware_buffer, test_firmware_size));
    interrupt_device_at_update_stage(&firmware_handle,
                                     FIRMWARE_OPERATION_START,
                                     FIRMWARE_OPERATION_RESET,
                                     &finalStatus,
                                     false);

    std::cout << "Updated completed with Audio: " << calculate_overall_component_status(finalStatus.audio)
              << " Depth Config: " << calculate_overall_component_status(finalStatus.depth_config)
              << " Depth: " << calculate_overall_component_status(finalStatus.depth)
              << " RGB: " << calculate_overall_component_status(finalStatus.rgb) << std::endl;

    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));

    // Check that we are still on the old version
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, candidate_firmware_package_info.audio))
        << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     candidate_firmware_package_info.depth_config_number_versions,
                                     candidate_firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, candidate_firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, candidate_firmware_package_info.rgb)) << "RGB mismatch";

    // Update to the Test firmware, but interrupt at the audio erase stage...
    LOG_INFO("Interrupting at Audio Erase stage");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, test_firmware_buffer, test_firmware_size));
    interrupt_device_at_update_stage(&firmware_handle,
                                     FIRMWARE_OPERATION_AUDIO_ERASE,
                                     FIRMWARE_OPERATION_RESET,
                                     &finalStatus,
                                     false);

    std::cout << "Updated completed with Audio: " << calculate_overall_component_status(finalStatus.audio)
              << " Depth Config: " << calculate_overall_component_status(finalStatus.depth_config)
              << " Depth: " << calculate_overall_component_status(finalStatus.depth)
              << " RGB: " << calculate_overall_component_status(finalStatus.rgb) << std::endl;

    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));

    // Check that we are still on the old version
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, candidate_firmware_package_info.audio))
        << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     candidate_firmware_package_info.depth_config_number_versions,
                                     candidate_firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, candidate_firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, candidate_firmware_package_info.rgb)) << "RGB mismatch";

    // Update to the Test firmware, but interrupt at the audio write stage...
    LOG_INFO("Interrupting at Audio Write stage");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, test_firmware_buffer, test_firmware_size));
    interrupt_device_at_update_stage(&firmware_handle,
                                     FIRMWARE_OPERATION_AUDIO_WRITE,
                                     FIRMWARE_OPERATION_RESET,
                                     &finalStatus,
                                     false);

    std::cout << "Updated completed with Audio: " << calculate_overall_component_status(finalStatus.audio)
              << " Depth Config: " << calculate_overall_component_status(finalStatus.depth_config)
              << " Depth: " << calculate_overall_component_status(finalStatus.depth)
              << " RGB: " << calculate_overall_component_status(finalStatus.rgb) << std::endl;

    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));

    // Check that we are still on the old version
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, candidate_firmware_package_info.audio))
        << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     candidate_firmware_package_info.depth_config_number_versions,
                                     candidate_firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, candidate_firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, candidate_firmware_package_info.rgb)) << "RGB mismatch";

    // Update to the Test firmware, but interrupt at the depth erase stage...
    LOG_INFO("Interrupting at Depth Erase stage");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, test_firmware_buffer, test_firmware_size));
    interrupt_device_at_update_stage(&firmware_handle,
                                     FIRMWARE_OPERATION_DEPTH_ERASE,
                                     FIRMWARE_OPERATION_RESET,
                                     &finalStatus,
                                     false);

    std::cout << "Updated completed with Audio: " << calculate_overall_component_status(finalStatus.audio)
              << " Depth Config: " << calculate_overall_component_status(finalStatus.depth_config)
              << " Depth: " << calculate_overall_component_status(finalStatus.depth)
              << " RGB: " << calculate_overall_component_status(finalStatus.rgb) << std::endl;

    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));

    // Check that we are still on the old version
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, candidate_firmware_package_info.audio))
        << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     candidate_firmware_package_info.depth_config_number_versions,
                                     candidate_firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, candidate_firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, candidate_firmware_package_info.rgb)) << "RGB mismatch";

    // Update to the Test firmware, but interrupt at the depth write stage...
    LOG_INFO("Interrupting at Depth Write stage");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, test_firmware_buffer, test_firmware_size));
    interrupt_device_at_update_stage(&firmware_handle,
                                     FIRMWARE_OPERATION_DEPTH_WRITE,
                                     FIRMWARE_OPERATION_RESET,
                                     &finalStatus,
                                     false);

    std::cout << "Updated completed with Audio: " << calculate_overall_component_status(finalStatus.audio)
              << " Depth Config: " << calculate_overall_component_status(finalStatus.depth_config)
              << " Depth: " << calculate_overall_component_status(finalStatus.depth)
              << " RGB: " << calculate_overall_component_status(finalStatus.rgb) << std::endl;

    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));

    // Check that we are still on the old version
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, candidate_firmware_package_info.audio))
        << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     candidate_firmware_package_info.depth_config_number_versions,
                                     candidate_firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, candidate_firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, candidate_firmware_package_info.rgb)) << "RGB mismatch";

    // Update to the Test firmware, but interrupt at the RGB erase stage...
    LOG_INFO("Interrupting at RGB Erase stage");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, test_firmware_buffer, test_firmware_size));
    interrupt_device_at_update_stage(&firmware_handle,
                                     FIRMWARE_OPERATION_RGB_ERASE,
                                     FIRMWARE_OPERATION_RESET,
                                     &finalStatus,
                                     false);

    std::cout << "Updated completed with Audio: " << calculate_overall_component_status(finalStatus.audio)
              << " Depth Config: " << calculate_overall_component_status(finalStatus.depth_config)
              << " Depth: " << calculate_overall_component_status(finalStatus.depth)
              << " RGB: " << calculate_overall_component_status(finalStatus.rgb) << std::endl;

    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));

    // Check that we are still on the old version
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, candidate_firmware_package_info.audio))
        << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     candidate_firmware_package_info.depth_config_number_versions,
                                     candidate_firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, candidate_firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, candidate_firmware_package_info.rgb)) << "RGB mismatch";

    // Update to the Test firmware, but interrupt at the RGB write stage...
    LOG_INFO("Interrupting at RGB Write stage");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, test_firmware_buffer, test_firmware_size));
    interrupt_device_at_update_stage(&firmware_handle,
                                     FIRMWARE_OPERATION_RGB_WRITE,
                                     FIRMWARE_OPERATION_RESET,
                                     &finalStatus,
                                     false);

    std::cout << "Updated completed with Audio: " << calculate_overall_component_status(finalStatus.audio)
              << " Depth Config: " << calculate_overall_component_status(finalStatus.depth_config)
              << " Depth: " << calculate_overall_component_status(finalStatus.depth)
              << " RGB: " << calculate_overall_component_status(finalStatus.rgb) << std::endl;

    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));

    // Check that we are still on the old version
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, candidate_firmware_package_info.audio))
        << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     candidate_firmware_package_info.depth_config_number_versions,
                                     candidate_firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, candidate_firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, candidate_firmware_package_info.rgb)) << "RGB mismatch";

    // Update back to the LKG firmware
    LOG_INFO("Updating the device back to the LKG firmware.");
    perform_device_update(&firmware_handle, lkg_firmware_buffer, lkg_firmware_size, lkg_firmware_package_info, false);
}
