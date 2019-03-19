// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define _CRT_NONSTDC_NO_DEPRECATE

#include <utcommon.h>
#include <ConnEx.h>

#include <k4ainternal/firmware.h>
#include <k4ainternal/logging.h>

#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

#define UPDATE_TIMEOUT_MS 10 * 60 * 1000 // 10 Minutes should be way more than enough.
#define K4A_DEPTH_MODE_NFOV_UNBINNED_EXPECTED_SIZE 737280

// This will define the path to the new firmware drop to test and the last known good firmware drop. The firmware update
// process runs from the current firmware. In order to test the firmware update process, the device must be on the
// firmware you want to test and then updated to a different firmware.
#define K4A_TEST_FIRMWARE_PATH "..\\..\\tools\\updater\\firmware\\AzureKinectDK_Fw_1.5.886314.bin"
#define K4A_LKG_FIRMWARE_PATH "..\\..\\tools\\updater\\firmware\\AzureKinectDK_Fw_1.5.786013.bin"

static k4a_result_t load_firmware_files(char *firmware_path, uint8_t **firmware_buffer, size_t *firmware_size);

typedef enum
{
    FIRMWARE_OPERATION_AUDIO,
    FIRMWARE_OPERATION_DEPTH_CONFIG,
    FIRMWARE_OPERATION_DEPTH,
    FIRMWARE_OPERATION_RGB,
    FIRMWARE_OPERATION_FULL_DEVICE,
} firmware_operation_component_t;

typedef enum
{
    FIRMWARE_OPERATION_RESET,
    FIRMWARE_OPERATION_DISCONNECT,
} firmware_operation_interruption_t;

