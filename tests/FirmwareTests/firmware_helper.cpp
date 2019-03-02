// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "firmware_helper.h"
#include <utcommon.h>

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