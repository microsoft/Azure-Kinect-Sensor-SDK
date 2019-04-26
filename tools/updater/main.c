// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifdef _WIN32
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <k4ainternal/firmware.h>
#include <k4ainternal/depth_mcu.h>
#include <k4ainternal/logging.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#include <io.h>
#define EXECUTABLE_NAME "AzureKinectFirmwareTool.exe"
#else
#include <unistd.h>

#include <strings.h>
#define _stricmp strcasecmp
#define EXECUTABLE_NAME "AzureKinectFirmwareTool"
#endif

// System dependencies
#include <stdlib.h>
#include <stdio.h>

#define UPDATE_TIMEOUT_MS 10 * 60 * 1000 // 10 Minutes should be way more than enough.

// Exit Codes
#define EXIT_OK 0      // successful termination
#define EXIT_FAILED -1 // general failure
#define EXIT_USAGE 64  // The arguments were incorrect

typedef enum
{
    K4A_FIRMWARE_COMMAND_UNKNOWN = 0,
    K4A_FIRMWARE_COMMAND_USAGE,
    K4A_FIRMWARE_COMMAND_LIST_DEVICES,
    K4A_FIRMWARE_COMMAND_QUERY_DEVICE,
    K4A_FIRMWARE_COMMAND_UPDATE_DEVICE,
    K4A_FIRMWARE_COMMAND_RESET_DEVICE,
    K4A_FIRMWARE_COMMAND_INSPECT_FIRMWARE,
} k4a_firmware_command_t;

typedef struct _updater_command_info_t
{
    k4a_firmware_command_t requested_command;

    char *firmware_path;

    char *device_serial_number;
    uint32_t device_index;

    firmware_t firmware_handle;

    uint8_t *firmware_buffer;
    size_t firmware_size;
    firmware_package_info_t firmware_package_info;

    k4a_hardware_version_t current_version;
    k4a_hardware_version_t updated_version;
} updater_command_info_t;

static void print_supprted_commands()
{
    printf("* Usage Info *\n");
    printf("    %s <Command> <Aguments> \n", EXECUTABLE_NAME);
    printf("\n");
    printf("Commands:\n");
    printf("    List Devices: -List, -l\n");
    printf("    Query Device: -Query, -q\n");
    printf("        Arguments: [Serial Number]\n");
    printf("    Update Device: -Update, -u\n");
    printf("        Arguments: <Firmware Package Path and FileName> [Serial Number]\n");
    printf("    Reset Device: -Reset, -r\n");
    printf("        Arguments: [Serial Number]\n");
    printf("    Inspect Firmware: -Inspect, -i\n");
    printf("        Arguments: <Firmware Package Path and FileName>\n");
    printf("\n");
    printf("    If no Serial Number is provided, the tool will just connect to the first device.\n");
    printf("\n");
    printf("Examples:\n");
    printf("    %s -List\n", EXECUTABLE_NAME);
    printf("    %s -Update c:\\data\\firmware.bin 0123456\n", EXECUTABLE_NAME);
}

static k4a_result_t ensure_firmware_open(updater_command_info_t *command_info);
static void command_list_devices(void);
static k4a_result_t command_query_device(updater_command_info_t *command_info);
static k4a_result_t command_inspect_firmware(updater_command_info_t *command_info);
static k4a_result_t command_update_device(updater_command_info_t *command_info);
static k4a_result_t command_reset_device(updater_command_info_t *command_info);

