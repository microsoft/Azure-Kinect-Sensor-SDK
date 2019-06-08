/** \file color.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef COLOR_H
#define COLOR_H

#include <k4a/k4atypes.h>
#include <k4ainternal/handle.h>
#include <k4ainternal/common.h>
#include <k4ainternal/color_mcu.h>
#include <k4ainternal/depth_mcu.h>
#include <azure_c_shared_utility/tickcounter.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Delivers a sample to the registered callback function when a capture is ready for processing.
 *
 * \param result
 * inidicates if the capture being passed in is complete
 *
 * \param capture_handle
 * Capture being read by hardware.
 *
 * \param callback_context
 * Context for the callback function
 *
 * \remarks
 * Capture is only of one type. At this point it is not linked to other captures. Capture is safe to use durring this
 * callback as the caller ensures a ref is held. If the callback function wants the capture to exist beyond this
 * callback, a ref must be taken with capture_inc_ref().
 */
typedef void(color_cb_streaming_capture_t)(k4a_result_t result, k4a_capture_t capture_handle, void *callback_context);

/** Handle to the color device.
 *
 * Handles are created with \ref color_create and closed
 * with \ref color_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(color_t);

/** Open a handle to the color device.
 *
 * \param tick_handle [IN]
 * Handle to access the system tick
 *
 * \param container_id [IN]
 * container id associated with the device being using by this instance of k4a_device_open
 *
 * \param serial_number [IN]
 * serial number associated with the device being using by this instance of k4a_device_open
 *
 * \param capture_ready_cb [IN]
 *    A function pointer to call when new depth captures are ready
 *
 * \param capture_ready_cb_context [IN]
 *    A context to go with the depth_capture_ready callback function
 *
 * \param color_handle
 *    A pointer to write the opened color device handle to
 *
 * \return K4A_RESULT_SUCCEEDED if the device was opened, otherwise an error code
 *
 * If successful, \ref color_create will return a color device handle in the color
 * parameter. This handle grants exclusive access to the device and may be used in
 * the other k4a API calls.
 *
 * When done with the device, close the handle with \ref color_destroy
 */
k4a_result_t color_create(TICK_COUNTER_HANDLE tick_handle,
                          const guid_t *container_id,
                          const char *serial_number,
                          color_cb_streaming_capture_t capture_ready_cb,
                          void *capture_ready_cb_context,
                          color_t *color_handle);

/** Closes the handle to the color device.
 *
 * \param color_handle
 * Handle to close
 */
void color_destroy(color_t color_handle);

/** Starts the color camera streaming
 *
 * \param color_handle
 * Handle to the color camera
 *
 * \param config
 * The configuration settings for the color camera being started
 *
 * \return ::K4A_RESULT_SUCCESS if successful. ::K4A_RESULT_FAILED if an error occurs. If this API's fails then there is
 * not need to call /ref color_stop
 */
k4a_result_t color_start(color_t color_handle, const k4a_device_configuration_t *config);

/** Stops the color camera when it is currently streaming
 *
 * \param color_handle
 * Handle to the color camera to stop
 *
 * \ref color_destroy implicitly stops the color camera when /ref color_destroy is called.
 */
void color_stop(color_t color_handle);

/** Returns the system tick count saved by the color camera when it was started.
 *
 * \param color_handle
 * Handle to the color camera
 *
 * \return the system tick count of when the color camera was started. zero is returned if the color camera is not
 * actively streaming
 */
tickcounter_ms_t color_get_sensor_start_time_tick(const color_t color_handle);

/** Gets the capabilities of the given color camera's control command setting.
 *
 * \param color_handle
 * Handle to the color camera
 *
 * \param command
 * The targeted color control command to read the the value for.
 *
 * \param supports_auto
 * Location to store whether the color sensor's control support auto mode or not.
 * true if it supports auto mode, otherwise false.
 *
 * \param min_value
 * Location to store the color sensor's control minimum value of /p command.
 *
 * \param max_value
 * Location to store the color sensor's control maximum value of /p command.
 *
 * \param step_value
 * Location to store the color sensor's control step value of /p command.
 *
 * \param default_value
 * Location to store the color sensor's control default value of /p command.
 *
 * \param default_mode
 * Location to store the color sensor's control default mode of /p command.
 *
 * \return ::K4A_RESULT_FAILED if the value could not be read. ::K4A_RESULT_SUCCEEDED if successful. Details of the
 * error can be read from the debug output
 */
k4a_result_t color_get_control_capabilities(const color_t color_handle,
                                            const k4a_color_control_command_t command,
                                            bool *supports_auto,
                                            int32_t *min_value,
                                            int32_t *max_value,
                                            int32_t *step_value,
                                            int32_t *default_value,
                                            k4a_color_control_mode_t *default_mode);

/** Gets the value of the given color camera's control command setting.
 *
 * \param color_handle
 * Handle to the color camera
 *
 * \param command
 * The targeted color control command to read the the value for.
 *
 * \param mode
 * The mode of the command in question.
 *
 * \param value
 * A pointer to the location to write to fetched value to.
 *
 * \return ::K4A_RESULT_FAILED if the value could not be read. ::K4A_RESULT_SUCCEEDED if successful. Details of the
 * error can be read from the debug output
 */
k4a_result_t color_get_control(const color_t color_handle,
                               const k4a_color_control_command_t command,
                               k4a_color_control_mode_t *mode,
                               int32_t *value);

/** Sets the value of the given color camera's control command setting.
 *
 * \param color_handle
 * Handle to the color camera
 *
 * \param command
 * The targeted color control command to write the the value to.
 *
 * \param mode
 * The mode of the command in question.
 *
 * \param value
 * The value to write to the control command setting.
 *
 * \return ::K4A_RESULT_FAILED if the value could not be set. ::K4A_RESULT_SUCCEEDED if successful. Details of the
 * error can be read from the debug output
 */
k4a_result_t color_set_control(const color_t color_handle,
                               const k4a_color_control_command_t command,
                               const k4a_color_control_mode_t mode,
                               int32_t value);

#ifdef __cplusplus
}
#endif

#endif /* COLOR_H */
