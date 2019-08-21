// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/firmware.h>

// Dependent libraries
#include <azure_c_shared_utility/lock.h>

#include <k4a/k4aversion.h>
#include <stdlib.h>

#define FIRMWARE_PACKAGE_MAX_NUMBER_DEPTH_CONFIG 5

#pragma pack(push, 1)
typedef struct _firmware_package_depth_config_header_t
{
    uint16_t version_major;
    uint16_t version_minor;
    uint8_t reserved_4[8];
} firmware_package_depth_config_header_t;

typedef struct _firmware_package_header_t
{
    uint8_t signature_type;
    uint8_t build_configuration;

    uint32_t auth_block_start;

    uint8_t depth_version_major;
    uint8_t depth_version_minor;
    uint16_t depth_version_build;

    uint8_t reserved_1[8];

    uint8_t rgb_version_major;
    uint8_t rgb_version_minor;
    uint16_t rgb_version_build;

    uint8_t reserved_2[8];

    uint8_t audio_version_major;
    uint8_t audio_version_minor;
    uint16_t audio_version_build;

    uint8_t reserved_3[8];

    uint16_t number_depth_config;
    firmware_package_depth_config_header_t depth_config[FIRMWARE_PACKAGE_MAX_NUMBER_DEPTH_CONFIG];
} firmware_package_header_t;
#pragma pack(pop)

typedef struct _firmware_context_t
{
    depthmcu_t depthmcu;
    colormcu_t colormcu;
    char *serial_number;
    LOCK_HANDLE lock;
} firmware_context_t;

K4A_DECLARE_CONTEXT(firmware_t, firmware_context_t);

static void depthmcu_to_firmware_status(uint16_t depthmcu_status, firmware_component_status_t *firmware_status)
{
    firmware_status->version_check = (depthmcu_status >> 0) & 0x3;
    firmware_status->authentication_check = (depthmcu_status >> 2) & 0x3;
    firmware_status->image_transfer = (depthmcu_status >> 4) & 0x3;
    firmware_status->flash_erase = (depthmcu_status >> 6) & 0x3;
    firmware_status->flash_write = (depthmcu_status >> 8) & 0x3;
    // Bits 10 and 11 = Unused
    // Bits 12 and 13 = Unused
    firmware_status->overall = (depthmcu_status >> 14) & 0x3;
}

static uint32_t calculate_crc32(const uint8_t *pData, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    uint32_t crc32_table[256] = { 0 };

    uint32_t element = 0;
    for (uint32_t i = 0; i < 256; i++)
    {
        element = i;
        for (uint32_t j = 0; j < 8; j++)
        {
            if (element & 1)
            {
                element = ((element >> 1) ^ 0xEDB88320L);
            }
            else
            {
                element = (element >> 1);
            }
        }

        crc32_table[i] = element;
    }

    for (size_t n = 0; n < len; n++)
    {
        crc = ((crc >> 8) & 0x00FFFFFFL) ^ (crc32_table[(crc ^ pData[n]) & 0xFF]);
    }

    return crc;
}

