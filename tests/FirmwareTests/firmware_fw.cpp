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

void firmware_fw::open_firmware_device()
{
    int retry = 0;
    uint32_t device_count = 0;

    while (device_count == 0 && retry++ < 20)
    {
        ThreadAPI_Sleep(500);
        usb_cmd_get_device_count(&device_count);
    }

    ASSERT_LE(retry, 20) << "Device never returned.";

    LOG_INFO("Opening firmware device...", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_create(K4A_DEVICE_DEFAULT, &firmware_handle))
        << "Couldn't open firmware\n";
    ASSERT_NE(firmware_handle, nullptr);
}

void firmware_fw::reset_device()
{
    LOG_INFO("Resetting device...", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_reset_device(firmware_handle));

    firmware_destroy(firmware_handle);
    firmware_handle = nullptr;

    // Re-open the device to ensure it is ready.
    open_firmware_device();
}

void firmware_fw::interrupt_operation(firmware_operation_interruption_t interruption)
{
    switch (interruption)
    {
    case FIRMWARE_OPERATION_RESET:
        reset_device();
        break;

    default:
        ASSERT_TRUE(false) << "Unknown interruption type";
    }
}

void firmware_fw::interrupt_device_at_update_stage(firmware_operation_component_t component,
                                                   firmware_operation_interruption_t interruption,
                                                   firmware_status_summary_t *finalStatus,
                                                   bool verbose_logging)
{
    ASSERT_NE(nullptr, finalStatus);

    bool all_complete = false;
    k4a_result_t result = K4A_RESULT_FAILED;
    firmware_status_summary_t previous_status = {};

    tickcounter_ms_t start_time_ms, now;

    TICK_COUNTER_HANDLE tick = tickcounter_create();
    ASSERT_NE(nullptr, tick) << "Failed to create tick counter.";

    ASSERT_EQ(0, tickcounter_get_current_ms(tick, &start_time_ms));

    do
    {
        // This is not the necessarily the final status we will get, but at any point could return and the caller needs
        // to know the state of the update when we return.
        result = firmware_get_download_status(firmware_handle, finalStatus);
        if (result == K4A_RESULT_SUCCEEDED)
        {
            if (verbose_logging)
            {
                if (memcmp(&previous_status.audio, &finalStatus->audio, sizeof(firmware_component_status_t)) != 0)
                {
                    LOG_INFO("Audio: A:%d V:%d T:%d E:%d W:%d O:%d",
                             finalStatus->audio.authentication_check,
                             finalStatus->audio.version_check,
                             finalStatus->audio.image_transfer,
                             finalStatus->audio.flash_erase,
                             finalStatus->audio.flash_write,
                             finalStatus->audio.overall);
                }

                if (memcmp(&previous_status.depth_config,
                           &finalStatus->depth_config,
                           sizeof(firmware_component_status_t)) != 0)
                {
                    LOG_INFO("Depth Config: A:%d V:%d T:%d E:%d W:%d O:%d",
                             finalStatus->depth_config.authentication_check,
                             finalStatus->depth_config.version_check,
                             finalStatus->depth_config.image_transfer,
                             finalStatus->depth_config.flash_erase,
                             finalStatus->depth_config.flash_write,
                             finalStatus->depth_config.overall);
                }

                if (memcmp(&previous_status.depth, &finalStatus->depth, sizeof(firmware_component_status_t)) != 0)
                {
                    LOG_INFO("Depth: A:%d V:%d T:%d E:%d W:%d O:%d",
                             finalStatus->depth.authentication_check,
                             finalStatus->depth.version_check,
                             finalStatus->depth.image_transfer,
                             finalStatus->depth.flash_erase,
                             finalStatus->depth.flash_write,
                             finalStatus->depth.overall);
                }

                if (memcmp(&previous_status.rgb, &finalStatus->rgb, sizeof(firmware_component_status_t)) != 0)
                {
                    LOG_INFO("RGB: A:%d V:%d T:%d E:%d W:%d O:%d",
                             finalStatus->rgb.authentication_check,
                             finalStatus->rgb.version_check,
                             finalStatus->rgb.image_transfer,
                             finalStatus->rgb.flash_erase,
                             finalStatus->rgb.flash_write,
                             finalStatus->rgb.overall);
                }

                previous_status = *finalStatus;
            }

            all_complete = ((finalStatus->audio.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                            (finalStatus->depth_config.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                            (finalStatus->depth.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                            (finalStatus->rgb.overall > FIRMWARE_OPERATION_INPROGRESS));

            // Check to see if now is the correct time to interrupt the device.
            switch (component)
            {
            case FIRMWARE_OPERATION_START:
                // As early as possible reset the device.
                interrupt_operation(interruption);
                return;

            case FIRMWARE_OPERATION_AUDIO_ERASE:
                if (finalStatus->audio.image_transfer == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The erase takes places after the transfer is complete and takes about 7.8 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 7600);
                    sleepTime = 3800;
                    LOG_INFO("Audio Erase started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(firmware_handle, finalStatus);
                    interrupt_operation(interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_AUDIO_WRITE:
                if (finalStatus->audio.flash_erase == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The write takes places after the erase is complete and takes about 20 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 19700);
                    sleepTime = 9850;
                    LOG_INFO("Audio Write started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(firmware_handle, finalStatus);
                    interrupt_operation(interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_DEPTH_ERASE:
                if (finalStatus->depth.image_transfer == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The erase takes places after the transfer is complete and takes about 0.25 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 100);
                    sleepTime = 50;
                    LOG_INFO("Depth Erase started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(firmware_handle, finalStatus);
                    interrupt_operation(interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_DEPTH_WRITE:
                if (finalStatus->depth.flash_erase == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The write takes places after the transfer is complete and takes about 5.8 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 5700);
                    sleepTime = 2850;
                    LOG_INFO("Depth Write started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(firmware_handle, finalStatus);
                    interrupt_operation(interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_RGB_ERASE:
                if (finalStatus->rgb.image_transfer == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The erase takes places after the transfer is complete and takes about 0.05 seconds.
                    LOG_INFO("RGB erase started...\n");
                    firmware_get_download_status(firmware_handle, finalStatus);
                    interrupt_operation(interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_RGB_WRITE:
                if (finalStatus->rgb.flash_erase == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The write takes places after the transfer is complete and takes about 6.1 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 6000);
                    sleepTime = 3000;
                    LOG_INFO("RGB Write started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(firmware_handle, finalStatus);
                    interrupt_operation(interruption);
                    return;
                }
                break;

            default:
                break;
            }
        }
        else
        {
            // Failed to get the status of the update operation. Break out of the loop to attempt to reset the device
            // and return.
            EXPECT_TRUE(false) << "ERROR: Failed to get the firmware update status.";
            break;
        }

        ASSERT_EQ(0, tickcounter_get_current_ms(tick, &now));

        if (!all_complete && (now - start_time_ms > UPDATE_TIMEOUT_MS))
        {
            // The update hasn't completed and too much time as passed. Break out of the loop to attempt to reset the
            // device and return.
            EXPECT_TRUE(false) << "ERROR: Timeout waiting for the update to complete.";
            break;
        }

        if (!all_complete)
        {
            ThreadAPI_Sleep(UPDATE_POLL_INTERVAL_MS);
        }
    } while (!all_complete);

    // At this point the update has either completed or timed out. Either way the device needs to be reset after the
    // update has progressed.
    interrupt_operation(FIRMWARE_OPERATION_RESET);
}

void firmware_fw::perform_device_update(uint8_t *firmware_buffer,
                                        size_t firmware_size,
                                        firmware_package_info_t firmware_package_info,
                                        bool verbose_logging)
{
    firmware_status_summary_t finalStatus;

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    log_firmware_version(firmware_package_info);

    // Perform upgrade...
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, firmware_buffer, firmware_size));
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_FULL_DEVICE,
                                     FIRMWARE_OPERATION_RESET,
                                     &finalStatus,
                                     verbose_logging);

    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.rgb));

    // Check upgrade...
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, firmware_package_info.audio)) << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     firmware_package_info.depth_config_number_versions,
                                     firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, firmware_package_info.rgb)) << "RGB mismatch";
}

TEST_F(firmware_fw, DISABLED_update_timing)
{
    LOG_INFO("Beginning the manual test to get update timings.", 0);
    connEx->set_usb_port(k4a_port_number);

    open_firmware_device();

    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(candidate_firmware_buffer, candidate_firmware_size, candidate_firmware_package_info, true);

    LOG_INFO("Updating the device to the Test firmware.");
    perform_device_update(test_firmware_buffer, test_firmware_size, test_firmware_package_info, true);
}

TEST_F(firmware_fw, simple_update_from_lkg)
{
    LOG_INFO("Beginning the basic update test from the LKG firmware.", 0);
    connEx->set_usb_port(k4a_port_number);

    open_firmware_device();

    LOG_INFO("Updating the device to the LKG firmware.");
    perform_device_update(lkg_firmware_buffer, lkg_firmware_size, lkg_firmware_package_info, false);

    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(candidate_firmware_buffer, candidate_firmware_size, candidate_firmware_package_info, false);

    LOG_INFO("Updating the device to the Test firmware.");
    perform_device_update(test_firmware_buffer, test_firmware_size, test_firmware_package_info, false);
}

TEST_F(firmware_fw, simple_update_from_factory)
{
    LOG_INFO("Beginning the basic update test from the Factory firmware.", 0);
    connEx->set_usb_port(k4a_port_number);

    open_firmware_device();

    LOG_INFO("Updating the device to the Factory firmware.");
    perform_device_update(factory_firmware_buffer, factory_firmware_size, factory_firmware_package_info, false);

    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(candidate_firmware_buffer, candidate_firmware_size, candidate_firmware_package_info, false);

    LOG_INFO("Updating the device to the Test firmware.");
    perform_device_update(test_firmware_buffer, test_firmware_size, test_firmware_package_info, false);
}

TEST_F(firmware_fw, interrupt_update_reboot)
{
    firmware_status_summary_t finalStatus;
    LOG_INFO("Beginning the update test with reboots interrupting the update", 0);

    LOG_INFO("Powering on the device...", 0);
    connEx->set_usb_port(k4a_port_number);

    open_firmware_device();

    // Update to the Candidate firmware
    LOG_INFO("Updating the device to the Candidate firmware.");
    perform_device_update(candidate_firmware_buffer, candidate_firmware_size, candidate_firmware_package_info, false);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));

    // Update to the Test firmware, but interrupt at the very beginning...
    LOG_INFO("Interrupting at the beginning of the update");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, test_firmware_buffer, test_firmware_size));
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_START, FIRMWARE_OPERATION_RESET, &finalStatus, false);

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
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_AUDIO_ERASE, FIRMWARE_OPERATION_RESET, &finalStatus, false);

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
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_AUDIO_WRITE, FIRMWARE_OPERATION_RESET, &finalStatus, false);

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
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_DEPTH_ERASE, FIRMWARE_OPERATION_RESET, &finalStatus, false);

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
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_DEPTH_WRITE, FIRMWARE_OPERATION_RESET, &finalStatus, false);

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
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_RGB_ERASE, FIRMWARE_OPERATION_RESET, &finalStatus, false);

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
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_RGB_WRITE, FIRMWARE_OPERATION_RESET, &finalStatus, false);

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
    perform_device_update(lkg_firmware_buffer, lkg_firmware_size, lkg_firmware_package_info, false);
}