static int try_parse_device(int argc, char **argv, int current, updater_command_info_t *command_info)
{
    int next = current + 1;
    if (next >= argc)
    {
        // There is no more arguments
        return current;
    }

    char firstCharacter = argv[next][0];
    if (firstCharacter == '-' || firstCharacter == '/')
    {
        // The next argument is a new flag.
        return current;
    }

    char *device_serial_number = argv[next];

    // NOTE: Leaving this as depthmcu since we only need access to the serial number and firmware will block until the
    // device is ready for other commands that aren't needed.
    depthmcu_t device = NULL;
    uint32_t device_count;
    usb_cmd_get_device_count(&device_count);

    command_info->device_index = UINT32_MAX;

    for (uint32_t deviceIndex = 0; deviceIndex < device_count; deviceIndex++)
    {
        if (K4A_RESULT_SUCCEEDED != depthmcu_create(deviceIndex, &device))
        {
            printf("ERROR: %d: Failed to open device\n", deviceIndex);
            continue;
        }

        char *serial_number = NULL;
        size_t serial_number_length = 0;

        if (K4A_BUFFER_RESULT_TOO_SMALL != depthmcu_get_serialnum(device, NULL, &serial_number_length))
        {
            printf("ERROR: %d: Failed to get serial number length\n", deviceIndex);
            depthmcu_destroy(device);
            device = NULL;
            continue;
        }

        serial_number = malloc(serial_number_length);
        if (serial_number == NULL)
        {
            printf("ERROR: %d: Failed to allocate memory for serial number (%zu bytes)\n",
                   deviceIndex,
                   serial_number_length);
            depthmcu_destroy(device);
            device = NULL;
            continue;
        }

        if (K4A_BUFFER_RESULT_SUCCEEDED != depthmcu_get_serialnum(device, serial_number, &serial_number_length))
        {
            printf("ERROR: %d: Failed to get serial number\n", deviceIndex);
            free(serial_number);
            serial_number = NULL;
            depthmcu_destroy(device);
            device = NULL;
            continue;
        }

        if (strcmp(device_serial_number, serial_number) == 0)
        {
            command_info->device_index = deviceIndex;
            command_info->device_serial_number = serial_number;
            depthmcu_destroy(device);
            device = NULL;
            break;
        }

        free(serial_number);
        serial_number = NULL;
        depthmcu_destroy(device);
        device = NULL;
    }

    if (command_info->device_index == UINT32_MAX)
    {
        printf("ERROR: Unable to find a device with serial number: %s\n", device_serial_number);
    }

    return next;
}

static int parse_command_line(int argc, char **argv, updater_command_info_t *command_info)
{
    if (argc == 1)
    {
        print_supprted_commands();
        return EXIT_OK;
    }

    for (int i = 1; i < argc; i++)
    {
        if (command_info->requested_command != K4A_FIRMWARE_COMMAND_UNKNOWN)
        {
            command_info->requested_command = K4A_FIRMWARE_COMMAND_USAGE;
            printf("ERROR: Too many arguments.\n");
            print_supprted_commands();
            return EXIT_OK;
        }

        if ((0 == _stricmp("-PrintUsage", argv[i])) || (0 == _stricmp("/PrintUsage", argv[i])) ||
            (0 == _stricmp("-Help", argv[i])) || (0 == _stricmp("/Help", argv[i])) || (0 == _stricmp("/?", argv[i])) ||
            (0 == _stricmp("/h", argv[i])) || (0 == _stricmp("-h", argv[i])))
        {
            command_info->requested_command = K4A_FIRMWARE_COMMAND_USAGE;

            print_supprted_commands();
            return EXIT_OK;
        }
        else if ((0 == _stricmp("-List", argv[i])) || (0 == _stricmp("-l", argv[i])) || (0 == _stricmp("/l", argv[i])))
        {
            command_info->requested_command = K4A_FIRMWARE_COMMAND_LIST_DEVICES;
        }
        else if ((0 == _stricmp("-Query", argv[i])) || (0 == _stricmp("-q", argv[i])) || (0 == _stricmp("/q", argv[i])))
        {
            command_info->requested_command = K4A_FIRMWARE_COMMAND_QUERY_DEVICE;
            i = try_parse_device(argc, argv, i, command_info);
            if (command_info->device_index == UINT32_MAX)
            {
                return EXIT_USAGE;
            }
        }
        else if ((0 == _stricmp("-Update", argv[i])) || (0 == _stricmp("-u", argv[i])) ||
                 (0 == _stricmp("/u", argv[i])))
        {
            command_info->requested_command = K4A_FIRMWARE_COMMAND_UPDATE_DEVICE;
            i++;
            if (i >= argc)
            {
                printf("ERROR: Not enough parameters.\n\n");
                print_supprted_commands();
                return EXIT_USAGE;
            }

            command_info->firmware_path = argv[i];

            i = try_parse_device(argc, argv, i, command_info);
            if (command_info->device_index == UINT32_MAX)
            {
                return EXIT_USAGE;
            }
        }
        else if ((0 == _stricmp("-Reset", argv[i])) || (0 == _stricmp("-r", argv[i])) || (0 == _stricmp("/r", argv[i])))
        {
            command_info->requested_command = K4A_FIRMWARE_COMMAND_RESET_DEVICE;
            i = try_parse_device(argc, argv, i, command_info);
            if (command_info->device_index == UINT32_MAX)
            {
                return EXIT_USAGE;
            }
        }
        else if ((0 == _stricmp("-Inspect", argv[i])) || (0 == _stricmp("-i", argv[i])) ||
                 (0 == _stricmp("/i", argv[i])))
        {
            command_info->requested_command = K4A_FIRMWARE_COMMAND_INSPECT_FIRMWARE;
            i++;
            if (i >= argc)
            {
                printf("ERROR: Not enough parameters.\n\n");
                print_supprted_commands();
                return EXIT_USAGE;
            }

            command_info->firmware_path = argv[i];
        }
        else
        {
            printf("ERROR: Unrecognized command.\n\n");
            print_supprted_commands();
            return EXIT_USAGE;
        }
    }

    return EXIT_OK;
}