class firmware_fw : public ::testing::Test
{
public:
    static void SetUpTestCase()
    {
        int port;
        float voltage;
        float current;

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

        std::cout << "Loading test firmware package: " << K4A_TEST_FIRMWARE_PATH << std::endl;
        load_firmware_files(K4A_TEST_FIRMWARE_PATH, &test_firmware_buffer, &test_firmware_size);
        parse_firmware_package(test_firmware_buffer, test_firmware_size, &test_firmware_package_info);

        std::cout << "Loading LKG firmware package: " << K4A_LKG_FIRMWARE_PATH << std::endl;
        load_firmware_files(K4A_LKG_FIRMWARE_PATH, &lkg_firmware_buffer, &lkg_firmware_size);
        parse_firmware_package(lkg_firmware_buffer, lkg_firmware_size, &lkg_firmware_package_info);
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

        if (lkg_firmware_buffer != nullptr)
        {
            free(lkg_firmware_buffer);
            lkg_firmware_buffer = nullptr;
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

    void open_k4a_device();
    void open_firmware_device();
    void interrupt_operation(firmware_operation_interruption_t interruption);
    void reset_device();
    void interrupt_device_at_update_stage(firmware_operation_component_t component,
                                          firmware_operation_interruption_t interruption,
                                          firmware_status_summary_t *finalStatus);

    static int k4a_port_number;
    static connection_exerciser *connEx;

    static uint8_t *test_firmware_buffer;
    static size_t test_firmware_size;
    static firmware_package_info_t test_firmware_package_info;

    static uint8_t *lkg_firmware_buffer;
    static size_t lkg_firmware_size;
    static firmware_package_info_t lkg_firmware_package_info;

    firmware_t firmware_handle = nullptr;

    k4a_hardware_version_t current_version = { 0 };
};

int firmware_fw::k4a_port_number = -1;
connection_exerciser *firmware_fw::connEx = nullptr;

uint8_t *firmware_fw::test_firmware_buffer = nullptr;
size_t firmware_fw::test_firmware_size = 0;
firmware_package_info_t firmware_fw::test_firmware_package_info = { 0 };

uint8_t *firmware_fw::lkg_firmware_buffer = nullptr;
size_t firmware_fw::lkg_firmware_size = 0;
firmware_package_info_t firmware_fw::lkg_firmware_package_info = { 0 };

static k4a_result_t load_firmware_files(char *firmware_path, uint8_t **firmware_buffer, size_t *firmware_size)
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

static void log_firmware_build_config(k4a_firmware_build_t build_config)
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

static void log_firmware_signature_type(k4a_firmware_signature_t signature_type, bool certificate)
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

static void log_firmware_version(firmware_package_info_t firmware_version)
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

static void log_device_version(k4a_hardware_version_t firmware_version)
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
                                                   firmware_status_summary_t *finalStatus)
{
    ASSERT_NE(nullptr, finalStatus);

    bool all_complete = false;
    k4a_result_t result = K4A_RESULT_FAILED;

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
            all_complete = ((finalStatus->audio.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                            (finalStatus->depth_config.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                            (finalStatus->depth.overall > FIRMWARE_OPERATION_INPROGRESS) &&
                            (finalStatus->rgb.overall > FIRMWARE_OPERATION_INPROGRESS));

            // Check to see if now is the correct time to interrupt the device.
            switch (component)
            {
            case FIRMWARE_OPERATION_AUDIO:
                if (finalStatus->audio.version_check != FIRMWARE_OPERATION_INPROGRESS)
                {
                    interrupt_operation(interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_DEPTH_CONFIG:
                if (finalStatus->depth_config.version_check != FIRMWARE_OPERATION_INPROGRESS)
                {
                    interrupt_operation(interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_DEPTH:
                if (finalStatus->depth.version_check != FIRMWARE_OPERATION_INPROGRESS)
                {
                    interrupt_operation(interruption);
                    return;
                }
                break;

            case FIRMWARE_OPERATION_RGB:
                if (finalStatus->rgb.version_check != FIRMWARE_OPERATION_INPROGRESS)
                {
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
            ThreadAPI_Sleep(500);
        }
    } while (!all_complete);

    // At this point the update has either completed or timed out. Either way the device needs to be reset after the
    // update has progressed.
    interrupt_operation(FIRMWARE_OPERATION_RESET);
}

TEST_F(firmware_fw, simple_update)
{
    firmware_status_summary_t finalStatus;

    LOG_INFO("Beginning the basic update test", 0);
    connEx->set_usb_port(k4a_port_number);

    open_firmware_device();

    // Update to the test firmware
    LOG_INFO("Updating the device to the test firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    log_firmware_version(test_firmware_package_info);

    // Perform upgrade...
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, test_firmware_buffer, test_firmware_size));
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_FULL_DEVICE, FIRMWARE_OPERATION_RESET, &finalStatus);

    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.rgb));

    // Check upgrade...
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, test_firmware_package_info.audio)) << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     test_firmware_package_info.depth_config_number_versions,
                                     test_firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, test_firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, test_firmware_package_info.rgb)) << "RGB mismatch";

    // Update back to the LKG firmware
    LOG_INFO("Updating the device back to the LKG firmware.");
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    log_firmware_version(lkg_firmware_package_info);

    // Perform upgrade...
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_download(firmware_handle, lkg_firmware_buffer, lkg_firmware_size));
    interrupt_device_at_update_stage(FIRMWARE_OPERATION_FULL_DEVICE, FIRMWARE_OPERATION_RESET, &finalStatus);

    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.audio));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth_config));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.depth));
    ASSERT_EQ(FIRMWARE_OPERATION_SUCCEEDED, calculate_overall_component_status(finalStatus.rgb));

    // Check upgrade...
    ASSERT_EQ(K4A_RESULT_SUCCEEDED, firmware_get_device_version(firmware_handle, &current_version));
    log_device_version(current_version);
    ASSERT_TRUE(compare_version(current_version.audio, lkg_firmware_package_info.audio)) << "Audio version mismatch";
    ASSERT_TRUE(compare_version_list(current_version.depth_sensor,
                                     lkg_firmware_package_info.depth_config_number_versions,
                                     lkg_firmware_package_info.depth_config_versions))
        << "Depth Config mismatch";
    ASSERT_TRUE(compare_version(current_version.depth, lkg_firmware_package_info.depth)) << "Depth mismatch";
    ASSERT_TRUE(compare_version(current_version.rgb, lkg_firmware_package_info.rgb)) << "RGB mismatch";
}

int main(int argc, char **argv)
{
    k4a_unittest_init();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}