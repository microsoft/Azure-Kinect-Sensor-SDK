/** \file matroska_read.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure Record SDK.
 * Internal MKV Reading Helpers
 */

#ifndef RECORD_READ_H
#define RECORD_READ_H

#include <k4ainternal/matroska_common.h>

namespace k4arecord
{
typedef struct _read_block_t
{
    struct _track_reader_t *reader = nullptr;
    std::shared_ptr<libmatroska::KaxCluster> cluster;
    libmatroska::KaxInternalBlock *block = nullptr;

    uint64_t timestamp_ns = 0;
    uint64_t sync_timestamp_ns = 0;
    int index = 0;
} read_block_t;

typedef struct _track_reader_t
{
    libmatroska::KaxTrackEntry *track = nullptr;
    uint64_t sync_delay_ns = 0;
    std::string codec_id;
    std::vector<uint8_t> codec_private;
    std::shared_ptr<read_block_t> current_block;
    uint64_t frame_period_ns = 0;
    track_type type = track_video;
    std::vector<uint64_t> block_index_timestamp_usec_map;

    // Fields specific to video track
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    k4a_image_format_t format = K4A_IMAGE_FORMAT_CUSTOM;
} track_reader_t;

typedef struct _k4a_playback_context_t
{
    const char *file_path;
    std::unique_ptr<IOCallback> ebml_file;
    logger_t logger_handle;

    uint64_t timecode_scale;
    k4a_record_configuration_t record_config;

    std::unique_ptr<libebml::EbmlStream> stream;
    std::unique_ptr<libmatroska::KaxSegment> segment;

    std::unique_ptr<libmatroska::KaxInfo> segment_info;
    std::unique_ptr<libmatroska::KaxTracks> tracks;
    std::unique_ptr<libmatroska::KaxCues> cues;
    std::unique_ptr<libmatroska::KaxAttachments> attachments;
    std::unique_ptr<libmatroska::KaxTags> tags;

    libmatroska::KaxAttached *calibration_attachment;
    std::unique_ptr<k4a_calibration_t> device_calibration;

    uint64_t sync_period_ns;
    uint64_t seek_timestamp_ns;
    std::shared_ptr<libmatroska::KaxCluster> seek_cluster;

    track_reader_t color_track;
    track_reader_t depth_track;
    track_reader_t ir_track;

    track_reader_t imu_track;

    std::unordered_map<std::string, track_reader_t> custom_track_map;
    std::unordered_map<uint64_t, std::string> track_number_name_map;

    int imu_sample_index;

    uint64_t segment_info_offset;
    uint64_t first_cluster_offset;
    uint64_t tracks_offset;
    uint64_t cues_offset;
    uint64_t attachments_offset;
    uint64_t tags_offset;

    uint64_t last_timestamp_ns;
} k4a_playback_context_t;

K4A_DECLARE_CONTEXT(k4a_playback_t, k4a_playback_context_t);

typedef struct _k4a_playback_data_block_context_t
{
    uint64_t timestamp_usec;
    std::vector<uint8_t> data_block;
} k4a_playback_data_block_context_t;

K4A_DECLARE_CONTEXT(k4a_playback_data_block_t, k4a_playback_data_block_context_t);

std::unique_ptr<EbmlElement> next_element(k4a_playback_context_t *context,
                                          EbmlElement *parent,
                                          int *upper_level = nullptr);
k4a_result_t skip_element(k4a_playback_context_t *context, EbmlElement *element);

void match_ebml_id(k4a_playback_context_t *context, EbmlId &id, uint64_t offset);
bool seek_info_ready(k4a_playback_context_t *context);
k4a_result_t parse_mkv(k4a_playback_context_t *context);
k4a_result_t parse_recording_config(k4a_playback_context_t *context);
k4a_result_t read_bitmap_info_header(track_reader_t *track);
void reset_seek_pointers(k4a_playback_context_t *context, uint64_t seek_timestamp_ns);

libmatroska::KaxTrackEntry *get_track_by_name(k4a_playback_context_t *context, const char *name);
track_reader_t *get_track_reader_by_name(k4a_playback_context_t *context, std::string track_name);
k4a_result_t parse_custom_tracks(k4a_playback_context_t *context);
libmatroska::KaxTrackEntry *get_track_by_tag(k4a_playback_context_t *context, const char *tag_name);
libmatroska::KaxTag *get_tag(k4a_playback_context_t *context, const char *name);
std::string get_tag_string(libmatroska::KaxTag *tag);
libmatroska::KaxAttached *get_attachment_by_name(k4a_playback_context_t *context, const char *file_name);
libmatroska::KaxAttached *get_attachment_by_tag(k4a_playback_context_t *context, const char *tag_name);

k4a_result_t parse_all_timestamps(k4a_playback_context_t *context);

k4a_result_t seek_offset(k4a_playback_context_t *context, uint64_t offset);
std::shared_ptr<libmatroska::KaxCluster> seek_timestamp(k4a_playback_context_t *context, uint64_t timestamp_ns);
libmatroska::KaxCuePoint *find_closest_cue(k4a_playback_context_t *context, uint64_t timestamp_ns);
std::shared_ptr<libmatroska::KaxCluster> find_cluster(k4a_playback_context_t *context,
                                                      uint64_t search_offset,
                                                      uint64_t timestamp_ns);
std::shared_ptr<libmatroska::KaxCluster> next_cluster(k4a_playback_context_t *context,
                                                      libmatroska::KaxCluster *current_cluster,
                                                      bool next);
std::shared_ptr<read_block_t> find_next_block(k4a_playback_context_t *context, track_reader_t *reader, bool next);
k4a_result_t new_capture(k4a_playback_context_t *context,
                         std::shared_ptr<read_block_t> &block,
                         k4a_capture_t *capture_handle);
k4a_stream_result_t get_capture(k4a_playback_context_t *context, k4a_capture_t *capture_handle, bool next);
k4a_stream_result_t get_imu_sample(k4a_playback_context_t *context, k4a_imu_sample_t *imu_sample, bool next);

// Template helper functions
template<typename T>
T *read_element(k4a_playback_context_t *context, EbmlElement *element, ScopeMode readFully = SCOPE_ALL_DATA)
{
    try
    {
        int upper_level = 0;
        EbmlElement *dummy = nullptr;

        T *read_element = static_cast<T *>(element);
        read_element->Read(*context->stream, T::ClassInfos.Context, upper_level, dummy, true, readFully);
        return read_element;
    }
    catch (std::ios_base::failure e)
    {
        logger_error(LOGGER_RECORD,
                     "Failed to read element %s in recording '%s': %s",
                     T::ClassInfos.GetName(),
                     context->file_path,
                     e.what());
        return nullptr;
    }
}

// Example usage: find_next<KaxSegment>(context, true, false);
template<typename T>
std::unique_ptr<T> find_next(k4a_playback_context_t *context, bool search = false, bool read = true)
{
    try
    {
        EbmlElement *element = nullptr;
        do
        {
            if (element)
            {
                if (!element->IsFiniteSize())
                {
                    logger_error(LOGGER_RECORD,
                                 "Failed to read recording: Element Id '%x' has unknown size",
                                 EbmlId(*element).GetValue());
                    delete element;
                    return nullptr;
                }
                element->SkipData(*context->stream, element->Generic().Context);
                delete element;
                element = nullptr;
            }
            if (!element)
            {
                element = context->stream->FindNextID(T::ClassInfos, UINT64_MAX);
            }
            if (!search)
            {
                break;
            }
        } while (element && EbmlId(*element) != T::ClassInfos.GlobalId);

        if (!element)
        {
            if (!search)
            {
                logger_error(LOGGER_RECORD,
                             "Failed to read recording: Element Id '%x' not found",
                             T::ClassInfos.GlobalId.GetValue());
            }
            return nullptr;
        }
        else if (EbmlId(*element) != T::ClassInfos.GlobalId)
        {
            logger_error(LOGGER_RECORD,
                         "Failed to read recording: Expected element %s (id %x), found id '%x'",
                         T::ClassInfos.GetName(),
                         T::ClassInfos.GlobalId.GetValue(),
                         EbmlId(*element).GetValue());
            delete element;
            return nullptr;
        }

        if (read)
        {
            return std::unique_ptr<T>(read_element<T>(context, element));
        }
        else
        {
            return std::unique_ptr<T>(static_cast<T *>(element));
        }
    }
    catch (std::ios_base::failure e)
    {
        logger_error(LOGGER_RECORD,
                     "Failed to find %s in recording '%s': %s",
                     T::ClassInfos.GetName(),
                     context->file_path,
                     e.what());
        return nullptr;
    }
}

template<typename T>
k4a_result_t read_offset(k4a_playback_context_t *context, std::unique_ptr<T> &element_out, uint64_t offset)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, offset == 0);

    RETURN_IF_ERROR(seek_offset(context, offset));
    element_out = find_next<T>(context);

    return element_out ? K4A_RESULT_SUCCEEDED : K4A_RESULT_FAILED;
}

template<typename T> bool check_element_type(EbmlElement *element, T **out)
{
    if (EbmlId(*element) == T::ClassInfos.GlobalId)
    {
        *out = static_cast<T *>(element);
        return true;
    }
    *out = nullptr;
    return false;
}

} // namespace k4arecord

#endif /* RECORD_READ_H */
