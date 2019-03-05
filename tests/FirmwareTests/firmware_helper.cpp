// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "firmware_helper.h"
#include <utcommon.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

char *g_candidate_firmware_path = nullptr;

void setup_common_test()
{
    return;
}

k4a_result_t load_firmware_files(char *firmware_path, uint8_t **firmware_buffer, size_t *firmware_size)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware_path == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware_buffer == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware_size == NULL);

    k4a_result_t result = K4A_RESULT_FAILED;
    FILE *pFirmwareFile = NULL;
    size_t numRead = 0;
    uint8_t *tempFirmwareBuffer = NULL;

    if ((pFirmwareFile = fopen(firmware_path, "rb")) != NULL)
    {
        fseek(pFirmwareFile, 0, SEEK_END);
        size_t tempFirmwareSize = (size_t)ftell(pFirmwareFile);
        if (tempFirmwareSize == (size_t)-1L)
        {
            std::cout << "ERROR: Failed to get size of the firmware package." << std::endl;
            return K4A_RESULT_FAILED;
        }

        fseek(pFirmwareFile, 0, SEEK_SET);

        tempFirmwareBuffer = (uint8_t *)malloc(tempFirmwareSize);
        if (tempFirmwareBuffer == NULL)
        {
            std::cout << "ERROR: Failed to allocate memory." << std::endl;
            return K4A_RESULT_FAILED;
        }

        std::cout << "File size: " << tempFirmwareSize << " bytes" << std::endl;
        numRead = fread(tempFirmwareBuffer, tempFirmwareSize, 1, pFirmwareFile);
        fclose(pFirmwareFile);

        if (numRead != 1)
        {
            std::cout << "ERROR: Could not read all data from the file" << std::endl;
        }
        else
        {
            *firmware_buffer = tempFirmwareBuffer;
            *firmware_size = tempFirmwareSize;
            tempFirmwareBuffer = NULL;
            result = K4A_RESULT_SUCCEEDED;
        }
    }
    else
    {
        std::cout << "ERROR: Cannot Open (" << firmware_path << ") errno=" << errno << std::endl;
    }

    if (tempFirmwareBuffer)
    {
        free(tempFirmwareBuffer);
    }

    return result;
}

firmware_operation_status_t calculate_overall_component_status(const firmware_component_status_t status)
{
    if (status.overall == FIRMWARE_OPERATION_SUCCEEDED)
    {
        return FIRMWARE_OPERATION_SUCCEEDED;
    }

    if (status.overall == FIRMWARE_OPERATION_INPROGRESS)
    {
        return FIRMWARE_OPERATION_INPROGRESS;
    }

    // If the version check failed, this component's update was skipped. This could because the new version is
    // an unsafe downgrade or the versions are the same and no update is required.
    if (status.version_check == FIRMWARE_OPERATION_FAILED)
    {
        return FIRMWARE_OPERATION_SUCCEEDED;
    }

    return FIRMWARE_OPERATION_FAILED;
}

bool compare_version(k4a_version_t left_version, k4a_version_t right_version)
{
    return left_version.major == right_version.major && left_version.minor == right_version.minor &&
           left_version.iteration == right_version.iteration;
}

bool compare_version_list(k4a_version_t device_version, uint8_t count, k4a_version_t versions[5])
{
    for (int i = 0; i < count; ++i)
    {
        if (device_version.major == versions[i].major && device_version.minor == versions[i].minor)
        {
            return true;
        }
    }

    return false;
}

void log_firmware_build_config(k4a_firmware_build_t build_config)
{
    std::cout << "  Build Config:             ";
    switch (build_config)
    {
    case K4A_FIRMWARE_BUILD_RELEASE:
        std::cout << "Production" << std::endl;
        break;

    case K4A_FIRMWARE_BUILD_DEBUG:
        std::cout << "Debug" << std::endl;
        break;

    default:
        std::cout << "Unknown" << std::endl;
    }
}

void log_firmware_signature_type(k4a_firmware_signature_t signature_type, bool certificate)
{
    if (certificate)
    {
        std::cout << "  Certificate Type:         ";
    }
    else
    {
        std::cout << "  Signature Type:           ";
    }

    switch (signature_type)
    {
    case K4A_FIRMWARE_SIGNATURE_MSFT:
        std::cout << "Microsoft" << std::endl;
        break;

    case K4A_FIRMWARE_SIGNATURE_TEST:
        std::cout << "Test" << std::endl;
        break;

    case K4A_FIRMWARE_SIGNATURE_UNSIGNED:
        std::cout << "Unsigned" << std::endl;
        break;

    default:
        std::cout << "Unknown (" << signature_type << ")" << std::endl;
    }
}

