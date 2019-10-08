// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "firmware_helper.h"
#include <utcommon.h>

#include <k4ainternal/logging.h>
#include <k4ainternal/usbcommand.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

#define K4A_TEST_VERIFY_SUCCEEDED(_result_)                                                                            \
    if (K4A_FAILED(TRACE_CALL(_result_)))                                                                              \
    {                                                                                                                  \
        return K4A_RESULT_FAILED;                                                                                      \
    }

#define K4A_TEST_VERIFY_TRUE(_condition_)                                                                              \
    if (!(_condition_))                                                                                                \
    {                                                                                                                  \
        LOG_ERROR("\"" #_condition_ "\" was false but was expected to be true.");                                      \
        return K4A_RESULT_FAILED;                                                                                      \
    }

#define K4A_TEST_VERIFY_LE(_val1_, _val2_)                                                                             \
    if ((_val1_) > (_val2_))                                                                                           \
    {                                                                                                                  \
        LOG_ERROR("\"" #_val1_ "\" > \"" #_val2_ "\" but was expected to be less than or equal.");                     \
        std::cout << "\"" #_val1_ "\" = " << (_val1_) << " \"" #_val2_ "\" = " << (_val2_) << std::endl;               \
        return K4A_RESULT_FAILED;                                                                                      \
    }

#define K4A_TEST_VERIFY_EQUAL(_val1_, _val2_)                                                                          \
    if ((_val1_) != (_val2_))                                                                                          \
    {                                                                                                                  \
        LOG_ERROR("\"" #_val1_ "\" != \"" #_val2_ "\" but was expected to be equal.");                                 \
        std::cout << "\"" #_val1_ "\" = " << (_val1_) << " \"" #_val2_ "\" = " << (_val2_) << std::endl;               \
        return K4A_RESULT_FAILED;                                                                                      \
    }

#define K4A_TEST_VERIFY_NOT_EQUAL(_val1_, _val2_)                                                                      \
    if ((_val1_) == (_val2_))                                                                                          \
    {                                                                                                                  \
        LOG_ERROR("\"" #_val1_ "\" == \"" #_val2_ "\" but was expected to not be equal.");                             \
        std::cout << "\"" #_val1_ "\" = " << (_val1_) << " \"" #_val2_ "\" = " << (_val2_) << std::endl;               \
        return K4A_RESULT_FAILED;                                                                                      \
    }

int g_k4a_port_number = -1;
connection_exerciser *g_connection_exerciser = nullptr;
char *g_serial_number;

char *g_candidate_firmware_path = nullptr;
uint8_t *g_candidate_firmware_buffer = nullptr;
size_t g_candidate_firmware_size = 0;
firmware_package_info_t g_candidate_firmware_package_info = { 0 };

char *g_test_firmware_path = nullptr;
uint8_t *g_test_firmware_buffer = nullptr;
size_t g_test_firmware_size = 0;
firmware_package_info_t g_test_firmware_package_info = { 0 };

char *g_lkg_firmware_path = nullptr;
uint8_t *g_lkg_firmware_buffer = nullptr;
size_t g_lkg_firmware_size = 0;
firmware_package_info_t g_lkg_firmware_package_info = { 0 };

char *g_factory_firmware_path = nullptr;
uint8_t *g_factory_firmware_buffer = nullptr;
size_t g_factory_firmware_size = 0;
firmware_package_info_t g_factory_firmware_package_info = { 0 };

static bool common_initialized = false;

k4a_result_t setup_common_test()
{
    int port;
    float voltage;
    float current;

    if (common_initialized)
    {
        return K4A_RESULT_SUCCEEDED;
    }

    if (g_candidate_firmware_path == nullptr)
    {
        std::cout << "The firmware path setting is required and wasn't supplied.\n\n";
        return K4A_RESULT_FAILED;
    }

    g_k4a_port_number = -1;
    g_connection_exerciser = new (std::nothrow) connection_exerciser();

    std::cout << "Searching for the connection exerciser and device..." << std::endl;
    LOG_INFO("Searching for the connection exerciser...", 0);
    K4A_TEST_VERIFY_SUCCEEDED(g_connection_exerciser->find_connection_exerciser());
    std::cout << "Found it, searching ports\n";
    K4A_TEST_VERIFY_SUCCEEDED(g_connection_exerciser->set_usb_port(0));

    LOG_INFO("Searching for device...", 0);

    for (int i = 0; i < CONN_EX_MAX_NUM_PORTS; ++i)
    {
        std::cout << "Inspecting port " << i << "\n";
        K4A_TEST_VERIFY_SUCCEEDED(g_connection_exerciser->set_usb_port(i));
        port = g_connection_exerciser->get_usb_port();
        K4A_TEST_VERIFY_EQUAL(port, i);

        voltage = g_connection_exerciser->get_voltage_reading();
        K4A_TEST_VERIFY_NOT_EQUAL(voltage, -1);
        current = g_connection_exerciser->get_current_reading();
        K4A_TEST_VERIFY_NOT_EQUAL(current, -1);

        if (current < 0)
        {
            current = current * -1;
        }

        if (voltage > 4.5 && voltage < 5.5 && current > 0.1)
        {
            if (g_k4a_port_number != -1)
            {
                std::cout << "More than one device was detected on the connection exerciser." << std::endl;
                return K4A_RESULT_FAILED;
            }
            std::cout << "Found a device on port " << i << "\n";
            g_k4a_port_number = port;
        }

        LOG_INFO("On port #%d: %4.2fV %4.2fA\n", port, voltage, current);
    }

    if (g_k4a_port_number == -1)
    {
        std::cout << "The Azure Kinect DK was not detected on any port of the connection exerciser." << std::endl;
        return K4A_RESULT_FAILED;
    }

    // Get the serial number of the connected device
    {
        depthmcu_t depthmcu;
        size_t serial_number_size = 0;
        std::cout << "Reading serial number of Azure Kinect\n";
        K4A_TEST_VERIFY_SUCCEEDED(g_connection_exerciser->set_usb_port(g_k4a_port_number));
        ThreadAPI_Sleep(3000);
        K4A_TEST_VERIFY_EQUAL(K4A_RESULT_SUCCEEDED, depthmcu_create(K4A_DEVICE_DEFAULT, &depthmcu));
        K4A_TEST_VERIFY_EQUAL(K4A_BUFFER_RESULT_TOO_SMALL,
                              depthmcu_get_serialnum(depthmcu, nullptr, &serial_number_size));
        K4A_TEST_VERIFY_NOT_EQUAL(NULL, g_serial_number = (char *)malloc(serial_number_size));
        K4A_TEST_VERIFY_EQUAL(K4A_BUFFER_RESULT_SUCCEEDED,
                              depthmcu_get_serialnum(depthmcu, g_serial_number, &serial_number_size));
        depthmcu_destroy(depthmcu);
        std::cout << "Azure Kinect S/N is " << g_serial_number << std::endl;
    }

    K4A_TEST_VERIFY_SUCCEEDED(g_connection_exerciser->set_usb_port(0));

    std::cout << "Loading Release Candidate firmware package: " << g_candidate_firmware_path << std::endl;
    load_firmware_files(g_candidate_firmware_path, &g_candidate_firmware_buffer, &g_candidate_firmware_size);
    g_candidate_firmware_package_info.buffer = g_candidate_firmware_buffer;
    g_candidate_firmware_package_info.size = g_candidate_firmware_size;
    parse_firmware_package(&g_candidate_firmware_package_info);

    std::cout << "Loading Test firmware package: " << g_test_firmware_path << std::endl;
    load_firmware_files(g_test_firmware_path, &g_test_firmware_buffer, &g_test_firmware_size);
    g_test_firmware_package_info.buffer = g_test_firmware_buffer;
    g_test_firmware_package_info.size = g_test_firmware_size;
    parse_firmware_package(&g_test_firmware_package_info);

    std::cout << "Loading LKG firmware package: " << g_lkg_firmware_path << std::endl;
    load_firmware_files(g_lkg_firmware_path, &g_lkg_firmware_buffer, &g_lkg_firmware_size);
    g_lkg_firmware_package_info.buffer = g_lkg_firmware_buffer;
    g_lkg_firmware_package_info.size = g_lkg_firmware_size;
    parse_firmware_package(&g_lkg_firmware_package_info);

    std::cout << "Loading Factory firmware package: " << g_factory_firmware_path << std::endl;
    load_firmware_files(g_factory_firmware_path, &g_factory_firmware_buffer, &g_factory_firmware_size);
    g_factory_firmware_package_info.buffer = g_factory_firmware_buffer;
    g_factory_firmware_package_info.size = g_factory_firmware_size;
    parse_firmware_package(&g_factory_firmware_package_info);

    common_initialized = true;
    return K4A_RESULT_SUCCEEDED;
}

void tear_down_common_test()
{
    if (g_connection_exerciser != nullptr)
    {
        g_k4a_port_number = -1;
        delete g_connection_exerciser;
        g_connection_exerciser = nullptr;
    }

    if (g_test_firmware_buffer != nullptr)
    {
        free(g_test_firmware_buffer);
        g_test_firmware_buffer = nullptr;
    }

    if (g_candidate_firmware_buffer != nullptr)
    {
        free(g_candidate_firmware_buffer);
        g_candidate_firmware_buffer = nullptr;
    }

    if (g_lkg_firmware_buffer != nullptr)
    {
        free(g_lkg_firmware_buffer);
        g_lkg_firmware_buffer = nullptr;
    }

    if (g_factory_firmware_buffer != nullptr)
    {
        free(g_factory_firmware_buffer);
        g_factory_firmware_buffer = nullptr;
    }

    if (g_serial_number != nullptr)
    {
        free(g_serial_number);
        g_serial_number = nullptr;
    }

    common_initialized = false;
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
            LOG_ERROR("Failed to get size of the firmware package.", 0);
            return K4A_RESULT_FAILED;
        }

        fseek(pFirmwareFile, 0, SEEK_SET);

        tempFirmwareBuffer = (uint8_t *)malloc(tempFirmwareSize);
        if (tempFirmwareBuffer == NULL)
        {
            LOG_ERROR("Failed to allocate memory.", 0);
            return K4A_RESULT_FAILED;
        }

        LOG_INFO("Loaded: \"%s\", File size: %d bytes.", firmware_path, tempFirmwareSize);
        numRead = fread(tempFirmwareBuffer, tempFirmwareSize, 1, pFirmwareFile);
        fclose(pFirmwareFile);

        if (numRead != 1)
        {
            LOG_ERROR("Failed to read all of the data from the file.", 0);
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
        LOG_ERROR("Failed to open \"%s\" errno=%d", firmware_path, errno);
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

    printf("Overall Component Status failed: A:%d V:%d T:%d E:%d W:%d O:%d\n",
           status.authentication_check,
           status.version_check,
           status.image_transfer,
           status.flash_erase,
           status.flash_write,
           status.overall);

    return FIRMWARE_OPERATION_FAILED;
}

bool compare_version(k4a_version_t left_version, k4a_version_t right_version)
{
    return left_version.major == right_version.major && left_version.minor == right_version.minor &&
           left_version.iteration == right_version.iteration;
}

bool compare_version(k4a_hardware_version_t device_version, firmware_package_info_t firmware_version)
{
    bool are_equal = true;

    if (!compare_version(device_version.audio, firmware_version.audio))
    {
        printf("Audio version mismatch.\n");
        are_equal = false;
    }

    if (!compare_version_list(device_version.depth_sensor,
                              firmware_version.depth_config_number_versions,
                              firmware_version.depth_config_versions))
    {
        printf("Depth sensor does not exist in package.\n");
        are_equal = false;
    }

    if (!compare_version(device_version.depth, firmware_version.depth))
    {
        printf("Depth version mismatch.\n");
        are_equal = false;
    }

    if (!compare_version(device_version.rgb, firmware_version.rgb))
    {
        printf("RGB version mismatch.\n");
        are_equal = false;
    }

    return are_equal;
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
    printf("Firmware Package Versions:\n");
    printf("  RGB camera firmware:      %d.%d.%d\n",
           firmware_version.rgb.major,
           firmware_version.rgb.minor,
           firmware_version.rgb.iteration);
    printf("  Depth camera firmware:    %d.%d.%d\n",
           firmware_version.depth.major,
           firmware_version.depth.minor,
           firmware_version.depth.iteration);

    printf("  Depth config file:        ");
    for (size_t i = 0; i < firmware_version.depth_config_number_versions; i++)
    {
        printf("%d.%d ",
               firmware_version.depth_config_versions[i].major,
               firmware_version.depth_config_versions[i].minor);
    }
    printf("\n");

    printf("  Audio firmware:           %d.%d.%d\n",
           firmware_version.audio.major,
           firmware_version.audio.minor,
           firmware_version.audio.iteration);

    log_firmware_build_config(firmware_version.build_config);

    log_firmware_signature_type(firmware_version.certificate_type, true);
    log_firmware_signature_type(firmware_version.signature_type, false);
}

void log_device_version(k4a_hardware_version_t firmware_version)
{
    printf("Current Firmware Versions:\n");
    printf("  RGB camera firmware:      %d.%d.%d\n",
           firmware_version.rgb.major,
           firmware_version.rgb.minor,
           firmware_version.rgb.iteration);
    printf("  Depth camera firmware:    %d.%d.%d\n",
           firmware_version.depth.major,
           firmware_version.depth.minor,
           firmware_version.depth.iteration);
    printf("  Depth config file:        %d.%d\n",
           firmware_version.depth_sensor.major,
           firmware_version.depth_sensor.minor);
    printf("  Audio firmware:           %d.%d.%d\n",
           firmware_version.audio.major,
           firmware_version.audio.minor,
           firmware_version.audio.iteration);

    log_firmware_build_config((k4a_firmware_build_t)firmware_version.firmware_build);
    log_firmware_signature_type((k4a_firmware_signature_t)firmware_version.firmware_signature, true);
}

k4a_result_t open_firmware_device(firmware_t *firmware_handle)
{
    int retry = 0;
    uint32_t device_count = 0;

    while (device_count == 0 && retry++ < 20)
    {
        K4A_TEST_VERIFY_SUCCEEDED(usb_cmd_get_device_count(&device_count));

        if (device_count == 0)
        {
            ThreadAPI_Sleep(500);
        }
    }

    K4A_TEST_VERIFY_LE(retry, 20);

    LOG_INFO("Opening firmware device...", 0);
    K4A_TEST_VERIFY_SUCCEEDED(firmware_create(g_serial_number, false, firmware_handle));
    K4A_TEST_VERIFY_TRUE(*firmware_handle != nullptr);

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t reset_device(firmware_t *firmware_handle)
{
    LOG_INFO("Resetting device...", 0);
    K4A_TEST_VERIFY_SUCCEEDED(firmware_reset_device(*firmware_handle));

    firmware_destroy(*firmware_handle);
    *firmware_handle = nullptr;

    ThreadAPI_Sleep(1000);

    // Re-open the device to ensure it is ready for use.
    return TRACE_CALL(open_firmware_device(firmware_handle));
}

k4a_result_t disconnect_device(firmware_t *firmware_handle)
{
    LOG_INFO("Disconnecting device...", 0);

    K4A_TEST_VERIFY_SUCCEEDED(g_connection_exerciser->set_usb_port(0));

    ThreadAPI_Sleep(1000);

    K4A_TEST_VERIFY_SUCCEEDED(g_connection_exerciser->set_usb_port(g_k4a_port_number));

    firmware_destroy(*firmware_handle);
    *firmware_handle = nullptr;

    ThreadAPI_Sleep(1000);

    // Re-open the device to ensure it is ready for use.
    return TRACE_CALL(open_firmware_device(firmware_handle));
}

k4a_result_t interrupt_operation(firmware_t *firmware_handle, firmware_operation_interruption_t interruption)
{
    switch (interruption)
    {
    case FIRMWARE_INTERRUPTION_RESET:
        return TRACE_CALL(reset_device(firmware_handle));
        break;

    case FIRMWARE_INTERRUPTION_DISCONNECT:
        return TRACE_CALL(disconnect_device(firmware_handle));
        break;

    default:
        LOG_ERROR("Unknown interruption type");
        return K4A_RESULT_FAILED;
    }
}

k4a_result_t interrupt_device_at_update_stage(firmware_t *firmware_handle,
                                              firmware_operation_component_t component,
                                              firmware_operation_interruption_t interruption,
                                              firmware_status_summary_t *final_status,
                                              bool verbose_logging)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware_handle == nullptr);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, final_status == nullptr);

    bool all_complete = false;
    k4a_result_t result = K4A_RESULT_FAILED;
    firmware_status_summary_t previous_status = {};

    tickcounter_ms_t start_time_ms, now;

    TICK_COUNTER_HANDLE tick = tickcounter_create();
    K4A_TEST_VERIFY_TRUE(tick != nullptr);

    K4A_TEST_VERIFY_EQUAL(0, tickcounter_get_current_ms(tick, &start_time_ms));

    do
    {
        // This is not necessarily the final status we will get, but at any point could return and the caller needs to
        // know the state of the update when we return.
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
                return TRACE_CALL(interrupt_operation(firmware_handle, interruption));

            case FIRMWARE_OPERATION_AUDIO_ERASE:
                if (final_status->audio.image_transfer == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The erase takes places after the transfer is complete and takes about 7.8 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 7600);
                    sleepTime = 3800;
                    LOG_INFO("Audio Erase started, waiting %dms.", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    TRACE_CALL(firmware_get_download_status(*firmware_handle, final_status));
                    return TRACE_CALL(interrupt_operation(firmware_handle, interruption));
                }
                break;

            case FIRMWARE_OPERATION_AUDIO_WRITE:
                if (final_status->audio.flash_erase == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The write takes places after the erase is complete and takes about 20 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 19700);
                    sleepTime = 9850;
                    LOG_INFO("Audio Write started, waiting %dms.", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    TRACE_CALL(firmware_get_download_status(*firmware_handle, final_status));
                    return TRACE_CALL(interrupt_operation(firmware_handle, interruption));
                }
                break;

            case FIRMWARE_OPERATION_DEPTH_ERASE:
                if (final_status->depth.image_transfer == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The erase takes places after the transfer is complete and takes about 0.25 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 100);
                    sleepTime = 50;
                    LOG_INFO("Depth Erase started, waiting %dms.", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    TRACE_CALL(firmware_get_download_status(*firmware_handle, final_status));
                    return TRACE_CALL(interrupt_operation(firmware_handle, interruption));
                }
                break;

            case FIRMWARE_OPERATION_DEPTH_WRITE:
                if (final_status->depth.flash_erase == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The write takes places after the transfer is complete and takes about 5.8 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 5700);
                    sleepTime = 2850;
                    LOG_INFO("Depth Write started, waiting %dms.", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    TRACE_CALL(firmware_get_download_status(*firmware_handle, final_status));
                    return TRACE_CALL(interrupt_operation(firmware_handle, interruption));
                }
                break;

            case FIRMWARE_OPERATION_RGB_ERASE:
                if (final_status->rgb.image_transfer == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The erase takes places after the transfer is complete and takes about 0.05 seconds.
                    LOG_INFO("RGB erase started...");
                    TRACE_CALL(firmware_get_download_status(*firmware_handle, final_status));
                    return TRACE_CALL(interrupt_operation(firmware_handle, interruption));
                }
                break;

            case FIRMWARE_OPERATION_RGB_WRITE:
                if (final_status->rgb.flash_erase == FIRMWARE_OPERATION_SUCCEEDED)
                {
                    // The write takes places after the transfer is complete and takes about 6.1 seconds.
                    int sleepTime = (int)((rand() / (float)RAND_MAX) * 6000);
                    sleepTime = 3000;
                    LOG_INFO("RGB Write started, waiting %dms.", sleepTime);
                    ThreadAPI_Sleep(sleepTime);
                    TRACE_CALL(firmware_get_download_status(*firmware_handle, final_status));
                    return TRACE_CALL(interrupt_operation(firmware_handle, interruption));
                }
                break;

            default:
                break;
            }
        }
        else
        {
            // Failed to get the status of the update operation. Attempt to reset the device and return.
            LOG_ERROR("Failed to get the firmware update status.");
            TRACE_CALL(interrupt_operation(firmware_handle, interruption));
            return K4A_RESULT_FAILED;
        }

        K4A_TEST_VERIFY_EQUAL(0, tickcounter_get_current_ms(tick, &now));

        if (!all_complete && (now - start_time_ms > UPDATE_TIMEOUT_MS))
        {
            // The update hasn't completed and too much time as passed. Attempt to reset the device and return.
            LOG_ERROR("Timeout waiting for the update to complete.");
            TRACE_CALL(interrupt_operation(firmware_handle, interruption));
            return K4A_RESULT_FAILED;
        }

        if (!all_complete)
        {
            ThreadAPI_Sleep(UPDATE_POLL_INTERVAL_MS);
        }
    } while (!all_complete);

    // At this point the update has completed, the device needs to be reset after the
    // update has progressed.
    return TRACE_CALL(interrupt_operation(firmware_handle, interruption));
}

