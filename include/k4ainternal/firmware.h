/** \file firmware.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <k4a/k4atypes.h>
#include <k4ainternal/color_mcu.h>
#include <k4ainternal/depth_mcu.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handle to the firmware device.
 *
 * Handles are created with \ref firmware_create and closed with \ref firmware_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(firmware_t);

typedef enum
{
    FIRMWARE_OPERATION_INPROGRESS = 0, /**< The operation is still in progress */
    FIRMWARE_OPERATION_FAILED = 1,     /**< The operation has completed and has failed */
    FIRMWARE_OPERATION_SUCCEEDED = 3   /**< The operation has completed and has succeeded */
} firmware_operation_status_t;

typedef struct _firmware_component_status_t
{
    firmware_operation_status_t version_check;
    firmware_operation_status_t authentication_check;
    firmware_operation_status_t image_transfer;
    firmware_operation_status_t flash_erase;
    firmware_operation_status_t flash_write;
    firmware_operation_status_t overall;
} firmware_component_status_t;

typedef struct _firmware_status_summary_t
{
    firmware_component_status_t depth;
    firmware_component_status_t rgb;
    firmware_component_status_t audio;
    firmware_component_status_t depth_config;
} firmware_status_summary_t;

typedef struct _firmware_package_info_t
{
    char *path;
    uint8_t *buffer;
    size_t size;

    bool package_valid;
    bool crc_valid;
    k4a_version_t rgb;
    k4a_version_t depth;
    k4a_version_t audio;
    uint8_t depth_config_number_versions;
    k4a_version_t depth_config_versions[5];
    k4a_firmware_build_t build_config;
    k4a_firmware_signature_t signature_type;
    k4a_firmware_signature_t certificate_type;
} firmware_package_info_t;

k4a_result_t firmware_create(char *device_serial_number, bool resetting_device, firmware_t *firmware_handle);
void firmware_destroy(firmware_t firmware_handle);

k4a_result_t firmware_download(firmware_t firmware_handle, uint8_t *pFirmwareBuffer, size_t firmwareSize);

k4a_result_t firmware_get_download_status(firmware_t firmware_handle, firmware_status_summary_t *status);

k4a_result_t firmware_reset_device(firmware_t firmware_handle);

k4a_result_t firmware_get_device_version(firmware_t firmware_handle, k4a_hardware_version_t *version);

k4a_result_t parse_firmware_package(firmware_package_info_t *package_info);

// If successful, the user must free the serial number
k4a_result_t firmware_get_serial_number(colormcu_t colormcu, depthmcu_t depthmcu, char **serial_number);

void firmware_free_serial_number(char *serial_number);

#ifdef __cplusplus
}
#endif

#endif /* FIRMWARE_H */
