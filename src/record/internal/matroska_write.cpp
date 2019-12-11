// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ctime>
#include <iostream>
#include <algorithm>
#include <sstream>

#include <k4a/k4a.h>
#include <k4ainternal/matroska_write.h>
#include <k4ainternal/logging.h>

using namespace LIBMATROSKA_NAMESPACE;

namespace k4arecord
{
std::set<uint64_t> unique_ids;

/**
 * Generates a unique id for use as a Matroska TrackUID, AttachmentUID, etc...
 *
 * Matroska UIDs are used to associate tags and metadata with tracks and must be unique within a file.
 * Matroska specifies that UIDs should not be changed when copying tracks, so that they can be identified by UID across
 * files. Best effort should be made to generate UIDs that are unique across files.
 *
 * This function randomly generates 60-bit UIDs that are guaranteed to be non-zero, and unique across calls in the same
 * library instance.
 */
uint64_t new_unique_id()
{
    if (unique_ids.empty())
    {
        srand((unsigned int)time(0));
    }

    uint64_t result = 0;
    do
    {
        for (int i = 0; i < 4; i++)
        {
            // Generate 4 random 15-bit numbers and merge them together for 60 bits of entropy.
            int x = -1;
#if RAND_MAX == 0x7FFF
            x = rand();
#else
            do
            {
                // Retry until we get a number in this range to keep random distribution.
                x = rand();
            } while (x < 0 || x >= 0x7FFF);
#endif
            result = (result << 15) | (uint64_t)x;
        }
    } while (result == 0 || !unique_ids.insert(result).second); // Loop until a unique and non-zero id is found.

    return result;
}

k4a_result_t
populate_bitmap_info_header(BITMAPINFOHEADER *header, uint64_t width, uint64_t height, k4a_image_format_t format)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, header == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, width > UINT32_MAX);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, height > UINT32_MAX);

    header->biWidth = static_cast<uint32_t>(width);
    header->biHeight = static_cast<uint32_t>(height);

    switch (format)
    {

    case K4A_IMAGE_FORMAT_COLOR_NV12:
        header->biBitCount = 12;
        header->biCompression = 0x3231564E; // NV12

        // NV12 uses 4:2:0 downsampling
        header->biSizeImage = sizeof(uint8_t) * header->biWidth * header->biHeight * 3 / 2;
        break;
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
        header->biBitCount = 16;
        header->biCompression = 0x32595559; // YUY2

        // YUY2 uses 4:2:2 downsampling
        header->biSizeImage = sizeof(uint8_t) * header->biWidth * header->biHeight * 2;
        break;
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
        header->biBitCount = 24;
        header->biCompression = 0x47504A4D; // MJPG

        header->biSizeImage = 0; // JPEG is variable size
        break;
    case K4A_IMAGE_FORMAT_DEPTH16:
    case K4A_IMAGE_FORMAT_IR16:
        // Store depth in b16g format, which is supported by ffmpeg.
        header->biBitCount = 16;
        header->biCompression = 0x67363162; // b16g (16 bit grayscale, big endian)
        header->biSizeImage = sizeof(uint8_t) * header->biWidth * header->biHeight * 2;
        break;
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        header->biBitCount = 32;
        header->biCompression = 0x41524742; // BGRA
        header->biSizeImage = sizeof(uint8_t) * header->biWidth * header->biHeight * 4;
        break;
    default:
        LOG_ERROR("Unsupported color format specified in recording: %d", format);
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

bool validate_name_characters(const char *name)
{
    const char *ch = name;
    while (*ch != 0)
    {
        if (*ch == '-' || *ch == '_' || (*ch >= '0' && *ch <= '9') || (*ch >= 'A' && *ch <= 'Z'))
        {
            // Valid character
            ch++;
        }
        else
        {
            LOG_ERROR("Names '%s' must be ALL CAPS and may only contain A-Z, 0-9, '-' and '_': ", name);
            return false;
        }
    }
    return true;
}

track_header_t *add_track(k4a_record_context_t *context,
                          const char *name,
                          track_type type,
                          const char *codec,
                          const uint8_t *codec_private,
                          size_t codec_private_size)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, name == NULL);
    RETURN_VALUE_IF_ARG(NULL, codec == NULL);
    RETURN_VALUE_IF_ARG(NULL, context->header_written);
    RETURN_VALUE_IF_ARG(NULL, !validate_name_characters(name));

    auto itr = context->tracks.find(name);
    if (itr != context->tracks.end())
    {
        LOG_ERROR("A track already exists with the name: %s", name);
        return NULL;
    }

    auto &tracks = GetChild<KaxTracks>(*context->file_segment);
    auto track = new KaxTrackEntry();
    tracks.PushElement(*track); // Track will be freed when the file is closed.
    track->SetGlobalTimecodeScale(context->timecode_scale);

    // Track numbers start at 1
    GetChild<KaxTrackNumber>(*track).SetValue(++context->track_count);
    GetChild<KaxTrackUID>(*track).SetValue(new_unique_id());
    GetChild<KaxTrackType>(*track).SetValue(type);
    GetChild<KaxTrackName>(*track).SetValueUTF8(name);
    GetChild<KaxCodecID>(*track).SetValue(codec);
    track->EnableLacing(true);

    if (codec_private != NULL)
    {
        assert(codec_private_size <= UINT32_MAX);
        GetChild<KaxCodecPrivate>(*track).CopyBuffer(codec_private, (uint32)codec_private_size);
    }

    track_header_t track_header;
    track_header.track = track;
    track_header.custom_track = false;
    track_header.high_freq_data = false;
    auto entry = context->tracks.emplace(std::string(name), track_header);

    return &entry.first->second;
}