void log_firmware_version(firmware_package_info_t firmware_version)
{
    std::cout << "Firmware Package Versions:" << std::endl;
    std::cout << "  RGB camera firmware:      " << firmware_version.rgb.major << "." << firmware_version.rgb.minor
              << "." << firmware_version.rgb.iteration << std::endl;
    std::cout << "  Depth camera firmware:    " << firmware_version.depth.major << "." << firmware_version.depth.minor
              << "." << firmware_version.depth.iteration << std::endl;

    std::cout << "  Depth config file:        ";
    for (size_t i = 0; i < firmware_version.depth_config_number_versions; i++)
    {
        std::cout << firmware_version.depth_config_versions[i].major << "."
                  << firmware_version.depth_config_versions[i].minor << " ";
    }
    std::cout << std::endl;

    std::cout << "  Audio firmware:           " << firmware_version.audio.major << "." << firmware_version.audio.minor
              << "." << firmware_version.audio.iteration << std::endl;

    log_firmware_build_config(firmware_version.build_config);

    log_firmware_signature_type(firmware_version.certificate_type, true);
    log_firmware_signature_type(firmware_version.signature_type, false);
}

void log_device_version(k4a_hardware_version_t firmware_version)
{
    std::cout << "Current Firmware Versions:" << std::endl;
    std::cout << "  RGB camera firmware:      " << (uint16_t)firmware_version.rgb.major << "."
              << (uint16_t)firmware_version.rgb.minor << "." << firmware_version.rgb.iteration << std::endl;
    std::cout << "  Depth camera firmware:    " << (uint16_t)firmware_version.depth.major << "."
              << (uint16_t)firmware_version.depth.minor << "." << firmware_version.depth.iteration << std::endl;
    std::cout << "  Depth config file:        " << firmware_version.depth_sensor.major << "."
              << firmware_version.depth_sensor.minor << std::endl;
    std::cout << "  Audio firmware:           " << (uint16_t)firmware_version.audio.major << "."
              << (uint16_t)firmware_version.audio.minor << "." << firmware_version.audio.iteration << std::endl;

    log_firmware_build_config((k4a_firmware_build_t)firmware_version.firmware_build);
    log_firmware_signature_type((k4a_firmware_signature_t)firmware_version.firmware_signature, true);
}

void open_firmware_device(firmware_t *firmware_handle)
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
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_create(K4A_DEVICE_DEFAULT, firmware_handle)) << "Couldn't open firmware\n";
    ASSERT_NE(*firmware_handle, nullptr);
}

void reset_device(firmware_t *firmware_handle)
{
    LOG_INFO("Resetting device...", 0);
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_reset_device(*firmware_handle));

    firmware_destroy(*firmware_handle);
    *firmware_handle = nullptr;

    // Re-open the device to ensure it is ready.
    open_firmware_device(firmware_handle);
}

void interrupt_operation(firmware_t *firmware_handle, firmware_operation_interruption_t interruption)
{
    switch (interruption)
    {
    case FIRMWARE_OPERATION_RESET:
        reset_device(firmware_handle);
        break;

    default:
        ASSERT_TRUE(false) << "Unknown interruption type";
    }
}

