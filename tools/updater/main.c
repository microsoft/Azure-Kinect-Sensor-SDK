// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifdef _WIN32
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <k4ainternal/firmware.h>
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

#define FW_OPEN_RESET (1)        // When firmware create is called, it will be done to enable reset.
#define FW_OPEN_FULL_FEATURE (0) // When firmware_create is called, it will ensure all resources are available.

char K4A_ENV_VAR_LOG_TO_A_FILE[] = K4A_ENABLE_LOG_TO_A_FILE;

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

    char *device_serial_number[10];
    uint32_t device_count;

    firmware_t firmware_handle;
    char *firmware_serial_number;

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
    printf("        Arguments: [Optional Serial Number for single device query]\n");
    printf("    Update Device: -Update, -u\n");
    printf("        Arguments: <Firmware Package Path and FileName> [Option Serial Number for single device update]\n");
    printf("    Reset Device: -Reset, -r\n");
    printf("        Arguments: [Optional Serial Number for single device reset]\n");
    printf("    Inspect Firmware: -Inspect, -i\n");
    printf("        Arguments: <Firmware Package Path and FileName>\n");
    printf("\n");
    printf("    If no Serial Number is provided, the tool will just connect to the first device.\n");
    printf("\n");
    printf("Examples:\n");
    printf("    %s -List\n", EXECUTABLE_NAME);
    printf("    %s -Update c:\\data\\firmware.bin 0123456\n", EXECUTABLE_NAME);
}

static k4a_result_t ensure_firmware_open(updater_command_info_t *command_info, bool resetting_device, uint32_t device);
static void command_list_devices(updater_command_info_t *command_info);
static k4a_result_t command_query_device(updater_command_info_t *command_info);
static k4a_result_t command_load_and_inspect_firmware(char *firmware_path, firmware_package_info_t *firmware_info);
static k4a_result_t command_update_device(updater_command_info_t *command_info);
static k4a_result_t command_reset_device(updater_command_info_t *command_info);
static void close_all_handles(updater_command_info_t *command_info, firmware_package_info_t *firmware_info);

static k4a_result_t add_device(updater_command_info_t *command_info, char *serial_number)
{
    if (command_info->device_count >= COUNTOF(command_info->device_serial_number))
    {
        printf("ERROR: too many devices connected\n");
        return K4A_RESULT_FAILED;
    }

    for (uint32_t index = 0; index < command_info->device_count; index++)
    {
        if (strcmp(serial_number, command_info->device_serial_number[index]) == 0)
        {
            // device present in list;
            return K4A_RESULT_FAILED;
        }
    }

    // Save the serial number.
    command_info->device_serial_number[command_info->device_count] = serial_number;
    command_info->device_count++;
    return K4A_RESULT_SUCCEEDED;
}