void set_track_info_video(track_header_t *track, uint64_t width, uint64_t height, uint64_t frame_rate)
{
    RETURN_VALUE_IF_ARG(VOID_VALUE, track == NULL);
    RETURN_VALUE_IF_ARG(VOID_VALUE, track->track == NULL);

    GetChild<KaxTrackDefaultDuration>(*track->track).SetValue(1_s / frame_rate);

    auto &video_track = GetChild<KaxTrackVideo>(*track->track);
    GetChild<KaxVideoPixelWidth>(video_track).SetValue(width);
    GetChild<KaxVideoPixelHeight>(video_track).SetValue(height);
}

// Buffer needs to be valid until it is flushed to disk. The DataBuffer free callback can be used to assist with this.
// If a failure is returned, the caller will need to free the buffer.
k4a_result_t
write_track_data(k4a_record_context_t *context, track_header_t *track, uint64_t timestamp_ns, DataBuffer *buffer)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, !context->header_written);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, track == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, track->track == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, buffer == NULL);

    try
    {
        std::lock_guard<std::mutex> lock(context->pending_cluster_lock);

        if (context->most_recent_timestamp < timestamp_ns)
        {
            context->most_recent_timestamp = timestamp_ns;
        }

        cluster_t *cluster = get_cluster_for_timestamp(context, timestamp_ns);
        if (cluster == NULL)
        {
            // The timestamp is too old, the block of data has already been written.
            return K4A_RESULT_FAILED;
        }

        track_data_t data = { track, buffer };
        cluster->data.push_back(std::make_pair(timestamp_ns, data));
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Failed to write track data to queue: %s", e.what());
        return K4A_RESULT_FAILED;
    }

    if (context->writer_notify)
    {
        context->writer_notify->notify_one();
    }

    return K4A_RESULT_SUCCEEDED;
}

// Lock(context->pending_cluster_lock) should be active when calling this function
cluster_t *get_cluster_for_timestamp(k4a_record_context_t *context, uint64_t timestamp_ns)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);

    if (context->last_written_timestamp > timestamp_ns)
    {
        LOG_ERROR("The cluster containing the timestamp %d has already been written to disk.", timestamp_ns);
        return NULL;
    }

    // Pending clusters are ordered from oldest to newest timestamp.
    // Clusters are not created until at least 1 track buffer is added within the timestamp range.
    // Find the newest pending cluster before this timestamp.
    auto selected_cluster = context->pending_clusters.rbegin();
    auto cluster_end = context->pending_clusters.rend();
    for (; selected_cluster != cluster_end; selected_cluster++)
    {
        if ((*selected_cluster)->time_start_ns <= timestamp_ns)
        {
            break;
        }
    }

    if (selected_cluster == cluster_end || (*selected_cluster)->time_end_ns <= timestamp_ns)
    {
        // If the exact cluster wasn't found, create a new one in the right range.
        // Calculate the new cluster start, aligned to the current cluster length.
        uint64_t time_start_ns = selected_cluster == cluster_end ? context->last_written_timestamp :
                                                                   (*selected_cluster)->time_end_ns;
        if (time_start_ns + MAX_CLUSTER_LENGTH_NS <= timestamp_ns)
        {
            uint64_t diff = timestamp_ns - time_start_ns;
            time_start_ns += diff - (diff % MAX_CLUSTER_LENGTH_NS);
        }

        cluster_t *new_cluster = new cluster_t;
        new_cluster->time_start_ns = time_start_ns;
        new_cluster->time_end_ns = time_start_ns + MAX_CLUSTER_LENGTH_NS;
        assert(new_cluster->time_start_ns <= timestamp_ns && new_cluster->time_end_ns > timestamp_ns);

        if (selected_cluster == cluster_end)
        {
            // The new cluster's timestamp is before all pending clusters.
            context->pending_clusters.push_front(new_cluster);
        }
        else
        {
            // The new cluster's timestamp is before the selected cluster.
            context->pending_clusters.insert(selected_cluster.base(), new_cluster);
        }
        return new_cluster;
    }
    else
    {
        return *selected_cluster;
    }
}

