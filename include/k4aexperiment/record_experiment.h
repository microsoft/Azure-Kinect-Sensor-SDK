/** \file record.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure Recording SDK.
 */

#ifndef K4A_RECORD_EXPERIMENT_H
#define K4A_RECORD_EXPERIMENT_H

#include <k4arecord/types.h>
#include <k4arecord/k4arecord_export.h>

#ifdef __cplusplus

extern "C" {
#endif

typedef enum
{
    K4A_RECORD_TRACK_TYPE_VIDEO = 0,
    K4A_RECORD_TRACK_TYPE_AUDIO,
    K4A_RECORD_TRACK_TYPE_SUBTITLE
} k4a_record_track_type_t;

typedef struct _k4a_record_video_info_t
{
    uint64_t width;
    uint64_t height;
    uint64_t frame_rate;
} k4a_record_video_info_t;

K4ARECORD_EXPORT k4a_result_t k4a_record_add_attachment(const k4a_record_t recording_handle,
                                                        const char *file_name,
                                                        const char *tag_name,
                                                        const uint8_t *buffer,
                                                        size_t buffer_size);

K4ARECORD_EXPORT k4a_result_t k4a_record_add_custom_track(const k4a_record_t recording_handle,
                                                          const char *track_name,
                                                          k4a_record_track_type_t type,
                                                          const char *codec,
                                                          const uint8_t *codec_private,
                                                          size_t codec_private_size);

K4ARECORD_EXPORT k4a_result_t k4a_record_set_custom_track_info_video(const k4a_record_t recording_handle,
                                                                     const char *track_name,
                                                                     const k4a_record_video_info_t *video_info);

K4ARECORD_EXPORT k4a_result_t k4a_record_add_custom_track_tag(const k4a_record_t recording_handle,
                                                              const char *track_name,
                                                              const char *tag_name,
                                                              const char *tag_value);

K4ARECORD_EXPORT k4a_result_t k4a_record_write_custom_track_data(const k4a_record_t recording_handle,
                                                                 const char *track_name,
                                                                 uint64_t timestamp_ns,
                                                                 uint8_t *buffer,
                                                                 uint32_t buffer_size,
                                                                 bool copy_buffer);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* K4A_RECORD_EXPERIMENT_H */