void interrupt_device_at_update_stage(firmware_t *firmware_handle,
                                      firmware_operation_component_t component,
                                      firmware_operation_interruption_t interruption,
                                      firmware_status_summary_t *final_status,
                                      bool verbose_logging)
{
    ASSERT_NE(nullptr, final_status);

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
        result = firmware_get_download_status(*firmware_handle, final_status);
        if (result == K4A_RESULT_SUCCEEDED)
        {
            if (verbose_logging)
            {
                if (memcmp(&previous_status.audio, &final_status->audio, sizeof(firmware_component_status_t)) != 0)
                {
                    LOG_INFO("Audio: A:%d V:%d T:%d E:%d W:%d O:%d",
                             final_status->audio.authentication_check,
                             final_status->audio.version_check,
                             final_status->audio.image_transfer,
                             final_status->audio.flash_erase,
                             final_status->audio.flash_write,
                             final_status->audio.overall);
                }

                if (memcmp(&previous_status.depth_config,
                           &final_status->depth_config,
                           sizeof(firmware_component_status_t)) != 0)
                {
                    LOG_INFO("Depth Config: A:%d V:%d T:%d E:%d W:%d O:%d",
                             final_status->depth_config.authentication_check,
                             final_status->depth_config.version_check,
                             final_status->depth_config.image_transfer,
                             final_status->depth_config.flash_erase,
                             final_status->depth_config.flash_write,
                             final_status->depth_config.overall);
                }

                if (memcmp(&previous_status.depth, &final_status->depth, sizeof(firmware_component_status_t)) != 0)
                {
                    LOG_INFO("Depth: A:%d V:%d T:%d E:%d W:%d O:%d",
                             final_status->depth.authentication_check,
                             final_status->depth.version_check,
                             final_status->depth.image_transfer,
                             final_status->depth.flash_erase,
                             final_status->depth.flash_write,
                             final_status->depth.overall);
                }

                if (memcmp(&previous_status.rgb, &final_status->rgb, sizeof(firmware_component_status_t)) != 0)
                {
                    LOG_INFO("RGB: A:%d V:%d T:%d E:%d W:%d O:%d",
                             final_status->rgb.authentication_check,
                             final_status->rgb.version_check,
                             final_status->rgb.image_transfer,
                             final_status->rgb.flash_erase,
                             final_status->rgb.flash_write,
                             final_status->rgb.overall);
                }

                previous_status = *final_status;
            }

            all_complete = ((final_status->audio.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                            (final_status->depth_config.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                            (final_status->depth.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                            (final_status->rgb.overall > FIRMWARE_OPERATION_INPROGRESS));

            // Check to see if now is the correct time to interrupt the device.
            switch (component)
            {
            case FIRMWARE_OPERATION_START:
                // As early as possible reset the device.
                interrupt_operation(firmware_handle, interruption);
                return;

            case FIRMWARE_OPERATION_AUDIO_ERASE:
                if (final_status->audio.image_transfer == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The erase takes places after the transfer is complete and takes about 7.8 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 7600);
                    sleepTime = 3800;
                    LOG_INFO("Audio Erase started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(*firmware_handle, final_status);
                    interrupt_operation(firmware_handle, interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_AUDIO_WRITE:
                if (final_status->audio.flash_erase == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The write takes places after the erase is complete and takes about 20 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 19700);
                    sleepTime = 9850;
                    LOG_INFO("Audio Write started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(*firmware_handle, final_status);
                    interrupt_operation(firmware_handle, interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_DEPTH_ERASE:
                if (final_status->depth.image_transfer == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The erase takes places after the transfer is complete and takes about 0.25 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 100);
                    sleepTime = 50;
                    LOG_INFO("Depth Erase started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(*firmware_handle, final_status);
                    interrupt_operation(firmware_handle, interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_DEPTH_WRITE:
                if (final_status->depth.flash_erase == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The write takes places after the transfer is complete and takes about 5.8 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 5700);
                    sleepTime = 2850;
                    LOG_INFO("Depth Write started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(*firmware_handle, final_status);
                    interrupt_operation(firmware_handle, interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_RGB_ERASE:
                if (final_status->rgb.image_transfer == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The erase takes places after the transfer is complete and takes about 0.05 seconds.
                    LOG_INFO("RGB erase started...\n");
                    firmware_get_download_status(*firmware_handle, final_status);
                    interrupt_operation(firmware_handle, interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_RGB_WRITE:
                if (final_status->rgb.flash_erase == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The write takes places after the transfer is complete and takes about 6.1 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 6000);
                    sleepTime = 3000;
                    LOG_INFO("RGB Write started, waiting %dms.\n", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    firmware_get_download_status(*firmware_handle, final_status);
                    interrupt_operation(firmware_handle, interruption);
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
    interrupt_operation(firmware_handle, FIRMWARE_OPERATION_RESET);
}

void perform_device_update(firmware_t *firmware_handle,
                           uint8_t *firmware_buffer,
                           size_t firmware_size,
                           firmware_package_info_t firmware_package_info,
                           k4a_hardware_version_t *final_version,
                           bool verbose_logging)
{
    firmware_status_summary_t finalStatus;

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(*firmware_handle, final_version));
    log_device_version(*final_version);
    log_firmware_version(firmware_package_info);

    // Perform upgrade...
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(*firmware_handle, firmware_buffer, firmware_size));
    interrupt_device_at_update_stage(firmware_handle,
                                     FIRMWARE_OPERATION_FULL_DEVICE,
                                     FIRMWARE_OPERATION_RESET,
                                     &finalStatus,
                                     verbose_logging);

    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.rgb));

    // Check upgrade...
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(*firmware_handle, final_version));
    log_device_version(*final_version);
    ASSERT_TRUE(compare_version(final_version->audio, firmware_package_info.audio)) << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(final_version->depth_sensor,
                                     firmware_package_info.depth_config_number_versions,
                                     firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(final_version->depth, firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(final_version->rgb, firmware_package_info.rgb)) << "RGB mismatch";
}

int main(int argc, char **argv)
{
    bool error = false;

    srand((unsigned int)time(0)); // use current time as seed for random generator

    k4a_unittest_init();
    ::testing::InitGoogleTest(&argc, argv);

    for (int i = 1; i < argc; ++i)
    {
        char *argument = argv[i];

        for (int j = 0; argument[j]; j++)
        {
            argument[j] = (char)tolower(argument[j]);
        }

        if (strcmp(argument, "--firmware") == 0)
        {
            if (i + 1 <= argc)
            {
                g_candidate_firmware_path = argv[++i];
                printf("Setting g_test_firmware_path = %s\n", g_candidate_firmware_path);
            }
            else
            {
                printf("Error: firmware path parameter missing\n");
                error = true;
            }
        }

        if ((strcmp(argument, "-h") == 0) || (strcmp(argument, "/h") == 0) || (strcmp(argument, "-?") == 0) ||
            (strcmp(argument, "/?") == 0))
        {
            error = true;
        }
    }

    if (error)
    {
        printf("\n\nCustom Test Settings:\n");
        printf("  --firmware <firmware path>\n");
        printf("      This is the path to the candidate firmware that should be tested.\n");

        return 1; // Indicates an error or warning
    }

    return RUN_ALL_TESTS();
}