static bool sort_by_pair_asc(const std::pair<uint64_t, track_data_t> &a, const std::pair<uint64_t, track_data_t> &b)
{
    return (a.first < b.first);
}

// Writes the cluster to disk and frees the cluster.
// Updated time_end_ns is optionally returned through the argument pointer.
k4a_result_t write_cluster(k4a_record_context_t *context, cluster_t *cluster, uint64_t *time_end_ns)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, !context->header_written);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, cluster == NULL);

    if (cluster->data.size() == 0)
    {
        LOG_WARNING("Tried to write empty cluster to disk", 0);
        delete cluster;
        return K4A_RESULT_FAILED;
    }

    // Sort the data in the cluster by timestamp so it can be written in order
    std::sort(cluster->data.begin(), cluster->data.end(), sort_by_pair_asc);

    KaxCluster *new_cluster = new KaxCluster();

    // KaxCluster will be freed by libmatroska when the file is closed.
    context->file_segment->PushElement(*new_cluster);

    cluster->time_start_ns = cluster->data.front().first;
    if (context->first_cluster_written)
    {
        new_cluster->InitTimecode((cluster->time_start_ns - context->start_timestamp_offset) / context->timecode_scale,
                                  (int64)context->timecode_scale);
    }
    else
    {
        context->start_timestamp_offset = cluster->time_start_ns;
        new_cluster->InitTimecode(0, (int64)context->timecode_scale);
        context->first_cluster_written = true;
    }

    if (!context->start_offset_tag_added)
    {
        std::ostringstream offset_str;
        offset_str << context->start_timestamp_offset;
        add_tag(context, "K4A_START_OFFSET_NS", offset_str.str().c_str());
        context->start_offset_tag_added = true;
    }

    new_cluster->SetParent(*context->file_segment);
    new_cluster->EnableChecksum();

    KaxBlockBlob *block_blob = NULL;
    KaxBlockGroup *block_group = NULL;
    track_header_t *current_track = NULL;
    uint64_t block_blob_start = 0;

    std::vector<std::unique_ptr<KaxBlockBlob>> blob_list;

    bool first = true;
    for (std::pair<uint64_t, track_data_t> data : cluster->data)
    {
        // Only store high frequency data together in a block group, all other tracks store 1 frame per block.
        if (block_blob == NULL || current_track != data.second.track || !data.second.track->high_freq_data)
        {
            // Automatically switching between SimpleBlock and BlockGroup is not implemented in libmatroska,
            // We need to decide the block type ahead of time to force high frequency data into a BlockGroup
            block_blob = new KaxBlockBlob(data.second.track->high_freq_data ? BLOCK_BLOB_NO_SIMPLE :
                                                                              BLOCK_BLOB_ALWAYS_SIMPLE);
            // BlockBlob needs to be valid until the cluster is rendered.
            // The blob will be freed at the end of write_cluster().
            blob_list.emplace_back(block_blob);
            new_cluster->AddBlockBlob(block_blob);
            block_blob->SetParent(*new_cluster);
            block_blob_start = data.first;

            if (!block_blob->IsSimpleBlock())
            {
                block_group = new KaxBlockGroup();
                block_blob->SetBlockGroup(*block_group); // Block group will be freed when BlockBlob is destroyed.
                block_group->SetParentTrack(*data.second.track->track);
            }
            else
            {
                block_group = NULL;
            }
            current_track = data.second.track;
        }

        if (!block_blob->IsSimpleBlock())
        {
            // Update the block duration to the last written sample
            block_group->SetBlockDuration(data.first - block_blob_start + context->timecode_scale);
        }

        block_blob->AddFrameAuto(*data.second.track->track,
                                 data.first - context->start_timestamp_offset,
                                 *data.second.buffer);

        // Only add one Cue entry once per cluster
        // We only need to write Cue entries for the first track.
        if (first && GetChild<KaxTrackNumber>(*data.second.track->track).GetValue() == 1)
        {
            // Add cue entries at a maximum rate specified by CUE_ENTRY_GAP_NS so that the index doesn't get too large.
            if (context->last_cues_entry_ns == 0 ||
                data.first - context->start_timestamp_offset >= context->last_cues_entry_ns + CUE_ENTRY_GAP_NS)
            {
                context->last_cues_entry_ns = data.first - context->start_timestamp_offset;
                auto &cues = GetChild<KaxCues>(*context->file_segment);
                cues.AddBlockBlob(*block_blob);
            }
            first = false;
        }
    }

    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    auto &cues = GetChild<KaxCues>(*context->file_segment);
    try
    {
        new_cluster->Render(*context->ebml_file, cues);
    }
    catch (std::ios_base::failure &e)
    {
        LOG_ERROR("Failed to write recording data '%s': %s", context->file_path, e.what());
        result = K4A_RESULT_FAILED;
    }

    if (time_end_ns != NULL)
    {
        // Cluster data is in the range [time_start_ns, time_end_ns), add 1 ns to the end timestamp.
        *time_end_ns = cluster->data.back().first + 1;
    }

    // KaxCluster->ReleaseFrames() has a bug and will not free SimpleBlocks, we need to do this ourselves.
    for (std::pair<uint64_t, track_data_t> data : cluster->data)
    {
        data.second.buffer->FreeBuffer(*data.second.buffer);
    }

    // Both KaxCluster and KaxBlockBlob will try to free the same element due to a bug in libmatroska.
    // In order to prevent this, we need to go through and remove the Block elements from the cluster.
    auto &elements = new_cluster->GetElementList();
    for (size_t i = 0; i < elements.size(); i++)
    {
        if (EbmlId(*elements[i]) == KaxBlockGroup::ClassInfos.GlobalId ||
            EbmlId(*elements[i]) == KaxSimpleBlock::ClassInfos.GlobalId)
        {
            // Remove this element from the list.
            // We don't care about preserving order because the Cluster has already been rendered.
            elements[i] = elements.back();
            elements.pop_back();
            i--;
        }
    }

    delete cluster;
    return result;
}