static k4a_result_t load_firmware_file(updater_command_info_t *command_info)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, command_info == NULL);

    k4a_result_t result = K4A_RESULT_FAILED;
    FILE *pFirmwareFile = NULL;
    size_t numRead = 0;
    uint8_t *tempFirmwareBuffer = NULL;

    if (command_info->firmware_path == NULL)
    {
        printf("ERROR: The firmware path was not specified.\n");
        return K4A_RESULT_FAILED;
    }

    if ((pFirmwareFile = fopen(command_info->firmware_path, "rb")) != NULL)
    {
        fseek(pFirmwareFile, 0, SEEK_END);
        size_t tempFirmwareSize = (size_t)ftell(pFirmwareFile);
        if (tempFirmwareSize == (size_t)-1L)
        {
            printf("ERROR: Failed to get size of the firmware package.\n");
            return K4A_RESULT_FAILED;
        }

        fseek(pFirmwareFile, 0, SEEK_SET);

        tempFirmwareBuffer = (uint8_t *)malloc(tempFirmwareSize);
        if (tempFirmwareBuffer == NULL)
        {
            printf("ERROR: Failed to allocate memory.\n");
            return K4A_RESULT_FAILED;
        }

        printf("File size: %zu bytes\n", tempFirmwareSize);
        numRead = fread(tempFirmwareBuffer, tempFirmwareSize, 1, pFirmwareFile);
        fclose(pFirmwareFile);

        if (numRead != 1)
        {
            printf("ERROR: Could not read all data from the file, %zu.\n", numRead);
        }
        else
        {
            command_info->firmware_buffer = tempFirmwareBuffer;
            command_info->firmware_size = tempFirmwareSize;
            tempFirmwareBuffer = NULL;
            result = K4A_RESULT_SUCCEEDED;
        }
    }
    else
    {
        printf("ERROR: Cannot Open (%s) errno=%d\n", command_info->firmware_path, errno);
    }

    if (tempFirmwareBuffer)
    {
        free(tempFirmwareBuffer);
    }

    return result;
}

static void print_firmware_build_config(k4a_firmware_build_t build_config)
{
    printf("  Build Config:             ");
    switch (build_config)
    {
    case K4A_FIRMWARE_BUILD_RELEASE:
        printf("Production\n");
        break;

    case K4A_FIRMWARE_BUILD_DEBUG:
        printf("Debug\n");
        break;

    default:
        printf("Unknown (%d)\n", build_config);
    }
}

static void print_firmware_signature_type(k4a_firmware_signature_t signature_type, bool certificate)
{
    if (certificate)
    {
        printf("  Certificate Type:         ");
    }
    else
    {
        printf("  Signature Type:           ");
    }

    switch (signature_type)
    {
    case K4A_FIRMWARE_SIGNATURE_MSFT:
        printf("Microsoft\n");
        break;

    case K4A_FIRMWARE_SIGNATURE_TEST:
        printf("Test\n");
        break;

    case K4A_FIRMWARE_SIGNATURE_UNSIGNED:
        printf("Unsigned\n");
        break;

    default:
        printf("Unknown (%d)\n", signature_type);
    }
}

static k4a_result_t print_firmware_package_info(firmware_package_info_t package_info)
{
    if (!package_info.crc_valid)
    {
        printf("ERROR: CRC check failed\n");
        return K4A_RESULT_FAILED;
    }

    if (!package_info.package_valid)
    {
        printf("ERROR: Firmware package is malformed.\n");
        return K4A_RESULT_FAILED;
    }

    printf("This package contains:\n");

    printf("  RGB camera firmware:      %d.%d.%d\n",
           package_info.rgb.major,
           package_info.rgb.minor,
           package_info.rgb.iteration);

    printf("  Depth camera firmware:    %d.%d.%d\n",
           package_info.depth.major,
           package_info.depth.minor,
           package_info.depth.iteration);

    printf("  Depth config files: ");

    for (size_t i = 0; i < package_info.depth_config_number_versions; i++)
    {
        printf("%d.%d ", package_info.depth_config_versions[i].major, package_info.depth_config_versions[i].minor);
    }
    printf("\n");

    printf("  Audio firmware:           %d.%d.%d\n",
           package_info.audio.major,
           package_info.audio.minor,
           package_info.audio.iteration);

    print_firmware_build_config(package_info.build_config);
    print_firmware_signature_type(package_info.certificate_type, true);
    print_firmware_signature_type(package_info.signature_type, false);
    printf("\n");

    return K4A_RESULT_SUCCEEDED;
}

