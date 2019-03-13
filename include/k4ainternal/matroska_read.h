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
typedef struct _cluster_info_t
{
    uint64_t timestamp_ns = 0;
    uint64_t file_offset = 0;
    uint64_t cluster_size = 0; // Set to 0 until cluster is loaded
    std::weak_ptr<libmatroska::KaxCluster> cluster;

    bool next_known = false;
    struct _cluster_info_t *next = NULL;
    struct _cluster_info_t *previous = NULL;
} cluster_info_t;

typedef std::unique_ptr<cluster_info_t, void (*)(cluster_info_t *)> cluster_cache_t;

typedef struct _read_block_t
{
    struct _track_reader_t *reader;
    cluster_info_t *cluster_info;
    std::shared_ptr<libmatroska::KaxCluster> cluster;
    libmatroska::KaxInternalBlock *block;

    uint64_t timestamp_ns;
    uint64_t sync_timestamp_ns;
    int index;
} read_block_t;

typedef struct _track_reader_t
{
    libmatroska::KaxTrackEntry *track;
    uint32_t width, height, stride;
    k4a_image_format_t format;
    uint64_t sync_delay_ns;
    BITMAPINFOHEADER *bitmap_header;
    std::shared_ptr<read_block_t> current_block;
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
    cluster_info_t *seek_cluster;

    // The cluster cache is initialized with metadata from the Cues block, and may contain gaps.
    // Once it is known that no gap is present between clusters, next_known is set to true.
    cluster_cache_t cluster_cache;

    track_reader_t color_track;
    track_reader_t depth_track;
    track_reader_t ir_track;

    track_reader_t imu_track;
    int imu_sample_index;

    uint64_t segment_info_offset;
    uint64_t first_cluster_offset;
    uint64_t tracks_offset;
    uint64_t cues_offset;
    uint64_t attachments_offset;
    uint64_t tags_offset;

    uint64_t last_timestamp_ns, read_count;
} k4a_playback_context_t;

K4A_DECLARE_CONTEXT(k4a_playback_t, k4a_playback_context_t);

std::unique_ptr<EbmlElement> next_child(k4a_playback_context_t *context, EbmlElement *parent);
k4a_result_t skip_element(k4a_playback_context_t *context, EbmlElement *element);

void match_ebml_id(k4a_playback_context_t *context, EbmlId &id, uint64_t offset);
bool seek_info_ready(k4a_playback_context_t *context);
k4a_result_t parse_mkv(k4a_playback_context_t *context);
k4a_result_t populate_cluster_cache(k4a_playback_context_t *context);
k4a_result_t parse_recording_config(k4a_playback_context_t *context);
k4a_result_t read_bitmap_info_header(track_reader_t *track);
void reset_seek_pointers(k4a_playback_context_t *context, uint64_t seek_timestamp_ns);

libmatroska::KaxTrackEntry *get_track_by_name(k4a_playback_context_t *context, const char *name);
libmatroska::KaxTrackEntry *get_track_by_tag(k4a_playback_context_t *context, const char *tag_name);
libmatroska::KaxTag *get_tag(k4a_playback_context_t *context, const char *name);
std::string get_tag_string(libmatroska::KaxTag *tag);
libmatroska::KaxAttached *get_attachment_by_name(k4a_playback_context_t *context, const char *file_name);
libmatroska::KaxAttached *get_attachment_by_tag(k4a_playback_context_t *context, const char *tag_name);

k4a_result_t seek_offset(k4a_playback_context_t *context, uint64_t offset);
k4a_result_t populate_cluster_info(k4a_playback_context_t *context,
                                   std::shared_ptr<libmatroska::KaxCluster> &cluster,
                                   cluster_info_t *cluster_info);
cluster_info_t *find_cluster(k4a_playback_context_t *context, uint64_t timestamp_ns);
cluster_info_t *next_cluster(k4a_playback_context_t *context, cluster_info_t *current, bool next);
std::shared_ptr<libmatroska::KaxCluster> load_cluster(k4a_playback_context_t *context, cluster_info_t *cluster_info);
std::shared_ptr<read_block_t> find_next_block(k4a_playback_context_t *context, track_reader_t *reader, bool next);
k4a_result_t new_capture(k4a_playback_context_t *context,
                         std::shared_ptr<read_block_t> &block,
                         k4a_capture_t *capture_handle);
k4a_stream_result_t get_capture(k4a_playback_context_t *context, k4a_capture_t *capture_handle, bool next);
k4a_stream_result_t get_imu_sample(k4a_playback_context_t *context, k4a_imu_sample_t *imu_sample, bool next);

// Template helper functions
template<typename T> T *read_element(k4a_playback_context_t *context, EbmlElement *element)
{
    try
    {
        int upper_level = 0;
        EbmlElement *dummy = nullptr;

        T *read_element = static_cast<T *>(element);
        read_element->Read(*context->stream, T::ClassInfos.Context, upper_level, dummy, true);
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
