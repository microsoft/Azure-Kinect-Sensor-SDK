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

/** Gets the number of connected devices
 *
 * \returns number of sensors connected to the PC
 *
 * \remarks
 * This API counts the number of K4A devices connected to the host PC
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

/** Open a k4a device.
 *
 * \param index
 * The index of the device to open, starting with 0. Optionally pass in #K4A_DEVICE_DEFAULT.
 *
 * \param device_handle
 * Output parameter which on success will return a handle to the device
 *
 * \return ::K4A_RESULT_SUCCEEDED if the device was opened successfully
 *
 * \remarks
 * If successful, k4a_device_open() will return a device handle in the device_handle parameter.
 * This handle grants exclusive access to the device and may be used in the other k4a API calls.
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

/** Closes a k4a device.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open()
 *
 * \relates k4a_device_t
 *
 * \remarks Once closed, the handle is no longer valid.
 *
 * \remarks Before closing the handle to the device, ensure that all k4a_capture_t captures have been released with
 * k4a_capture_release()
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
 * If successful, this contains a handle to a capture object. Caller must call k4a_capture_release() when it's done
 * using this capture
 *
 * \param timeout_in_ms
 * Specifies the time in milliseconds the function should block waiting for the capture. 0 is a check of the queue
 * without blocking. Passing a value of #K4A_WAIT_INFINITE will block indefinitely.
 *
 * \returns
 * ::K4A_WAIT_RESULT_SUCCEEDED if a capture is returned. If a capture is not available before the timeout elapses, the
 * function will return ::K4A_WAIT_RESULT_TIMEOUT. All other failures will return ::K4A_WAIT_RESULT_FAILED.
 *
 * \relates k4a_device_t
 *
 * \remarks
 * Gets the next capture in the streamed sequence of captures from the camera. If a new capture is not currently
 * available, this function will block up until the timeout is reached. The SDK will buffer at least two captures worth
 * of data before dropping the oldest capture. Callers needing to capture all data need to ensure they call this
 * function at least once per capture interval on average. Capture data read must call k4a_capture_release() to return
 * the allocated memory to the SDK.
 *
 * \remarks
 * Upon successfully reading a capture this function will return success and populate \p capture.
 * If a capture is not available in the configured \p timeout_in_ms, then the API will return ::K4A_WAIT_RESULT_TIMEOUT.
 *
 * \remarks
 * This function returns an error when an internal problem is encountered, such as loss of the USB connection, a low
 * memory condition, or other unexpected issues. Once an error is returned, the API will continue to return an error
 * until k4a_device_stop_cameras() is called to clear the condition.
 *
 * \remarks
 * If this function is waiting for data (non-zero timeout) when k4a_device_stop_cameras() or k4a_device_close() is
 * called, this function will return an error. This function needs to be called while the device is in a running state;
 * after k4a_device_start_cameras() is called and before k4a_device_stop_cameras() is called.
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
 * \param imu_sample [out]
 * pointer to a location to write the IMU sample to
 *
 * \param timeout_in_ms
 * Specifies the time in milliseconds the function should block waiting for the IMU sample. 0 is a check of the queue
 * without blocking. Passing a value of #K4A_WAIT_INFINITE will block indefinitely.
 *
 * \returns
 * ::K4A_WAIT_RESULT_SUCCEEDED if a sample is returned. If a sample is not available before the timeout elapses, the
 * function will return ::K4A_WAIT_RESULT_TIMEOUT. All other failures will return ::K4A_WAIT_RESULT_FAILED.
 *
 * \relates k4a_device_t
 *
 * \remarks
 * Gets the next sample in the streamed sequence of samples from the device. If a new sample is not currently
 * available, this function will block up until the timeout is reached. The SDK will buffer at least two samples worth
 * of data before dropping the oldest sample. Callers needing to see all data must ensure they call this
 * function at least once per IMU sample interval on average.
 *
 * \remarks
 * Upon successfully reading a sample this function will return success and populate \p imu_sample.
 * If a sample is not available in the configured \p timeout_in_ms, then the API will return ::K4A_WAIT_RESULT_TIMEOUT.
 *
 * \remarks
 * This function returns an error when an internal problem is encountered; such as loss of the USB connection, a low
 * memory condition, or other unexpected issues. Once an error is returned, the API will continue to return an error
 * until k4a_device_stop_imu() is called to clear the condition.
 *
 * \remarks
 * If this function is waiting for data (non-zero timeout) when k4a_device_stop_imu() or k4a_device_close() is
 * called, this function will return an error. This function needs to be called while the device is in a running state;
 * after k4a_device_start_imu() is called and before k4a_device_stop_imu() is called.
 *
 * \remarks
 * There is no need to free the imu_sample after using imu_sample.
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

/** create an empty capture object
 *
 * \param capture_handle
 * Pointer to a location to write an empty capture handle
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function to create a capture handle. Release it with k4a_capture_release().
 *
 * k4a_capture_t is created with a reference of 1.
 *
 * \returns
 * Returns K4A_RESULT_SUCCEEDED on success. Errors are indicated with K4A_RESULT_FAILED and error specific data can be
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

/** Release a capture back to the SDK
 *
 * \param capture_handle
 * capture to return to SDK
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Called when the user is finished using the capture. All captures must be released prior to calling
 * k4a_device_close(), not doing so will result in undefined behavior.
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

/** Add a reference to a capture
 *
 * \param capture_handle
 * capture to add a reference to
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Called when the user wants to add an additional reference to a capture. This reference must be removed with
 * k4a_capture_release() to allow the capture to be released.
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

/** Get the color image associated with the given capture
 *
 * \param capture_handle
 * Capture handle containing the image
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function to access the given image. Release the image with k4a_image_release();
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

/** Get the depth image associated with the given capture
 *
 * \param capture_handle
 * Capture handle containing the image
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function to access the given image. Release the image with k4a_image_release();
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

/** Get the ir image associated with the given capture
 *
 * \param capture_handle
 * Capture handle containing the image
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * Call this function to access the given image. Release the image with k4a_image_release();
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

/** Set / add a color image to the associated capture
 *
 * \param capture_handle
 * Capture handle containing to hold the image
 *
 * \param image_handle
 * Image handle containing the image
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * If there is already an image of this type contained by the capture, it will be dropped. The caller can pass in a NULL
 * image to drop the existing image without having to add a new one. Calling capture_release() will also remove the
 * image reference the capture has on the image and may result in the image being freed.
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

/** Set / add a depth image to the associated capture
 *
 * \param capture_handle
 * Capture handle containing to hold the image
 *
 * \param image_handle
 * Image handle containing the image
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * If there is already an image of this type contained by the capture, it will be dropped. The caller can pass in a NULL
 * image to drop the existing image without having to add a new one. Calling capture_release() will also remove the
 * image reference the capture has on the image and may result in the image being freed.
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

/** Set / add an IR image to the associated capture
 *
 * \param capture_handle
 * Capture handle containing to hold the image
 *
 * \param image_handle
 * Image handle containing the image
 *
 * \relates k4a_capture_t
 *
 * \remarks
 * If there is already an image of this type contained by the capture, it will be dropped. The caller can pass in a NULL
 * image to drop the existing image without having to add a new one. Calling capture_release() will also remove the
 * image reference the capture has on the image and may result in the image being freed.
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
 * Capture handle for the temperature to modify
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
 * Capture handle for the temperature to access
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

/** Create an image
 *
 * \param format
 * The format of the image that will be stored in this image container
 *
 * \param width_pixels
 * width in pixels
 *
 * \param height_pixels
 * height in pixels
 *
 * \param stride_bytes
 * stride in bytes
 *
 * \param image_handle
 * pointer to store image handle in.
 *
 * \remarks
 * Call this API for image formats that have consistent stride, aka no compression. Image size is calculated by
 * height_pixels * stride_bytes. For advanced options use k4a_image_create_from_buffer().
 *
 * \remarks
 * k4a_image_t is created with a reference of 1.
 *
 * \remarks
 * Release the reference on this function with k4a_image_release
 *
 * \returns
 * Returns K4A_RESULT_SUCCEEDED on success. Errors are indicated with K4A_RESULT_FAILED and error specific data can be
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
K4A_EXPORT k4a_result_t k4a_image_create(k4a_image_format_t format,
                                         int width_pixels,
                                         int height_pixels,
                                         int stride_bytes,
                                         k4a_image_t *image_handle);

/** Create an image from a pre-allocated buffer
 *
 * \param format
 * The format of the image that will be stored in this image container
 *
 * \param width_pixels
 * width in pixels
 *
 * \param height_pixels
 * height in pixels
 *
 * \param stride_bytes
 * stride in bytes
 *
 * \param buffer
 * pointer to a pre-allocated image buffer
 *
 * \param buffer_size
 * size in bytes of the pre-allocated image buffer
 *
 * \param buffer_release_cb
 * buffer free function, called when all references to the buffer have been released
 *
 * \param buffer_release_cb_context
 * Context for the memory free function
 *
 * \param image_handle
 * pointer to store image handle in.
 *
 * \remarks
 * This function creates a k4a_image_t from a pre-allocated buffer. When all references to this object reach zero the
 * provided buffer_release_cb callback function is called to release the memory. If this function fails, then the caller
 * must free the pre-allocated memory, if this function succeeds, then the k4a_image_t is responsible for freeing the
 * memory. k4a_image_t is created with a reference of 1.
 *
 * \remarks
 * Release the reference on this function with k4a_image_release.
 *
 * \returns
 * Returns K4A_RESULT_SUCCEEDED on success. Errors are indicated with K4A_RESULT_FAILED and error specific data can be
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

/** Get the image buffer
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the image buffer
 *
 * \remarks
 * Use this buffer to access the raw image data.
 *
 * \returns
 * This function is not expected to return null, all k4a_image_t's are created with a buffer.
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

/** Get the image buffer size
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the size of the image buffer in bytes
 *
 * \remarks
 * Use this function to get the size of the image buffer returned by k4a_image_get_buffer()
 *
 * \returns
 * This function is not expected to return 0, all k4a_image_t's are created with a buffer.
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

/** Get the image format of the image
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the image format
 *
 * \remarks
 * Use this function to know what the image format is.
 *
 * \returns
 * This function is not expected to fail, all k4a_image_t's are created with a known format.
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

/** Get the image width in pixels
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the image width in pixels
 *
 * \returns
 * This function is not expected to fail, all k4a_image_t's are created with a known width.
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

/** Get the image height in pixels
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the image height in pixels
 *
 * \returns
 * This function is not expected to fail, all k4a_image_t's are created with a known height.
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

/** Get the image stride in bytes
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the image stride in bytes
 *
 * \returns
 * This function is not expected to fail, all k4a_image_t's are created with a known stride.
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
 * Returning a timestamp of 0 is considered an error.
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

/** Get the image exposure in microseconds
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns an exposure time in microseconds. This is only supported on color image formats.
 *
 * \returns
 * Returning an exposure of 0 is considered an error.
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

/** Get the image white balance
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the image's white balance. This function is only valid for color captures, and not for depth captures.
 *
 * \returns
 * White balance in Kelvin, 0 if image format is not supported by the given capture.
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

/** Get the image's ISO speed
 *
 * \param image_handle
 * Handle of the image for which the get operation is performed on.
 *
 * \remarks
 * Returns the image's ISO speed
 * This function is only valid for color captures, and not for depth captures.
 *
 * \returns
 * 0 indicates the ISO speed was not available or an error occurred.
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
 * k4a_image_t. A timestamp of 0 is not valid.
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
 * exposure time of the image in microseconds.
 *
 * \remarks
 * Use this function in conjunction with k4a_image_create() or k4a_image_create_from_buffer() to construct a
 * k4a_image_t. An exposure time of 0 is not valid. Only use this function with color image formats.
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
 * White balance of the image.
 *
 * \remarks
 * Use this function in conjunction with k4a_image_create() or k4a_image_create_from_buffer() to construct a
 * k4a_image_t. A white balance of 0 is not valid. Only use this function with color image formats.
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
 * k4a_image_t. An ISO speed of 0 is not valid. Only use this function with color image formats.
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
 * References manage the lifetime of the object. When the references reach zero the object is destroyed.
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
 * References manage the lifetime of the object. When the references reach zero the object is destroyed. Caller should
 * assume this function frees the object, as a result the object should not be accessed after this call completes.
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

/** Starts the K4A device's cameras
 *
 * \param device_handle
 * Handle obtained by k4a_device_open()
 *
 * \param config
 * The configuration we want to run the device in. This can be initialized with K4A_DEVICE_CONFIG_INIT_DISABLE_ALL.
 *
 * \returns
 * K4A_RESULT_SUCCEEDED is returned on success
 *
 * \relates k4a_device_t
 *
 * \remarks Individual sensors configured to run will now start stream capture data.
 *
 * \remarks
 * It is not valid to call k4a_device_start_cameras() a second time on the same k4a_device_t until
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

/** Stops the K4A device's cameras
 *
 * \param device_handle
 * Handle obtained by k4a_device_open()
 *
 * \relates k4a_device_t
 *
 * \remarks
 * The streaming of individual sensors stops as a result of this call. Once called, k4a_device_start_cameras() may
 * be called again to resume sensor streaming.
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

/** Starts the K4A IMU
 *
 * \param device_handle
 * Handle obtained by k4a_device_open()
 *
 * \returns
 * K4A_RESULT_SUCCEEDED is returned on success. ::K4A_RESULT_FAILED if the sensor is already running or a failure is
 * encountered
 *
 * \relates k4a_device_t
 *
 * \remarks
 * Call this API to start streaming IMU data. It is not valid to call this API a second time without calling stop after
 * the first call. This function is dependent on the state of the cameras. The color or depth camera must be started
 * before the IMU. K4A_RESULT_FAILED will be returned if one of the cameras is not running.
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

/** Stops the K4A IMU
 *
 * \param device_handle
 * Handle obtained by k4a_device_open()
 *
 * \relates k4a_device_t
 *
 * \remarks
 * The streaming of the imu stops as a result of this call. Once called, k4a_device_start_imu() may
 * be called again to resume sensor streaming. It is ok to call the API twice, if the IMU is not running nothing will
 * happen.
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

/** Get the K4A device serial number
 *
 * \param device_handle
 * Handle obtained by k4a_device_open()
 *
 * \param serial_number
 * Location to write the serial number to. On output, this will be an ASCII null terminated string. This value may be
 * NULL, so that \p serial_number_size may be used to return the size of the buffer needed to store the string.
 *
 * \param serial_number_size
 * On input, the size of the \p serial_number buffer. On output, this value is set to the actual number of bytes in the
 * serial number (including the null terminator)
 *
 * \returns
 * A return of ::K4A_BUFFER_RESULT_SUCCEEDED means that the \p serial_number has been filled in. If the buffer is too
 * small the function returns ::K4A_BUFFER_RESULT_TOO_SMALL and the needed size of the \p serial_number buffer is
 * returned in the \p serial_number_size parameter. All other failures return ::K4A_BUFFER_RESULT_FAILED.
 *
 * \relates k4a_device_t
 *
 * \remarks
 * Queries the device for its serial number. Set serial_number to NULL to query for serial number size required by the
 * API
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

/** Get the version numbers of the K4A subsystems' firmware
 *
 * \param device_handle
 * Handle obtained by k4a_device_open()
 *
 * \param version
 * Location to write the version info to
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

/** Get the K4A color sensor control value
 *
 * \param device_handle
 * Handle obtained by k4a_device_open()
 *
 * \param command
 * Color sensor control command
 *
 * \param mode
 * Color sensor control mode (auto / manual)
 *
 * \param value
 * Color sensor control value. Only valid when mode is manual
 *
 * \returns
 * K4A_RESULT_SUCCEEDED if the value was successfully returned, K4A_RESULT_FAILED if an error occurred
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

/** Set the K4A color sensor control value
 *
 * \param device_handle
 * Handle obtained by k4a_device_open()
 *
 * \param command
 * Color sensor control command
 *
 * \param mode
 * Color sensor control mode (auto / manual)
 *
 * \param value
 * Color sensor control value. Only valid when mode is manual
 *
 * \returns
 * K4A_RESULT_SUCCEEDED if the value was successfully set, K4A_RESULT_FAILED if an error occurred
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

/** Get the raw calibration blob for the entire K4A device.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param data
 * Location to write the calibration data to. This field may optionally be set to NULL if the caller wants to query for
 * the needed data size.
 *
 * \param data_size
 * On passing \p data_size into the function this variable represents the available size to write the raw data to. On
 * return this variable is updated with the amount of data actually written to the buffer.
 *
 * \returns
 * ::K4A_BUFFER_RESULT_SUCCEEDED if \p data was successfully written. If \p data_size points to a buffer size that is
 * too small to hold the output, ::K4A_BUFFER_RESULT_TOO_SMALL is returned and \p data_size is updated to contain the
 * minimum buffer size needed to capture the calibration data.
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

/** Get the camera calibration for the entire K4A device. The output struct is used as input to all transformation
 * functions.
 *
 * \param device_handle
 * Handle obtained by k4a_device_open().
 *
 * \param depth_mode
 * Mode in which depth camera is operated.
 *
 * \param color_resolution
 * Resolution in which color camera is operated
 *
 * \param calibration
 * Location to write the calibration
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p calibration was successfully written. ::K4A_RESULT_FAILED otherwise.
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
 * If sync_out_jack_connected is true then \ref k4a_device_configuration_t wired_sync_mode mode can be set to \ref
 * K4A_WIRED_SYNC_MODE_STANDALONE or \ref K4A_WIRED_SYNC_MODE_MASTER. If sync_in_jack_connected is true then \ref
 * k4a_device_configuration_t wired_sync_mode mode can be set to \ref K4A_WIRED_SYNC_MODE_STANDALONE or \ref
 * K4A_WIRED_SYNC_MODE_SUBORDINATE.
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
 * Raw calibration blob obtained from a device or recording. The raw calibration must be NULL terminated
 *
 * \param raw_calibration_size
 * The size, in bytes, of raw_calibration including the NULL termination.
 *
 * \param depth_mode
 * Mode in which depth camera is operated.
 *
 * \param color_resolution
 * Resolution in which color camera is operated
 *
 * \param calibration
 * Location to write the calibration
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p calibration was successfully written. ::K4A_RESULT_FAILED otherwise.
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

/** Transform a 3d point of a source coordinate system into a 3d point of the target coordinate system
 *
 * \param calibration
 * Location to read the camera calibration obtained by k4a_transformation_get_calibration().
 *
 * \param source_point3d
 * The 3d coordinates in millimeters representing a point in \p source_camera
 *
 * \param source_camera
 * The current camera
 *
 * \param target_camera
 * The target camera
 *
 * \param target_point3d
 * The new 3d coordinates of the input point in \p target_camera
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p target_point3d was successfully written. ::K4A_RESULT_FAILED if \p calibration
 * contained invalid transformation parameters.
 *
 * \remarks
 * This function is used to transform 3d points between depth and color camera coordinate systems. The function uses the
 * extrinsic camera calibration. It computes the output via multiplication with a precomputed matrix encoding a 3d
 * rotation and a 3d translation. The values \p source_camera and \p target_point3d can be identical in which case \p
 * target_point3d will be identical to \p source_point3d.
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
                                                 const k4a_float3_t *source_point3d,
                                                 const k4a_calibration_type_t source_camera,
                                                 const k4a_calibration_type_t target_camera,
                                                 k4a_float3_t *target_point3d);

/** Transform a 2d pixel coordinate with an associated depth value of the source camera into a 3d point of the target
 * coordinate system
 *
 * \param calibration
 * Location to read the camera calibration obtained by k4a_transformation_get_calibration().
 *
 * \param source_point2d
 * The 2d pixel coordinates in \p source_camera
 *
 * \param source_depth
 * The depth of \p source_point2d in millimeters
 *
 * \param source_camera
 * The current camera
 *
 * \param target_camera
 * The target camera
 *
 * \param target_point3d
 * The 3d coordinates of the input pixel in the coordinate system of \p target_camera
 *
 * \param valid
 * Takes a value of 1 if the transformation was successful and 0, otherwise
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p target_point3d was successfully written. ::K4A_RESULT_FAILED if \p calibration
 * contained invalid transformation parameters.
 *
 * \remarks
 * This function applies the intrinsic calibration of \p source_camera to compute the 3d ray from the focal point of the
 * camera through pixel \p source_point2d. The 3d point on this ray is then found using \p source_depth. If \p
 * target_camera is different from \p source_camera, the 3d point is transformed to \p target_camera using
 * k4a_transformation_3d_to_3d(). In practice, \p source_camera and \p target_camera will often be identical. In this
 * case, no 3d to 3d transformation is applied. If \p source_point2d is not considered as valid pixel coordinate
 * according to the intrinsic camera model, \p valid is set to 0 and 1, otherwise. The user should not use the result
 * of this transformation if \p valid was set to 0.
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
                                                 const float source_depth,
                                                 const k4a_calibration_type_t source_camera,
                                                 const k4a_calibration_type_t target_camera,
                                                 k4a_float3_t *target_point3d,
                                                 int *valid);

/** Transform a 3d point of a source coordinate system into a 2d pixel coordinate of the target camera
 *
 * \param calibration
 * Location to read the camera calibration obtained by k4a_transformation_get_calibration().
 *
 * \param source_point3d
 * The 3d coordinates in millimeters representing a point in \p source_camera
 *
 * \param source_camera
 * The current camera
 *
 * \param target_camera
 * The target camera
 *
 * \param target_point2d
 * The 2d pixel coordinates in \p target_camera
 *
 * \param valid
 * Takes a value of 1 if the transformation was successful and 0, otherwise
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p target_point2d was successfully written. ::K4A_RESULT_FAILED if \p calibration
 * contained invalid transformation parameters.
 *
 * \remarks
 * If \p target_camera is different from \p source_camera, \p source_point3d is transformed to \p target_camera using
 * k4a_transformation_3d_to_3d(). In practice, \p source_camera and \p target_camera will often be identical. In this
 * case, no 3d to 3d transformation is applied. The 3d point in the coordinate system of \p target_camera is then
 * projected onto the image plane using the intrinsic calibration of \p target_camera. If the intrinsic camera model
 * cannot handle this projection, \p valid is set to 0 and 1, otherwise. The user should not use the result of this
 * transformation if \p valid was set to 0.
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
                                                 const k4a_float3_t *source_point3d,
                                                 const k4a_calibration_type_t source_camera,
                                                 const k4a_calibration_type_t target_camera,
                                                 k4a_float2_t *target_point2d,
                                                 int *valid);

/** Transform a 2d pixel coordinate with an associated depth value of the source camera into a 2d pixel coordinate of
 * the target camera
 *
 * \param calibration
 * Location to read the camera calibration obtained by k4a_transformation_get_calibration().
 *
 * \param source_point2d
 * The 2d pixel coordinates in \p source_camera
 *
 * \param source_depth
 * The depth of \p source_point2d in millimeters
 *
 * \param source_camera
 * The current camera
 *
 * \param target_camera
 * The target camera
 *
 * \param target_point2d
 * The 2d pixel coordinates in \p target_camera
 *
 * \param valid
 * Takes a value of 1 if the transformation was successful and 0, otherwise
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p target_point2d was successfully written. ::K4A_RESULT_FAILED if \p calibration
 * contained invalid transformation parameters.
 *
 * \remarks
 * This function allows generating a mapping between pixel coordinates of depth and color cameras. Internally, the
 * function calls k4a_transformation_2d_to_3d() to compute the 3d point corresponding to \p source_point2d and to
 * transform the resulting 3d point into the camera coordinate system of \p target_camera. The function then calls
 * k4a_transformation_3d_to_2d() to project this 3d point onto the image plane of \p target_camera. If \p source_camera
 * and \p target_camera are identical, the function immediately sets \p target_point2d to \p source_point2d and returns
 * without computing any transformations. The parameter valid is to 0 if k4a_transformation_2d_to_3d() or
 * k4a_transformation_3d_to_2d() return an invalid transformation. The user should not use the result of this
 * transformation if \p valid was set to 0.
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
                                                 const float source_depth,
                                                 const k4a_calibration_type_t source_camera,
                                                 const k4a_calibration_type_t target_camera,
                                                 k4a_float2_t *target_point2d,
                                                 int *valid);

/** Get handle to transformation context.
 *
 * \param calibration
 * Location to read calibration obtained by k4a_device_get_calibration().
 *
 * \returns
 * Location of handle to the transformation context. A null pointer is returned if creation fails.
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

/** Destroy transformation context.
 *
 * \param transformation_handle
 * Location of handle to transformation context
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
 * Handle to transformation context
 *
 * \param depth_image
 * Handle to input depth image
 *
 * \param transformed_depth_image
 * Handle to output transformed depth image
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

/** Transforms the color image into the geometry of the depth camera.
 *
 * \param transformation_handle
 * Handle to transformation context
 *
 * \param depth_image
 * Handle to input depth image
 *
 * \param color_image
 * Handle to input color image
 *
 * \param transformed_color_image
 * Handle to output transformed color image
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

/** Transforms the depth image into 3 planar images representing X, Y and Z-coordinates of corresponding 3d points.
 *
 * \param transformation_handle
 * Handle to transformation context
 *
 * \param depth_image
 * Handle to input depth image
 *
 * \param camera
 * Geometry in which depth map was computed (depth or color camera)
 *
 * \param xyz_image
 * Handle to output xyz image
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

#ifdef __cplusplus
}
#endif

#endif /* K4A_H */
