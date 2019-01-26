/** \file capturesync.h
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
k4a_result_t capturesync_start(capturesync_t capturesync_handle, k4a_device_configuration_t *config);

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

/** TODO
 */
void capturesync_add_capture(capturesync_t capturesync_handle,
                             k4a_result_t result,
                             k4a_capture_t capture_raw,
                             bool color_capture);

/** TODO remove this hacky API
 *
 * \param capturesync_handle
 * The capturesync handle from capturesync_create()
 *
 * Private temporary function to disable hardware hacks so that unit tests can keep running successfully
 */
void private_capturesync_disable_hardware_hacks(capturesync_t capturesync_handle);

#ifdef __cplusplus
}
#endif

#endif /* CAPTURESYNC_H */