k4a_result_t firmware_create(char *device_serial_number, bool resetting_device, firmware_t *firmware_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, device_serial_number == NULL);

    firmware_context_t *firmware = NULL;
    const guid_t *container_id = NULL;
    k4a_result_t result = K4A_RESULT_FAILED;
    uint32_t device_count;

    firmware = firmware_t_create(firmware_handle);
    firmware->lock = Lock_Init();

    usb_cmd_get_device_count(&device_count);
    if (device_count == 0)
    {
        result = K4A_RESULT_FAILED;
    }

    for (uint32_t device_index = 0; device_index < device_count; device_index++)
    {
        result = TRACE_CALL(depthmcu_create(device_index, &firmware->depthmcu));
        if (K4A_SUCCEEDED(result))
        {
            result = firmware_get_serial_number(NULL, firmware->depthmcu, &firmware->serial_number);
        }

        if ((K4A_SUCCEEDED(result)) && (strcmp(device_serial_number, firmware->serial_number) == 0))
        {
            result = K4A_RESULT_SUCCEEDED;
            break;
        }

        if (firmware->serial_number)
        {
            firmware_free_serial_number(firmware->serial_number);
        }
        if (firmware->depthmcu)
        {
            depthmcu_destroy(firmware->depthmcu);
        }
        firmware->depthmcu = NULL;
        firmware->serial_number = NULL;
        result = K4A_RESULT_FAILED;
    }

    if (K4A_SUCCEEDED(result))
    {
        // Wait until the device is responding correctly...
        result = TRACE_CALL(K4A_RESULT_FROM_BOOL(depthmcu_wait_is_ready(firmware->depthmcu)));
        if (K4A_SUCCEEDED(result))
        {
            result = K4A_RESULT_FROM_BOOL((container_id = depthmcu_get_container_id(firmware->depthmcu)) != NULL);
        }

        if (K4A_SUCCEEDED(result))
        {
            result = TRACE_CALL(colormcu_create(container_id, &firmware->colormcu));
            if ((resetting_device) && K4A_FAILED(result))
            {
                // We only need 1 USB device to reset
                result = K4A_RESULT_SUCCEEDED;
            }
        }
    }
    else if (resetting_device) // Search for just the colormcu
    {
        // When resetting the device we only need the depthmcu or the colormcu
        for (uint32_t device_index = 0; device_index < device_count; device_index++)
        {
            result = TRACE_CALL(colormcu_create_by_index(device_index, &firmware->colormcu));
            if (K4A_SUCCEEDED(result))
            {
                result = firmware_get_serial_number(firmware->colormcu, NULL, &firmware->serial_number);
            }

            if ((K4A_SUCCEEDED(result)) && (strcmp(device_serial_number, firmware->serial_number) == 0))
            {
                result = K4A_RESULT_SUCCEEDED;
                break;
            }

            if (firmware->serial_number)
            {
                firmware_free_serial_number(firmware->serial_number);
            }
            if (firmware->colormcu)
            {
                colormcu_destroy(firmware->colormcu);
            }
            firmware->serial_number = NULL;
            firmware->colormcu = NULL;
            result = K4A_RESULT_FAILED;
            continue;
        }
    }

    if (K4A_FAILED(result))
    {
        firmware_destroy(*firmware_handle);
        *firmware_handle = NULL;
    }

    return result;
}

void firmware_destroy(firmware_t firmware_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, firmware_t, firmware_handle);
    firmware_context_t *firmware = firmware_t_get_context(firmware_handle);

    // Make sure that we can get the lock. Hopefully this means that no other process is running.
    Lock(firmware->lock);

    if (firmware->depthmcu)
    {
        depthmcu_destroy(firmware->depthmcu);
        firmware->depthmcu = NULL;
    }

    if (firmware->colormcu)
    {
        colormcu_destroy(firmware->colormcu);
        firmware->colormcu = NULL;
    }

    if (firmware->serial_number)
    {
        firmware_free_serial_number(firmware->serial_number);
        firmware->serial_number = NULL;
    }

    Unlock(firmware->lock);
    Lock_Deinit(firmware->lock);

    firmware_t_destroy(firmware_handle);
}

k4a_result_t firmware_download(firmware_t firmware_handle, uint8_t *pFirmwareBuffer, size_t firmwareSize)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, firmware_t, firmware_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, pFirmwareBuffer == NULL);

    k4a_result_t result = K4A_RESULT_FAILED;
    firmware_context_t *firmware = firmware_t_get_context(firmware_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware->depthmcu == NULL);

    Lock(firmware->lock);
    result = TRACE_CALL(depthmcu_download_firmware(firmware->depthmcu, pFirmwareBuffer, firmwareSize));
    Unlock(firmware->lock);
    return result;
}

k4a_result_t firmware_get_download_status(firmware_t firmware_handle, firmware_status_summary_t *status)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, firmware_t, firmware_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, status == NULL);

    k4a_result_t result = K4A_RESULT_FAILED;
    depthmcu_firmware_update_status_t depthmcu_status = { 0 };
    firmware_context_t *firmware = firmware_t_get_context(firmware_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware->depthmcu == NULL);

    // NOTE: The spec says this shouldn't be called more frequently than 2Hz.
    result = TRACE_CALL(depthmcu_get_firmware_update_status(firmware->depthmcu, &depthmcu_status));
    if (K4A_SUCCEEDED(result))
    {
        depthmcu_to_firmware_status(depthmcu_status.audio_status, &status->audio);
        depthmcu_to_firmware_status(depthmcu_status.depth_status, &status->depth);
        depthmcu_to_firmware_status(depthmcu_status.depth_configure_status, &status->depth_config);
        depthmcu_to_firmware_status(depthmcu_status.rgb_status, &status->rgb);
    }

    return result;
}

