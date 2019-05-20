/** \file record.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure Recording SDK.
 */

#ifndef K4A_RECORD_H
#define K4A_RECORD_H

#include <k4arecord/types.h>
#include <k4arecord/k4arecord_export.h>

#ifdef __cplusplus

extern "C" {
#endif

/**
 *
 * \addtogroup Functions
 *
 * @{
 */

/** Opens a new recording file for writing.
 *
 * The file will be created if it doesn't exist, or overwritten if an existing file is specified.
 *
 * \param path
 * Filesystem path for the new recording.
 *
 * \param device
 * The Azure Kinect device that is being recorded. The device handle is used to store device calibration and serial
 * number information. May be NULL if recording user-generated data.
 *
 * \param device_config
 * The configuration the Azure Kinect device was started with.
 *
 * \param recording_handle
 * If successful, this contains a pointer to the new recording handle. Caller must call k4a_record_close()
 * when finished with recording.
 *
 * \remarks
 * Streaming does not need to be started on the device at the time this function is called, but when it is started
 * it should be started with the same configuration provided in \p device_config.
 *
 * \remarks
 * Subsequent calls to k4a_record_write_capture() will need to have images in the resolution and format defined
 * in \p device_config.
 *
 * \headerfile record.h <k4arecord/record.h>
 *
 * \returns ::K4A_RESULT_SUCCEEDED is returned on success
 *
 * \relates k4a_record_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">record.h (include k4arecord/record.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_record_create(const char *path,
                                                k4a_device_t device,
                                                const k4a_device_configuration_t device_config,
                                                k4a_record_t *recording_handle);

/** Adds a tag to the recording. All tags need to be added before the recording header is written.
 *
 * \param recording_handle
 * The handle of a new recording, obtained by k4a_record_create().
 *
 * \param name
 * The name of the tag to write.
 *
 * \param value
 * The string value to store in the tag.
 *
 * \headerfile record.h <k4arecord/record.h>
 *
 * \relates k4a_record_t
 *
 * \returns ::K4A_RESULT_SUCCEEDED is returned on success.
 *
 * \remarks
 * Tags are global to a file, and should store data related to the entire recording, such as camera configuration or
 * recording location.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">record.h (include k4arecord/record.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_record_add_tag(k4a_record_t recording_handle, const char *name, const char *value);

/** Adds the track header for recording IMU.
 *
 * The track needs to be added before the recording header is written.
 *
 * \param recording_handle
 * The handle of a new recording, obtained by k4a_record_create().
 *
 * \headerfile record.h <k4arecord/record.h>
 *
 * \relates k4a_record_t
 *
 * \returns ::K4A_RESULT_SUCCEEDED is returned on success
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">record.h (include k4arecord/record.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_record_add_imu_track(k4a_record_t recording_handle);

/** Writes the recording header and metadata to file.
 *
 * This must be called before captures can be written.
 *
 * \param recording_handle
 * The handle of a new recording, obtained by k4a_record_create().
 *
 * \headerfile record.h <k4arecord/record.h>
 *
 * \relates k4a_record_t
 *
 * \returns ::K4A_RESULT_SUCCEEDED is returned on success
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">record.h (include k4arecord/record.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_record_write_header(k4a_record_t recording_handle);

/** Writes a camera capture to file.
 *
 * Captures must be written in increasing order of timestamp, and the file's header must already be written.
 *
 * \param recording_handle
 * The handle of a new recording, obtained by k4a_record_create().
 *
 * \param capture_handle
 * The handle of a capture to write to file.
 *
 * \headerfile record.h <k4arecord/record.h>
 *
 * \relates k4a_record_t
 *
 * \returns ::K4A_RESULT_SUCCEEDED is returned on success
 *
 * \remarks
 * k4a_record_write_capture() will write all images in the capture to the corresponding tracks in the recording file.
 * If any of the images fail to write, other images will still be written before a failure is returned.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">record.h (include k4arecord/record.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_record_write_capture(k4a_record_t recording_handle, k4a_capture_t capture_handle);

/** Writes an imu sample to file.
 *
 * Samples must be written in increasing order of timestamp, and the file's header must already be written.
 * When writing imu samples at the same time as captures, the samples should be within 1 second of the most recently
 * written capture.
 *
 * \param recording_handle
 * The handle of a new recording, obtained by k4a_record_create().
 *
 * \param imu_sample
 * A structure containing the imu sample data and timestamps.
 *
 * \headerfile record.h <k4arecord/record.h>
 *
 * \relates k4a_record_t
 *
 * \returns ::K4A_RESULT_SUCCEEDED is returned on success
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">record.h (include k4arecord/record.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_record_write_imu_sample(k4a_record_t recording_handle, k4a_imu_sample_t imu_sample);

/** Flushes all pending recording data to disk.
 *
 * \param recording_handle
 * Handle obtained by k4a_record_create().
 *
 * \headerfile record.h <k4arecord/record.h>
 *
 * \relates k4a_record_t
 *
 * \returns ::K4A_RESULT_SUCCEEDED is returned on success, or ::K4A_RESULT_FAILED if an error occurred.
 *
 * \remarks
 * k4a_record_flush() ensures that all data passed to the recording API prior to calling flush is written to disk.
 * If continuing to write recording data, care must be taken to ensure no new timestamps are added from before the
 * flush.
 *
 * \remarks
 * If an error occurs, best effort is made to flush as much data to disk as possible, but the integrity of the file is
 * not guaranteed.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">record.h (include k4arecord/record.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_record_flush(k4a_record_t recording_handle);

/** Closes a recording handle.
 *
 * \param recording_handle
 * Handle obtained by k4a_record_create().
 *
 * \headerfile record.h <k4arecord/record.h>
 *
 * \relates k4a_record_t
 *
 * \remarks
 * If there is any unwritten data it will be flushed to disk before closing the recording.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">record.h (include k4arecord/record.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT void k4a_record_close(k4a_record_t recording_handle);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* K4A_RECORD_H */