k4a_result_t perform_device_update(firmware_t *firmware_handle,
                                   uint8_t *firmware_buffer,
                                   size_t firmware_size,
                                   firmware_package_info_t firmware_package_info,
                                   bool verbose_logging)
{
    firmware_status_summary_t finalStatus;
    k4a_hardware_version_t current_version;

    K4A_TEST_VERIFY_SUCCEEDED(firmware_get_device_version(*firmware_handle, &current_version));
    log_device_version(current_version);
    log_firmware_version(firmware_package_info);

    // Perform upgrade...
    K4A_TEST_VERIFY_SUCCEEDED(firmware_download(*firmware_handle, firmware_buffer, firmware_size));
    interrupt_device_at_update_stage(firmware_handle,
                                     FIRMWARE_OPERATION_FULL_DEVICE,
                                     FIRMWARE_INTERRUPTION_RESET,
                                     &finalStatus,
                                     verbose_logging);

    K4A_TEST_VERIFY_EQUAL(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
    K4A_TEST_VERIFY_EQUAL(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
    K4A_TEST_VERIFY_EQUAL(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth));
    K4A_TEST_VERIFY_EQUAL(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.rgb));

    // Check upgrade...
    K4A_TEST_VERIFY_SUCCEEDED(firmware_get_device_version(*firmware_handle, &current_version));
    printf("Post full update ");
    log_device_version(current_version);
    K4A_TEST_VERIFY_TRUE(compare_version(current_version, firmware_package_info));

    return K4A_RESULT_SUCCEEDED;
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

        if (strcmp(argument, "-cf") == 0 || strcmp(argument, "--candidate-firmware") == 0)
        {
            if (i + 1 <= argc)
            {
                g_candidate_firmware_path = argv[++i];
                printf("Setting g_candidate_firmware_path = %s\n", g_candidate_firmware_path);
            }
            else
            {
                printf("Error: candidate firmware path parameter missing\n");
                error = true;
            }
        }

        if (strcmp(argument, "-tf") == 0 || strcmp(argument, "--test-firmware") == 0)
        {
            if (i + 1 <= argc)
            {
                g_test_firmware_path = argv[++i];
                printf("Setting g_test_firmware_path = %s\n", g_test_firmware_path);
            }
            else
            {
                printf("Error: test firmware path parameter missing\n");
                error = true;
            }
        }

        if (strcmp(argument, "-lf") == 0 || strcmp(argument, "--lkg-firmware") == 0)
        {
            if (i + 1 <= argc)
            {
                g_lkg_firmware_path = argv[++i];
                printf("Setting g_lkg_firmware_path = %s\n", g_lkg_firmware_path);
            }
            else
            {
                printf("Error: lkg firmware path parameter missing\n");
                error = true;
            }
        }

        if (strcmp(argument, "-ff") == 0 || strcmp(argument, "--factory-firmware") == 0)
        {
            if (i + 1 <= argc)
            {
                g_factory_firmware_path = argv[++i];
                printf("Setting g_factory_firmware_path = %s\n", g_factory_firmware_path);
            }
            else
            {
                printf("Error: factory firmware path parameter missing\n");
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
        printf("  -cf | --candidate-firmware <firmware path>\n");
        printf("      This is the path to the candidate firmware that should be tested.\n");
        printf("  -tf | --test-firmware <firmware path>\n");
        printf("      This is the path to the test firmware that will be used to test the candidate. It should have a "
               "different version for all components.\n");
        printf("  -lf | --lkg-firmware <firmware path>\n");
        printf("      This is the path to the LKG firmware that will be used as a starting point for updating to the "
               "candidate.\n");
        printf("  -ff | --factory-firmware <firmware path>\n");
        printf("      This is the path to the factory firmware that will be used as a starting point for updating to "
               "the candidate.\n");

        return 1; // Indicates an error or warning
    }

    int result = RUN_ALL_TESTS();

    tear_down_common_test();
    k4a_unittest_deinit();
    return result;
}