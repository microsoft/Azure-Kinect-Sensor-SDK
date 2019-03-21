/** \file depth_mcu.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef DEPTH_MCU_H
#define DEPTH_MCU_H

#include <k4a/k4atypes.h>
#include <k4ainternal/common.h>
#include <k4ainternal/handle.h>
#include <k4ainternal/usbcommand.h>
#include <k4ainternal/allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

typedef struct _depthmcu_firmware_versions_t
{
    uint8_t rgb_major;
    uint8_t rgb_minor;
    uint16_t rgb_build;

    uint8_t depth_major;
    uint8_t depth_minor;
    uint16_t depth_build;

    uint8_t audio_major;
    uint8_t audio_minor;
    uint16_t audio_build;

    uint16_t depth_sensor_cfg_major;
    uint16_t depth_sensor_cfg_minor;

    uint8_t build_config;   // 0x00 = Release; 0x01 = Debug
    uint8_t signature_type; // 0x00 = MSFT; 0x01 = test; 0x02 = unsigned
} depthmcu_firmware_versions_t;

typedef struct _depthmcu_firmware_update_status_t
{
    uint16_t depth_status;
    uint16_t rgb_status;
    uint16_t audio_status;
    uint16_t depth_configure_status;
} depthmcu_firmware_update_status_t;

#pragma pack(pop)

/** Delivers a sample to the registered callback function when a capture is ready for processing.
 *
 * \param result
 * indicates if the capture being passed in is complete
 *
 * \param image_handle
 * Image being read by hardware.
 *
 * \param callback_context
 * Context for the callback function
 *
 * \remarks
 * Capture is only of one type. At this point it is not linked to other captures. Capture is safe to use durring this
 * callback as the caller ensures a ref is held. If the callback function wants the capture to exist beyond this
 * callback, a ref must be taken with capture_inc_ref().
 */
typedef void(depthmcu_stream_cb_t)(k4a_result_t result, k4a_image_t image_handle, void *callback_context);

/** Handle to the depth mcu (micro controll unit) device.
 *
 * Handles are created with \ref depthmcu_create and closed
 * with \ref depthmcu_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(depthmcu_t);

/** Open a handle to the depth mcu device.
 *
 * \param depthmcu_handle [OUT]
 *    A pointer to write the opened depthmcu device handle to
 *
 * \return K4A_RESULT_SUCCEEDED if the device was opened, otherwise an error code
 *
 * If successful, \ref depthmcu_create will return a depthmcu device handle in the depth mcu
 * parameter. This handle grants exclusive access to the device and may be used in
 * the other k4a API calls.
 *
 * When done with the device, close the handle with \ref depthmcu_destroy
 */
k4a_result_t depthmcu_create(uint32_t device_index, depthmcu_t *depthmcu_handle);

/** Closes the depth mcu module and free's it resources
 * */
void depthmcu_destroy(depthmcu_t depthmcu_handle);

const guid_t *depthmcu_get_container_id(depthmcu_t depthmcu_handle);

k4a_result_t depthmcu_depth_start_streaming(depthmcu_t depthmcu_handle,
                                            depthmcu_stream_cb_t *callback,
                                            void *callback_context);

void depthmcu_depth_stop_streaming(depthmcu_t depthmcu_handle, bool quiet);

k4a_result_t depthmcu_depth_set_capture_mode(depthmcu_t depthmcu_handle, k4a_depth_mode_t depth_mode);
k4a_result_t depthmcu_depth_get_capture_mode(depthmcu_t depthmcu_handle, k4a_depth_mode_t *depth_mode);

k4a_result_t depthmcu_depth_set_fps(depthmcu_t depthmcu_handle, k4a_fps_t depth_fps);
k4a_result_t depthmcu_depth_get_fps(depthmcu_t depthmcu_handle, k4a_fps_t *depth_fps);

k4a_result_t depthmcu_get_color_imu_calibration(depthmcu_t depthmcu_handle,
                                                k4a_capture_t cal,
                                                size_t *size_read_written);
k4a_result_t depthmcu_color_imu_calibration(depthmcu_t depthmcu_handle, k4a_capture_t cal, size_t *size);

k4a_buffer_result_t depthmcu_get_serialnum(depthmcu_t depthmcu_handle, char *serial_number, size_t *serial_number_size);

k4a_result_t depthmcu_get_version(depthmcu_t depthmcu_handle, depthmcu_firmware_versions_t *version);
bool depthmcu_wait_is_ready(depthmcu_t depthmcu_handle);

k4a_result_t depthmcu_get_cal(depthmcu_t depthmcu_handle, uint8_t *calibration, size_t cal_size, size_t *bytes_read);

k4a_result_t
depthmcu_get_extrinsic_calibration(depthmcu_t depthmcu_handle, char *json, size_t json_size, size_t *bytes_read);

k4a_result_t depthmcu_reset(depthmcu_t depthmcu_handle);
k4a_result_t depthmcu_reset_depth_sensor(depthmcu_t depthmcu_handle);

k4a_result_t depthmcu_download_firmware(depthmcu_t depthmcu_handle, uint8_t *firmwarePayload, size_t firmwareSize);
k4a_result_t depthmcu_get_firmware_update_status(depthmcu_t depthmcu_handle,
                                                 depthmcu_firmware_update_status_t *update_status);

/** Sends a command to the Depth MCU to reset the Depth and RGB ISPs.**/
k4a_result_t depthmcu_reset_device(depthmcu_t depthmcu_handle);

#ifdef __cplusplus
}
#endif

#endif /* DEPTH_MCU_H */
