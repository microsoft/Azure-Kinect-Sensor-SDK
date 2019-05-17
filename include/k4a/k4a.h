/** \file k4a.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef K4A_H
#define K4A_H

#ifdef __cplusplus
#include <cinttypes>
#else
#include <inttypes.h>
#endif
#include <k4a/k4aversion.h>
#include <k4a/k4atypes.h>
#include <k4a/k4a_export.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup Functions Functions
 * \ingroup csdk
 *
 * Public functions of the API
 *
 * @{
 */

/** Gets the number of connected devices
 *
 * \returns Number of sensors connected to the PC.
 *
 * \relates k4a_device_t
 *
 * \remarks
 * This API counts the number of Azure Kinect devices connected to the host PC.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT uint32_t k4a_device_get_installed_count(void);

/** Sets and clears the callback function to recieve debug messages from the Azure Kinect device.
 *
 * \param message_cb
 * The callback function to recieve messages from. Set to NULL to unregister the callback function.
 *
 * \param message_cb_context
 * The callback functions context.
 *
 * \param min_level
 * The least critical error the user wants to be notified about.
 *
 * \return ::K4A_RESULT_SUCCEEDED if the callback function was set or cleared successfully. ::K4A_RESULT_FAILED if an
 * error is encountered or the callback function has already been set.
 *
 * \remarks
 * Call this function to set or clear the callback function that is used to deliver debug messages to the caller. This
 * callback may be called concurrently, it is up to the implementation of the callback function to ensure the
 * parallelization is handled.
 *
 * \remarks
 * Clearing the callback function will block until all pending calls to the callback function have completed.
 *
 * \remarks
 * To update \p min_level, \p k4a_set_debug_message_handler can be called with the same value \p message_cb and by
 * specifying a new \p min_level.
 *
 * \remarks
 * Logging provided via this API is independent of the logging controlled by the environmental variable controls \p
 * K4A_ENABLE_LOG_TO_STDOUT, \p K4A_ENABLE_LOG_TO_A_FILE, and \p K4A_LOG_LEVEL. However there is a slight change in
 * default behavior when using this function. By default, when \p k4a_set_debug_message_handler() has not been used to
 * register a message callback, the default for environmental variable controls is to send debug messages as if
 * K4A_ENABLE_LOG_TO_STDOUT=1 were set. If \p k4a_set_debug_message_handler registers a callback function before
 * k4a_device_open() is called, then the default for environmental controls is as if K4A_ENABLE_LOG_TO_STDOUT=0 was
 * specified. Physically specifying the environmental control will override the default.
 *
 * \p min_level
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_set_debug_message_handler(k4a_logging_message_cb_t *message_cb,
                                                      void *message_cb_context,
                                                      k4a_log_level_t min_level);

/** Open an Azure Kinect device.
 *
 * \param index
 * The index of the device to open, starting with 0. Optionally pass in #K4A_DEVICE_DEFAULT.
 *
 * \param device_handle
 * Output parameter which on success will return a handle to the device.
 *
 * \relates k4a_device_t
 *
 * \return ::K4A_RESULT_SUCCEEDED if the device was opened successfully.
 *
 * \remarks
 * If successful, k4a_device_open() will return a device handle in the device_handle parameter.
 * This handle grants exclusive access to the device and may be used in the other Azure Kinect API calls.
 *
 * \remarks
 * When done with the device, close the handle with k4a_device_close()
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_device_open(uint32_t index, k4a_device_t *device_handle);

/** Closes an Azure Kinect device.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \relates k4a_device_t
 *
 * \remarks Once closed, the handle is no longer valid.
 *
 * \remarks Before closing the handle to the device, ensure that all \ref k4a_capture_t captures have been released with
 * k4a_capture_release().
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_device_close(k4a_device_t device_handle);

/** Reads a sensor capture.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param capture_handle
 * If successful this contains a handle to a capture object. Caller must call k4a_capture_release() when its done using
 * this capture.
 *
 * \param timeout_in_ms
 * Specifies the time in milliseconds the function should block waiting for the capture. If set to 0, the function will
 * return without blocking. Passing a value of #K4A_WAIT_INFINITE will block indefinitely until data is available, the
 * device is disconnected, or another error occurs.
 *
 * \returns
 * ::K4A_WAIT_RESULT_SUCCEEDED if a capture is returned. If a capture is not available before the timeout elapses, the
 * function will return ::K4A_WAIT_RESULT_TIMEOUT. All other failures will return ::K4A_WAIT_RESULT_FAILED.
 *
 * \relates k4a_device_t
 *
 * \remarks
 * Gets the next capture in the streamed sequence of captures from the camera. If a new capture is not currently
 * available, this function will block until the timeout is reached. The SDK will buffer at least two captures worth
 * of data before dropping the oldest capture. Callers needing to capture all data need to ensure they read the data as
 * fast as the data is being produced on average.
 *
 * \remarks
 * Upon successfully reading a capture this function will return success and populate \p capture.
 * If a capture is not available in the configured \p timeout_in_ms, then the API will return ::K4A_WAIT_RESULT_TIMEOUT.
 *
 * \remarks
 * If the call is successful and a capture is returned, callers must call k4a_capture_release() to return the allocated
 * memory.
 *
 * \remarks
 * This function needs to be called while the device is in a running state;
 * after k4a_device_start_cameras() is called and before k4a_device_stop_cameras() is called.
 *
 * \remarks
 * This function returns an error when an internal problem is encountered; such as loss of the USB connection, inability
 * to allocate enough memory, and other unexpected issues. Any error returned by this function signals the end of
 * streaming data, and caller should stop the stream using k4a_device_stop_cameras().
 *
 * \remarks
 * If this function is waiting for data (non-zero timeout) when k4a_device_stop_cameras() or k4a_device_close() is
 * called on another thread, this function will return an error.
 *
 * \returns ::K4A_WAIT_RESULT_SUCCEEDED if a capture is returned. If a capture is not available before the timeout
 * elapses, the function will return ::K4A_WAIT_RESULT_TIMEOUT. All other failures will return ::K4A_WAIT_RESULT_FAILED.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 *
 */
