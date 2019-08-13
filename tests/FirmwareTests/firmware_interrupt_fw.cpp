// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "firmware_helper.h"
#include <utcommon.h>

#include <k4ainternal/logging.h>
#include <k4ainternal/usbcommand.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

struct firmware_interrupt_parameters
{
    int test_number;
    const char *test_name;
    firmware_operation_component_t component;
    firmware_operation_interruption_t interruption;

    friend std::ostream &operator<<(std::ostream &os, const firmware_interrupt_parameters &obj)
    {
        return os << "test " << (int)obj.test_number << ": " << obj.test_name;
    }
};

class firmware_interrupt_fw : public ::testing::Test,
                              public ::testing::WithParamInterface<firmware_interrupt_parameters>
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, TRACE_CALL(setup_common_test()));
        const ::testing::TestInfo *const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        printf("\nStarting test %s. This requires a connection exerciser to be connected.\n", test_info->name());
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

        // Make sure that the Test firmware has all components with a different version when compared to the Release
        // Candidate firmware.
        // Depth Sensor isn't expect to change.
        ASSERT_FALSE(compare_version(g_test_firmware_package_info.audio, g_candidate_firmware_package_info.audio));
        ASSERT_FALSE(compare_version(g_test_firmware_package_info.depth, g_candidate_firmware_package_info.depth));
        ASSERT_FALSE(compare_version(g_test_firmware_package_info.rgb, g_candidate_firmware_package_info.rgb));

        // There should be no other devices as the tests use the default device to connect to.
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
    k4a_hardware_version_t current_version = { 0 };
};

TEST_P(firmware_interrupt_fw, interrupt_update)
{
    firmware_interrupt_parameters parameters = GetParam();
    firmware_status_summary_t finalStatus;

    printf("Beginning the \'%s\' test. Stage: %d Interruption: %d",
           parameters.test_name,
           parameters.component,
           parameters.interruption);

    LOG_INFO("Powering on the device...", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, g_connection_exerciser->set_usb_port(g_k4a_port_number));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, open_firmware_device(&firmware_handle));

    // Update to the Candidate firmware
    printf("\n == Updating the device to the Candidate firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_candidate_firmware_buffer,
                                    g_candidate_firmware_size,
                                    g_candidate_firmware_package_info,
                                    false));

    // Update to the Test firmware, but interrupt...
    printf("\n == Beginning of the firmware update to the Test Firmware with interruption...\n");

    // Prepend the "Firmware Package Versions:\n" with "Test".
    printf("Test ");
    log_firmware_version(g_test_firmware_package_info);

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, g_test_firmware_buffer, g_test_firmware_size));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              interrupt_device_at_update_stage(&firmware_handle,
                                               parameters.component,
                                               parameters.interruption,
                                               &finalStatus,
                                               false));

    std::cout << "Updated completed with Audio: " << calculate_overall_component_status(finalStatus.audio)
              << " Depth Config: " << calculate_overall_component_status(finalStatus.depth_config)
              << " Depth: " << calculate_overall_component_status(finalStatus.depth)
              << " RGB: " << calculate_overall_component_status(finalStatus.rgb) << std::endl;

    // Check that we are still on the version we expect
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);

    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     g_candidate_firmware_package_info.depth_config_number_versions,
                                     g_candidate_firmware_package_info.depth_config_versions))
        << "Depth sensor does not exist in package.";

    switch (parameters.component)
    {
    case FIRMWARE_OPERATION_START:
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.audio));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth_config));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));
        ASSERT_TRUE(compare_version(current_version.audio, g_candidate_firmware_package_info.audio))
            << "Audio version mismatch";
        ASSERT_TRUE(compare_version(current_version.depth, g_candidate_firmware_package_info.depth))
            << "Depth mismatch";
        ASSERT_TRUE(compare_version(current_version.rgb, g_candidate_firmware_package_info.rgb)) << "RGB mismatch";
        break;

    case FIRMWARE_OPERATION_AUDIO_ERASE:
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.audio));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth_config));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));
        ASSERT_TRUE(compare_version(current_version.audio, { 0 })) << "Audio version mismatch";
        ASSERT_TRUE(compare_version(current_version.depth, g_candidate_firmware_package_info.depth))
            << "Depth mismatch";
        ASSERT_TRUE(compare_version(current_version.rgb, g_candidate_firmware_package_info.rgb)) << "RGB mismatch";
        break;

    case FIRMWARE_OPERATION_AUDIO_WRITE:
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.audio));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth_config));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));
        ASSERT_TRUE(compare_version(current_version.audio, g_test_firmware_package_info.audio))
            << "Audio version mismatch";
        ASSERT_TRUE(compare_version(current_version.depth, g_candidate_firmware_package_info.depth))
            << "Depth mismatch";
        ASSERT_TRUE(compare_version(current_version.rgb, g_candidate_firmware_package_info.rgb)) << "RGB mismatch";
        break;

    case FIRMWARE_OPERATION_DEPTH_ERASE:
    case FIRMWARE_OPERATION_DEPTH_WRITE:
        ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
        ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.depth));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));
        ASSERT_TRUE(compare_version(current_version.audio, g_test_firmware_package_info.audio))
            << "Audio version mismatch";
        ASSERT_TRUE(compare_version(current_version.rgb, g_candidate_firmware_package_info.rgb)) << "RGB mismatch";

        // Don't fail on the Depth version as it appears to be non-deterministic based on when the reset actually
        // happened.
        if (!compare_version(current_version.depth, { 0 }))
        {
            printf("  ** The Depth version was not expected\n");
        }
        break;

    case FIRMWARE_OPERATION_RGB_ERASE:
    case FIRMWARE_OPERATION_RGB_WRITE:
        ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
        ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
        ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth));
        ASSERT_EQ(FIRMWARE_OPERATION_INPROGRESS, calculate_overall_component_status(finalStatus.rgb));

        // Don't fail on the Audio, Depth, and RGB versions as they appear to be non-deterministic based on when the
        // reset actually happened.
        if (!compare_version(current_version.audio, g_test_firmware_package_info.audio))
        {
            printf("  **  The Audio version was not expected\n");
        }
        if (!compare_version(current_version.depth, g_test_firmware_package_info.depth))
        {
            printf("  **  The Depth version was not expected\n");
        }
        if (!compare_version(current_version.rgb, { 0 }))
        {
            printf("  **  The RGB version was not expected\n");
        }
        break;

    default:
        ASSERT_TRUE(false) << "Unhandled component type. " << parameters.component;
    }

    // Update back to the LKG firmware to make sure that works.
    printf("\n == Updating the device back to the LKG firmware.\n");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              perform_device_update(&firmware_handle,
                                    g_lkg_firmware_buffer,
                                    g_lkg_firmware_size,
                                    g_lkg_firmware_package_info,
                                    false));
}