static int try_parse_device(int argc, char **argv, int current, updater_command_info_t *command_info)
{
    bool device_found = false;
    depthmcu_t depth = NULL;
    colormcu_t color = NULL;
    uint32_t device_count;
    char *device_serial_number = NULL;
    int next = current + 1;

    if (next < argc)
    {
        char firstCharacter = argv[next][0];
        if (firstCharacter != '-' && firstCharacter != '/')
        {
            // If the user is passing in a serial number, then we use it and exit. We will later check for it
            device_serial_number = argv[next];
        }
    }
    usb_cmd_get_device_count(&device_count);

    // The purpose of this loop to walk through all depth devices first and then walk through all color devices.
    // add_device() ensures there are no duplicate entries.
    for (uint32_t color_loop = 0; color_loop < 2 && !device_found; color_loop++)
    {
        for (uint32_t device_index = 0; device_index < device_count && !device_found; device_index++)
        {
            char *serial_number = NULL;
            k4a_result_t result = K4A_RESULT_FAILED;

            if (color_loop)
            {
                result = colormcu_create_by_index(device_index, &color);
            }
            else
            {
                result = depthmcu_create(device_index, &depth);
            }

            if (K4A_SUCCEEDED(result))
            {
                // If successful, this function owns the serial_number memory
                result = firmware_get_serial_number(color, depth, &serial_number);
            }

            if (K4A_SUCCEEDED(result))
            {
                // Are we looking for a particular serial number?
                if (device_serial_number)
                {
                    if (strcmp(device_serial_number, serial_number) == 0)
                    {
                        if (K4A_SUCCEEDED(add_device(command_info, serial_number)))
                        {
                            // Add_device took ownership of serial_number
                            serial_number = NULL;
                        }
                        else
                        {
                            printf("ERROR: Unable to store device & serial number: %s\n", device_serial_number);
                        }

                        // Success or failure, the serial number being searched for was found.
                        device_found = true;
                    }
                }
                // We are saving all unique devices found to the list
                else if (K4A_SUCCEEDED(add_device(command_info, serial_number)))
                {
                    // Add_device took ownership of serial_number
                    serial_number = NULL;
                }
            }

            if (serial_number)
            {
                free(serial_number);
            }
            if (color)
            {
                colormcu_destroy(color);
            }
            if (depth)
            {
                depthmcu_destroy(depth);
            }
            color = NULL;
            depth = NULL;
            serial_number = NULL;
        }
    }

    if (device_serial_number && command_info->device_count == 0)
    {
        printf("ERROR: Unable to find a device with serial number: %s\n", device_serial_number);
    }

    if (command_info->device_count == 0)
    {
        printf("ERROR: Unable to find a device\n");
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
            i = try_parse_device(argc, argv, i, command_info);
        }
        else if ((0 == _stricmp("-Query", argv[i])) || (0 == _stricmp("-q", argv[i])) || (0 == _stricmp("/q", argv[i])))
        {
            command_info->requested_command = K4A_FIRMWARE_COMMAND_QUERY_DEVICE;
            i = try_parse_device(argc, argv, i, command_info);
            if (command_info->device_count == 0)
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
            if (command_info->device_count == 0)
            {
                return EXIT_USAGE;
            }
        }
        else if ((0 == _stricmp("-Reset", argv[i])) || (0 == _stricmp("-r", argv[i])) || (0 == _stricmp("/r", argv[i])))
        {
            command_info->requested_command = K4A_FIRMWARE_COMMAND_RESET_DEVICE;
            i = try_parse_device(argc, argv, i, command_info);
            if (command_info->device_count == 0)
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

static k4a_result_t load_firmware_file(firmware_package_info_t *firmware_info)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware_info == NULL);

    k4a_result_t result = K4A_RESULT_FAILED;
    FILE *pFirmwareFile = NULL;
    size_t numRead = 0;
    uint8_t *tempFirmwareBuffer = NULL;

    if (firmware_info->path == NULL)
    {
        printf("ERROR: The firmware path was not specified.\n");
        return K4A_RESULT_FAILED;
    }

    if ((pFirmwareFile = fopen(firmware_info->path, "rb")) != NULL)
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
            firmware_info->buffer = tempFirmwareBuffer;
            firmware_info->size = tempFirmwareSize;
            tempFirmwareBuffer = NULL;
            result = K4A_RESULT_SUCCEEDED;
        }
    }
    else
    {
        printf("ERROR: Cannot Open (%s) errno=%d\n", firmware_info->path, errno);
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

static k4a_result_t print_firmware_package_info(firmware_package_info_t *firmware_info)
{
    if (!firmware_info->crc_valid)
    {
        printf("ERROR: CRC check failed\n");
        return K4A_RESULT_FAILED;
    }

    if (!firmware_info->package_valid)
    {
        printf("ERROR: Firmware package is malformed.\n");
        return K4A_RESULT_FAILED;
    }

    printf("This package contains:\n");

    printf("  RGB camera firmware:      %d.%d.%d\n",
           firmware_info->rgb.major,
           firmware_info->rgb.minor,
           firmware_info->rgb.iteration);

    printf("  Depth camera firmware:    %d.%d.%d\n",
           firmware_info->depth.major,
           firmware_info->depth.minor,
           firmware_info->depth.iteration);

    printf("  Depth config files: ");

    for (size_t i = 0; i < firmware_info->depth_config_number_versions; i++)
    {
        printf("%d.%d ", firmware_info->depth_config_versions[i].major, firmware_info->depth_config_versions[i].minor);
    }
    printf("\n");

    printf("  Audio firmware:           %d.%d.%d\n",
           firmware_info->audio.major,
           firmware_info->audio.minor,
           firmware_info->audio.iteration);

    print_firmware_build_config(firmware_info->build_config);
    print_firmware_signature_type(firmware_info->certificate_type, true);
    print_firmware_signature_type(firmware_info->signature_type, false);
    printf("\n");

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

static k4a_result_t ensure_firmware_open(updater_command_info_t *command_info,
                                         bool resetting_device,
                                         uint32_t device_index)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    int retry = 0;

    // Close the handle if open and the firmware_serial_number doesn't match what we want to open
    if (command_info->firmware_handle != NULL &&
        (command_info->firmware_serial_number == NULL ||
         strcmp(command_info->firmware_serial_number, command_info->device_serial_number[device_index]) != 0))
    {
        close_all_handles(command_info, NULL);
    }

    if (command_info->firmware_handle == NULL)
    {
        LOG_INFO("Connecting to device...", 0);

        // Wait until the device is available...
        do
        {
            result = firmware_create(command_info->device_serial_number[device_index],
                                     resetting_device,
                                     &command_info->firmware_handle);
            if (!K4A_SUCCEEDED(result))
            {
                LOG_INFO("Failed to connect to the Azure Kinect device", 0);
                result = K4A_RESULT_FAILED;
                ThreadAPI_Sleep(500);
                continue;
            }

            // Store a copy of the serial number - so we later know what firmware_handle points to.
            command_info->firmware_serial_number = command_info->device_serial_number[device_index];
        } while (!K4A_SUCCEEDED(result) && retry++ < 20);

        LOG_INFO("Finished attempting to connect to device. Result: %d Retries: %d", result, retry);
    }

    return result;
}

static void close_all_handles(updater_command_info_t *command_info, firmware_package_info_t *firmware_info)
{
    if (firmware_info && firmware_info->buffer != NULL)
    {
        free(firmware_info->buffer);
        firmware_info->buffer = NULL;
        firmware_info->size = 0;
    }

    if (command_info && command_info->firmware_handle)
    {
        firmware_destroy(command_info->firmware_handle);
        command_info->firmware_handle = NULL;
        command_info->firmware_serial_number = NULL;
    }
}

static void command_list_devices(updater_command_info_t *command_info)
{
    printf("Found %d connected devices:\n", command_info->device_count);

    for (uint32_t deviceIndex = 0; deviceIndex < command_info->device_count; deviceIndex++)
    {
        printf("%d: Device \"%s\"\n", deviceIndex + 1, command_info->device_serial_number[deviceIndex]);
    }
}

static k4a_result_t command_query_device(updater_command_info_t *command_info)
{
    for (uint32_t device = 0; device < command_info->device_count; device++)
    {
        k4a_result_t result = ensure_firmware_open(command_info, FW_OPEN_FULL_FEATURE, device);
        if (!K4A_SUCCEEDED(result))
        {
            return result;
        }

        printf("Device Serial Number: %s\n", command_info->firmware_serial_number);

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
    }

    return K4A_RESULT_SUCCEEDED;
}

static k4a_result_t command_load_and_inspect_firmware(char *firmware_path, firmware_package_info_t *firmware_info)
{
    k4a_result_t result;

    // Load the firmware from the file...
    printf("Loading firmware package %s.\n", firmware_path);
    firmware_info->path = firmware_path;

    if (access(firmware_path, 04) == -1)
    {
        printf("ERROR: Firmware Path is invalid.\n");
        return K4A_RESULT_FAILED;
    }

    result = load_firmware_file(firmware_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    parse_firmware_package(firmware_info);

    result = print_firmware_package_info(firmware_info);

    return result;
}

static k4a_result_t command_update_device(updater_command_info_t *command_info)
{
    firmware_status_summary_t finalFwCommandStatus;
    k4a_result_t finalCmdStatus = K4A_RESULT_SUCCEEDED;

    // Load and parse the firmware file information...
    firmware_package_info_t firmware_info;
    k4a_result_t result = command_load_and_inspect_firmware(command_info->firmware_path, &firmware_info);
    if (!K4A_SUCCEEDED(result))
    {
        return result;
    }

    for (uint32_t device_index = 0; device_index < command_info->device_count; device_index++)
    {
        result = ensure_firmware_open(command_info, FW_OPEN_FULL_FEATURE, device_index);
        if (!K4A_SUCCEEDED(result))
        {
            return result;
        }

        // Make a copy of the command so that only 1 device is updated at a time.
        updater_command_info_t sub_command_info = { 0 };
        sub_command_info.device_count = 1;
        sub_command_info.device_serial_number[0] = command_info->device_serial_number[device_index];
        sub_command_info.firmware_handle = command_info->firmware_handle;
        sub_command_info.firmware_serial_number = command_info->firmware_serial_number;

        // Passed the ownership of the handle to sub_command_info
        command_info->firmware_handle = NULL;
        command_info->firmware_serial_number = NULL;

        // Query the current device information...
        result = command_query_device(&sub_command_info);
        if (!K4A_SUCCEEDED(result))
        {
            return result;
        }

        bool audio_current_version_same = compare_version(sub_command_info.current_version.audio, firmware_info.audio);
        bool depth_config_current_version_same = compare_version_list(sub_command_info.current_version.depth_sensor,
                                                                      firmware_info.depth_config_number_versions,
                                                                      firmware_info.depth_config_versions);
        bool depth_current_version_same = compare_version(sub_command_info.current_version.depth, firmware_info.depth);
        bool rgb_current_version_same = compare_version(sub_command_info.current_version.rgb, firmware_info.rgb);

        printf("Please wait, updating device firmware. Don't unplug the device. This operation can take a few "
               "minutes...\n");

        // Write the loaded firmware to the device...
        result = firmware_download(sub_command_info.firmware_handle, firmware_info.buffer, firmware_info.size);
        if (!K4A_SUCCEEDED(result))
        {
            printf("ERROR: Downloading the firmware failed! %d\n", result);
            return result;
        }

        // Wait until the update operation is complete and query the device to get status...
        bool update_failed = K4A_FAILED(
            wait_update_operation_complete(sub_command_info.firmware_handle, &finalFwCommandStatus));

        // Always reset the device.
        result = command_reset_device(&sub_command_info);
        if (K4A_FAILED(result))
        {
            printf("ERROR: The device failed to reset after an update. Please manually power cycle the device.\n");
        }

        bool allSuccess =
            ((calculate_overall_component_status(finalFwCommandStatus.audio) == FIRMWARE_OPERATION_SUCCEEDED) &&
             (calculate_overall_component_status(finalFwCommandStatus.depth_config) == FIRMWARE_OPERATION_SUCCEEDED) &&
             (calculate_overall_component_status(finalFwCommandStatus.depth) == FIRMWARE_OPERATION_SUCCEEDED) &&
             (calculate_overall_component_status(finalFwCommandStatus.rgb) == FIRMWARE_OPERATION_SUCCEEDED));

        if (update_failed || !allSuccess)
        {
            printf("\nERROR: The update process failed. One or more stages failed.\n");
            printf("  Audio's last known state:        %s\n",
                   component_status_to_string(finalFwCommandStatus.audio, audio_current_version_same));
            printf("  Depth config's last known state: %s\n",
                   component_status_to_string(finalFwCommandStatus.depth_config, depth_config_current_version_same));
            printf("  Depth's last known state:        %s\n",
                   component_status_to_string(finalFwCommandStatus.depth, depth_current_version_same));
            printf("  RGB's last known state:          %s\n",
                   component_status_to_string(finalFwCommandStatus.rgb, rgb_current_version_same));

            return K4A_RESULT_FAILED;
        }

        // Pull the updated version number:
        result = firmware_get_device_version(sub_command_info.firmware_handle, &sub_command_info.updated_version);
        if (K4A_SUCCEEDED(result))
        {
            bool audio_updated_version_same = compare_version(sub_command_info.updated_version.audio,
                                                              firmware_info.audio);
            bool depth_config_updated_version_same = compare_version_list(sub_command_info.updated_version.depth_sensor,
                                                                          firmware_info.depth_config_number_versions,
                                                                          firmware_info.depth_config_versions);
            bool depth_updated_version_same = compare_version(sub_command_info.updated_version.depth,
                                                              firmware_info.depth);
            bool rgb_updated_version_same = compare_version(sub_command_info.updated_version.rgb, firmware_info.rgb);

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
                       sub_command_info.current_version.rgb.major,
                       sub_command_info.current_version.rgb.minor,
                       sub_command_info.current_version.rgb.iteration,
                       sub_command_info.updated_version.rgb.major,
                       sub_command_info.updated_version.rgb.minor,
                       sub_command_info.updated_version.rgb.iteration);
                printf("  Depth camera firmware:  %d.%d.%d => %d.%d.%d\n",
                       sub_command_info.current_version.depth.major,
                       sub_command_info.current_version.depth.minor,
                       sub_command_info.current_version.depth.iteration,
                       sub_command_info.updated_version.depth.major,
                       sub_command_info.updated_version.depth.minor,
                       sub_command_info.updated_version.depth.iteration);
                printf("  Depth config file:      %d.%d => %d.%d\n",
                       sub_command_info.current_version.depth_sensor.major,
                       sub_command_info.current_version.depth_sensor.minor,
                       sub_command_info.updated_version.depth_sensor.major,
                       sub_command_info.updated_version.depth_sensor.minor);
                printf("  Audio firmware:         %d.%d.%d => %d.%d.%d\n",
                       sub_command_info.current_version.audio.major,
                       sub_command_info.current_version.audio.minor,
                       sub_command_info.current_version.audio.iteration,
                       sub_command_info.updated_version.audio.major,
                       sub_command_info.updated_version.audio.minor,
                       sub_command_info.updated_version.audio.iteration);
            }
        }
        else
        {
            printf("ERROR: Failed to get updated versions\n\n");
        }

        // Ensure the final handle state is copied back.
        command_info->firmware_handle = sub_command_info.firmware_handle;
        command_info->firmware_serial_number = sub_command_info.firmware_serial_number;

        printf("\n\n");

        if (K4A_FAILED(result))
        {
            // keep track of overal status
            finalCmdStatus = K4A_RESULT_FAILED;
        }
    }

    close_all_handles(command_info, &firmware_info);

    return finalCmdStatus;
}

