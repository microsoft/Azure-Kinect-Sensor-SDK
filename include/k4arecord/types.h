/** \file types.h
 * Kinect For Azure Playback/Record type definitions.
 */

#ifndef K4ARECORD_TYPES_H
#define K4ARECORD_TYPES_H

#include <k4a/k4atypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \class k4a_record_t types.h <k4arecord/types.h>
 * Handle to a k4a recording opened for writing.
 *
 * Handles are created with k4a_record_create(), and closed with k4a_record_close().
 * Invalid handles are set to 0.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_DECLARE_HANDLE(k4a_record_t);

/** \class k4a_playback_t types.h <k4arecord/types.h>
 * Handle to a k4a recording opened for playback.
 *
 * Handles are created with k4a_playback_open(), and closed with k4a_playback_close().
 * Invalid handles are set to 0.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_DECLARE_HANDLE(k4a_playback_t);

/** Return codes returned by k4a playback API
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_STREAM_RESULT_SUCCEEDED = 0, /**< The result was successful */
    K4A_STREAM_RESULT_FAILED,        /**< The result was a failure */
    K4A_STREAM_RESULT_EOF,           /**< The end of the data stream was reached */
} k4a_stream_result_t;

/** Playback seeking positions
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_PLAYBACK_SEEK_BEGIN, /**< Seek relative to the beginning of a recording */
    K4A_PLAYBACK_SEEK_END    /**< Seek relative to the end of a recording */
} k4a_playback_seek_origin_t;

/** Structure containing the device configuration used to record.
 *
 * \relates k4a_device_configuration_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_record_configuration_t
{
    /** Image format used to record the color camera */
    k4a_image_format_t color_format;

    /** Image resolution used to record the color camera */
    k4a_color_resolution_t color_resolution;

    /** Mode used to record the depth camera */
    k4a_depth_mode_t depth_mode;

    /** Frame rate used to record the color and depth camera */
    k4a_fps_t camera_fps;

    /**
     * The delay between color and depth images in the recording.
     * A negative delay means depth images are first, and a positive delay means color images are first.
     */
    int32_t depth_delay_off_color_usec;
} k4a_record_configuration_t;

#ifdef __cplusplus
}
#endif

#endif /* K4ARECORD_TYPES_H */
