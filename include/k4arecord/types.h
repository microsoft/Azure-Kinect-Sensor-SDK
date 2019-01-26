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
 * Handle to a k4a recording.
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
 * Handle to a k4a recording.
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

typedef enum
{
    K4A_PLAYBACK_SEEK_BEGIN, /**< Seek relative to the beginning of a recording */
    K4A_PLAYBACK_SEEK_END    /**< Seek relative to the end of a recording */
} k4a_playback_seek_origin_t;

#ifdef __cplusplus
}
#endif

#endif /* K4ARECORD_TYPES_H */
