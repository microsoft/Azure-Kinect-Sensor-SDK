/** \file playback.h
 * Kinect For Azure Recording Playback SDK.
 */

#ifndef K4A_PLAYBACK_H
#define K4A_PLAYBACK_H

#include <k4arecord/types.h>
#include <k4arecord/k4arecord_export.h>

#ifdef __cplusplus

extern "C" {
#endif

/** Opens an existing recording file for reading.
 *
 * \param path
 * Filesystem path of the existing recording.
 *
 * \param playback_handle [OUT]
 * If successful, this contains a pointer to the recording handle. Caller must call k4a_playback_close() when
 * finished with the recording.
 *
 * \headerfile playback.h <k4arecord/playback.h>
 *
 * \returns ::K4A_RESULT_SUCCEEDED is returned on success
 *
 * \relates k4a_playback_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_playback_open(const char *path, k4a_playback_t *playback_handle);

/** Get the raw calibration blob for the K4A device used during recording.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
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
 * \relates k4a_playback_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_buffer_result_t k4a_playback_get_raw_calibration(k4a_playback_t playback_handle,
                                                                      uint8_t *data,
                                                                      size_t *data_size);

/** Get the camera calibration for K4A device used during recording. The output struct is used as input to all
 * transformation functions.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \param calibration [OUT]
 * Location to write the calibration
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if \p calibration was successfully written. ::K4A_RESULT_FAILED otherwise.
 *
 * \relates k4a_playback_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_playback_get_calibration(k4a_playback_t playback_handle,
                                                           k4a_calibration_t *calibration);

/** Get the color format used in a recording.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \param format [OUT]
 * Location to write the color format
 *
 * \remarks
 * ::K4A_RESULT_SUCCEEDED if \p format was successfully written. ::K4A_RESULT_FAILED is returned if the color camera is
 * not enabled in the recording.
 *
 * \relates k4a_playback_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_playback_get_color_format(k4a_playback_t playback_handle, k4a_image_format_t *format);

/** Get the color resolution used in a recording.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \returns
 * The resolution used by the color camera to record, or ::K4A_COLOR_RESOLUTION_OFF if the recording contains no color
 * data.
 *
 * \relates k4a_playback_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_color_resolution_t k4a_playback_get_color_resolution(k4a_playback_t playback_handle);

/** Get the depth mode used a the recording.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \returns
 * The mode used by the depth camera to record, or ::K4A_DEPTH_MODE_OFF if the recording contains no depth data.
 *
 * \relates k4a_playback_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_depth_mode_t k4a_playback_get_depth_mode(k4a_playback_t playback_handle);

/** Get the camera framerate used in a recording.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \returns
 * The framerate used by the camera during recording. ::K4A_FRAMES_PER_SECOND_30 is returned by default if the recording
 * contains no camera data.
 *
 * \relates k4a_playback_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_fps_t k4a_playback_get_camera_fps(k4a_playback_t playback_handle);

/** Read the value of a tag from a recording.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \param name
 * The name of the tag to read
 *
 * \returns
 * The value of the tag, or NULL if the tag does not exist. This string is valid for the lifetime of the
 * \p playback_handle, and will be freed when the recording is closed.
 *
 * \relates k4a_playback_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT const char *k4a_playback_get_tag(k4a_playback_t playback_handle, const char *name);

/** Read the next capture in the recording sequence.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \param capture_handle
 * If successful this contains a handle to a capture object. Caller must call k4a_capture_release() when its done using
 * this capture
 *
 * \returns
 * ::K4A_STREAM_RESULT_SUCCEEDED if a capture is returned, or ::K4A_STREAM_RESULT_EOF if the end of the recording is
 * reached. All other failures will return ::K4A_STREAM_RESULT_FAILED.
 *
 * \relates k4a_playback_t
 *
 * \remarks
 * k4a_playback_get_next_capture() always returns the capture after the last returned capture. If the last call to
 * k4a_playback_get_previous_capture() returned EOF, k4a_playback_get_next_capture() will return the first capture in
 * the recording.
 *
 * \remarks
 * The first call to k4a_playback_get_next_capture() after k4a_playback_seek_timestamp() will return the first capture
 * with all timestamps >= the seek time.
 *
 * \remarks
 * Capture objects will always contain at least 1 image, but may have images missing if frames were dropped in the
 * original recording. When calling k4a_capture_get_*_image(), the image should be checked for NULL.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_stream_result_t k4a_playback_get_next_capture(k4a_playback_t playback_handle,
                                                                   k4a_capture_t *capture_handle);

/** Read the previous capture in the recording sequence.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \param capture_handle
 * If successful this contains a handle to a capture object. Caller must call k4a_capture_release() when its done using
 * this capture
 *
 * \returns
 * ::K4A_STREAM_RESULT_SUCCEEDED if a capture is returned, or ::K4A_STREAM_RESULT_EOF if the start of the recording is
 * reached. All other failures will return ::K4A_STREAM_RESULT_FAILED.
 *
 * \relates k4a_playback_t
 *
 * \remarks
 * k4a_playback_get_previous_capture() always returns the capture before the last returned capture. If the last call to
 * k4a_playback_get_next_capture() returned EOF, k4a_playback_get_previous_capture() will return the last capture in the
 * recording.
 *
 * \remarks
 * The first call to k4a_playback_get_previous_capture() after k4a_playback_seek_timestamp() will return the first
 * capture with all timestamps < the seek time.
 *
 * \remarks
 * Capture objects will always contain at least 1 image, but may have images missing if frames were dropped in the
 * original recording. When calling k4a_capture_get_*_image(), the image should be checked for NULL.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_stream_result_t k4a_playback_get_previous_capture(k4a_playback_t playback_handle,
                                                                       k4a_capture_t *capture_handle);

/** Read the next imu sample in the recording sequence.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \param imu_sample [OUT]
 * The location to write the imu sample.
 *
 * \returns
 * ::K4A_STREAM_RESULT_SUCCEEDED if a sample is returned, or ::K4A_STREAM_RESULT_EOF if the end of the recording is
 * reached. All other failures will return ::K4A_STREAM_RESULT_FAILED.
 *
 * \relates k4a_playback_t
 *
 * \remarks
 * k4a_playback_get_next_imu_sample() always returns the imu sample after the last returned sample. If the last call to
 * k4a_playback_get_previous_imu_sample() returned EOF, k4a_playback_get_next_imu_sample() will return the first imu
 * sample in the recording.
 *
 * \remarks
 * The first call to k4a_playback_get_next_imu_sample() after k4a_playback_seek_timestamp() will return the first imu
 * sample with a timestamp >= the seek time.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_stream_result_t k4a_playback_get_next_imu_sample(k4a_playback_t playback_handle,
                                                                      k4a_imu_sample_t *imu_sample);

/** Read the previous imu sample in the recording sequence.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \param imu_sample [OUT]
 * The location to write the imu sample.
 *
 * \returns
 * ::K4A_STREAM_RESULT_SUCCEEDED if a sample is returned, or ::K4A_STREAM_RESULT_EOF if the start of the recording is
 * reached. All other failures will return ::K4A_STREAM_RESULT_FAILED.
 *
 * \relates k4a_playback_t
 *
 * \remarks
 * k4a_playback_get_previous_imu_sample() always returns the imu sample before the last returned sample. If the last
 * call to k4a_playback_get_next_imu_sample() returned EOF, k4a_playback_get_previous_imu_sample() will return the last
 * imu sample in the recording.
 *
 * \remarks
 * The first call to k4a_playback_get_previous_imu_sample() after k4a_playback_seek_timestamp() will return the first
 * imu sample with a timestamp < the seek time.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_stream_result_t k4a_playback_get_previous_imu_sample(k4a_playback_t playback_handle,
                                                                          k4a_imu_sample_t *imu_sample);

/** Seek to a specific timestamp within a recording.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \param offset_usec
 * The timestamp offset to seek to relative to \p origin
 *
 * \param origin
 * Specifies if the seek operation should be done relative to the beginning or end of the recording.
 *
 * \returns
 * ::K4A_RESULT_SUCCEEDED if the seek operation was successful. If the timestamp is out of range ::K4A_RESULT_FAILED is
 * returned. The current seek position is left unchanged if a failure is returned.
 *
 * \relates k4a_playback_t
 *
 * \remarks
 * The first call to k4a_playback_get_next_capture() after k4a_playback_seek_timestamp() will return the first capture
 * with all timestamps >= the seek time.
 *
 * \remarks
 * The first call to k4a_playback_get_previous_capture() after k4a_playback_seek_timestamp() will return the first
 * capture with all timestamps < the seek time.
 *
 * \remarks
 * The first call to k4a_playback_get_next_imu_sample() after k4a_playback_seek_timestamp() will return the first imu
 * sample with a timestamp >= the seek time.
 *
 * \remarks
 * The first call to k4a_playback_get_previous_imu_sample() after k4a_playback_seek_timestamp() will return the first
 * imu sample with a timestamp < the seek time.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT k4a_result_t k4a_playback_seek_timestamp(k4a_playback_t playback_handle,
                                                          int64_t offset_usec,
                                                          k4a_playback_seek_origin_t origin);

/** Gets the last timestamp in a recording.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \returns
 * The timestamp of the last capture image or imu sample in microseconds.
 *
 * \relates k4a_playback_t
 *
 * \remarks
 * Recordings start at timestamp 0, and end at the timestamp returned by k4a_playback_get_last_timestamp_usec().
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT uint64_t k4a_playback_get_last_timestamp_usec(k4a_playback_t playback_handle);

/** Closes a recording playback handle.
 *
 * \param playback_handle
 * Handle obtained by k4a_playback_open().
 *
 * \headerfile playback.h <k4arecord/playback.h>
 *
 * \relates k4a_playback_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">playback.h (include k4arecord/playback.h)</requirement>
 *   <requirement name="Library">k4arecord.lib</requirement>
 *   <requirement name="DLL">k4arecord.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
K4ARECORD_EXPORT void k4a_playback_close(k4a_playback_t playback_handle);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* K4A_PLAYBACK_H */
