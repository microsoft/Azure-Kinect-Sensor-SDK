// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef FIRMWARE_HELPER_H
#define FIRMWARE_HELPER_H

#include <k4a/k4atypes.h>
#include <k4ainternal/firmware.h>
#include <ConnEx.h>

#define UPDATE_TIMEOUT_MS 10 * 60 * 1000 // 10 Minutes should be way more than enough.
#define UPDATE_POLL_INTERVAL_MS 5

// This will define the path to the firmware packages to use in testing the firmware update process. The firmware update
// is executed by the firmware that is currently on the device. In order to test the firmware update process for a
// candidate, the device must be on the candidate firmware and then updated to a different test firmware where all of
// the versions are different.
// Factory firmware - This should be the oldest available firmware that we can roll back to.
// LKG firmware - This should be the last firmware that was released.
// Test firmware - This should be a firmware where all components have different versions than the candidate firmware.
// Candidate firmware - This should be the firmware that is being validated. It will be passed in via command line
// parameter.
#define K4A_FACTORY_FIRMWARE_PATH "..\\tools\\updater\\firmware\\AzureKinectDK_Fw_1.5.786013.bin"
#define K4A_LKG_FIRMWARE_PATH "..\\tools\\updater\\firmware\\AzureKinectDK_Fw_1.5.786013.bin"
#define K4A_TEST_FIRMWARE_PATH "..\\tools\\updater\\firmware\\AzureKinectDK_Fw_1.5.786013.bin"

typedef enum
{
    FIRMWARE_OPERATION_START,
    FIRMWARE_OPERATION_AUDIO_ERASE,
    FIRMWARE_OPERATION_AUDIO_WRITE,
    FIRMWARE_OPERATION_DEPTH_ERASE,
    FIRMWARE_OPERATION_DEPTH_WRITE,
    FIRMWARE_OPERATION_RGB_ERASE,
    FIRMWARE_OPERATION_RGB_WRITE,
    FIRMWARE_OPERATION_FULL_DEVICE,
} firmware_operation_component_t;

typedef enum
{
    FIRMWARE_OPERATION_RESET,
    FIRMWARE_OPERATION_DISCONNECT,
} firmware_operation_interruption_t;

extern int g_k4a_port_number;
extern connection_exerciser *g_connection_exerciser;

extern uint8_t *g_test_firmware_buffer;
extern size_t g_test_firmware_size;
extern firmware_package_info_t g_test_firmware_package_info;

extern uint8_t *g_candidate_firmware_buffer;
extern size_t g_candidate_firmware_size;
extern firmware_package_info_t g_candidate_firmware_package_info;

extern uint8_t *g_lkg_firmware_buffer;
extern size_t g_lkg_firmware_size;
extern firmware_package_info_t g_lkg_firmware_package_info;

extern uint8_t *g_factory_firmware_buffer;
extern size_t g_factory_firmware_size;
extern firmware_package_info_t g_factory_firmware_package_info;

k4a_result_t setup_common_test();

k4a_result_t load_firmware_files(char *firmware_path, uint8_t **firmware_buffer, size_t *firmware_size);
firmware_operation_status_t calculate_overall_component_status(const firmware_component_status_t status);

bool compare_version(k4a_version_t left_version, k4a_version_t right_version);
bool compare_version_list(k4a_version_t device_version, uint8_t count, k4a_version_t versions[5]);

void log_firmware_build_config(k4a_firmware_build_t build_config);
void log_firmware_signature_type(k4a_firmware_signature_t signature_type, bool certificate);
void log_firmware_version(firmware_package_info_t firmware_version);
void log_device_version(k4a_hardware_version_t firmware_version);

k4a_result_t open_firmware_device(firmware_t *firmware_handle);
k4a_result_t reset_device(firmware_t *firmware_handle);
k4a_result_t interrupt_operation(firmware_t *firmware_handle, firmware_operation_interruption_t interruption);
k4a_result_t interrupt_device_at_update_stage(firmware_t *firmware_handle,
                                              firmware_operation_component_t component,
                                              firmware_operation_interruption_t interruption,
                                              firmware_status_summary_t *final_status,
                                              bool verbose_logging);
k4a_result_t perform_device_update(firmware_t *firmware_handle,
                                   uint8_t *firmware_buffer,
                                   size_t firmware_size,
                                   firmware_package_info_t firmware_package_info,
                                   bool verbose_logging);

#endif /* FIRMWARE_HELPER_H */