static void matroska_writer_thread(k4a_record_context_t *context)
{
    assert(context->writer_notify);

    try
    {
        std::unique_lock<std::mutex> lock(context->writer_lock);

        LargeFileIOCallback *file_io = dynamic_cast<LargeFileIOCallback *>(context->ebml_file.get());
        if (file_io != NULL)
        {
            file_io->setOwnerThread();
        }

        while (!context->writer_stopping)
        {
            context->pending_cluster_lock.lock();

            // Check the oldest pending cluster to see if we should write to disk.
            cluster_t *oldest_cluster = NULL;
            if (!context->pending_clusters.empty())
            {
                oldest_cluster = context->pending_clusters.front();
                if (context->most_recent_timestamp >= oldest_cluster->time_end_ns)
                {
                    uint64_t age = context->most_recent_timestamp - oldest_cluster->time_end_ns;
                    if (age > CLUSTER_WRITE_DELAY_NS)
                    {
                        assert(oldest_cluster->time_start_ns >= context->last_written_timestamp);
                        context->pending_clusters.pop_front();
                        context->last_written_timestamp = oldest_cluster->time_end_ns;
                        if (age > CLUSTER_WRITE_QUEUE_WARNING_NS)
                        {
                            LOG_ERROR("Disk write speed is too low, write queue is filling up.", 0);
                        }
                    }
                    else
                    {
                        oldest_cluster = NULL;
                    }
                }
                else
                {
                    oldest_cluster = NULL;
                }
            }

            context->pending_cluster_lock.unlock();

            if (oldest_cluster)
            {
                k4a_result_t result = TRACE_CALL(write_cluster(context, oldest_cluster));
                if (K4A_FAILED(result))
                {
                    // write_cluster failures are not recoverable (file IO errors only, the file is likely corrupt)
                    LOG_ERROR("Cluster write failed, writer thread exiting.", 0);
                    break;
                }
            }

            // Wait until more clusters arrive up to 100ms, or 1ms if the queue is not empty.
            context->writer_notify->wait_for(lock, std::chrono::milliseconds(oldest_cluster ? 1 : 100));

            if (file_io != NULL)
            {
                file_io->setOwnerThread();
            }
        }
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Writer thread threw exception: %s", e.what());
    }
}

