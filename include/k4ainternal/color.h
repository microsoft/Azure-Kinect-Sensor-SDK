/** \file color.h
 * Kinect For Azure SDK.
 */

#ifndef COLOR_H
#define COLOR_H

#include <k4a/k4atypes.h>
#include <k4ainternal/handle.h>
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
 * \param colormcu
 * Handle to the color mcu for sending commands to.
 *
 * \param depthmcu
 * Handle to the depth mcu for sending commands to.
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
                          colormcu_t colormcu,
                          depthmcu_t depthmcu,
                          color_cb_streaming_capture_t capture_ready_cb,
                          void *capture_ready_cb_context,
                          color_t *color_handle);

/** Closes the color module and free's it resources
 * */
void color_destroy(color_t color_handle);

// TODO
k4a_result_t color_start(color_t color_handle, const k4a_device_configuration_t *config);

// TODO
void color_stop(color_t color_handle);

// TODO
tickcounter_ms_t color_get_sensor_start_time_tick(const color_t handle);

// TODO
k4a_result_t color_get_control(const color_t handle,
                               const k4a_color_control_command_t command,
                               k4a_color_control_mode_t *mode,
                               int32_t *value);

// TODO
k4a_result_t color_set_control(const color_t handle,
                               const k4a_color_control_command_t command,
                               const k4a_color_control_mode_t mode,
                               int32_t value);

#ifdef __cplusplus
}
#endif

#endif /* COLOR_H */
