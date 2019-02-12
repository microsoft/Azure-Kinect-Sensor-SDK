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

k4a_result_t firmware_create(depthmcu_t depthmcu, firmware_t *firmware_handle)
{
    firmware_context_t *firmware = NULL;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, depthmcu == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware_handle == NULL);

    firmware = firmware_t_create(firmware_handle);
    firmware->depthmcu = depthmcu;
    firmware->lock = Lock_Init();

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

    k4a_result_t result = K4A_RESULT_FAILED;
    firmware_context_t *firmware = firmware_t_get_context(firmware_handle);

    Lock(firmware->lock);
    result = TRACE_CALL(depthmcu_reset_device(firmware->depthmcu));
    Unlock(firmware->lock);
    return result;
}

k4a_result_t parse_firmware_package(const uint8_t *firmware_buffer,
                                    size_t firmware_size,
                                    firmware_package_info_t *package_info)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware_buffer == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmware_size < sizeof(firmware_package_header_t));
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, package_info == NULL);

    const firmware_package_header_t *package_header = (const firmware_package_header_t *)firmware_buffer;

    package_info->package_valid = true;
    package_info->signature_type = (k4a_firmware_signature_t)package_header->signature_type;
    package_info->build_config = (k4a_firmware_build_t)package_header->build_configuration;

    uint32_t crc = 0;
    size_t crc_offset = firmware_size - sizeof(crc);
    memcpy(&crc, firmware_buffer + crc_offset, sizeof(crc));
    uint32_t calculatedCRC = calculate_crc32(firmware_buffer, crc_offset);
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

    if (package_header->auth_block_start + 6 > firmware_size)
    {
        LOG_ERROR("Firmware Package Authentication block not found.", 0);
        package_info->package_valid = false;
    }
    else
    {
        memcpy(&certificate_block_start_offset,
               firmware_buffer + package_header->auth_block_start,
               sizeof(certificate_block_start_offset));
        memcpy(&certificate_block_length,
               firmware_buffer + package_header->auth_block_start + sizeof(certificate_block_start_offset),
               sizeof(certificate_block_length));

        if (certificate_block_length < 1 || (certificate_block_start_offset + certificate_block_length) > firmware_size)
        {
            LOG_ERROR("Firmware Package Authentication invalid.", 0);
            package_info->package_valid = false;
        }
        else
        {
            uint8_t certificate_type;
            memcpy(&certificate_type, firmware_buffer + certificate_block_start_offset, sizeof(certificate_type));

            package_info->certificate_type = (k4a_firmware_signature_t)certificate_type;
        }
    }

    return K4A_RESULT_SUCCEEDED;
}
