/** \file depth.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef DEPTH_H
#define DEPTH_H

#include <k4a/k4atypes.h>
#include <k4ainternal/handle.h>
#include <k4ainternal/depth_mcu.h>
#include <k4ainternal/calibration.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Delivers a sample to the registered callback function when a capture is ready for processing.
 *
 * \param result
 * indicates if the capture being passed in is complete
 *
 * \param capture_handle
 * Capture being read by hardware.
 *
 * \param callback_context
 * Context for the callback function
 *
 * \remarks
 * Capture is only of one type. At this point it is not linked to other captures. Capture is safe to use during this
 * callback as the caller ensures a ref is held. If the callback function wants the capture to exist beyond this
 * callback, a ref must be taken with capture_inc_ref().
 */
typedef void(depth_cb_streaming_capture_t)(k4a_result_t result, k4a_capture_t capture_handle, void *callback_context);

/** Handle to the depth device.
 *
 * Handles are created with \ref depth_create and closed
 * with \ref depth_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(depth_t);

/** Open a handle to the depth device.
 *
 * \param depthmcu_handle [IN]
 * Handle to the MCU communication module
 *
 * \param calibration_handle [IN]
 * Handle to the calibration module.
 *
 * \param capture_ready_cb [IN]
 *    A function pointer to call when new depth captures are ready
 *
 * \param capture_ready_cb_context [IN]
 *    A context to go with the depth_capture_ready callback function
 *
 * \param depth_handle [OUT]
 * A pointer to write the opened depth device handle to
 *
 * \return K4A_RESULT_SUCCEEDED if the device was opened, otherwise an error code
 *
 * If successful, \ref depth_create will return a depth device handle in the depth
 * parameter. This handle grants exclusive access to the device and may be used in
 * the other k4a API calls.
 *
 * When done with the device, close the handle with \ref depth_destroy
 */
k4a_result_t depth_create(depthmcu_t depthmcu_handle,
                          calibration_t calibration_handle,
                          depth_cb_streaming_capture_t *capture_ready_cb,
                          void *capture_ready_cb_context,
                          depth_t *depth_handle);

/** Closes the handle to the depth device.
 *
 * \param depth_handle [IN]
 * The depth device handle to close
 *
 * If the sensor was streaming at the time /ref depth_destroy is called, /ref depth_stop will be implicitly called to
 * stop the sensor.
 */
void depth_destroy(depth_t depth_handle);

/** Returns the Azure Kinect device serial number.
 *
 * \param depth_handle [IN]
 * The depth device handle.
 *
 * \param serial_number [IN]
 * char array to write to serial number to.
 *
 * \param serial_number_size [IN OUT]
 * [IN] the maximum size of the memory pointed to by /ref serial_number
 * [OUT] When used with /ref serial_number set to NULL, this value will represent the minimum allocation size needed for
 * this call to succeed.
 *
 * \return ::K4A_BUFFER_RESULT_SUCCEEDED if the serial number was successfully read and written into /ref serial_number.
 * ::K4A_BUFFER_RESULT_TOO_SMALL if the size of serial_number was too small, in this case /ref serial_number_size will
 * have been updated with the size needed. ::K4A_BUFFER_RESULT_FAILED if an error was encountered. device was opened,
 * otherwise an error code
 *  *
 * If successful, /ref serial_number will contain a NULL terminated string.
 */
k4a_buffer_result_t depth_get_device_serialnum(depth_t depth_handle, char *serial_number, size_t *serial_number_size);

/** Returns the Azure Kinect device versions.
 *
 * \param depth_handle [IN]
 * The depth device handle.
 *
 * \param version [OUT]
 * Pre allocated version structure for the function to write the version info to.
 *
 * \return ::K4A_RESULT_SUCCEEDED if the verison was successfully from the device and written into /ref version.
 * ::K4A_RESULT_FAILED if an error was encountered.
 */
k4a_result_t depth_get_device_version(depth_t depth_handle, k4a_hardware_version_t *version);

/** Starts the depth sensor streaming
 *
 * \param depth_handle [IN]
 * The depth device handle.
 *
 * \param config [IN]
 * The configuration of the depth sensor the user wants the sensor to run in.
 *
 * \return ::K4A_RESULT_SUCCEEDED if the depth sensor was successfully started. ::K4A_RESULT_FAILED if an error was
 * encountered.
 *
 * call /ref depth_stop when the sensor no longer needs to stream.
 */
k4a_result_t depth_start(depth_t depth_handle, const k4a_device_configuration_t *config);

/** Stops the depth sensor when it has been streaming
 *
 * \param depth_handle [IN]
 * The depth device handle.
 *
 * Stops depth sensor when it has been running. /ref depth_destroy implicitly calls this API if the sensor is in a
 * running state.
 */
void depth_stop(depth_t depth_handle);

#ifdef __cplusplus
}
#endif

#endif /* DEPTH_H */
