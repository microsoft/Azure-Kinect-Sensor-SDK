/** \file color_mcu *h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef COLOR_MCU_H
#define COLOR_MCU_H

#include <k4a/k4atypes.h>
#include <k4ainternal/handle.h>
#include <k4ainternal/usbcommand.h>
#include <k4ainternal/allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handle to the color mcu (micro controller unit) device.
 *
 * Handles are created with \ref colormcu_create and closed
 * with \ref colormcu_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(colormcu_t);

#pragma pack(push, 1)
typedef struct
{
    uint64_t pts; // 8 bytes PTS timestamp
    int16_t rx;
    int16_t ry;
    int16_t rz;
} xyz_vector_t;

typedef struct _temperature_t
{
    uint32_t reporting_rate_in_us;    // 19.23ms, 52Hz
    uint16_t temperature_sensitivity; // in milli degrees C
    int16_t value;                    // temperature value
} temperature_t;

typedef struct _gyroscope_t
{
    uint16_t sensitivity; // in micro dps
    uint32_t sample_rate_in_us;
    uint32_t sample_count;
} gyroscope_t;

typedef struct _accelerometer_t
{
    uint16_t sensitivity; // in micro Gs
    uint32_t sample_rate_in_us;
    uint32_t sample_count;
} accelerometer_t;

typedef struct _imu_payload_metadata_t
{
    temperature_t temperature;
    gyroscope_t gyro;
    accelerometer_t accel;
} imu_payload_metadata_t;

#pragma pack(pop)

#define IMU_MAX_ACC_COUNT_IN_PAYLOAD 8
#define IMU_MAX_GYRO_COUNT_IN_PAYLOAD 8
#define IMU_MAX_PAYLOAD_SIZE                                                                                           \
    (sizeof(imu_payload_metadata_t) + (sizeof(xyz_vector_t) * IMU_MAX_ACC_COUNT_IN_PAYLOAD) +                          \
     (sizeof(xyz_vector_t) * IMU_MAX_GYRO_COUNT_IN_PAYLOAD))

/** Open a handle to the color mcu device.
 *
 * \param container_id
 * The container ID of the device to open
 *
 * \param colormcu_handle [OUT]
 * A pointer to write the opened color mcu handle to
 *
 * \return K4A_RESULT_SUCCEEDED if the device was opened, otherwise an error code
 *
 * If successful, \ref colormcu_create will return a color mcu device handle in the color mcu
 * parameter. This handle grants exclusive access to the device and may be used in
 * the other k4a API calls.
 *
 * When done with the device, close the handle with \ref colormcu_destroy
 */
k4a_result_t colormcu_create(const guid_t *container_id, colormcu_t *colormcu_handle);

/** Open a handle to the color mcu device.
 *
 * \param device_index
 * The index number of the color MCU device to open.
 *
 * \param colormcu_handle [OUT]
 * A pointer to write the opened color mcu handle to
 *
 * \return K4A_RESULT_SUCCEEDED if the device was opened, otherwise an error code
 *
 * If successful, \ref colormcu_create will return a color mcu device handle in the color mcu
 * parameter. This handle grants exclusive access to the device and may be used in
 * the other k4a API calls.
 *
 * When done with the device, close the handle with \ref colormcu_destroy
 *
 * NOTE: This API is not garanteed to return a matching colormcu for depthmcu_create(device_index). Container id or
 * serial number must be matched to ensure the two devices are on the same phyisical Kinect.
 *
 */
k4a_result_t colormcu_create_by_index(uint32_t device_index, colormcu_t *colormcu_handle);

/** Get the serial number associated with the USB descriptor.
 *
 * \param colormcu_handle
 *  Colormcu handle provided by the colormcu_create() call
 *
 * \param serial_number [OUT]
 * A pointer to write the serial number to
 *
 * \param serial_number_size [IN OUT]
 * IN: a pointer to the size of the serial number buffer passed in
 * OUT: the size of the serial number written to the buffer including the NULL.
 *
 * \return K4A_BUFFER_RESULT_SUCCEEDED if the serial number was successfully opened, K4A_BUFFER_RESULT_TOO_SMALL is the
 * memory passed in is insufficient, K4A_BUFFER_RESULT_FAILED if an error occurs.
 *
 * If successful, \ref serial_number will contain a serial number and serial_number_size will be the null terminated
 * string length. If K4A_BUFFER_RESULT_TOO_SMALL is returned, then serial_number_size will contain the required size.
 */

k4a_buffer_result_t colormcu_get_usb_serialnum(colormcu_t colormcu_handle,
                                               char *serial_number,
                                               size_t *serial_number_size);

/** Closes the color mcu module and free's it resources
 * */
void colormcu_destroy(colormcu_t colormcu_handle);

/** Writes the device synchronization settings
 *
 * \param colormcu_handle
 * Colormcu handle provided by the colormcu_create() call
 *
 * \param config [IN]
 * A pointer to the configuration settings that we want reflected in the hardware
 *
 * \return K4A_RESULT_SUCCEEDED if the call succeeded
 */
k4a_result_t colormcu_set_multi_device_mode(colormcu_t colormcu_handle, const k4a_device_configuration_t *config);

/** Writes the device synchronization settings
 *
 * \param colormcu_handle
 * Colormcu handle provided by the colormcu_create() call
 *
 * \param sync_in_jack_connected [OUT]
 * A pointer to a location to write the sync in jack status
 *
 * \param sync_out_jack_connected [OUT]
 * A pointer to a location to write the sync out jack status
 *
 * \return K4A_RESULT_SUCCEEDED if the call succeeded
 */
k4a_result_t colormcu_get_external_sync_jack_state(colormcu_t colormcu_handle,
                                                   bool *sync_in_jack_connected,
                                                   bool *sync_out_jack_connected);

// general

k4a_result_t colormcu_reset_device(colormcu_t colormcu_handle);

// IMU functions
k4a_result_t colormcu_imu_start_streaming(colormcu_t colormcu_handle);
void colormcu_imu_stop_streaming(colormcu_t colormcu_handle);
k4a_result_t colormcu_imu_register_stream_cb(colormcu_t colormcu_handle,
                                             usb_cmd_stream_cb_t *capture_ready_cb,
                                             void *context);
k4a_result_t colormcu_imu_get_calibration(colormcu_t colormcu_handle,
                                          void *memory); // RGB_CAMERA_USB_COMMAND_READ_IMU_CALIDATA

#ifdef __cplusplus
}
#endif

#endif /* COLOR_MCU_H */