k4a_result_t start_matroska_writer_thread(k4a_record_context_t *context)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->writer_thread.joinable());

    try
    {
        context->writer_notify.reset(new std::condition_variable());

        context->writer_stopping = false;
        context->writer_thread = std::thread(matroska_writer_thread, context);
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Failed to start recording writer thread: %s", e.what());
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

void stop_matroska_writer_thread(k4a_record_context_t *context)
{
    RETURN_VALUE_IF_ARG(VOID_VALUE, context == NULL);
    RETURN_VALUE_IF_ARG(VOID_VALUE, context->writer_notify == nullptr);
    RETURN_VALUE_IF_ARG(VOID_VALUE, !context->writer_thread.joinable());

    try
    {
        context->writer_stopping = true;
        context->writer_notify->notify_one();
        context->writer_thread.join();
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Failed to stop recording writer thread: %s", e.what());
    }
}

KaxTag *
add_tag(k4a_record_context_t *context, const char *name, const char *value, TagTargetType target, uint64_t target_uid)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, !validate_name_characters(name));

    auto &tags = GetChild<KaxTags>(*context->file_segment);
    auto tag = new KaxTag();
    tags.PushElement(*tag); // Tag will be freed when the file is closed.

    auto &tagTargets = GetChild<KaxTagTargets>(*tag);
    switch (target)
    {
    case TAG_TARGET_TYPE_NONE:
        // Force KaxTagTargets element to get rendered since it is a "mandatory" element.
        GetChild<KaxTagTrackUID>(tagTargets).SetValue(0);
        break;
    case TAG_TARGET_TYPE_TRACK:
        GetChild<KaxTagTargetType>(tagTargets).SetValue("TRACK");
        GetChild<KaxTagTrackUID>(tagTargets).SetValue(target_uid);
        break;
    case TAG_TARGET_TYPE_ATTACHMENT:
        GetChild<KaxTagTargetType>(tagTargets).SetValue("ATTACHMENT");
        GetChild<KaxTagAttachmentUID>(tagTargets).SetValue(target_uid);
        break;
    }

    auto &tagSimple = GetChild<KaxTagSimple>(*tag);
    GetChild<KaxTagName>(tagSimple).SetValueUTF8(name);
    GetChild<KaxTagString>(tagSimple).SetValueUTF8(value);

    return tag;
}

KaxAttached *add_attachment(k4a_record_context_t *context,
                            const char *file_name,
                            const char *mime_type,
                            const uint8_t *buffer,
                            size_t buffer_size)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, file_name == NULL);
    RETURN_VALUE_IF_ARG(NULL, mime_type == NULL);
    RETURN_VALUE_IF_ARG(NULL, buffer == NULL);
    RETURN_VALUE_IF_ARG(NULL, context->header_written);

    auto &attachments = GetChild<KaxAttachments>(*context->file_segment);
    auto attached_file = new KaxAttached();
    attachments.PushElement(*attached_file); // File will be freed when the file is closed.

    GetChild<KaxFileName>(*attached_file).SetValueUTF8(file_name);
    GetChild<KaxMimeType>(*attached_file).SetValue(mime_type);
    GetChild<KaxFileUID>(*attached_file).SetValue(new_unique_id());

    assert(buffer_size <= UINT32_MAX);
    GetChild<KaxFileData>(*attached_file).CopyBuffer(buffer, (uint32)buffer_size);

    return attached_file;
}

uint64_t get_attachment_uid(KaxAttached *attachment)
{
    return GetChild<KaxFileUID>(*attachment).GetValue();
}

k4a_result_t get_matroska_segment(k4a_record_context_t *context,
                                  KaxSegment **file_segment,
                                  libebml::IOCallback **iocallback)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, file_segment == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, iocallback == NULL);

    *file_segment = context->file_segment.get();
    *iocallback = context->ebml_file.get();
    return K4A_RESULT_SUCCEEDED;
}

} // namespace k4arecord
