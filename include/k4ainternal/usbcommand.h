/** \file usbcommand.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef USB_COMMAND_H
#define USB_COMMAND_H

//************************ Includes *****************************
#include <k4a/k4atypes.h>
#include <k4ainternal/image.h>
#include <k4ainternal/common.h>

#ifdef __cplusplus
extern "C" {
#endif

//**************Symbolic Constant Macros (defines)  *************

//************************ Typedefs *****************************
typedef enum
{
    USB_DEVICE_DEPTH_PROCESSOR = 0,
    USB_DEVICE_COLOR_IMU_PROCESSOR,

    USB_DEVICE_TYPE_COUNT
} usb_command_device_type_t;

#define NULL_INDEX 0xFF

typedef enum
{
    CMD_STATUS_PASS = 0,
} usb_cmd_responses_t;

K4A_DECLARE_HANDLE(usbcmd_t);

/** Delivers a sample to the registered callback function when a capture is ready for processing.
 *
 * \param result
 * indicates if the capture being passed in is complete
 *
 * \param image_handle
 * image read by hardware.
 *
 * \param context
 * Context for the callback function
 *
 * \remarks
 * Capture is only of one type. At this point it is not linked to other captures. Capture is safe to use durring this
 * callback as the caller ensures a ref is held. If the callback function wants the capture to exist beyond this
 * callback, a ref must be taken with capture_inc_ref().
 */
typedef void(usb_cmd_stream_cb_t)(k4a_result_t result, k4a_image_t image_handle, void *context);

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************

/** Open a handle to a USB Function device.
 *
 * \param usb_handle [OUT]
 *    A pointer to write the opened usbcmd_t device handle to
 *
 * \return K4A_RESULT_SUCCEEDED if the device was opened, otherwise an error code
 *
 * If successful, \ref usb_cmd_create will return a USB device handle in the USB
 * parameter. This handle grants exclusive access to the device and may be used in
 * the other k4a API calls.
 *
 * When done with the device, close the handle with \ref usb_cmd_shutdown and then
 * \ref usb_cmd_destroy
 */
k4a_result_t usb_cmd_create(usb_command_device_type_t device_type,
                            uint32_t device_index,
                            const guid_t *container_id,
                            usbcmd_t *usb_handle);

void usb_cmd_destroy(usbcmd_t usb_handle);

/** Get the serial number associated with the USB descriptor.
 *
 * \param usb_handle [IN]
 *    A pointer to the opened usbcmd_t device handle
 *
 * \param serial_number [OUT]
 *    A pointer to write the serial number to
 *
 * \param serial_number_size [IN OUT]
 *    IN: a pointer to the size of the serial number buffer passed in
      OUT: the size of the serial number written to the buffer including the NULL.
 *
 * \return K4A_BUFFER_RESULT_SUCCEEDED if the serial number was successfully opened, K4A_BUFFER_RESULT_TOO_SMALL is the
 memory passed in is insufficient, K4A_BUFFER_RESULT_FAILED if an error occurs.
 *
 * If successful, \ref serial_number will contain a serial number and serial_number_size will be the null terminated
 string length. If K4A_BUFFER_RESULT_TOO_SMALL is returned, then serial_number_size will contain the required size.
 */
k4a_buffer_result_t usb_cmd_get_serial_number(usbcmd_t usb_handle, char *serial_number, size_t *serial_number_size);

// Read command
k4a_result_t usb_cmd_read(usbcmd_t usb_handle,
                          uint32_t cmd,
                          uint8_t *p_cmd_data,
                          size_t cmd_data_size,
                          uint8_t *p_data,
                          size_t data_size,
                          size_t *bytes_read);

k4a_result_t usb_cmd_read_with_status(usbcmd_t usbcmd_handle,
                                      uint32_t cmd,
                                      uint8_t *p_cmd_data,
                                      size_t cmd_data_size,
                                      uint8_t *p_data,
                                      size_t data_size,
                                      size_t *bytes_read,
                                      uint32_t *cmd_status);

// Write command
k4a_result_t usb_cmd_write(usbcmd_t usb_handle,
                           uint32_t cmd,
                           uint8_t *p_cmd_data,
                           size_t cmd_data_size,
                           uint8_t *p_data,
                           size_t data_size);

k4a_result_t usb_cmd_write_with_status(usbcmd_t usb_handle,
                                       uint32_t cmd,
                                       uint8_t *p_cmd_data,
                                       size_t cmd_data_size,
                                       uint8_t *p_data,
                                       size_t data_size,
                                       uint32_t *cmd_status);

// stream data callback
k4a_result_t usb_cmd_stream_register_cb(usbcmd_t usbcmd, usb_cmd_stream_cb_t *frame_ready_cb, void *context);

k4a_result_t usb_cmd_stream_start(usbcmd_t usb_handle, size_t payload_size);

k4a_result_t usb_cmd_stream_stop(usbcmd_t usb_handle);

// Get the number of connected devices
k4a_result_t usb_cmd_get_device_count(uint32_t *p_device_count);

const guid_t *usb_cmd_get_container_id(usbcmd_t usbcmd_handle);

#ifdef __cplusplus
} // namespace k4a
#endif

#endif /* USB_COMMAND_H */