k4a_result_t firmware_reset_device(firmware_t firmware_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, firmware_t, firmware_handle);

    LOG_INFO("Issuing reset command to device.", 0);
    k4a_result_t result = K4A_RESULT_FAILED;
    firmware_context_t *firmware = firmware_t_get_context(firmware_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware->depthmcu == NULL && firmware->colormcu == NULL);

    Lock(firmware->lock);
    if (firmware->colormcu)
    {
        LOG_INFO("Issuing reset command to Color MCU.", 0);
        result = TRACE_CALL(colormcu_reset_device(firmware->colormcu));
    }

    if (K4A_FAILED(result) && firmware->depthmcu)
    {
        LOG_INFO("Issuing reset command to Depth MCU.", 0);
        result = TRACE_CALL(depthmcu_reset_device(firmware->depthmcu));
    }

    Unlock(firmware->lock);
    return result;
}

k4a_result_t firmware_get_device_version(firmware_t firmware_handle, k4a_hardware_version_t *version)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, firmware_t, firmware_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, version == NULL);

    firmware_context_t *firmware = firmware_t_get_context(firmware_handle);
    depthmcu_firmware_versions_t mcu_version;
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware->depthmcu == NULL);

    result = TRACE_CALL(depthmcu_get_version(firmware->depthmcu, &mcu_version));

    if (K4A_SUCCEEDED(result))
    {
        version->rgb.major = mcu_version.rgb_major;
        version->rgb.minor = mcu_version.rgb_minor;
        version->rgb.iteration = mcu_version.rgb_build;

        version->depth.major = mcu_version.depth_major;
        version->depth.minor = mcu_version.depth_minor;
        version->depth.iteration = mcu_version.depth_build;

        version->audio.major = mcu_version.audio_major;
        version->audio.minor = mcu_version.audio_minor;
        version->audio.iteration = mcu_version.audio_build;

        version->depth_sensor.major = mcu_version.depth_sensor_cfg_major;
        version->depth_sensor.minor = mcu_version.depth_sensor_cfg_minor;
        version->depth_sensor.iteration = 0;

        switch (mcu_version.build_config)
        {
        case 0:
            version->firmware_build = K4A_FIRMWARE_BUILD_RELEASE;
            break;
        case 1:
            version->firmware_build = K4A_FIRMWARE_BUILD_DEBUG;
            break;
        default:
            LOG_WARNING("Hardware reported unknown firmware build: %d", mcu_version.build_config);
            version->firmware_build = K4A_FIRMWARE_BUILD_DEBUG;
            break;
        }

        switch (mcu_version.signature_type)
        {
        case 0:
            version->firmware_signature = K4A_FIRMWARE_SIGNATURE_MSFT;
            break;
        case 1:
            version->firmware_signature = K4A_FIRMWARE_SIGNATURE_TEST;
            break;
        case 2:
            version->firmware_signature = K4A_FIRMWARE_SIGNATURE_UNSIGNED;
            break;
        default:
            LOG_WARNING("Hardware reported unknown signature type: %d", mcu_version.signature_type);
            version->firmware_signature = K4A_FIRMWARE_SIGNATURE_UNSIGNED;
            break;
        }
    }

    return result;
}

