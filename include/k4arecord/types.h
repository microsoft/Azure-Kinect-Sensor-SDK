/** \file types.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure Playback/Record type definitions.
 */

#ifndef K4ARECORD_TYPES_H
#define K4ARECORD_TYPES_H

#include <k4a/k4atypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup Handles
 * @{
 */

/** \class k4a_record_t types.h <k4arecord/types.h>
 * Handle to a k4a recording opened for writing.
 *
 * \remarks
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
 * \remarks
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

/** \class k4a_playback_data_block_t types.h <k4arecord/types.h>
 * Handle to a block of data read from a k4a_playback_t custom track.
 *
 * \remarks
 * Handles are obtained from k4a_playback_get_next_data_block() or k4a_playback_get_previous_data_block(), and released
 * with k4a_playback_data_block_release(). Invalid handles are set to 0.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
K4A_DECLARE_HANDLE(k4a_playback_data_block_t)

/**
 * @}
 *
 * \addtogroup Definitions
 * @{
 */

/** Name of the built-in color track used in recordings.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define K4A_TRACK_NAME_COLOR "COLOR"

/** Name of the built-in depth track used in recordings.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define K4A_TRACK_NAME_DEPTH "DEPTH"

/** Name of the built-in IR track used in recordings.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define K4A_TRACK_NAME_IR "IR"

/** Name of the built-in imu track used in recordings.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define K4A_TRACK_NAME_IMU "IMU"

/**
 * @}
 *
 * \addtogroup Enumerations
 * @{
 */

/** Return codes returned by Azure Kinect playback API.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_STREAM_RESULT_SUCCEEDED = 0, /**< The result was successful. */
    K4A_STREAM_RESULT_FAILED,        /**< The result was a failure. */
    K4A_STREAM_RESULT_EOF,           /**< The end of the data stream was reached. */
} k4a_stream_result_t;

/** Playback seeking positions.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    K4A_PLAYBACK_SEEK_BEGIN,      /**< Seek relative to the beginning of a recording. */
    K4A_PLAYBACK_SEEK_END,        /**< Seek relative to the end of a recording. */
    K4A_PLAYBACK_SEEK_DEVICE_TIME /**< Seek to an absolute device timestamp. */
} k4a_playback_seek_origin_t;

/**
 * @}
 *
 * \addtogroup Structures
 * @{
 */

/** Structure containing the device configuration used to record.
 *
 * \see k4a_device_configuration_t
 * \see k4a_playback_get_record_configuration()
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_record_configuration_t
{
    /** Image format used to record the color camera. */
    k4a_image_format_t color_format;

    /** Image resolution used to record the color camera. */
    k4a_color_resolution_t color_resolution;

    /** Mode used to record the depth camera. */
    k4a_depth_mode_t depth_mode;

    /** Frame rate used to record the color and depth camera. */
    k4a_fps_t camera_fps;

    /** True if the recording contains Color camera frames. */
    bool color_track_enabled;

    /** True if the recording contains Depth camera frames. */
    bool depth_track_enabled;

    /** True if the recording contains IR camera frames. */
    bool ir_track_enabled;

    /** True if the recording contains IMU sample data. */
    bool imu_track_enabled;

    /**
     * The delay between color and depth images in the recording.
     * A negative delay means depth images are first, and a positive delay means color images are first.
     */
    int32_t depth_delay_off_color_usec;

    /** External synchronization mode */
    k4a_wired_sync_mode_t wired_sync_mode;

    /**
     * The delay between this recording and the externally synced master camera.
     * This value is 0 unless \p wired_sync_mode is set to ::K4A_WIRED_SYNC_MODE_SUBORDINATE
     */
    uint32_t subordinate_delay_off_master_usec;

    /**
     * The timestamp offset of the start of the recording. All recorded timestamps are offset by this value such that
     * the recording starts at timestamp 0. This value can be used to synchronize timestamps between 2 recording files.
     */
    uint32_t start_timestamp_offset_usec;
} k4a_record_configuration_t;

/** Structure containing additional metadata specific to custom video tracks.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_record_video_settings_t
{
    uint64_t width;      /**< Frame width of the video */
    uint64_t height;     /**< Frame height of the video  */
    uint64_t frame_rate; /**< Frame rate (frames-per-second) of the video */
} k4a_record_video_settings_t;

/** Structure containing additional metadata specific to custom subtitle tracks.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">types.h (include k4arecord/types.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_record_subtitle_settings_t
{
    /**
     * If true, data will be grouped together in batches to reduce overhead. In this mode, only a single timestamp will
     * be stored per batch, and an estimated timestamp will be used by k4a_playback_seek_timestamp() and
     * k4a_playback_data_block_get_timestamp_usec(). The estimated timestamp is calculated with the assumption that
     * blocks are evenly spaced within a batch. If precise timestamps are required, the timestamp should be added to
     * each data block itself.
     *
     * If false, data will be stored as individual blocks with full timestamp information (Default).
     */
    bool high_freq_data;
} k4a_record_subtitle_settings_t;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* K4ARECORD_TYPES_H */
