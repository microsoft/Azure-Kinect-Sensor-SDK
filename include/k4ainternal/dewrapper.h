/** \file DEWRAPPER.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef DEWRAPPER_H
#define DEWRAPPER_H

#include <k4a/k4atypes.h>
#include <k4ainternal/handle.h>
#include <k4ainternal/depth_mcu.h>
#include <k4ainternal/calibration.h>
#include <k4ainternal/capture.h>

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
typedef void(dewrapper_streaming_capture_cb_t)(k4a_result_t result,
                                               k4a_capture_t capture_handle,
                                               void *callback_context);

/** Handle to the dewrapper device.
 *
 * Handles are created with \ref dewrapper_create and closed
 * with \ref dewrapper_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(dewrapper_t);

dewrapper_t dewrapper_create(k4a_calibration_camera_t *calibration,
                             dewrapper_streaming_capture_cb_t *capture_ready,
                             void *capture_ready_context);
void dewrapper_destroy(dewrapper_t dewrapper_handle);
k4a_result_t dewrapper_start(dewrapper_t dewrapper_handle,
                             const k4a_device_configuration_t *config,
                             uint8_t *calibration_memory,
                             size_t calibration_memory_size);
void dewrapper_stop(dewrapper_t dewrapper_handle);
void dewrapper_post_capture(k4a_result_t cb_result, k4a_capture_t capture_raw, void *context);

#ifdef __cplusplus
}
#endif

#endif /* DEWRAPPER_H */
