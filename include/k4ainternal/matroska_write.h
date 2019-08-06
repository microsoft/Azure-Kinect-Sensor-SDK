/** \file matroska_write.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure Record SDK.
 * Internal MKV Writing Helpers
 */

#ifndef RECORD_WRITE_H
#define RECORD_WRITE_H

#include <k4ainternal/matroska_common.h>
#include <set>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace k4arecord
{

typedef struct _track_header_t
{
    libmatroska::KaxTrackEntry *track;
    bool custom_track = false;

    /**
     * Some tracks such as IMU record small samples at a high rate. This setting changes the recording mode to use
     * Matroska BlockGroups and lacing to reduce overhead. Lacing will only record a single timestamp for a group of
     * samples, so the data structure should contain its own timestamp information to maintain accuracy.
     *
     * See k4a_record_subtitle_settings_t::high_freq_data in types.h for more information on timestamp behavior.
     */
    bool high_freq_data = false;
} track_header_t;

typedef struct _track_data_t
{
    track_header_t *track;
    libmatroska::DataBuffer *buffer;
} track_data_t;

typedef struct _cluster_t
{
    // Clusters contain timestamps in the range: time_start_ns <= timestamp_ns < time_end_ns
    uint64_t time_start_ns;
    uint64_t time_end_ns;
    std::vector<std::pair<uint64_t, track_data_t>> data;
} cluster_t;

typedef struct _k4a_record_context_t
{
    const char *file_path;
    std::unique_ptr<IOCallback> ebml_file;

    uint64_t timecode_scale;
    uint32_t camera_fps;

    k4a_device_configuration_t device_config;

    /**
     * The timestamp of the first piece of data in the recording.
     * Used to offset the recording to ensure it starts at timestamp 0.
     */
    uint64_t start_timestamp_offset;
    bool start_offset_tag_added;

    /**
     * The end timestamp of the last cluster written to disk.
     * The next cluster to be written will start at this timestamp.
     */
    uint64_t last_written_timestamp;

    /**
     * The timestamp of the most recent piece of data passed to the recording API.
     * The amount of buffered data is calculated as `most_recent_timestamp - last_written_timestamp`.
     */
    uint64_t most_recent_timestamp;

    uint64_t last_cues_entry_ns;
    uint32_t track_count;

    std::unique_ptr<libmatroska::KaxSegment> file_segment;
    std::unique_ptr<libebml::EbmlVoid> seek_void;
    std::unique_ptr<libebml::EbmlVoid> segment_info_void;
    std::unique_ptr<libebml::EbmlVoid> tags_void;

    track_header_t *color_track = nullptr;
    track_header_t *depth_track = nullptr;
    track_header_t *ir_track = nullptr;
    track_header_t *imu_track = nullptr;
    std::unordered_map<std::string, track_header_t> tracks;

    std::list<cluster_t *> pending_clusters;
    std::mutex pending_cluster_lock; // Locks last_written_timestamp, most_recent_timestamp, and pending_clusters

    bool writer_stopping;
    std::thread writer_thread;
    // std::condition_variable constructor may throw, so wrap this in a pointer.
    std::unique_ptr<std::condition_variable> writer_notify;
    std::mutex writer_lock;

    bool header_written, first_cluster_written;
} k4a_record_context_t;

K4A_DECLARE_CONTEXT(k4a_record_t, k4a_record_context_t);

enum TagTargetType
{
    TAG_TARGET_TYPE_NONE = 0,
    TAG_TARGET_TYPE_TRACK,
    TAG_TARGET_TYPE_ATTACHMENT
};

extern std::set<uint64_t> unique_ids;
uint64_t new_unique_id();

k4a_result_t
populate_bitmap_info_header(BITMAPINFOHEADER *header, uint64_t width, uint64_t height, k4a_image_format_t format);

bool validate_name_characters(const char *name);

track_header_t *add_track(k4a_record_context_t *context,
                          const char *name,
                          track_type type,
                          const char *codec,
                          const uint8_t *codec_private = NULL,
                          size_t codec_private_size = 0);

void set_track_info_video(track_header_t *track, uint64_t width, uint64_t height, uint64_t frame_rate);

k4a_result_t write_track_data(k4a_record_context_t *context,
                              track_header_t *track,
                              uint64_t timestamp_ns,
                              libmatroska::DataBuffer *buffer);

cluster_t *get_cluster_for_timestamp(k4a_record_context_t *context, uint64_t timestamp_ns);

k4a_result_t write_cluster(k4a_record_context_t *context, cluster_t *cluster, uint64_t *time_end_ns = NULL);

k4a_result_t start_matroska_writer_thread(k4a_record_context_t *context);

void stop_matroska_writer_thread(k4a_record_context_t *context);

libmatroska::KaxTag *add_tag(k4a_record_context_t *context,
                             const char *name,
                             const char *value,
                             TagTargetType target = TAG_TARGET_TYPE_NONE,
                             uint64_t target_uid = 0);

libmatroska::KaxAttached *add_attachment(k4a_record_context_t *context,
                                         const char *file_name,
                                         const char *mime_type,
                                         const uint8_t *buffer,
                                         size_t buffer_size);

uint64_t get_attachment_uid(libmatroska::KaxAttached *attachment);

k4a_result_t get_matroska_segment(k4a_record_context_t *context,
                                  libmatroska::KaxSegment **file_segment,
                                  libebml::IOCallback **iocallback);

} // namespace k4arecord

#endif /* RECORD_WRITE_H */