K4A_EXPORT k4a_wait_result_t k4a_device_get_capture(k4a_device_t device_handle,
                                                    k4a_capture_t *capture_handle,
                                                    int32_t timeout_in_ms);

/** Reads an IMU sample.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param imu_sample
 * Pointer to the location for the API to write the IMU sample.
 *
 * \param timeout_in_ms
 * Specifies the time in milliseconds the function should block waiting for the sample. If set to 0, the function will
 * return without blocking. Passing a value of #K4A_WAIT_INFINITE will block indefinitely until data is available, the
 * device is disconnected, or another error occurs.
 *
 * \returns
 * ::K4A_WAIT_RESULT_SUCCEEDED if a sample is returned. If a sample is not available before the timeout elapses, the
 * function will return ::K4A_WAIT_RESULT_TIMEOUT. All other failures will return ::K4A_WAIT_RESULT_FAILED.
 *
 * \relates k4a_device_t
 *
 * \remarks
 * Gets the next sample in the streamed sequence of IMU samples from the device. If a new sample is not currently
 * available, this function will block until the timeout is reached. The API will buffer at least two camera capture
 * intervals worth of samples before dropping the oldest sample. Callers needing to capture all data need to ensure they
 * read the data as fast as the data is being produced on average.
 *
 * \remarks
 * Upon successfully reading a sample this function will return success and populate \p imu_sample.
 * If a sample is not available in the configured \p timeout_in_ms, then the API will return ::K4A_WAIT_RESULT_TIMEOUT.
 *
 * \remarks
 * This function needs to be called while the device is in a running state;
 * after k4a_device_start_imu() is called and before k4a_device_stop_imu() is called.
 *
 * \remarks
 * This function returns an error when an internal problem is encountered; such as loss of the USB connection, inability
 * to allocate enough memory, and other unexpected issues. Any error returned by this function signals the end of
 * streaming data, and caller should stop the stream using k4a_device_stop_imu().
 *
 * \remarks
 * If this function is waiting for data (non-zero timeout) when k4a_device_stop_imu() or k4a_device_close() is
 * called on another thread, this function will return an error.
 *
 * \remarks
 * The memory the IMU sample is written to is allocated and owned by the caller, so there is no need to call an Azure
 * Kinect API to free or release the sample.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_wait_result_t k4a_device_get_imu_sample(k4a_device_t device_handle,
                                                       k4a_imu_sample_t *imu_sample,
                                                       int32_t timeout_in_ms);

/** Create an empty capture object.
 *
 * \param capture_handle
 * Pointer to a location to store the handle.
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function to create a \ref k4a_capture_t handle for a new capture. Release it with k4a_capture_release().
 *
 * The new capture is created with a reference count of 1.
 *
 * \returns
 * Returns #K4A_RESULT_SUCCEEDED on success. Errors are indicated with #K4A_RESULT_FAILED and error specific data can be
 * found in the log.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_capture_create(k4a_capture_t *capture_handle);

/** Release a capture.
 *
 * \param capture_handle
 * Capture to release.
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function when finished using the capture.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_capture_release(k4a_capture_t capture_handle);

/** Add a reference to a capture.
 *
 * \param capture_handle
 * Capture to add a reference to.
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function to add an additional reference to a capture. This reference must be removed with
 * k4a_capture_release().
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_capture_reference(k4a_capture_t capture_handle);

/** Get the color image associated with the given capture.
 *
 * \param capture_handle
 * Capture handle containing the image.
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function to access the color image part of this capture. Release the \ref k4a_image_t with
 * k4a_image_release();
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_image_t k4a_capture_get_color_image(k4a_capture_t capture_handle);

/** Get the depth image associated with the given capture.
 *
 * \param capture_handle
 * Capture handle containing the image.
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function to access the depth image part of this capture. Release the \ref k4a_image_t with
 * k4a_image_release();
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_image_t k4a_capture_get_depth_image(k4a_capture_t capture_handle);

/** Get the IR image associated with the given capture.
 *
 * \param capture_handle
 * Capture handle containing the image.
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function to access the IR image part of this capture. Release the \ref k4a_image_t with
 * k4a_image_release();
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_image_t k4a_capture_get_ir_image(k4a_capture_t capture_handle);

/** Set or add a color image to the associated capture.
 *
 * \param capture_handle
 * Capture handle to hold the image.
 *
 * \param image_handle
 * Image handle containing the image.
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * When a \ref k4a_image_t is added to a \ref k4a_capture_t, the \ref k4a_capture_t will automatically add a reference
 * to the \ref k4a_image_t.
 *
 * \remarks
 * If there is already a color image contained in the capture, the existing image will be dereferenced and replaced with
 * the new image.
 *
 * \remarks
 * To remove a color image to the capture without adding a new image, this function can be called with a NULL
 * image_handle.
 *
 * \remarks
 * Any \ref k4a_image_t contained in this \ref k4a_capture_t will automatically be dereferenced when all references to
 * the \ref k4a_capture_t are released with k4a_capture_release().
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_capture_set_color_image(k4a_capture_t capture_handle, k4a_image_t image_handle);

/** Set or add a depth image to the associated capture.
 *
 * \param capture_handle
 * Capture handle to hold the image.
 *
 * \param image_handle
 * Image handle containing the image.
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * When a \ref k4a_image_t is added to a \ref k4a_capture_t, the \ref k4a_capture_t will automatically add a reference
 * to the \ref k4a_image_t.
 *
 * \remarks
 * If there is already an image depth image contained in the capture, the existing image will be dereferenced and
 * replaced with the new image.
 *
 * \remarks
 * To remove a depth image to the capture without adding a new image, this function can be called with a NULL
 * image_handle.
 *
 * \remarks
 * Any \ref k4a_image_t contained in this \ref k4a_capture_t will automatically be dereferenced when all references to
 * the \ref k4a_capture_t are released with k4a_capture_release().
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_capture_set_depth_image(k4a_capture_t capture_handle, k4a_image_t image_handle);

/** Set or add an IR image to the associated capture.
 *
 * \param capture_handle
 * Capture handle to hold the image.
 *
 * \param image_handle
 * Image handle containing the image.
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * When a \ref k4a_image_t is added to a \ref k4a_capture_t, the \ref k4a_capture_t will automatically add a reference
 * to the \ref k4a_image_t.
 *
 * \remarks
 * If there is already an IR image contained in the capture, the existing image will be dereferenced and replaced with
 * the new image.
 *
 * \remarks
 * To remove a IR image to the capture without adding a new image, this function can be called with a NULL image_handle.
 *
 * \remarks
 * Any \ref k4a_image_t contained in this \ref k4a_capture_t will automatically be dereferenced when all references to
 * the \ref k4a_capture_t are released with k4a_capture_release().
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_capture_set_ir_image(k4a_capture_t capture_handle, k4a_image_t image_handle);

/** Set the temperature associated with the capture.
 *
 * \param capture_handle
 * Capture handle to set the temperature on.
 *
 * \param temperature_c
 * Temperature in Celsius to store.
 *
 * \relates k4a_capture_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_capture_set_temperature_c(k4a_capture_t capture_handle, float temperature_c);

/** Get the temperature associated with the capture.
 *
 * \param capture_handle
 * Capture handle to retrieve the temperature from.
 *
 * \return
 * This function returns the temperature of the device at the time of the capture in Celsius. If
 * the temperature is unavailable, the function will return NAN.
 *
 * \relates k4a_capture_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT float k4a_capture_get_temperature_c(k4a_capture_t capture_handle);

/** Create an image.
 *
 * \param format
 * The format of the image that will be stored in this image container.
 *
 * \param width_pixels
 * Width in pixels.
 *
 * \param height_pixels
 * Height in pixels.
 *
 * \param stride_bytes
 * The number of bytes per horizontal line of the image.
 *
 * \param image_handle
 * Pointer to store image handle in.
 *
 * \remarks
 * This function is used to create images of formats that have consistent stride. The function is not suitable for
 * compressed formats that may not be represented by the same number of bytes per line.
 *
 * \remarks
 * The function will allocate an image buffer of size \p height_pixels * \p stride_bytes.
 *
 * \remarks
 * To create an image object without the API allocating memory, or to represent an image that has a non-deterministic
 * stride, use k4a_image_create_from_buffer().
 *
 * \remarks
 * The \ref k4a_image_t is created with a reference count of 1.
 *
 * \remarks
 * When finished using the created image, release it with k4a_image_release.
 *
 * \returns
 * Returns #K4A_RESULT_SUCCEEDED on success. Errors are indicated with #K4A_RESULT_FAILED.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_image_create(k4a_image_format_t format,
                                         int width_pixels,
                                         int height_pixels,
                                         int stride_bytes,
                                         k4a_image_t *image_handle);

/** Create an image from a pre-allocated buffer.
 *
 * \param format
 * The format of the image that will be stored in this image container.
 *
 * \param width_pixels
 * Width in pixels.
 *
 * \param height_pixels
 * Height in pixels.
 *
 * \param stride_bytes
 * The number of bytes per horizontal line of the image.
 *
 * \param buffer
 * Pointer to a pre-allocated image buffer.
 *
 * \param buffer_size
 * Size in bytes of the pre-allocated image buffer.
 *
 * \param buffer_release_cb
 * Callback to the buffer free function, called when all references to the buffer have been released. This parameter is
 * optional.
 *
 * \param buffer_release_cb_context
 * Context for the buffer free function. This value will be called as a parameter to \p buffer_release_cb when
 * the callback is invoked.
 *
 * \param image_handle
 * Pointer to store image handle in.
 *
 * \remarks
 * This function creates a \ref k4a_image_t from a pre-allocated buffer. When all references to this object reach zero
 * the provided \p buffer_release_cb callback function is called so that the memory can be released. If this function
 * fails, the API will not use the memory provided in \p buffer, and the API will not call \p buffer_release_cb.
 *
 * \remarks
 * The \ref k4a_image_t is created with a reference count of 1.
 *
 * \remarks
 * Release the reference on this function with k4a_image_release().
 *
 * \returns
 * Returns #K4A_RESULT_SUCCEEDED on success. Errors are indicated with #K4A_RESULT_FAILED and error specific data can be
 * found in the log.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_image_create_from_buffer(k4a_image_format_t format,
                                                     int width_pixels,
                                                     int height_pixels,
                                                     int stride_bytes,
                                                     uint8_t *buffer,
                                                     size_t buffer_size,
                                                     k4a_memory_destroy_cb_t *buffer_release_cb,
                                                     void *buffer_release_cb_context,
                                                     k4a_image_t *image_handle);

/** Get the image buffer.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Use this buffer to access the raw image data.
 *
 * \returns
 * The function will return NULL if there is an error, and will normally return a pointer to the image buffer.
 * Since all \ref k4a_image_t instances are created with an image buffer, this function should only return NULL if the
 * \p image_handle is invalid.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT uint8_t *k4a_image_get_buffer(k4a_image_t image_handle);

/** Get the image buffer size.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Use this function to know what the size of the image buffer is returned by k4a_image_get_buffer().
 *
 * \returns
 * The function will return 0 if there is an error, and will normally return the image size.
 * Since all \ref k4a_image_t instances are created with an image buffer, this function should only return 0 if the
 * \p image_handle is invalid.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT size_t k4a_image_get_size(k4a_image_t image_handle);

/** Get the format of the image.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Use this function to determine the format of the image buffer.
 *
 * \returns
 * This function is not expected to fail, all \ref k4a_image_t's are created with a known format. If the
 * \p image_handle is invalid, the function will return ::K4A_IMAGE_FORMAT_CUSTOM.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_image_format_t k4a_image_get_format(k4a_image_t image_handle);

/** Get the image width in pixels.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \returns
 * This function is not expected to fail, all \ref k4a_image_t's are created with a known width. If the \p
 * image_handle is invalid, the function will return 0.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT int k4a_image_get_width_pixels(k4a_image_t image_handle);

/** Get the image height in pixels.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \returns
 * This function is not expected to fail, all \ref k4a_image_t's are created with a known height. If the \p
 * image_handle is invalid, the function will return 0.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT int k4a_image_get_height_pixels(k4a_image_t image_handle);

/** Get the image stride in bytes.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \returns
 * This function is not expected to fail, all \ref k4a_image_t's are created with a known stride. If the
 * \p image_handle is invalid, or the image's format does not have a stride, the function will return 0.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT int k4a_image_get_stride_bytes(k4a_image_t image_handle);

/** Get the image timestamp in microseconds
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the timestamp of the image. Timestamps are recorded by the device and represent the mid-point of exposure.
 * They may be used for relative comparison, but their absolute value has no defined meaning.
 *
 * \returns
 * If the \p image_handle is invalid or if no timestamp was set for the image,
 * this function will return 0. It is also possible for 0 to be a valid timestamp originating from the beginning
 * of a recording or the start of streaming.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT uint64_t k4a_image_get_timestamp_usec(k4a_image_t image_handle);

/** Get the image exposure in microseconds.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns an exposure time in microseconds. This is only supported on color image formats.
 *
 * \returns
 * If the \p image_handle is invalid, or no exposure was set on the image, the function will return 0. Otherwise,
 * it will return the image exposure time in microseconds.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT uint64_t k4a_image_get_exposure_usec(k4a_image_t image_handle);

/** Get the image white balance.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the image's white balance. This function is only valid for color captures, and not for depth or IR captures.
 *
 * \returns
 * Returns the image white balance in Kelvin. If \p image_handle is invalid, or the white balance was not set or
 * not applicable to the image, the function will return 0.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT uint32_t k4a_image_get_white_balance(k4a_image_t image_handle);

/** Get the image ISO speed.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * This function is only valid for color captures, and not for depth  or IR captures.
 *
 * \returns
 * Returns the ISO speed of the image. 0 indicates the ISO speed was not available or an error occurred.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT uint32_t k4a_image_get_iso_speed(k4a_image_t image_handle);

/** Set the time stamp, in microseconds, of the image.
 *
 * \param image_handle
 * Handle of the image to set the timestamp on.
 *
 * \param timestamp_usec
 * Timestamp of the image in microseconds.
 *
 * \remarks
 * Use this function in conjunction with k4a_image_create() or k4a_image_create_from_buffer() to construct a
 * \ref k4a_image_t.
 *
 * \remarks
 * Set a timestamp of 0 to indicate that the timestamp is not valid.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_image_set_timestamp_usec(k4a_image_t image_handle, uint64_t timestamp_usec);

/** Set the exposure time, in microseconds, of the image.
 *
 * \param image_handle
 * Handle of the image to set the exposure time on.
 *
 * \param exposure_usec
 * Exposure time of the image in microseconds.
 *
 * \remarks
 * Use this function in conjunction with k4a_image_create() or k4a_image_create_from_buffer() to construct a
 * \ref k4a_image_t. An exposure time of 0 is considered invalid. Only color image formats are expected to have a valid
 * exposure time.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_image_set_exposure_time_usec(k4a_image_t image_handle, uint64_t exposure_usec);

/** Set the white balance of the image.
 *
 * \param image_handle
 * Handle of the image to set the white balance on.
 *
 * \param white_balance
 * White balance of the image in degrees Kelvin.
 *
 * \remarks
 * Use this function in conjunction with k4a_image_create() or k4a_image_create_from_buffer() to construct a
 * \ref k4a_image_t. A white balance of 0 is considered invalid. White balance is only meaningful for color images,
 * and not expected on depth or IR images.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_image_set_white_balance(k4a_image_t image_handle, uint32_t white_balance);

/** Set the ISO speed of the image.
 *
 * \param image_handle
 * Handle of the image to set the ISO speed on.
 *
 * \param iso_speed
 * ISO speed of the image.
 *
 * \remarks
 * Use this function in conjunction with k4a_image_create() or k4a_image_create_from_buffer() to construct a
 * \ref k4a_image_t. An ISO speed of 0 is considered invalid. Only color images are expected to have a valid ISO speed.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_image_set_iso_speed(k4a_image_t image_handle, uint32_t iso_speed);

/** Add a reference to the k4a_image_t.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * References manage the lifetime of the object. When the references reach zero the object is destroyed. A caller must
 * not access the object after its reference is released.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_image_reference(k4a_image_t image_handle);

/** Remove a reference from the k4a_image_t.
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * References manage the lifetime of the object. When the references reach zero the object is destroyed. A caller must
 * not access the object after its reference is released.
 *
 * \relates k4a_image_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_image_release(k4a_image_t image_handle);

/** Starts color and depth camera capture.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param config
 * The configuration we want to run the device in. This can be initialized with ::K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED is returned on success.
 *
 * \relates k4a_device_t
 *
 * \remarks
 * Individual sensors configured to run will now start to stream captured data.
 *
 * \remarks
 * It is not valid to call k4a_device_start_cameras() a second time on the same \ref k4a_device_t until
 * k4a_device_stop_cameras() has been called.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_device_start_cameras(k4a_device_t device_handle, k4a_device_configuration_t *config);

/** Stops the color and depth camera capture.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \relates k4a_device_t
 *
 * \remarks
 * The streaming of individual sensors stops as a result of this call. Once called, k4a_device_start_cameras() may
 * be called again to resume sensor streaming.
 *
 * \remarks
 * This function may be called while another thread is blocking in k4a_device_get_capture().
 * Calling this function while another thread is in that function will result in that function returning a failure.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_device_stop_cameras(k4a_device_t device_handle);

/** Starts the IMU sample stream.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED is returned on success. ::K4A_RESULT_FAILED if the sensor is already running or a failure is
 * encountered
 *
 * \relates k4a_device_t
 *
 * \remarks
 * Call this API to start streaming IMU data. It is not valid to call this function a second time on the same
 * \ref k4a_device_t until k4a_device_stop_imu() has been called.
 *
 * \remarks
 * This function is dependent on the state of the cameras. The color or depth camera must be started before the IMU.
 * ::K4A_RESULT_FAILED will be returned if one of the cameras is not running.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_device_start_imu(k4a_device_t device_handle);

/** Stops the IMU capture.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \relates k4a_device_t
 *
 * \remarks
 * The streaming of the IMU stops as a result of this call. Once called, k4a_device_start_imu() may
 * be called again to resume sensor streaming, so long as the cameras are running.
 *
 * \remarks
 * This function may be called while another thread is blocking in k4a_device_get_imu_sample().
 * Calling this function while another thread is in that function will result in that function returning a failure.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_device_stop_imu(k4a_device_t device_handle);

/** Get the Azure Kinect device serial number.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param serial_number
 * Location to write the serial number to. If the function returns ::K4A_BUFFER_RESULT_SUCCEEDED, this will be a NULL
 * terminated string of ASCII characters. If this input is NULL \p serial_number_size will still be updated to return
 * the size of the buffer needed to store the string.
 *
 * \param serial_number_size
 * On input, the size of the \p serial_number buffer if that pointer is not NULL. On output, this value is set to the
 * actual number of bytes in the serial number (including the null terminator).
 *
 * \returns
 * A return of ::K4A_BUFFER_RESULT_SUCCEEDED means that the \p serial_number has been filled in. If the buffer is too
 * small the function returns ::K4A_BUFFER_RESULT_TOO_SMALL and the size of the serial number is
 * returned in the \p serial_number_size parameter. All other failures return ::K4A_BUFFER_RESULT_FAILED.
 *
 * \relates k4a_device_t
 *
 * \remarks
 * Queries the device for its serial number. If the caller needs to know the size of the serial number to allocate
 * memory, the function should be called once with a NULL \p serial_number to get the needed size in the \p
 * serial_number_size output, and then again with the allocated buffer.
 *
 * \remarks
 * Only a complete serial number will be returned. If the caller's buffer is too small, the function will return
 * ::K4A_BUFFER_RESULT_TOO_SMALL without returning any data in \p serial_number.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_buffer_result_t k4a_device_get_serialnum(k4a_device_t device_handle,
                                                        char *serial_number,
                                                        size_t *serial_number_size);

/** Get the version numbers of the device's subsystems.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param version
 * Location to write the version info to.
 *
 * \returns A return of ::K4A_RESULT_SUCCEEDED means that the version structure has been filled in.
 * All other failures return ::K4A_RESULT_FAILED.
 *
 * \relates k4a_device_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_device_get_version(k4a_device_t device_handle, k4a_hardware_version_t *version);

/** Get the Azure Kinect color sensor control capabilities.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param command
 * Color sensor control command.
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
 * \returns
 * ::K4A_RESULT_SUCCEEDED if the value was successfully returned, ::K4A_RESULT_FAILED if an error occurred
 *
 * \relates k4a_device_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_device_get_color_control_capabilities(k4a_device_t device_handle,
                                                                  k4a_color_control_command_t command,
                                                                  bool *supports_auto,
                                                                  int32_t *min_value,
                                                                  int32_t *max_value,
                                                                  int32_t *step_value,
                                                                  int32_t *default_value,
                                                                  k4a_color_control_mode_t *default_mode);

/** Get the Azure Kinect color sensor control
 * value.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param command
 * Color sensor control command.
 *
 * \param mode
 * Location to store the color sensor's control mode. This mode represents whether the command is in automatic or
 * manual mode.
 *
 * \param value
 * Location to store the color sensor's control value. This value is always written, but is only valid when the \p
 * mode returned is ::K4A_COLOR_CONTROL_MODE_MANUAL for the current \p command.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if the value was successfully returned, ::K4A_RESULT_FAILED if an error occurred
 *
 * \remarks
 * Each control command may be set to manual or automatic. See the definition of \ref k4a_color_control_command_t on
 * how to interpret the \p value for each command.
 *
 * \remarks
 * Some control commands are only supported in manual mode. When a command is in automatic mode, the \p value for
 * that command is not valid.
 *
 * \remarks
 * Control values set on a device are reset only when the device is power cycled. The device will retain the
 * settings even if the \ref k4a_device_t is closed or the application is restarted.
 *
 * \relates k4a_device_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_device_get_color_control(k4a_device_t device_handle,
                                                     k4a_color_control_command_t command,
                                                     k4a_color_control_mode_t *mode,
                                                     int32_t *value);

/** Set the Azure Kinect color sensor control value.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param command
 * Color sensor control command.
 *
 * \param mode
 * Color sensor control mode to set. This mode represents whether the command is in automatic or manual mode.
 *
 * \param value
 * Value to set the color sensor's control to. The value is only valid if \p mode
 * is set to ::K4A_COLOR_CONTROL_MODE_MANUAL, and is otherwise ignored.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if the value was successfully set, ::K4A_RESULT_FAILED if an error occurred
 *
 * \remarks
 * Each control command may be set to manual or automatic. See the definition of \ref k4a_color_control_command_t on how
 * to interpret the \p value for each command.
 *
 * \remarks
 * Some control commands are only supported in manual mode. When a command is in automatic mode, the \p value for that
 * command is not valid.
 *
 * \remarks
 * Control values set on a device are reset only when the device is power cycled. The device will retain the settings
 * even if the \ref k4a_device_t is closed or the application is restarted.
 *
 * \relates k4a_device_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_device_set_color_control(k4a_device_t device_handle,
                                                     k4a_color_control_command_t command,
                                                     k4a_color_control_mode_t mode,
                                                     int32_t value);

/** Get the raw calibration blob for the entire Azure Kinect device.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param data
 * Location to write the calibration data to. This field may optionally be set to NULL for the caller to query for
 * the needed data size.
 *
 * \param data_size
 * On passing \p data_size into the function this variable represents the available size of the \p data
 * buffer. On return this variable is updated with the amount of data actually written to the buffer, or the size
 * required to store the calibration buffer if \p data is NULL.
 *
 * \returns
 * ::K4A_BUFFER_RESULT_SUCCEEDED if \p data was successfully written. If \p data_size points to a buffer size that is
 * too small to hold the output or \p data is NULL, ::K4A_BUFFER_RESULT_TOO_SMALL is returned and \p data_size is
 * updated to contain the minimum buffer size needed to capture the calibration data.
 *
 * \relates k4a_device_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_buffer_result_t k4a_device_get_raw_calibration(k4a_device_t device_handle,
                                                              uint8_t *data,
                                                              size_t *data_size);

/** Get the camera calibration for the entire Azure Kinect device.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param depth_mode
 * Mode in which depth camera is operated.
 *
 * \param color_resolution
 * Resolution in which color camera is operated.
 *
 * \param calibration
 * Location to write the calibration
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p calibration was successfully written. ::K4A_RESULT_FAILED otherwise.
 *
 * \remarks
 * The \p calibration represents the data needed to transform between the camera views and may be
 * different for each operating \p depth_mode and \p color_resolution the device is configured to operate in.
 *
 * \remarks
 * The \p calibration output is used as input to all calibration and transformation functions.
 *
 * \see k4a_calibration_2d_to_2d()
 * \see k4a_calibration_2d_to_3d()
 * \see k4a_calibration_3d_to_2d()
 * \see k4a_calibration_3d_to_3d()
 *
 * \relates k4a_device_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_device_get_calibration(k4a_device_t device_handle,
                                                   const k4a_depth_mode_t depth_mode,
                                                   const k4a_color_resolution_t color_resolution,
                                                   k4a_calibration_t *calibration);

/** Get the device jack status for the synchronization in and synchronization out connectors.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param sync_in_jack_connected
 * Upon successful return this value will be set to true if a cable is connected to this sync in jack.
 *
 * \param sync_out_jack_connected
 * Upon successful return this value will be set to true if a cable is connected to this sync out jack.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if the connector status was successfully read.
 *
 * \relates k4a_device_t
 *
 * \remarks
 * If \p sync_out_jack_connected is true then \ref k4a_device_configuration_t wired_sync_mode mode can be set to \ref
 * K4A_WIRED_SYNC_MODE_STANDALONE or \ref K4A_WIRED_SYNC_MODE_MASTER. If \p sync_in_jack_connected is true then \ref
 * k4a_device_configuration_t wired_sync_mode mode can be set to \ref K4A_WIRED_SYNC_MODE_STANDALONE or \ref
 * K4A_WIRED_SYNC_MODE_SUBORDINATE.
 *
 * \see k4a_device_start_cameras()
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_device_get_sync_jack(k4a_device_t device_handle,
                                                 bool *sync_in_jack_connected,
                                                 bool *sync_out_jack_connected);

/** Get the camera calibration for a device from a raw calibration blob.
 *
 * \param raw_calibration
 * Raw calibration blob obtained from a device or recording. The raw calibration must be NULL terminated.
 *
 * \param raw_calibration_size
 * The size, in bytes, of raw_calibration including the NULL termination.
 *
 * \param depth_mode
 * Mode in which depth camera is operated.
 *
 * \param color_resolution
 * Resolution in which color camera is operated.
 *
 * \param calibration
 * Location to write the calibration.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p calibration was successfully written. ::K4A_RESULT_FAILED otherwise.
 *
 * \remarks
 * The \p calibration represents the data needed to transform between the camera views and is
 * different for each operating \p depth_mode and \p color_resolution the device is configured to operate in.
 *
 * \remarks
 * The \p calibration output is used as input to all transformation functions.
 *
 * \see k4a_calibration_2d_to_2d()
 * \see k4a_calibration_2d_to_3d()
 * \see k4a_calibration_3d_to_2d()
 * \see k4a_calibration_3d_to_3d()
 *
 * \relates k4a_device_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_calibration_get_from_raw(char *raw_calibration,
                                                     size_t raw_calibration_size,
                                                     const k4a_depth_mode_t depth_mode,
                                                     const k4a_color_resolution_t color_resolution,
                                                     k4a_calibration_t *calibration);

/** Transform a 3D point of a source coordinate system into a 3D point of the target coordinate system
 *
 * \param calibration
 * Location to read the camera calibration data.
 *
 * \param source_point3d_mm
 * The 3D coordinates in millimeters representing a point in \p source_camera.
 *
 * \param source_camera
 * The current camera.
 *
 * \param target_camera
 * The target camera.
 *
 * \param target_point3d_mm
 * Pointer to the output where the new 3D coordinates of the input point in the coordinate space of \p target_camera is
 * stored in millimeters.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p target_point3d_mm was successfully written. ::K4A_RESULT_FAILED if \p calibration
 * contained invalid transformation parameters.
 *
 * \remarks
 * This function is used to transform 3D points between depth and color camera coordinate systems. The function uses the
 * extrinsic camera calibration. It computes the output via multiplication with a precomputed matrix encoding a 3D
 * rotation and a 3D translation. If \p source_camera and \p target_camera are the same, then \p target_point3d_mm will
 * be identical to \p source_point3d_mm.
 *
 * \relates k4a_calibration_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_calibration_3d_to_3d(const k4a_calibration_t *calibration,
                                                 const k4a_float3_t *source_point3d_mm,
                                                 const k4a_calibration_type_t source_camera,
                                                 const k4a_calibration_type_t target_camera,
                                                 k4a_float3_t *target_point3d_mm);

/** Transform a 2D pixel coordinate with an associated depth value of the source camera into a 3D point of the target
 * coordinate system.
 *
 * \param calibration
 * Location to read the camera calibration obtained by k4a_device_get_calibration().
 *
 * \param source_point2d
 * The 2D pixel in \p source_camera coordinates.
 *
 * \param source_depth_mm
 * The depth of \p source_point2d in millimeters.
 *
 * \param source_camera
 * The current camera.
 *
 * \param target_camera
 * The target camera.
 *
 * \param target_point3d_mm
 * Pointer to the output where the 3D coordinates of the input pixel in the coordinate system of \p target_camera is
 * stored in millimeters.
 *
 * \param valid
 * The output parameter returns a value of 1 if the \p source_point2d is a valid coordinate, and will return 0 if
 * the coordinate is not valid in the calibration model.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p target_point3d_mm was successfully written. ::K4A_RESULT_FAILED if \p calibration
 * contained invalid transformation parameters. If the function returns ::K4A_RESULT_SUCCEEDED, but \p valid is 0,
 * the transformation was computed, but the results in \p target_point3d_mm are outside of the range of valid
 * calibration and should be ignored.
 *
 * \remarks
 * This function applies the intrinsic calibration of \p source_camera to compute the 3D ray from the focal point of the
 * camera through pixel \p source_point2d. The 3D point on this ray is then found using \p source_depth_mm. If \p
 * target_camera is different from \p source_camera, the 3D point is transformed to \p target_camera using
 * k4a_calibration_3d_to_3d(). In practice, \p source_camera and \p target_camera will often be identical. In this
 * case, no 3D to 3D transformation is applied.
 *
 * \remarks
 * If \p source_point2d is not considered as valid pixel coordinate
 * according to the intrinsic camera model, \p valid is set to 0. If it is valid, \p valid will be set to 1. The user
 * should not use the value of \p target_point3d_mm if \p valid was set to 0.
 *
 * \relates k4a_calibration_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_calibration_2d_to_3d(const k4a_calibration_t *calibration,
                                                 const k4a_float2_t *source_point2d,
                                                 const float source_depth_mm,
                                                 const k4a_calibration_type_t source_camera,
                                                 const k4a_calibration_type_t target_camera,
                                                 k4a_float3_t *target_point3d_mm,
                                                 int *valid);

/** Transform a 3D point of a source coordinate system into a 2D pixel coordinate of the target camera.
 *
 * \param calibration
 * Location to read the camera calibration obtained by k4a_device_get_calibration().
 *
 * \param source_point3d_mm
 * The 3D coordinates in millimeters representing a point in \p source_camera
 *
 * \param source_camera
 * The current camera.
 *
 * \param target_camera
 * The target camera.
 *
 * \param target_point2d
 * Pointer to the output where the 2D pixel in \p target_camera coordinates is stored.
 *
 * \param valid
 * The output parameter returns a value of 1 if the \p source_point3d_mm is a valid coordinate in the \p target_camera
 * coordinate system, and will return 0 if the coordinate is not valid in the calibration model.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p target_point2d was successfully written. ::K4A_RESULT_FAILED if \p calibration
 * contained invalid transformation parameters. If the function returns ::K4A_RESULT_SUCCEEDED, but \p valid is 0,
 * the transformation was computed, but the results in \p target_point2d are outside of the range of valid calibration
 * and should be ignored.
 *
 * \remarks
 * If \p target_camera is different from \p source_camera, \p source_point3d_mm is transformed to \p target_camera using
 * k4a_calibration_3d_to_3d(). In practice, \p source_camera and \p target_camera will often be identical. In this
 * case, no 3D to 3D transformation is applied. The 3D point in the coordinate system of \p target_camera is then
 * projected onto the image plane using the intrinsic calibration of \p target_camera.
 *
 * \remarks
 * If \p source_point3d_mm does not map to a valid 2D coordinate in the \p target_camera coordinate system, \p valid is
 * set to 0. If it is valid, \p valid will be set to 1. The user should not use the value of \p target_point2d if \p
 * valid was set to 0.
 *
 * \relates k4a_calibration_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_calibration_3d_to_2d(const k4a_calibration_t *calibration,
                                                 const k4a_float3_t *source_point3d_mm,
                                                 const k4a_calibration_type_t source_camera,
                                                 const k4a_calibration_type_t target_camera,
                                                 k4a_float2_t *target_point2d,
                                                 int *valid);

/** Transform a 2D pixel coordinate with an associated depth value of the source camera into a 2D pixel coordinate of
 * the target camera.
 *
 * \param calibration
 * Location to read the camera calibration obtained by k4a_device_get_calibration().
 *
 * \param source_point2d
 * The 2D pixel in \p source_camera coordinates.
 *
 * \param source_depth_mm
 * The depth of \p source_point2d in millimeters.
 *
 * \param source_camera
 * The current camera.
 *
 * \param target_camera
 * The target camera.
 *
 * \param target_point2d
 * The 2D pixel in \p target_camera coordinates.
 *
 * \param valid
 * The output parameter returns a value of 1 if the \p source_point2d is a valid coordinate in the \p target_camera
 * coordinate system, and will return 0 if the coordinate is not valid in the calibration model.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p target_point2d was successfully written. ::K4A_RESULT_FAILED if \p calibration
 * contained invalid transformation parameters. If the function returns ::K4A_RESULT_SUCCEEDED, but \p valid is 0,
 * the transformation was computed, but the results in \p target_point2d are outside of the range of valid calibration
 * and should be ignored.
 *
 * \remarks
 * This function maps a pixel between the coordinate systems of the depth and color cameras. It is equivalent to calling
 * k4a_calibration_2d_to_3d() to compute the 3D point corresponding to \p source_point2d and then using
 * k4a_calibration_3d_to_2d() to map the 3D point into the coordinate system of the \p target_camera.
 *
 * \remarks
 * If \p source_camera and \p target_camera are identical, the function immediately sets \p target_point2d to \p
 * source_point2d and returns without computing any transformations.
 *
 * \remarks
 * If \p source_point2d does not map to a valid 2D coordinate in the \p target_camera coordinate system, \p valid is set
 * to 0. If it is valid, \p valid will be set to 1. The user should not use the value of \p target_point2d if \p valid
 * was set to 0.
 *
 * \relates k4a_calibration_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_calibration_2d_to_2d(const k4a_calibration_t *calibration,
                                                 const k4a_float2_t *source_point2d,
                                                 const float source_depth_mm,
                                                 const k4a_calibration_type_t source_camera,
                                                 const k4a_calibration_type_t target_camera,
                                                 k4a_float2_t *target_point2d,
                                                 int *valid);

/** Get handle to transformation handle.
 *
 * \param calibration
 * A calibration structure obtained by k4a_device_get_calibration().
 *
 * \returns
 * A transformation handle. A NULL is returned if creation fails.
 *
 * \remarks
 * The transformation handle is used to transform images from the coordinate system of one camera into the other. Each
 * transformation handle requires some pre-computed resources to be allocated, which are retained until the handle is
 * destroyed.
 *
 * \remarks
 * The transformation handle must be destroyed with k4a_transformation_destroy() when it is no longer to be used.
 *
 * \relates k4a_calibration_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_transformation_t k4a_transformation_create(const k4a_calibration_t *calibration);

/** Destroy transformation handle.
 *
 * \param transformation_handle
 * Transformation handle to destroy.
 *
 * \relates k4a_transformation_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT void k4a_transformation_destroy(k4a_transformation_t transformation_handle);

/** Transforms the depth map into the geometry of the color camera.
 *
 * \param transformation_handle
 * Transformation handle.
 *
 * \param depth_image
 * Handle to input depth image.
 *
 * \param transformed_depth_image
 * Handle to output transformed depth image.
 *
 * \remarks
 * This produces a depth image for which each pixel matches the corresponding pixel coordinates of the color camera.
 *
 * \remarks
 * \p depth_image and \p transformed_depth_image must be of format ::K4A_IMAGE_FORMAT_DEPTH16.
 *
 * \remarks
 * \p transformed_depth_image must have a width and height matching the width and height of the color camera in the mode
 * specified by the \ref k4a_calibration_t used to create the \p transformation_handle with k4a_transformation_create().
 *
 * \remarks
 * The contents \p transformed_depth_image will be filled with the depth values derived from \p depth_image in the color
 * camera's coordinate space.
 *
 * \remarks
 * \p transformed_depth_image should be created by the caller using k4a_image_create() or
 * k4a_image_create_from_buffer().
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p transformed_depth_image was successfully written and ::K4A_RESULT_FAILED otherwise.
 *
 * \relates k4a_transformation_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_transformation_depth_image_to_color_camera(k4a_transformation_t transformation_handle,
                                                                       const k4a_image_t depth_image,
                                                                       k4a_image_t transformed_depth_image);

/** Transforms a color image into the geometry of the depth camera.
 *
 * \param transformation_handle
 * Transformation handle.
 *
 * \param depth_image
 * Handle to input depth image.
 *
 * \param color_image
 * Handle to input color image.
 *
 * \param transformed_color_image
 * Handle to output transformed color image.
 *
 * \remarks
 * This produces a color image for which each pixel matches the corresponding pixel coordinates of the depth camera.
 *
 * \remarks
 * \p depth_image and \p color_image need to represent the same moment in time. The depth data will be applied to the
 * color image to properly warp the color data to the perspective of the depth camera.
 *
 * \remarks
 * \p depth_image must be of type ::K4A_IMAGE_FORMAT_DEPTH16. \p color_image must be of format
 * ::K4A_IMAGE_FORMAT_COLOR_BGRA32.
 *
 * \remarks
 * \p transformed_color_image image must be of format ::K4A_IMAGE_FORMAT_COLOR_BGRA32. \p transformed_color_image must
 * have the width and height of the depth camera in the mode specified by the \ref k4a_calibration_t used to create
 * the \p transformation_handle with k4a_transformation_create().
 *
 * \remarks
 * \p transformed_color_image should be created by the caller using k4a_image_create() or
 * k4a_image_create_from_buffer().
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p transformed_color_image was successfully written and ::K4A_RESULT_FAILED otherwise.
 *
 * \relates k4a_transformation_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_transformation_color_image_to_depth_camera(k4a_transformation_t transformation_handle,
                                                                       const k4a_image_t depth_image,
                                                                       const k4a_image_t color_image,
                                                                       k4a_image_t transformed_color_image);

/** Transforms the depth image into 3 planar images representing X, Y and Z-coordinates of corresponding 3D points.
 *
 * \param transformation_handle
 * Transformation handle.
 *
 * \param depth_image
 * Handle to input depth image.
 *
 * \param camera
 * Geometry in which depth map was computed.
 *
 * \param xyz_image
 * Handle to output xyz image.
 *
 * \remarks
 * \p depth_image must be of format ::K4A_IMAGE_FORMAT_DEPTH16.
 *
 * \remarks
 * The \p camera parameter tells the function what the perspective of the \p depth_image is. If the \p depth_image was
 * captured directly from the depth camera, the value should be ::K4A_CALIBRATION_TYPE_DEPTH. If the \p depth_image is
 * the result of a transformation into the color camera's coordinate space using
 * k4a_transformation_depth_image_to_color_camera(), the value should be ::K4A_CALIBRATION_TYPE_COLOR.
 *
 * \remarks
 * The format of \p xyz_image must be ::K4A_IMAGE_FORMAT_CUSTOM. The width and height of \p xyz_image must match the
 * width and height of \p depth_image. \p xyz_image must have a stride in bytes of at least 6 times its width in pixels.
 *
 * \remarks
 * Each pixel of the \p xyz_image consists of three int16_t values, totaling 6 bytes. The three int16_t values are the
 * X, Y, and Z values of the point.
 *
 * \remarks
 * \p xyz_image should be created by the caller using k4a_image_create() or k4a_image_create_from_buffer().
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p xyz_image was successfully written and ::K4A_RESULT_FAILED otherwise.
 *
 * \relates k4a_transformation_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_EXPORT k4a_result_t k4a_transformation_depth_image_to_point_cloud(k4a_transformation_t transformation_handle,
                                                                      const k4a_image_t depth_image,
                                                                      const k4a_calibration_type_t camera,
                                                                      k4a_image_t xyz_image);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* K4A_H */