static struct firmware_interrupt_parameters tests_interrupt_reboot[] = {
    { 0, "Reset device at update start", FIRMWARE_OPERATION_START, FIRMWARE_INTERRUPTION_RESET },
    { 1, "Reset device during Audio erase", FIRMWARE_OPERATION_AUDIO_ERASE, FIRMWARE_INTERRUPTION_RESET },
    { 2, "Reset device during Audio write", FIRMWARE_OPERATION_AUDIO_WRITE, FIRMWARE_INTERRUPTION_RESET },
    { 3, "Reset device during Depth erase", FIRMWARE_OPERATION_DEPTH_ERASE, FIRMWARE_INTERRUPTION_RESET },
    { 4, "Reset device during Depth write", FIRMWARE_OPERATION_DEPTH_WRITE, FIRMWARE_INTERRUPTION_RESET },
    { 5, "Reset device during RGB erase", FIRMWARE_OPERATION_RGB_ERASE, FIRMWARE_INTERRUPTION_RESET },
    { 6, "Reset device during RGB write", FIRMWARE_OPERATION_RGB_WRITE, FIRMWARE_INTERRUPTION_RESET },
};

INSTANTIATE_TEST_CASE_P(interrupt_reboot, firmware_interrupt_fw, ::testing::ValuesIn(tests_interrupt_reboot));

static struct firmware_interrupt_parameters tests_interrupt_disconnect[] = {
    { 0, "Disconnect device at update start", FIRMWARE_OPERATION_START, FIRMWARE_INTERRUPTION_DISCONNECT },
    { 1, "Disconnect device during Audio erase", FIRMWARE_OPERATION_AUDIO_ERASE, FIRMWARE_INTERRUPTION_DISCONNECT },
    { 2, "Disconnect device during Audio write", FIRMWARE_OPERATION_AUDIO_WRITE, FIRMWARE_INTERRUPTION_DISCONNECT },
    { 3, "Disconnect device during Depth erase", FIRMWARE_OPERATION_DEPTH_ERASE, FIRMWARE_INTERRUPTION_DISCONNECT },
    { 4, "Disconnect device during Depth write", FIRMWARE_OPERATION_DEPTH_WRITE, FIRMWARE_INTERRUPTION_DISCONNECT },
    { 5, "Disconnect device during RGB erase", FIRMWARE_OPERATION_RGB_ERASE, FIRMWARE_INTERRUPTION_DISCONNECT },
    { 6, "Disconnect device during RGB write", FIRMWARE_OPERATION_RGB_WRITE, FIRMWARE_INTERRUPTION_DISCONNECT },
};

INSTANTIATE_TEST_CASE_P(interrupt_disconnect, firmware_interrupt_fw, ::testing::ValuesIn(tests_interrupt_disconnect));