k4a_result_t parse_firmware_package(firmware_package_info_t *package_info)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, package_info == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, package_info->buffer == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, package_info->size < sizeof(firmware_package_header_t));

    const firmware_package_header_t *package_header = (const firmware_package_header_t *)package_info->buffer;

    package_info->package_valid = true;
    package_info->signature_type = (k4a_firmware_signature_t)package_header->signature_type;
    package_info->build_config = (k4a_firmware_build_t)package_header->build_configuration;

    uint32_t crc = 0;
    size_t crc_offset = package_info->size - sizeof(crc);
    memcpy(&crc, package_info->buffer + crc_offset, sizeof(crc));
    uint32_t calculatedCRC = calculate_crc32(package_info->buffer, crc_offset);
    if (crc != calculatedCRC)
    {
        LOG_ERROR("Firmware Package CRC error. original crc: 0x%08X calculated crc: 0x%08X", crc, calculatedCRC);
        package_info->package_valid = false;
        package_info->crc_valid = false;
    }
    else
    {
        package_info->crc_valid = true;
    }

    package_info->rgb.major = package_header->rgb_version_major;
    package_info->rgb.minor = package_header->rgb_version_minor;
    package_info->rgb.iteration = package_header->rgb_version_build;

    package_info->depth.major = package_header->depth_version_major;
    package_info->depth.minor = package_header->depth_version_minor;
    package_info->depth.iteration = package_header->depth_version_build;

    package_info->audio.major = package_header->audio_version_major;
    package_info->audio.minor = package_header->audio_version_minor;
    package_info->audio.iteration = package_header->audio_version_build;

    if (package_header->number_depth_config > FIRMWARE_PACKAGE_MAX_NUMBER_DEPTH_CONFIG)
    {
        LOG_ERROR("Firmware Package too many Depth Configurations. %d", package_header->number_depth_config);
        package_info->package_valid = false;
    }
    else
    {
        package_info->depth_config_number_versions = (uint8_t)package_header->number_depth_config;
    }

    for (uint8_t i = 0; i < FIRMWARE_PACKAGE_MAX_NUMBER_DEPTH_CONFIG; i++)
    {
        package_info->depth_config_versions[i].major = package_header->depth_config[i].version_major;
        package_info->depth_config_versions[i].minor = package_header->depth_config[i].version_minor;
    }

    uint32_t certificate_block_start_offset = 0;
    uint16_t certificate_block_length = 0;

    if (package_header->auth_block_start + 6 > package_info->size)
    {
        LOG_ERROR("Firmware Package Authentication block not found.", 0);
        package_info->package_valid = false;
    }
    else
    {
        memcpy(&certificate_block_start_offset,
               package_info->buffer + package_header->auth_block_start,
               sizeof(certificate_block_start_offset));
        memcpy(&certificate_block_length,
               package_info->buffer + package_header->auth_block_start + sizeof(certificate_block_start_offset),
               sizeof(certificate_block_length));

        if (certificate_block_length < 1 ||
            (certificate_block_start_offset + certificate_block_length) > package_info->size)
        {
            LOG_ERROR("Firmware Package Authentication invalid.", 0);
            package_info->package_valid = false;
        }
        else
        {
            uint8_t certificate_type;
            memcpy(&certificate_type, package_info->buffer + certificate_block_start_offset, sizeof(certificate_type));

            package_info->certificate_type = (k4a_firmware_signature_t)certificate_type;
        }
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t firmware_get_serial_number(colormcu_t color, depthmcu_t depth, char **serial_number)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, color == NULL && depth == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, serial_number == NULL);

    char *ser_num;
    size_t serial_number_length = 0;
    k4a_buffer_result_t b_result;

    // Get the serial_number length
    if (color)
    {
        b_result = colormcu_get_usb_serialnum(color, NULL, &serial_number_length);
    }
    else
    {
        b_result = depthmcu_get_serialnum(depth, NULL, &serial_number_length);
    }

    if (b_result != K4A_BUFFER_RESULT_TOO_SMALL)
    {
        LOG_ERROR("Failed to get serial number length\n", 0);
        return K4A_RESULT_FAILED;
    }

    ser_num = malloc(serial_number_length);
    if (ser_num == NULL)
    {
        LOG_ERROR("Failed to allocate memory for serial number (%zu bytes)\n", serial_number_length);
        return K4A_RESULT_FAILED;
    }

    if (color)
    {
        b_result = colormcu_get_usb_serialnum(color, ser_num, &serial_number_length);
    }
    else
    {
        b_result = depthmcu_get_serialnum(depth, ser_num, &serial_number_length);
    }

    if (b_result != K4A_BUFFER_RESULT_SUCCEEDED)
    {
        LOG_ERROR("Failed to get serial number: %s\n",
                  b_result == K4A_BUFFER_RESULT_FAILED ? "K4A_BUFFER_RESULT_FAILED" : "K4A_BUFFER_RESULT_TOO_SMALL");
        firmware_free_serial_number(ser_num);
        return K4A_RESULT_FAILED;
    }

    *serial_number = ser_num;
    return K4A_RESULT_SUCCEEDED;
}

void firmware_free_serial_number(char *serial_number)
{
    RETURN_VALUE_IF_ARG(VOID_VALUE, serial_number == NULL);
    free(serial_number);
}