static k4a_result_t print_device_serialnum(updater_command_info_t *command_info)
{
    k4a_result_t result = ensure_firmware_open(command_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    char *serial_number = NULL;
    size_t serial_number_length = 0;

    if (K4A_BUFFER_RESULT_TOO_SMALL !=
        firmware_get_device_serialnum(command_info->firmware_handle, NULL, &serial_number_length))
    {
        printf("ERROR: Failed to get serial number length\n");
        return K4A_RESULT_FAILED;
    }

    serial_number = malloc(serial_number_length);
    if (serial_number == NULL)
    {
        printf("ERROR: Failed to allocate memory for serial number (%zu bytes)\n", serial_number_length);
        return K4A_RESULT_FAILED;
    }

    if (K4A_BUFFER_RESULT_SUCCEEDED !=
        firmware_get_device_serialnum(command_info->firmware_handle, serial_number, &serial_number_length))
    {
        printf("ERROR: Failed to get serial number\n");
        free(serial_number);
        serial_number = NULL;
        return K4A_RESULT_FAILED;
    }

    printf("Device Serial Number: %s\n", serial_number);
    free(serial_number);
    serial_number = NULL;
    return K4A_RESULT_SUCCEEDED;
}

static firmware_operation_status_t calculate_overall_component_status(const firmware_component_status_t status)
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

static char *component_status_to_string(const firmware_component_status_t status, bool same_version)
{
    if (status.overall == FIRMWARE_OPERATION_SUCCEEDED)
    {
        return "PASS";
    }

    if (status.overall == FIRMWARE_OPERATION_INPROGRESS)
    {
        if (status.authentication_check != FIRMWARE_OPERATION_SUCCEEDED)
        {
            return "IN PROGRESS (Authentication Check)";
        }
        if (status.version_check != FIRMWARE_OPERATION_SUCCEEDED)
        {
            return "IN PROGRESS (Version Check)";
        }
        if (status.image_transfer != FIRMWARE_OPERATION_SUCCEEDED)
        {
            return "IN PROGRESS (Image Transfer)";
        }
        if (status.flash_erase != FIRMWARE_OPERATION_SUCCEEDED)
        {
            return "IN PROGRESS (Flash Erase)";
        }
        if (status.flash_write != FIRMWARE_OPERATION_SUCCEEDED)
        {
            return "IN PROGRESS (Flash Write)";
        }

        return "IN PROGRESS (Unknown)";
    }

    // If the version check failed, this component's update was skipped. This could because the new version is
    // an unsafe downgrade or the versions are the same and no update is required.
    if (same_version && status.version_check == FIRMWARE_OPERATION_FAILED)
    {
        return "SKIPPPED";
    }

    if (status.authentication_check != FIRMWARE_OPERATION_SUCCEEDED)
    {
        return "FAILED (Authentication Check)";
    }
    if (status.version_check != FIRMWARE_OPERATION_SUCCEEDED)
    {
        return "FAILED (Version Check)";
    }
    if (status.image_transfer != FIRMWARE_OPERATION_SUCCEEDED)
    {
        return "FAILED (Image Transfer)";
    }
    if (status.flash_erase != FIRMWARE_OPERATION_SUCCEEDED)
    {
        return "FAILED (Flash Erase)";
    }
    if (status.flash_write != FIRMWARE_OPERATION_SUCCEEDED)
    {
        return "FAILED (Flash Write)";
    }

    return "FAILED (Unknown)";
}

static bool compare_version(k4a_version_t left_version, k4a_version_t right_version)
{
    return left_version.major == right_version.major && left_version.minor == right_version.minor &&
           left_version.iteration == right_version.iteration;
}

static bool compare_version_list(k4a_version_t device_version, uint8_t count, k4a_version_t versions[5])
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

static k4a_result_t wait_update_operation_complete(firmware_t firmware_handle, firmware_status_summary_t *finalStatus)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, finalStatus == NULL);
    LOG_INFO("Waiting for the update operation to complete...", 0);

    bool allComplete = false;
    k4a_result_t result = K4A_RESULT_FAILED;

    tickcounter_ms_t start_time_ms, now;

    TICK_COUNTER_HANDLE tick = tickcounter_create();
    if (tick == NULL)
    {
        printf("ERROR: Failed to create a counter.\n");
        return K4A_RESULT_FAILED;
    }

    if (0 != tickcounter_get_current_ms(tick, &start_time_ms))
    {
        printf("ERROR: Failed to get the current time.\n");
        return K4A_RESULT_FAILED;
    }

    do
    {
        result = firmware_get_download_status(firmware_handle, finalStatus);
        if (result == K4A_RESULT_SUCCEEDED)
        {
            allComplete = ((finalStatus->audio.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                           (finalStatus->depth_config.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                           (finalStatus->depth.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                           (finalStatus->rgb.overall > FIRMWARE_OPERATION_INPROGRESS));
        }
        else
        {
            printf("ERROR: Failed to get the firmware update status.\n");
            return K4A_RESULT_FAILED;
        }

        if (0 != tickcounter_get_current_ms(tick, &now))
        {
            printf("ERROR: Failed to get the current time.\n");
            return K4A_RESULT_FAILED;
        }

        if (!allComplete && (now - start_time_ms > UPDATE_TIMEOUT_MS))
        {
            printf("ERROR: Timeout waiting for the update to complete.\n");
            return K4A_RESULT_FAILED;
        }

        if (!allComplete)
        {
            ThreadAPI_Sleep(500);
        }
    } while (!allComplete);

    LOG_INFO("Firmware update operation has completed.", 0);

    return K4A_RESULT_SUCCEEDED;
}

static k4a_result_t ensure_firmware_open(updater_command_info_t *command_info)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    bool serial_mismatch = false;
    int retry = 0;
    uint32_t device_count = 0;

    if (command_info->device_serial_number == NULL && command_info->firmware_handle == NULL)
    {
        // Never connected to the device, connect and save the serial number for future connections
        LOG_INFO("Connecting to device based on index to get Serial Number...", 0);

        usb_cmd_get_device_count(&device_count);
        if (device_count == 0)
        {
            printf("ERROR: No connected Azure Kinect devices found\n");
            return K4A_RESULT_FAILED;
        }

        result = firmware_create(command_info->device_index, &command_info->firmware_handle);
        if (!K4A_SUCCEEDED(result))
        {
            printf("ERROR: Failed to connect to an Azure Kinect device\n");
            return result;
        }

        char *serial_number = NULL;
        size_t serial_number_length = 0;

        if (K4A_BUFFER_RESULT_TOO_SMALL !=
            firmware_get_device_serialnum(command_info->firmware_handle, NULL, &serial_number_length))
        {
            printf("ERROR: Failed to get serial number length\n");
            return K4A_RESULT_FAILED;
        }

        serial_number = malloc(serial_number_length);
        if (serial_number == NULL)
        {
            printf("ERROR: Failed to allocate memory for serial number (%zu bytes)\n", serial_number_length);
            return K4A_RESULT_FAILED;
        }

        if (K4A_BUFFER_RESULT_SUCCEEDED !=
            firmware_get_device_serialnum(command_info->firmware_handle, serial_number, &serial_number_length))
        {
            printf("ERROR: Failed to get serial number\n");
            free(serial_number);
            return K4A_RESULT_FAILED;
        }

        command_info->device_serial_number = serial_number;
    }

    if (command_info->firmware_handle == NULL)
    {
        LOG_INFO("Connecting to device...", 0);

        // Wait until the device is available...
        do
        {
            usb_cmd_get_device_count(&device_count);
            if (device_count > command_info->device_index)
            {
                result = firmware_create(command_info->device_index, &command_info->firmware_handle);
                if (!K4A_SUCCEEDED(result))
                {
                    LOG_INFO("Failed to connect to the Azure Kinect device", 0);
                    result = K4A_RESULT_FAILED;
                    ThreadAPI_Sleep(500);
                    continue;
                }

                char *serial_number = NULL;
                size_t serial_number_length = 0;

                if (K4A_BUFFER_RESULT_TOO_SMALL !=
                    firmware_get_device_serialnum(command_info->firmware_handle, NULL, &serial_number_length))
                {
                    printf("ERROR: Failed to get serial number length\n");
                    return K4A_RESULT_FAILED;
                }

                serial_number = malloc(serial_number_length);
                if (serial_number == NULL)
                {
                    printf("ERROR: Failed to allocate memory for serial number (%zu bytes)\n", serial_number_length);
                    return K4A_RESULT_FAILED;
                }

                if (K4A_BUFFER_RESULT_SUCCEEDED !=
                    firmware_get_device_serialnum(command_info->firmware_handle, serial_number, &serial_number_length))
                {
                    printf("ERROR: Failed to get serial number\n");
                    free(serial_number);
                    return K4A_RESULT_FAILED;
                }

                if (strcmp(command_info->device_serial_number, serial_number) != 0)
                {
                    serial_mismatch = true;
                    result = K4A_RESULT_FAILED;
                }

                free(serial_number);
            }
            else
            {
                result = K4A_RESULT_FAILED;
            }

            if (!K4A_SUCCEEDED(result))
            {
                if (command_info->firmware_handle != NULL)
                {
                    firmware_destroy(command_info->firmware_handle);
                    command_info->firmware_handle = NULL;
                }

                ThreadAPI_Sleep(500);
            }
        } while (!K4A_SUCCEEDED(result) && retry++ < 20);

        if (!K4A_SUCCEEDED(result))
        {
            if (serial_mismatch)
            {
                printf("\nERROR: Product Serial Number does not match\n");
            }

            return result;
        }
    }

    LOG_INFO("Finished attempting to connect to device. Result: %d Retries: %d", result, retry);

    return result;
}

static void close_all_handles(updater_command_info_t *command_info)
{
    if (command_info->firmware_buffer != NULL)
    {
        free(command_info->firmware_buffer);
        command_info->firmware_buffer = NULL;
        command_info->firmware_size = 0;
    }

    if (command_info->firmware_handle)
    {
        firmware_destroy(command_info->firmware_handle);
        command_info->firmware_handle = NULL;
    }
}

static void command_list_devices()
{
    firmware_t device = NULL;
    uint32_t device_count;

    usb_cmd_get_device_count(&device_count);
    printf("Found %d connected devices:\n", device_count);

    for (uint32_t deviceIndex = 0; deviceIndex < device_count; deviceIndex++)
    {
        if (K4A_RESULT_SUCCEEDED != firmware_create(deviceIndex, &device))
        {
            printf("ERROR: %d: Failed to open device\n", deviceIndex);
        }

        char *serial_number = NULL;
        size_t serial_number_length = 0;

        if (K4A_BUFFER_RESULT_TOO_SMALL != firmware_get_device_serialnum(device, NULL, &serial_number_length))
        {
            printf("ERROR: %d: Failed to get serial number length\n", deviceIndex);
            firmware_destroy(device);
            device = NULL;
            continue;
        }

        serial_number = malloc(serial_number_length);
        if (serial_number == NULL)
        {
            printf("ERROR: %d: Failed to allocate memory for serial number (%zu bytes)\n",
                   deviceIndex,
                   serial_number_length);
            firmware_destroy(device);
            device = NULL;
            continue;
        }

        if (K4A_BUFFER_RESULT_SUCCEEDED != firmware_get_device_serialnum(device, serial_number, &serial_number_length))
        {
            printf("ERROR: %d: Failed to get serial number\n", deviceIndex);
            free(serial_number);
            serial_number = NULL;
            firmware_destroy(device);
            device = NULL;
            continue;
        }

        printf("%d: Device \"%s\"\n", deviceIndex, serial_number);

        free(serial_number);
        serial_number = NULL;
        firmware_destroy(device);
        device = NULL;
    }
}

static k4a_result_t command_query_device(updater_command_info_t *command_info)
{
    k4a_result_t result = ensure_firmware_open(command_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    result = print_device_serialnum(command_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    result = firmware_get_device_version(command_info->firmware_handle, &command_info->current_version);
    if (K4A_SUCCEEDED(result))
    {
        printf("Current Firmware Versions:\n");
        printf("  RGB camera firmware:      %d.%d.%d\n",
               command_info->current_version.rgb.major,
               command_info->current_version.rgb.minor,
               command_info->current_version.rgb.iteration);
        printf("  Depth camera firmware:    %d.%d.%d\n",
               command_info->current_version.depth.major,
               command_info->current_version.depth.minor,
               command_info->current_version.depth.iteration);
        printf("  Depth config file:        %d.%d\n",
               command_info->current_version.depth_sensor.major,
               command_info->current_version.depth_sensor.minor);
        printf("  Audio firmware:           %d.%d.%d\n",
               command_info->current_version.audio.major,
               command_info->current_version.audio.minor,
               command_info->current_version.audio.iteration);

        print_firmware_build_config(command_info->current_version.firmware_build);
        print_firmware_signature_type(command_info->current_version.firmware_signature, true);
        printf("\n");
    }
    else
    {
        printf("ERROR: Failed to get current versions\n\n");
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

static k4a_result_t command_inspect_firmware(updater_command_info_t *command_info)
{
    k4a_result_t result;

    // Load the firmware from the file...
    printf("Loading firmware package %s.\n", command_info->firmware_path);

    if (access(command_info->firmware_path, 04) == -1)
    {
        printf("ERROR: Firmware Path is invalid.\n");
        return K4A_RESULT_FAILED;
    }

    result = load_firmware_file(command_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    parse_firmware_package(command_info->firmware_buffer,
                           command_info->firmware_size,
                           &command_info->firmware_package_info);

    result = print_firmware_package_info(command_info->firmware_package_info);

    return result;
}

static k4a_result_t command_update_device(updater_command_info_t *command_info)
{
    firmware_status_summary_t finalStatus;

    k4a_result_t result = ensure_firmware_open(command_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    // Query the current device information...
    result = command_query_device(command_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    // Load and parse the firmware file information...
    result = command_inspect_firmware(command_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    bool audio_current_version_same = compare_version(command_info->current_version.audio,
                                                      command_info->firmware_package_info.audio);
    bool depth_config_current_version_same =
        compare_version_list(command_info->current_version.depth_sensor,
                             command_info->firmware_package_info.depth_config_number_versions,
                             command_info->firmware_package_info.depth_config_versions);
    bool depth_current_version_same = compare_version(command_info->current_version.depth,
                                                      command_info->firmware_package_info.depth);
    bool rgb_current_version_same = compare_version(command_info->current_version.rgb,
                                                    command_info->firmware_package_info.rgb);

    printf(
        "Please wait, updating device firmware. Don't unplug the device. This operation can take a few minutes...\n");

    // Write the loaded firmware to the device...
    result = firmware_download(command_info->firmware_handle,
                               command_info->firmware_buffer,
                               command_info->firmware_size);
    if (!K4A_SUCCEEDED(result))
    {
        printf("ERROR: Downloading the firmware failed! %d\n", result);
        return result;
    }

    // Wait until the update operation is complete and query the device to get status...
    bool update_failed = K4A_FAILED(wait_update_operation_complete(command_info->firmware_handle, &finalStatus));

    // Always reset the device.
    result = command_reset_device(command_info);
    if (K4A_FAILED(result))
    {
        printf("ERROR: The device failed to reset after an update. Please manually power cycle the device.\n");
    }

    bool allSuccess = ((calculate_overall_component_status(finalStatus.audio) == FIRMWARE_OPERATION_SUCCEEDED) &&
                       (calculate_overall_component_status(finalStatus.depth_config) == FIRMWARE_OPERATION_SUCCEEDED) &&
                       (calculate_overall_component_status(finalStatus.depth) == FIRMWARE_OPERATION_SUCCEEDED) &&
                       (calculate_overall_component_status(finalStatus.rgb) == FIRMWARE_OPERATION_SUCCEEDED));

    if (update_failed || !allSuccess)
    {
        printf("\nERROR: The update process failed. One or more stages failed.\n");
        printf("  Audio's last known state:        %s\n",
               component_status_to_string(finalStatus.audio, audio_current_version_same));
        printf("  Depth config's last known state: %s\n",
               component_status_to_string(finalStatus.depth_config, depth_config_current_version_same));
        printf("  Depth's last known state:        %s\n",
               component_status_to_string(finalStatus.depth, depth_current_version_same));
        printf("  RGB's last known state:          %s\n",
               component_status_to_string(finalStatus.rgb, rgb_current_version_same));

        return K4A_RESULT_FAILED;
    }

    // Pull the updated version number:
    result = firmware_get_device_version(command_info->firmware_handle, &command_info->updated_version);
    if (K4A_SUCCEEDED(result))
    {
        bool audio_updated_version_same = compare_version(command_info->updated_version.audio,
                                                          command_info->firmware_package_info.audio);
        bool depth_config_updated_version_same =
            compare_version_list(command_info->updated_version.depth_sensor,
                                 command_info->firmware_package_info.depth_config_number_versions,
                                 command_info->firmware_package_info.depth_config_versions);
        bool depth_updated_version_same = compare_version(command_info->updated_version.depth,
                                                          command_info->firmware_package_info.depth);
        bool rgb_updated_version_same = compare_version(command_info->updated_version.rgb,
                                                        command_info->firmware_package_info.rgb);

        if (audio_current_version_same && audio_updated_version_same && depth_config_current_version_same &&
            depth_config_updated_version_same && depth_current_version_same && depth_updated_version_same &&
            rgb_current_version_same && rgb_updated_version_same)
        {
            printf("SUCCESS: The firmware was already up-to-date.\n");
        }
        else if (audio_updated_version_same && depth_config_updated_version_same && depth_updated_version_same &&
                 rgb_updated_version_same)
        {
            printf("SUCCESS: The firmware has been successfully updated.\n");
        }
        else
        {
            printf("The firmware has been updated to the following firmware Versions:\n");
            printf("  RGB camera firmware:    %d.%d.%d => %d.%d.%d\n",
                   command_info->current_version.rgb.major,
                   command_info->current_version.rgb.minor,
                   command_info->current_version.rgb.iteration,
                   command_info->updated_version.rgb.major,
                   command_info->updated_version.rgb.minor,
                   command_info->updated_version.rgb.iteration);
            printf("  Depth camera firmware:  %d.%d.%d => %d.%d.%d\n",
                   command_info->current_version.depth.major,
                   command_info->current_version.depth.minor,
                   command_info->current_version.depth.iteration,
                   command_info->updated_version.depth.major,
                   command_info->updated_version.depth.minor,
                   command_info->updated_version.depth.iteration);
            printf("  Depth config file:      %d.%d => %d.%d\n",
                   command_info->current_version.depth_sensor.major,
                   command_info->current_version.depth_sensor.minor,
                   command_info->updated_version.depth_sensor.major,
                   command_info->updated_version.depth_sensor.minor);
            printf("  Audio firmware:         %d.%d.%d => %d.%d.%d\n",
                   command_info->current_version.audio.major,
                   command_info->current_version.audio.minor,
                   command_info->current_version.audio.iteration,
                   command_info->updated_version.audio.major,
                   command_info->updated_version.audio.minor,
                   command_info->updated_version.audio.iteration);
        }
    }
    else
    {
        printf("ERROR: Failed to get updated versions\n\n");
    }

    return result;
}

static k4a_result_t command_reset_device(updater_command_info_t *command_info)
{
    k4a_result_t result = ensure_firmware_open(command_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    result = firmware_reset_device(command_info->firmware_handle);
    if (!K4A_SUCCEEDED(result))
    {
        printf("ERROR: Failed to send the reset command.\n");
        return result;
    }

    // We should have just reset, close out all of our connections.
    close_all_handles(command_info);
    // Sleeping for a second to allow the device to reset and the system to properly de-enumerate the device. One second
    // is an arbitrary value that appeared to work on most systems.
    // Ideally this should be a wait until an event where the OS indicates the device has de-enumerated.
    ThreadAPI_Sleep(1000);

    // Re-open the device to ensure it is ready.
    result = ensure_firmware_open(command_info);
    printf("\n\n");

    return result;
}

int main(int argc, char **argv)
{
    updater_command_info_t command_info;
    memset(&command_info, 0, sizeof(updater_command_info_t));
    logger_t logger_handle = NULL;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    // Instantiate the logger as early as possible
    logger_config_t logger_config;
    logger_config_init_default(&logger_config);
    result = logger_create(&logger_config, &logger_handle);
    if (!K4A_SUCCEEDED(result))
    {
        printf("ERROR: Failed to initialize the logger!\n");
    }

    printf(" == Azure Kinect DK Firmware Tool == \n");

    int exit_code = parse_command_line(argc, argv, &command_info);
    if (exit_code != EXIT_OK || command_info.requested_command == K4A_FIRMWARE_COMMAND_USAGE)
    {
        // If there were issues parsing, or if the command was just to output help return now.
        return exit_code;
    }

    result = K4A_RESULT_SUCCEEDED;

    switch (command_info.requested_command)
    {
    case K4A_FIRMWARE_COMMAND_USAGE:
        break;

    case K4A_FIRMWARE_COMMAND_LIST_DEVICES:
        command_list_devices();
        break;

    case K4A_FIRMWARE_COMMAND_QUERY_DEVICE:
        result = command_query_device(&command_info);
        break;

    case K4A_FIRMWARE_COMMAND_UPDATE_DEVICE:
        result = command_update_device(&command_info);
        break;

    case K4A_FIRMWARE_COMMAND_RESET_DEVICE:
        result = command_reset_device(&command_info);
        break;

    case K4A_FIRMWARE_COMMAND_INSPECT_FIRMWARE:
        result = command_inspect_firmware(&command_info);
        break;

    default:
        break;
    }

    close_all_handles(&command_info);

    if (command_info.device_serial_number)
    {
        free(command_info.device_serial_number);
        command_info.device_serial_number = NULL;
    }

    if (logger_handle)
    {
        logger_destroy(logger_handle);
    }

    if (!K4A_SUCCEEDED(result))
    {
        return EXIT_FAILED;
    }

    return EXIT_OK;
}
