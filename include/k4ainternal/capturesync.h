/** \file capturesync.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 *
 * Synchronize depth and color captures
 */

#ifndef CAPTURESYNC_H
#define CAPTURESYNC_H

#include <k4a/k4atypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handle to the capturesync module
 *
 * Handles are created with capturesync_create() and closed
 * with \ref color_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(capturesync_t);

/** Creates an capturesync instance
 *
 * \param capturesync_handle
 * pointer to a handle location to store the handle. This is only written on K4A_RESULT_SUCCEEDED;
 *
 * To cleanup this resource call capturesync_destroy().
 *
 * \ref K4A_RESULT_SUCCEEDED is returned on success
 */
k4a_result_t capturesync_create(capturesync_t *capturesync_handle);

/** Destroys an capturesync instance
 *
 * \param capturesync_handle
 * The capturesync handle to destroy
 *
 * This call only cleans up the capturesync handle.
 * This function should not be called until all outstanding ::k4a_capture_t objects are freed.
 *
 * This function should return 0 to indicate the number of outstanding allocations. Consider this the
 * number of leaked allocations.
 */
void capturesync_destroy(capturesync_t capturesync_handle);

/** Prepares the capturesync object for synchronizing color and depth captures
 *
 * \param capturesync_handle
 * The capturesync handle from capturesync_create()
 *
 * \param config
 * The device configuration provided by the caller
 *
 * \remarks
 * Enables the capturesync to enable its queues and begin synchronizing depth and color frames
 */
k4a_result_t capturesync_start(capturesync_t capturesync_handle, const k4a_device_configuration_t *config);

/** Prepares the capturesync object to stop synchronizing color and depth captures
 *
 * \param capturesync_handle
 * The capturesync handle from capturesync_create()
 *
 * \remarks
 * Closes queues and unblocks any waiters waiting for the queue
 */
void capturesync_stop(capturesync_t capturesync_handle);

/** Reads a sample from the synchronized capture queue
 *
 * \param capturesync_handle
 * The capturesync handle from capturesync_create()
 *
 * \param capture_handle
 * The location to write the capture handle to
 *
 * \remarks
 * Closes queues and unblocks any waiters waiting for the queue
 */
k4a_wait_result_t capturesync_get_capture(capturesync_t capturesync_handle,
                                          k4a_capture_t *capture_handle,
                                          int32_t timeout_in_ms);

/** Capturesync module asynchronously accepts new captures from color and depth modules through this API.
 *
 * \param capturesync_handle
 * The capturesync handle from capturesync_create()
 *
 * \param result
 * The result of the USB opperation providing the sample.
 *
 * \param capture_raw
 * A capture of a single color image, or a capture with upto 2 images; depth and IR.
 *
 * \param color_capture
 * True if the capture contains a single color capture. False is the capture contains upto 2 images; depth and IR.
 *
 * \remarks
 * If ::K4A_SUCCEEDED(result) is true then capture_raw must be valid. If ::K4A_SUCCEEDED(result) is false then
 * capture_raw is optional and may be NULL.
 */
void capturesync_add_capture(capturesync_t capturesync_handle,
                             k4a_result_t result,
                             k4a_capture_t capture_raw,
                             bool color_capture);

#ifdef __cplusplus
}
#endif

#endif /* CAPTURESYNC_H */