static k4a_result_t command_reset_device(updater_command_info_t *command_info)
{
    k4a_result_t finalCmdStatus = K4A_RESULT_SUCCEEDED;
    k4a_result_t result;

    for (uint32_t device_index = 0; device_index < command_info->device_count; device_index++)
    {
        printf("\nResetting Azure Kinect S/N: %s\n", command_info->device_serial_number[device_index]);
        result = ensure_firmware_open(command_info, FW_OPEN_RESET, device_index);
        if (!K4A_SUCCEEDED(result))
        {
            printf("\nERROR: Failed to open device S/N: %s\n", command_info->device_serial_number[device_index]);
            finalCmdStatus = K4A_RESULT_FAILED;
            continue;
        }

        result = firmware_reset_device(command_info->firmware_handle);

        if (K4A_FAILED(result))
        {
            finalCmdStatus = K4A_RESULT_FAILED;
            printf("ERROR: Failed to send the reset command to device S/N: %s.\n",
                   command_info->device_serial_number[device_index]);
            continue;
        }

        // We should have just reset, close out all of our connections.
        close_all_handles(command_info, NULL);
        // Sleeping for a second to allow the device to reset and the system to properly de-enumerate the device.
        // One second is an arbitrary value that appeared to work on most systems. Ideally this should be a wait
        // until an event where the OS indicates the device has de-enumerated.
        ThreadAPI_Sleep(1000);

        // Re-open the device to ensure it is ready.
        printf("Waiting for reset of S/N: %s to complete.\n", command_info->device_serial_number[device_index]);
        result = ensure_firmware_open(command_info, FW_OPEN_FULL_FEATURE, device_index);

        if (K4A_SUCCEEDED(result))
        {
            printf("Reset of S/N: %s completed successfully.\n", command_info->device_serial_number[device_index]);
        }
        else
        {
            printf("Reset of S/N: %s failed. Device did not re-enumerate\n",
                   command_info->device_serial_number[device_index]);
            finalCmdStatus = K4A_RESULT_FAILED;
        }
    }
    printf("\n");

    return finalCmdStatus;
}

int main(int argc, char **argv)
{
    updater_command_info_t command_info;
    memset(&command_info, 0, sizeof(updater_command_info_t));
    firmware_package_info_t firmware_info = { 0 };
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

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
        command_list_devices(&command_info);
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
        result = command_load_and_inspect_firmware(command_info.firmware_path, &firmware_info);
        break;

    default:
        break;
    }

    close_all_handles(&command_info, &firmware_info);

    for (uint32_t device_index = 0; device_index < command_info.device_count; device_index++)
    {
        if (command_info.device_serial_number[device_index])
        {
            firmware_free_serial_number(command_info.device_serial_number[device_index]);
            command_info.device_serial_number[device_index] = NULL;
        }
    }

    if (!K4A_SUCCEEDED(result))
    {
        return EXIT_FAILED;
    }

    return EXIT_OK;
}
