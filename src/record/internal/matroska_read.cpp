// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ctime>
#include <iostream>
#include <algorithm>
#include <climits>
#include <sstream>

#include <k4a/k4a.h>
#include <k4ainternal/matroska_read.h>
#include <k4ainternal/common.h>
#include <k4ainternal/logging.h>

#include <turbojpeg.h>
#include <libyuv.h>

using namespace LIBMATROSKA_NAMESPACE;

namespace k4arecord
{
std::unique_ptr<EbmlElement> next_child(k4a_playback_context_t *context, EbmlElement *parent)
{
    try
    {
        int upper_level = 0;
        EbmlElement *element =
            context->stream->FindNextElement(parent->Generic().Context, upper_level, parent->GetSize(), false, 0);

        // upper_level shows the relationship of the element to the parent element
        // -1 : global element
        //  0 : child
        //  1 : same level
        //  + : further parent
        if (upper_level > 0)
        {
            // This element is not a child of the parent, set the file pointer back to the start of the element and
            // return nullptr.
            uint64_t file_offset = element->GetElementPosition();
            assert(file_offset <= INT64_MAX);
            context->ebml_file->setFilePointer((int64_t)file_offset);
            delete element;
            return nullptr;
        }

        return std::unique_ptr<EbmlElement>(element);
    }
    catch (std::ios_base::failure &e)
    {
        LOG_ERROR("Failed to get next child (parent id %x) in recording '%s': %s",
                  EbmlId(*parent).GetValue(),
                  context->file_path,
                  e.what());
        return nullptr;
    }
}

k4a_result_t skip_element(k4a_playback_context_t *context, EbmlElement *element)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);

    try
    {
        element->SkipData(*context->stream, element->Generic().Context);

        return K4A_RESULT_SUCCEEDED;
    }
    catch (std::ios_base::failure &e)
    {
        LOG_ERROR("Failed seek past element (id %x) in recording '%s': %s",
                  EbmlId(*element).GetValue(),
                  context->file_path,
                  e.what());
        return K4A_RESULT_FAILED;
    }
}

void match_ebml_id(k4a_playback_context_t *context, EbmlId &id, uint64_t offset)
{
    RETURN_VALUE_IF_ARG(VOID_VALUE, context == NULL);

    LOG_TRACE("Matching seek location: %x -> %llu", id.GetValue(), offset);

    if (id == KaxSeekHead::ClassInfos.GlobalId || id == KaxChapters::ClassInfos.GlobalId ||
        id == EbmlVoid::ClassInfos.GlobalId)
    {
        return;
    }
    else if (id == KaxInfo::ClassInfos.GlobalId)
    {
        context->segment_info_offset = offset;
    }
    else if (id == KaxCluster::ClassInfos.GlobalId)
    {
        if (context->first_cluster_offset == 0 || context->first_cluster_offset > offset)
        {
            context->first_cluster_offset = offset;
        }
    }
    else if (id == KaxTracks::ClassInfos.GlobalId)
    {
        context->tracks_offset = offset;
    }
    else if (id == KaxCues::ClassInfos.GlobalId)
    {
        context->cues_offset = offset;
    }
    else if (id == KaxAttachments::ClassInfos.GlobalId)
    {
        context->attachments_offset = offset;
    }
    else if (id == KaxTags::ClassInfos.GlobalId)
    {
        context->tags_offset = offset;
    }
    else
    {
        LOG_WARNING("Unknown element being matched: %x at %llu", id.GetValue(), offset);
    }
}

bool seek_info_ready(k4a_playback_context_t *context)
{
    RETURN_VALUE_IF_ARG(false, context == NULL);

    return context->segment_info_offset > 0 && context->tracks_offset > 0 && context->tags_offset > 0 &&
           context->attachments_offset > 0 && context->first_cluster_offset > 0;
}

k4a_result_t parse_mkv(k4a_playback_context_t *context)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->stream == nullptr);

    // Read and verify the EBML head information from the file
    auto ebmlHead = find_next<EbmlHead>(context);
    if (ebmlHead)
    {
        if (read_element<EbmlHead>(context, ebmlHead.get()) == NULL)
        {
            LOG_ERROR("Failed to read EBML head.", 0);
            return K4A_RESULT_FAILED;
        }

        EDocType &eDocType = GetChild<EDocType>(*ebmlHead);
        EDocTypeVersion &eDocTypeVersion = GetChild<EDocTypeVersion>(*ebmlHead);
        EDocTypeReadVersion &eDocTypeReadVersion = GetChild<EDocTypeReadVersion>(*ebmlHead);
        if (eDocType.GetValue() != std::string("matroska"))
        {
            LOG_ERROR("DocType is not matroska: %s", eDocType.GetValue().c_str());
            return K4A_RESULT_FAILED;
        }

        if (eDocTypeReadVersion.GetValue() > 2)
        {
            LOG_ERROR("DocTypeReadVersion (%d) > 2 is not supported.", eDocTypeReadVersion.GetValue());
            return K4A_RESULT_FAILED;
        }

        if (eDocTypeVersion.GetValue() < eDocTypeReadVersion.GetValue())
        {
            LOG_ERROR("DocTypeVersion (%d) is great than DocTypeReadVersion (%d)",
                      eDocTypeVersion.GetValue(),
                      eDocTypeReadVersion.GetValue());
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        LOG_ERROR("Matroska / EBML head is missing, recording is not valid.", 0);
        return K4A_RESULT_FAILED;
    }

    // Find the offsets for each section of the file
    context->segment = find_next<KaxSegment>(context, true);
    if (context->segment)
    {
        auto element = next_child(context, context->segment.get());

        while (element != nullptr && !seek_info_ready(context))
        {
            EbmlId element_id(*element);
            match_ebml_id(context, element_id, context->segment->GetRelativePosition(*element.get()));

            if (element_id == KaxSeekHead::ClassInfos.GlobalId)
            {
                // Parse SeekHead offset positions
                KaxSeekHead *seek_head = read_element<KaxSeekHead>(context, element.get());

                KaxSeek *seek = NULL;
                for (EbmlElement *e : seek_head->GetElementList())
                {
                    if (check_element_type(e, &seek))
                    {
                        KaxSeekID &seek_id = GetChild<KaxSeekID>(*seek);
                        EbmlId ebml_id(seek_id.GetBuffer(), static_cast<unsigned int>(seek_id.GetSize()));
                        int64_t seek_location = seek->Location();
                        assert(seek_location >= 0);
                        match_ebml_id(context, ebml_id, (uint64_t)seek_location);
                    }
                }
            }
            else
            {
                skip_element(context, element.get());
            }

            element = next_child(context, context->segment.get());
        }
    }

    if (context->first_cluster_offset == 0)
    {
        LOG_ERROR("Recording file does not contain any frames!", 0);
        return K4A_RESULT_FAILED;
    }

    // Populate each element from the file (minus the actual Cluster data)
    RETURN_IF_ERROR(read_offset(context, context->segment_info, context->segment_info_offset));
    RETURN_IF_ERROR(read_offset(context, context->tracks, context->tracks_offset));
    if (context->cues_offset > 0)
        RETURN_IF_ERROR(read_offset(context, context->cues, context->cues_offset));
    if (context->attachments_offset > 0)
        RETURN_IF_ERROR(read_offset(context, context->attachments, context->attachments_offset));
    if (context->tags_offset > 0)
        RETURN_IF_ERROR(read_offset(context, context->tags, context->tags_offset));

    RETURN_IF_ERROR(parse_recording_config(context));
    RETURN_IF_ERROR(populate_cluster_cache(context));

    // Find the last timestamp in the file
    context->last_file_timestamp_ns = 0;
    cluster_info_t *cluster_info = find_cluster(context, UINT64_MAX);
    if (cluster_info == NULL)
    {
        LOG_ERROR("Failed to find end of recording.", 0);
        return K4A_RESULT_FAILED;
    }

    KaxSimpleBlock *simple_block = NULL;
    KaxBlockGroup *block_group = NULL;
    std::shared_ptr<KaxCluster> last_cluster = load_cluster_internal(context, cluster_info);
    if (last_cluster == nullptr)
    {
        LOG_ERROR("Failed to load end of recording.", 0);
        return K4A_RESULT_FAILED;
    }
    for (EbmlElement *e : last_cluster->GetElementList())
    {
        if (check_element_type(e, &simple_block))
        {
            simple_block->SetParent(*last_cluster);
            uint64_t block_timestamp_ns = simple_block->GlobalTimecode();
            if (block_timestamp_ns > context->last_file_timestamp_ns)
            {
                context->last_file_timestamp_ns = block_timestamp_ns;
            }
        }
        else if (check_element_type(e, &block_group))
        {
            block_group->SetParent(*last_cluster);

            KaxTrackEntry *parent_track = NULL;
            for (EbmlElement *e2 : context->tracks->GetElementList())
            {
                if (check_element_type(e2, &parent_track))
                {
                    if (GetChild<KaxTrackNumber>(*parent_track).GetValue() == (uint64)block_group->TrackNumber())
                    {
                        parent_track->SetGlobalTimecodeScale(context->timecode_scale);
                    }
                }
            }
            uint64_t block_timestamp_ns = block_group->GlobalTimecode();
            if (parent_track)
            {
                block_group->SetParentTrack(*parent_track);
                uint64_t block_duration_ns = 0;
                if (block_group->GetBlockDuration(block_duration_ns))
                {
                    block_timestamp_ns += block_duration_ns - 1;
                }
            }
            if (block_timestamp_ns > context->last_file_timestamp_ns)
            {
                context->last_file_timestamp_ns = block_timestamp_ns;
            }
        }
    }
    LOG_TRACE("Found last file timestamp: %llu", context->last_file_timestamp_ns);

    return K4A_RESULT_SUCCEEDED;
}

static void cluster_cache_deleter(cluster_info_t *cluster_cache)
{
    while (cluster_cache)
    {
        cluster_info_t *tmp = cluster_cache;
        cluster_cache = cluster_cache->next;
        delete tmp;
    }
}

k4a_result_t populate_cluster_cache(k4a_playback_context_t *context)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->cluster_cache != nullptr);

    // Read the first cluster to use as the cache root.
    if (K4A_FAILED(seek_offset(context, context->first_cluster_offset)))
    {
        LOG_ERROR("Failed to seek to first recording cluster.", 0);
        return K4A_RESULT_FAILED;
    }
    std::shared_ptr<KaxCluster> first_cluster = find_next<KaxCluster>(context);
    if (first_cluster == nullptr)
    {
        LOG_ERROR("Failed to read element for first recording cluster.", 0);
        return K4A_RESULT_FAILED;
    }

    try
    {
        std::lock_guard<std::recursive_mutex> lock(context->cache_lock);

        context->cluster_cache = cluster_cache_t(new cluster_info_t, cluster_cache_deleter);
        populate_cluster_info(context, first_cluster, context->cluster_cache.get());

        // Populate the rest of the cache with the Cue data stored in the file.
        cluster_info_t *cluster_cache_end = context->cluster_cache.get();
        if (context->cues)
        {
            uint64_t last_offset = context->first_cluster_offset;
            uint64_t last_timestamp_ns = context->cluster_cache->timestamp_ns;
            KaxCuePoint *cue = NULL;
            for (EbmlElement *e : context->cues->GetElementList())
            {
                if (check_element_type(e, &cue))
                {
                    const KaxCueTrackPositions *positions = cue->GetSeekPosition();
                    if (positions)
                    {
                        uint64_t timestamp_ns = GetChild<KaxCueTime>(*cue).GetValue() * context->timecode_scale;
                        uint64_t file_offset = positions->ClusterPosition();

                        if (file_offset == last_offset)
                        {
                            // This cluster is already in the cache, skip it.
                            continue;
                        }
                        else if (file_offset > last_offset && timestamp_ns >= last_timestamp_ns)
                        {
                            cluster_info_t *cluster_info = new cluster_info_t;
                            // This timestamp might not actually be the start of the cluster.
                            // The start timestamp is not known until populate_cluster_info is called.
                            cluster_info->timestamp_ns = timestamp_ns;
                            cluster_info->file_offset = file_offset;
                            cluster_info->previous = cluster_cache_end;

                            cluster_cache_end->next = cluster_info;
                            cluster_cache_end = cluster_info;

                            last_offset = file_offset;
                            last_timestamp_ns = timestamp_ns;
                        }
                        else
                        {
                            LOG_WARNING("Cluster or Cue entry is out of order.", 0);
                        }
                    }
                }
            }
        }
        else
        {
            LOG_WARNING("Recording is missing Cue entries, playback performance may be impacted.", 0);
        }
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Failed to populate cluster cache: %s", e.what());
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t parse_recording_config(k4a_playback_context_t *context)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->segment_info == nullptr);

    context->timecode_scale = GetChild<KaxTimecodeScale>(*context->segment_info).GetValue();

    if (K4A_FAILED(parse_tracks(context)))
    {
        LOG_ERROR("Reading track data failed.", 0);
        return K4A_RESULT_FAILED;
    }

    context->color_track = find_track(context, "COLOR", "K4A_COLOR_TRACK");
    context->depth_track = find_track(context, "DEPTH", "K4A_DEPTH_TRACK");
    context->ir_track = find_track(context, "IR", "K4A_IR_TRACK");
    if (context->ir_track == NULL)
    {
        // Support legacy IR track naming.
        context->ir_track = find_track(context, "DEPTH_IR", NULL);
    }
    context->imu_track = find_track(context, "IMU", "K4A_IMU_TRACK");

    // Read device calibration attachment
    context->calibration_attachment = get_attachment_by_tag(context, "K4A_CALIBRATION_FILE");
    if (context->calibration_attachment == NULL)
    {
        context->calibration_attachment = get_attachment_by_name(context, "calibration.json");
    }
    if (context->calibration_attachment == NULL)
    {
        // The rest of the recording can still be read if no device calibration blob exists.
        LOG_WARNING("Device calibration is missing from recording.", 0);
    }

    uint64_t frame_period_ns = 0;
    if (context->color_track)
    {
        if (context->color_track->type != track_video)
        {
            LOG_ERROR("Color track is not a video track.", 0);
            return K4A_RESULT_FAILED;
        }

        frame_period_ns = context->color_track->frame_period_ns;

        RETURN_IF_ERROR(read_bitmap_info_header(context->color_track));
        context->record_config.color_resolution = K4A_COLOR_RESOLUTION_OFF;
        for (size_t i = 0; i < arraysize(color_resolutions); i++)
        {
            uint32_t width, height;
            if (k4a_convert_resolution_to_width_height(color_resolutions[i], &width, &height))
            {
                if (context->color_track->width == width && context->color_track->height == height)
                {
                    context->record_config.color_resolution = color_resolutions[i];
                    break;
                }
            }
        }

        if (context->record_config.color_resolution == K4A_COLOR_RESOLUTION_OFF)
        {
            LOG_WARNING("The color resolution is not officially supported: %dx%d. You cannot get the calibration "
                        "information for this color resolution",
                        context->color_track->width,
                        context->color_track->height);
        }

        context->record_config.color_track_enabled = true;
        context->record_config.color_format = context->color_track->format;
        context->color_format_conversion = context->color_track->format;
    }
    else
    {
        context->record_config.color_resolution = K4A_COLOR_RESOLUTION_OFF;
        // Set to a default color format if color track is disabled.
        context->record_config.color_format = K4A_IMAGE_FORMAT_CUSTOM;
        context->color_format_conversion = K4A_IMAGE_FORMAT_CUSTOM;
    }

    KaxTag *depth_mode_tag = get_tag(context, "K4A_DEPTH_MODE");
    if (depth_mode_tag == NULL && (context->depth_track || context->ir_track))
    {
        LOG_ERROR("K4A_DEPTH_MODE tag is missing.", 0);
        return K4A_RESULT_FAILED;
    }

    std::string depth_mode_str;
    uint32_t depth_width = 0;
    uint32_t depth_height = 0;
    context->record_config.depth_mode = K4A_DEPTH_MODE_OFF;

    if (depth_mode_tag != NULL)
    {
        depth_mode_str = get_tag_string(depth_mode_tag);
        for (size_t i = 0; i < arraysize(depth_modes); i++)
        {
            if (depth_mode_str == depth_modes[i].second)
            {
                if (k4a_convert_depth_mode_to_width_height(depth_modes[i].first, &depth_width, &depth_height))
                {
                    context->record_config.depth_mode = depth_modes[i].first;
                    break;
                }
            }
        }

        if (context->record_config.depth_mode == K4A_DEPTH_MODE_OFF)
        {
            // Try to find the mode matching strings in the legacy modes
            for (size_t i = 0; i < arraysize(legacy_depth_modes); i++)
            {
                if (depth_mode_str == legacy_depth_modes[i].second)
                {
                    if (k4a_convert_depth_mode_to_width_height(legacy_depth_modes[i].first,
                                                               &depth_width,
                                                               &depth_height))
                    {
                        context->record_config.depth_mode = legacy_depth_modes[i].first;
                        break;
                    }
                }
            }
        }
        if (context->record_config.depth_mode == K4A_DEPTH_MODE_OFF)
        {
            LOG_ERROR("Unsupported depth mode: %s", depth_mode_str.c_str());
            return K4A_RESULT_FAILED;
        }
    }

    if (context->depth_track)
    {
        if (context->depth_track->type != track_video)
        {
            LOG_ERROR("Depth track is not a video track.", 0);
            return K4A_RESULT_FAILED;
        }

        if (frame_period_ns == 0)
        {
            frame_period_ns = context->depth_track->frame_period_ns;
        }
        else if (frame_period_ns != context->depth_track->frame_period_ns)
        {
            LOG_ERROR("Track frame durations don't match (Depth): %llu ns != %llu ns",
                      frame_period_ns,
                      context->depth_track->frame_period_ns);
            return K4A_RESULT_FAILED;
        }

        if (context->depth_track->width != depth_width || context->depth_track->height != depth_height)
        {
            LOG_ERROR("Unsupported depth resolution / mode: %dx%d (%s)",
                      context->depth_track->width,
                      context->depth_track->height,
                      depth_mode_str.c_str());
            return K4A_RESULT_FAILED;
        }

        RETURN_IF_ERROR(read_bitmap_info_header(context->depth_track));

        context->record_config.depth_track_enabled = true;
    }

    if (context->ir_track)
    {
        if (context->ir_track->type != track_video)
        {
            LOG_ERROR("IR track is not a video track.", 0);
            return K4A_RESULT_FAILED;
        }

        if (frame_period_ns == 0)
        {
            frame_period_ns = context->ir_track->frame_period_ns;
        }
        else if (frame_period_ns != context->ir_track->frame_period_ns)
        {
            LOG_ERROR("Track frame durations don't match (IR): %llu ns != %llu ns",
                      frame_period_ns,
                      context->ir_track->frame_period_ns);
            return K4A_RESULT_FAILED;
        }

        if (context->depth_track)
        {
            if (context->ir_track->width != context->depth_track->width ||
                context->ir_track->height != context->depth_track->height)
            {
                LOG_ERROR("Depth and IR track have different resolutions: Depth %dx%d, IR %dx%d",
                          context->depth_track->width,
                          context->depth_track->height,
                          context->ir_track->width,
                          context->ir_track->height);
                return K4A_RESULT_FAILED;
            }
        }
        else if (context->ir_track->width != depth_width || context->ir_track->height != depth_height)
        {
            LOG_ERROR("Unsupported IR resolution / depth mode: %dx%d (%s)",
                      context->ir_track->width,
                      context->ir_track->height,
                      depth_mode_str.c_str());
            return K4A_RESULT_FAILED;
        }

        RETURN_IF_ERROR(read_bitmap_info_header(context->ir_track));
        if (context->ir_track->format == K4A_IMAGE_FORMAT_DEPTH16)
        {
            context->ir_track->format = K4A_IMAGE_FORMAT_IR16;
        }

        context->record_config.ir_track_enabled = true;
    }

    context->sync_period_ns = frame_period_ns;
    if (frame_period_ns > 0)
    {
        switch (1_s / frame_period_ns)
        {
        case 5:
            context->record_config.camera_fps = K4A_FRAMES_PER_SECOND_5;
            break;
        case 15:
            context->record_config.camera_fps = K4A_FRAMES_PER_SECOND_15;
            break;
        case 30:
            context->record_config.camera_fps = K4A_FRAMES_PER_SECOND_30;
            break;
        default:
            LOG_ERROR("Unsupported recording frame period: %llu ns (%llu fps)",
                      frame_period_ns,
                      (1_s / frame_period_ns));
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        // Default to 30 fps if no video tracks are enabled.
        context->record_config.camera_fps = K4A_FRAMES_PER_SECOND_30;
    }

    // Read depth_delay_off_color_usec and set offsets for each builtin track accordingly.
    KaxTag *depth_delay_tag = get_tag(context, "K4A_DEPTH_DELAY_NS");
    if (depth_delay_tag != NULL)
    {
        int64_t depth_delay_ns;
        std::istringstream depth_delay_str(get_tag_string(depth_delay_tag));
        depth_delay_str >> depth_delay_ns;
        if (!depth_delay_str.fail())
        {
            assert(depth_delay_ns / 1000 <= INT32_MAX);
            context->record_config.depth_delay_off_color_usec = (int32_t)(depth_delay_ns / 1000);

            // Only set positive delays so that we don't wrap around near 0.
            if (depth_delay_ns > 0 && context->color_track)
            {
                context->color_track->sync_delay_ns = (uint64_t)depth_delay_ns;
            }
            else if (depth_delay_ns < 0)
            {
                if (context->depth_track)
                    context->depth_track->sync_delay_ns = (uint64_t)(-depth_delay_ns);
                if (context->ir_track)
                    context->ir_track->sync_delay_ns = (uint64_t)(-depth_delay_ns);
            }
        }
        else
        {
            LOG_ERROR("Tag K4A_DEPTH_DELAY_NS contains invalid value: %s", get_tag_string(depth_delay_tag).c_str());
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        context->record_config.depth_delay_off_color_usec = 0;
    }

    if (context->imu_track)
    {
        if (context->imu_track->type == track_subtitle)
        {
            context->record_config.imu_track_enabled = true;
        }
        else
        {
            LOG_WARNING("IMU track is not correct type, treating as a custom track.", 0);
            context->imu_track = nullptr;
        }
    }

    // Read wired_sync_mode and subordinate_delay_off_master_usec.
    KaxTag *sync_mode_tag = get_tag(context, "K4A_WIRED_SYNC_MODE");
    if (sync_mode_tag != NULL)
    {
        bool sync_mode_found = false;
        std::string sync_mode_str = get_tag_string(sync_mode_tag);
        for (size_t i = 0; i < arraysize(external_sync_modes); i++)
        {
            if (sync_mode_str == external_sync_modes[i].second)
            {
                context->record_config.wired_sync_mode = external_sync_modes[i].first;
                sync_mode_found = true;
                break;
            }
        }
        if (!sync_mode_found)
        {
            LOG_ERROR("Unsupported wired sync mode: %s", sync_mode_str.c_str());
            return K4A_RESULT_FAILED;
        }

        if (context->record_config.wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE)
        {
            KaxTag *subordinate_delay_tag = get_tag(context, "K4A_SUBORDINATE_DELAY_NS");
            if (subordinate_delay_tag != NULL)
            {
                uint64_t subordinate_delay_ns;
                std::istringstream subordinate_delay_str(get_tag_string(subordinate_delay_tag));
                subordinate_delay_str >> subordinate_delay_ns;
                if (!subordinate_delay_str.fail())
                {
                    assert(subordinate_delay_ns / 1000 <= UINT32_MAX);
                    context->record_config.subordinate_delay_off_master_usec = (uint32_t)(subordinate_delay_ns / 1000);
                }
                else
                {
                    LOG_ERROR("Tag K4A_SUBORDINATE_DELAY_NS contains invalid value: %s",
                              get_tag_string(subordinate_delay_tag).c_str());
                    return K4A_RESULT_FAILED;
                }
            }
            else
            {
                context->record_config.subordinate_delay_off_master_usec = 0;
            }
        }
        else
        {
            context->record_config.subordinate_delay_off_master_usec = 0;
        }
    }
    else
    {
        context->record_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
        context->record_config.subordinate_delay_off_master_usec = 0;
    }

    KaxTag *start_offset_tag = get_tag(context, "K4A_START_OFFSET_NS");
    if (start_offset_tag != NULL)
    {
        uint64_t start_offset_ns;
        std::istringstream start_offset_str(get_tag_string(start_offset_tag));
        start_offset_str >> start_offset_ns;
        if (!start_offset_str.fail())
        {
            assert(start_offset_ns / 1000 <= UINT32_MAX);
            context->record_config.start_timestamp_offset_usec = (uint32_t)(start_offset_ns / 1000);
        }
        else
        {
            LOG_ERROR("Tag K4A_START_OFFSET_NS contains invalid value: %s", get_tag_string(start_offset_tag).c_str());
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        context->record_config.start_timestamp_offset_usec = 0;
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t read_bitmap_info_header(track_reader_t *track)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, track == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, track->track == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, track->codec_id.empty());

    if (track->codec_id == "V_MS/VFW/FOURCC")
    {
        KaxCodecPrivate &codec_private = GetChild<KaxCodecPrivate>(*track->track);
        RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, codec_private.GetSize() != sizeof(BITMAPINFOHEADER));
        track->codec_private.assign(codec_private.GetBuffer(), codec_private.GetBuffer() + codec_private.GetSize());

        BITMAPINFOHEADER *bitmap_header = reinterpret_cast<BITMAPINFOHEADER *>(track->codec_private.data());

        assert(track->width == bitmap_header->biWidth);
        assert(track->height == bitmap_header->biHeight);

        switch (bitmap_header->biCompression)
        {
        case 0x3231564E: // NV12
            track->format = K4A_IMAGE_FORMAT_COLOR_NV12;
            track->stride = track->width;
            break;
        case 0x32595559: // YUY2
            track->format = K4A_IMAGE_FORMAT_COLOR_YUY2;
            track->stride = track->width * 2;
            break;
        case 0x47504A4D: // MJPG
            track->format = K4A_IMAGE_FORMAT_COLOR_MJPG;
            track->stride = 0;
            break;
        case 0x67363162: // b16g
            track->format = K4A_IMAGE_FORMAT_DEPTH16;
            track->stride = track->width * 2;
            break;
        case 0x41524742: // BGRA
            track->format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
            track->stride = track->width * 4;
            break;
        default:
            LOG_ERROR("Unsupported FOURCC format for track '%s': %x",
                      GetChild<KaxTrackName>(*track->track).GetValueUTF8().c_str(),
                      bitmap_header->biCompression);
            return K4A_RESULT_FAILED;
        }

        return K4A_RESULT_SUCCEEDED;
    }
    else
    {
        LOG_ERROR("Unsupported codec id for track '%s': %s",
                  GetChild<KaxTrackName>(*track->track).GetValueUTF8().c_str(),
                  track->codec_id.c_str());
        return K4A_RESULT_FAILED;
    }
}

void reset_seek_pointers(k4a_playback_context_t *context, uint64_t seek_timestamp_ns)
{
    RETURN_VALUE_IF_ARG(VOID_VALUE, context == NULL);

    context->seek_timestamp_ns = seek_timestamp_ns;

    for (auto &itr : context->track_map)
    {
        auto &track_reader = itr.second;
        track_reader.current_block.reset();
    }
}

k4a_result_t parse_tracks(k4a_playback_context_t *context)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->tracks == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->timecode_scale == 0);

    KaxTrackEntry *track = NULL;
    for (EbmlElement *e : context->tracks->GetElementList())
    {
        if (check_element_type(e, &track))
        {
            std::string track_name = GetChild<KaxTrackName>(*track).GetValueUTF8();

            track_reader_t track_reader;
            track_reader.track = track;

            // Read generic tracks information
            track_reader.track_name = GetChild<KaxTrackName>(*track).GetValueUTF8();
            track_reader.track_uid = GetChild<KaxTrackUID>(*track).GetValue();
            track_reader.codec_id = GetChild<KaxCodecID>(*track).GetValue();
            KaxCodecPrivate &codec_private = GetChild<KaxCodecPrivate>(*track);
            track_reader.codec_private.assign(codec_private.GetBuffer(),
                                              codec_private.GetBuffer() + codec_private.GetSize());

            track_reader.frame_period_ns = GetChild<KaxTrackDefaultDuration>(*track).GetValue();

            track_reader.track->SetGlobalTimecodeScale(context->timecode_scale);

            // Read track type specific information
            track_reader.type = static_cast<track_type>(GetChild<KaxTrackType>(*track).GetValue());
            if (track_reader.type == track_video)
            {
                KaxTrackVideo &video_track = GetChild<KaxTrackVideo>(*track);
                track_reader.width = static_cast<uint32_t>(GetChild<KaxVideoPixelWidth>(video_track).GetValue());
                track_reader.height = static_cast<uint32_t>(GetChild<KaxVideoPixelHeight>(video_track).GetValue());
            }

            context->track_map.insert(std::pair<std::string, track_reader_t>(track_name, track_reader));
        }
    }

    return K4A_RESULT_SUCCEEDED;
}

track_reader_t *find_track(k4a_playback_context_t *context, const char *name, const char *tag_name)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, context->tracks == nullptr);
    RETURN_VALUE_IF_ARG(NULL, name == NULL);

    std::string search_name(name);
    uint64_t search_uid = 0;

    if (tag_name != NULL)
    {
        KaxTag *track_tag = get_tag(context, tag_name);
        if (track_tag)
        {
            KaxTagTargets &tagTargets = GetChild<KaxTagTargets>(*track_tag);
            if (GetChild<KaxTagTargetType>(tagTargets).GetValue() == "TRACK")
            {
                search_uid = GetChild<KaxTagTrackUID>(tagTargets).GetValue();
            }
            if (search_uid == 0)
            {
                std::istringstream search_uid_str(get_tag_string(track_tag));
                search_uid_str >> search_uid;
                if (search_uid_str.fail())
                {
                    LOG_ERROR("Track tag '%s' for track %s is not valid.", tag_name, name);
                    search_uid = 0;
                }
            }
        }
    }

    track_reader_t *found_reader = NULL;
    for (auto &itr : context->track_map)
    {
        auto &reader = itr.second;
        if (search_uid != 0 && reader.track_uid == search_uid)
        {
            found_reader = &reader;
            break;
        }
        else if (reader.track_name == search_name)
        {
            found_reader = &reader;
            // Search by UID has priority over search by name, keep searching for a matching UID.
        }
    }

    return found_reader;
}

bool check_track_reader_is_builtin(k4a_playback_context_t *context, track_reader_t *track_reader)
{
    RETURN_VALUE_IF_ARG(false, context == NULL);
    RETURN_VALUE_IF_ARG(false, track_reader == NULL);

    return track_reader == context->color_track || track_reader == context->depth_track ||
           track_reader == context->ir_track || track_reader == context->imu_track;
}

track_reader_t *get_track_reader_by_name(k4a_playback_context_t *context, std::string track_name)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);

    auto itr = context->track_map.find(track_name);
    if (itr != context->track_map.end())
    {
        return &itr->second;
    }

    return nullptr;
}

KaxTag *get_tag(k4a_playback_context_t *context, const char *name)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, name == NULL);

    // Only search for the tag if the parent element exists
    if (context->tags)
    {
        std::string search(name);
        KaxTag *tag = NULL;
        for (EbmlElement *e : context->tags->GetElementList())
        {
            if (check_element_type(e, &tag))
            {
                KaxTagSimple &tagSimple = GetChild<KaxTagSimple>(*tag);
                if (GetChild<KaxTagName>(tagSimple).GetValueUTF8() == search)
                {
                    return tag;
                }
            }
        }
    }

    return NULL;
}

std::string get_tag_string(KaxTag *tag)
{
    RETURN_VALUE_IF_ARG(std::string(), tag == NULL);

    KaxTagSimple &tagSimple = GetChild<KaxTagSimple>(*tag);
    return GetChild<KaxTagString>(tagSimple).GetValueUTF8();
}

KaxAttached *get_attachment_by_name(k4a_playback_context_t *context, const char *file_name)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, file_name == NULL);

    // Only search for the attachment if the parent element exists
    if (context->attachments)
    {
        std::string search(file_name);
        KaxAttached *attached = NULL;
        for (EbmlElement *e : context->attachments->GetElementList())
        {
            if (check_element_type(e, &attached))
            {
                if (GetChild<KaxFileName>(*attached).GetValueUTF8() == search)
                {
                    return attached;
                }
            }
        }
    }

    return NULL;
}

KaxAttached *get_attachment_by_tag(k4a_playback_context_t *context, const char *tag_name)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, tag_name == NULL);

    // Only search for the attachment if the parent element exists
    if (context->attachments)
    {
        KaxTag *attachment_tag = get_tag(context, tag_name);
        if (attachment_tag)
        {
            KaxTagTargets &tagTargets = GetChild<KaxTagTargets>(*attachment_tag);
            if (GetChild<KaxTagTargetType>(tagTargets).GetValue() != "ATTACHMENT")
            {
                return NULL;
            }
            uint64_t search_uid = GetChild<KaxTagAttachmentUID>(tagTargets).GetValue();

            KaxAttached *attached = NULL;
            for (EbmlElement *e : context->attachments->GetElementList())
            {
                if (check_element_type(e, &attached))
                {
                    if (GetChild<KaxFileUID>(*attached).GetValue() == search_uid)
                    {
                        return attached;
                    }
                }
            }
        }
    }

    return NULL;
}

k4a_result_t seek_offset(k4a_playback_context_t *context, uint64_t offset)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->segment == nullptr);

    context->seek_count++;
    uint64_t file_offset = context->segment->GetGlobalPosition(offset);
    try
    {
        LOG_TRACE("Seeking to file position: %llu", file_offset);
        assert(file_offset <= INT64_MAX);
        context->ebml_file->setFilePointer((int64_t)file_offset);
        return K4A_RESULT_SUCCEEDED;
    }
    catch (std::ios_base::failure &e)
    {
        LOG_ERROR("Failed to seek file to %llu (relative %llu) '%s': %s",
                  file_offset,
                  offset,
                  context->file_path,
                  e.what());
        return K4A_RESULT_FAILED;
    }
}

// Read the cluster metadata from a Matroska element and add it to a cache entry.
// File read pointer should already be at the start of the cluster.
// The caller should currently own the lock for the cluster cache.
void populate_cluster_info(k4a_playback_context_t *context,
                           std::shared_ptr<KaxCluster> &cluster,
                           cluster_info_t *cluster_info)
{
    RETURN_VALUE_IF_ARG(VOID_VALUE, context == NULL);
    RETURN_VALUE_IF_ARG(VOID_VALUE, context->segment == nullptr);
    RETURN_VALUE_IF_ARG(VOID_VALUE, cluster == nullptr);
    RETURN_VALUE_IF_ARG(VOID_VALUE, cluster_info == NULL);
    RETURN_VALUE_IF_ARG(VOID_VALUE, cluster_info->previous && cluster_info->previous->next != cluster_info);
    RETURN_VALUE_IF_ARG(VOID_VALUE, cluster_info->next && cluster_info->next->previous != cluster_info);

    if (cluster_info->cluster_size > 0)
    {
        // If the cluster size is already set, then we have nothing to do.
        return;
    }

    cluster_info->file_offset = context->segment->GetRelativePosition(*cluster.get());
    cluster_info->cluster_size = cluster->HeadSize() + cluster->GetSize();

    // Check the previous and next cluster entry to see if we can link them together without a gap.
    if (cluster_info->previous &&
        (cluster_info->previous->file_offset + cluster_info->previous->cluster_size) == cluster_info->file_offset)
    {
        cluster_info->previous->next_known = true;
    }
    if (cluster_info->next &&
        (cluster_info->file_offset + cluster_info->cluster_size) == cluster_info->next->file_offset)
    {
        cluster_info->next_known = true;
    }

    // Update the timestamp to the real cluster start read from the file.
    auto element = next_child(context, cluster.get());
    while (element != nullptr)
    {
        EbmlId element_id(*element);
        if (element_id == KaxClusterTimecode::ClassInfos.GlobalId)
        {
            KaxClusterTimecode *cluster_timecode = read_element<KaxClusterTimecode>(context, element.get());
            cluster_info->timestamp_ns = cluster_timecode->GetValue() * context->timecode_scale;
            break;
        }
        else
        {
            skip_element(context, element.get());
        }

        element = next_child(context, cluster.get());
    }
}

// Find the cluster containing the specified timestamp and return a pointer to its cache entry.
// If an exact cluster is not found, the cluster with the closest timestamp will be returned.
cluster_info_t *find_cluster(k4a_playback_context_t *context, uint64_t timestamp_ns)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, context->cluster_cache == nullptr);

    try
    {
        std::lock_guard<std::recursive_mutex> lock(context->cache_lock);

        // Find the closest cluster in the cache
        cluster_info_t *cluster_info = context->cluster_cache.get();
        while (cluster_info->next)
        {
            if (cluster_info->next->timestamp_ns > timestamp_ns)
            {
                break;
            }
            cluster_info = cluster_info->next;
        }

        // Make sure there are no gaps in the cache and ensure this really is the closest cluster.
        cluster_info_t *next_cluster_info = next_cluster(context, cluster_info, true);
        while (next_cluster_info)
        {
            if (next_cluster_info->timestamp_ns > timestamp_ns)
            {
                break;
            }
            cluster_info = next_cluster_info;
            next_cluster_info = next_cluster(context, cluster_info, true);
        }
        return cluster_info;
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Failed to find cluster for timestamp %llu: %s", timestamp_ns, e.what());
        return NULL;
    }
}

// Finds the next or previous cluster given a current cluster. This function checks the cluster_cache first to see if
// the next cluster is known, otherwise a cache entry will be added. If there is no next cluster, NULL is returned.
cluster_info_t *next_cluster(k4a_playback_context_t *context, cluster_info_t *current_cluster, bool next)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, context->segment == nullptr);
    RETURN_VALUE_IF_ARG(NULL, context->cluster_cache == nullptr);
    RETURN_VALUE_IF_ARG(NULL, current_cluster == NULL);

    try
    {
        std::lock_guard<std::recursive_mutex> lock(context->cache_lock);

        if (next)
        {
            if (current_cluster->next_known)
            {
                // If end of file, next will be NULL
                return current_cluster->next;
            }
            else
            {
                std::lock_guard<std::mutex> io_lock(context->io_lock);
                if (context->file_closing)
                {
                    // User called k4a_playback_close(), return immediately.
                    return NULL;
                }

                LargeFileIOCallback *file_io = dynamic_cast<LargeFileIOCallback *>(context->ebml_file.get());
                if (file_io != NULL)
                {
                    file_io->setOwnerThread();
                }

                // Read forward in file to find next cluster and fill in cache
                if (K4A_FAILED(seek_offset(context, current_cluster->file_offset)))
                {
                    LOG_ERROR("Failed to seek to current cluster element.", 0);
                    return NULL;
                }
                std::shared_ptr<KaxCluster> current_element = find_next<KaxCluster>(context);
                if (current_element == nullptr)
                {
                    LOG_ERROR("Failed to find current cluster element.", 0);
                    return NULL;
                }
                populate_cluster_info(context, current_element, current_cluster);
                if (current_cluster->next_known)
                {
                    // If populate_cluster_info() just connected the next entry, we can exit early.
                    return current_cluster->next;
                }

                // Seek to the end of the current cluster so that find_next returns the next cluster in the file.
                if (K4A_FAILED(skip_element(context, current_element.get())))
                {
                    LOG_ERROR("Failed to seek to next cluster element.", 0);
                    return NULL;
                }

                std::shared_ptr<KaxCluster> next_cluster = find_next<KaxCluster>(context, true);
                if (next_cluster)
                {
                    if (current_cluster->next && current_cluster->next->file_offset ==
                                                     context->segment->GetRelativePosition(*next_cluster.get()))
                    {
                        // If there is a non-cluster element between these entries, they may not get connected
                        // otherwise.
                        current_cluster->next_known = true;
                        current_cluster = current_cluster->next;
                    }
                    else
                    {
                        // Add a new entry to the cache for the cluster we just found.
                        cluster_info_t *next_cluster_info = new cluster_info_t;
                        next_cluster_info->previous = current_cluster;
                        next_cluster_info->next = current_cluster->next;
                        current_cluster->next = next_cluster_info;
                        current_cluster->next_known = true;
                        if (next_cluster_info->next)
                        {
                            next_cluster_info->next->previous = next_cluster_info;
                        }
                        current_cluster = next_cluster_info;
                    }
                    populate_cluster_info(context, next_cluster, current_cluster);
                    return current_cluster;
                }
                else
                {
                    // End of file reached
                    current_cluster->next_known = true;
                    return NULL;
                }
            }
        }
        else
        {
            if (current_cluster->previous)
            {
                if (current_cluster->previous->next_known)
                {
                    return current_cluster->previous;
                }
                else
                {
                    // Read forward from previous cached cluster to fill in gap
                    cluster_info_t *next_cluster_info = next_cluster(context, current_cluster->previous, true);
                    while (next_cluster_info && next_cluster_info != current_cluster)
                    {
                        next_cluster_info = next_cluster(context, next_cluster_info, true);
                    }
                    return current_cluster->previous;
                }
            }
            else
            {
                // Beginning of file reached
                return NULL;
            }
        }
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Failed to find next cluster: %s", e.what());
        return NULL;
    }
}

// Load a cluster from the cluster cache / disk without any neighbor preloading.
// This should never fail unless there is a file IO error.
std::shared_ptr<KaxCluster> load_cluster_internal(k4a_playback_context_t *context, cluster_info_t *cluster_info)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);
    RETURN_VALUE_IF_ARG(nullptr, context->ebml_file == nullptr);

    try
    {
        // Check if the cluster already exists in memory, and if so, return it.
        std::shared_ptr<KaxCluster> cluster = cluster_info->cluster.lock();
        if (cluster)
        {
            context->cache_hits++;
        }
        else
        {
            std::lock_guard<std::mutex> lock(context->io_lock);
            if (context->file_closing)
            {
                // User called k4a_playback_close(), return immediately.
                return nullptr;
            }

            // The cluster may have been loaded while we were acquiring the io lock, check again before actually loading
            // from disk.
            cluster = cluster_info->cluster.lock();
            if (cluster)
            {
                context->cache_hits++;
            }
            else
            {
                context->load_count++;

                // Start reading the actual cluster data from disk.
                LargeFileIOCallback *file_io = dynamic_cast<LargeFileIOCallback *>(context->ebml_file.get());
                if (file_io != NULL)
                {
                    file_io->setOwnerThread();
                }

                if (K4A_FAILED(seek_offset(context, cluster_info->file_offset)))
                {
                    LOG_ERROR("Failed to seek to cluster cluster at: %llu", cluster_info->file_offset);
                    return nullptr;
                }
                cluster = find_next<KaxCluster>(context, true);
                if (cluster)
                {
                    if (read_element<KaxCluster>(context, cluster.get()) == NULL)
                    {
                        LOG_ERROR("Failed to load cluster at: %llu", cluster_info->file_offset);
                        return nullptr;
                    }

                    uint64_t timecode = GetChild<KaxClusterTimecode>(*cluster).GetValue();
                    assert(context->timecode_scale <= INT64_MAX);
                    cluster->InitTimecode(timecode, (int64_t)context->timecode_scale);

                    cluster_info->cluster = cluster;
                }
            }
        }
        return cluster;
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Failed to load cluster from disk: %s", e.what());
        return nullptr;
    }
}

// Load the actual block data for a cluster off the disk, and start preloading the neighboring clusters.
// This should never fail unless there is a file IO error.
std::shared_ptr<loaded_cluster_t> load_cluster(k4a_playback_context_t *context, cluster_info_t *cluster_info)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);
    RETURN_VALUE_IF_ARG(nullptr, context->cluster_cache == nullptr);
    RETURN_VALUE_IF_ARG(nullptr, cluster_info == NULL);

    std::shared_ptr<KaxCluster> cluster = load_cluster_internal(context, cluster_info);
    if (cluster == nullptr)
    {
        return nullptr;
    }

    std::shared_ptr<loaded_cluster_t> result = std::shared_ptr<loaded_cluster_t>(new loaded_cluster_t());
    result->cluster_info = cluster_info;
    result->cluster = cluster;

#if CLUSTER_READ_AHEAD_COUNT
    try
    {
        // Preload the neighboring clusters immediately
        cluster_info_t *previous_cluster_info = cluster_info;
        cluster_info_t *next_cluster_info = cluster_info;
        for (size_t i = 0; i < CLUSTER_READ_AHEAD_COUNT; i++)
        {
            if (previous_cluster_info != NULL)
            {
                previous_cluster_info = next_cluster(context, previous_cluster_info, false);
            }
            if (next_cluster_info != NULL)
            {
                next_cluster_info = next_cluster(context, next_cluster_info, true);
            }
            result->previous_clusters[i] = std::async(std::launch::deferred, [context, previous_cluster_info] {
                return previous_cluster_info ? load_cluster_internal(context, previous_cluster_info) : nullptr;
            });
            result->next_clusters[i] = std::async(std::launch::deferred, [context, next_cluster_info] {
                return next_cluster_info ? load_cluster_internal(context, next_cluster_info) : nullptr;
            });
            result->previous_clusters[i].wait();
            result->next_clusters[i].wait();
        }
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Failed to load read-ahead clusters: %s", e.what());
        return nullptr;
    }
#endif

    return result;
}

// Load the next or previous cluster off the disk using the existing preloaded neighbors.
// The next neighbor in sequence will start being preloaded asynchronously.
std::shared_ptr<loaded_cluster_t> load_next_cluster(k4a_playback_context_t *context,
                                                    loaded_cluster_t *current_cluster,
                                                    bool next)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);
    RETURN_VALUE_IF_ARG(nullptr, context->cluster_cache == nullptr);
    RETURN_VALUE_IF_ARG(nullptr, current_cluster == NULL);

    cluster_info_t *cluster_info = next_cluster(context, current_cluster->cluster_info, next);
    if (cluster_info == NULL)
    {
        // End of file reached.
        return nullptr;
    }

    std::shared_ptr<loaded_cluster_t> result = std::shared_ptr<loaded_cluster_t>(new loaded_cluster_t());
    result->cluster_info = cluster_info;

#if CLUSTER_READ_AHEAD_COUNT
    try
    {
        // Use the current cluster as one of the neightbors, and then wait for the target cluster to be available.
        std::shared_ptr<KaxCluster> old_cluster = current_cluster->cluster;
        if (next)
        {
            result->previous_clusters[0] = std::async(std::launch::deferred, [old_cluster] { return old_cluster; });
            for (size_t i = 1; i < CLUSTER_READ_AHEAD_COUNT; i++)
            {
                result->previous_clusters[i] = current_cluster->previous_clusters[i - 1];
            }

            current_cluster->next_clusters[0].wait();
            result->cluster = current_cluster->next_clusters[0].get();
        }
        else
        {
            result->next_clusters[0] = std::async(std::launch::deferred, [old_cluster] { return old_cluster; });
            for (size_t i = 1; i < CLUSTER_READ_AHEAD_COUNT; i++)
            {
                result->next_clusters[i] = current_cluster->next_clusters[i - 1];
            }

            current_cluster->previous_clusters[0].wait();
            result->cluster = current_cluster->previous_clusters[0].get();
        }

        // Spawn a new async task to preload the next cluster in sequence.
        if (next)
        {
            for (size_t i = 0; i < CLUSTER_READ_AHEAD_COUNT - 1; i++)
            {
                result->next_clusters[i] = current_cluster->next_clusters[i + 1];
            }
            result->next_clusters[CLUSTER_READ_AHEAD_COUNT - 1] = std::async([context, cluster_info] {
                cluster_info_t *new_cluster = cluster_info;
                for (size_t i = 0; i < CLUSTER_READ_AHEAD_COUNT && new_cluster != NULL; i++)
                {
                    new_cluster = next_cluster(context, new_cluster, true);
                }
                return new_cluster ? load_cluster_internal(context, new_cluster) : nullptr;
            });
        }
        else
        {
            for (size_t i = 0; i < CLUSTER_READ_AHEAD_COUNT - 1; i++)
            {
                result->previous_clusters[i] = current_cluster->previous_clusters[i + 1];
            }
            result->previous_clusters[CLUSTER_READ_AHEAD_COUNT - 1] = std::async([context, cluster_info] {
                cluster_info_t *new_cluster = cluster_info;
                for (size_t i = 0; i < CLUSTER_READ_AHEAD_COUNT && new_cluster != NULL; i++)
                {
                    new_cluster = next_cluster(context, new_cluster, false);
                }
                return new_cluster ? load_cluster_internal(context, new_cluster) : nullptr;
            });
        }
    }
    catch (std::system_error &e)
    {
        LOG_ERROR("Failed to load next cluster: %s", e.what());
        return nullptr;
    }
#else
    result->cluster = load_cluster_internal(context, cluster_info);
#endif

    return result;
}

// If the block contains more than 1 frame, estimate the timestamp for the current frame based on the block duration.
// See k4a_record_subtitle_settings_t::high_freq_data in types.h for more info on timestamp estimation behavior.
uint64_t estimate_block_timestamp_ns(std::shared_ptr<block_info_t> &block)
{
    uint64_t timestamp_ns = block->sync_timestamp_ns;
    size_t sample_count = block->block->NumberFrames();
    if (block->sub_index > 0 && sample_count > 0)
    {
        timestamp_ns += (uint64_t)block->sub_index * (block->block_duration_ns - 1) / (sample_count - 1);
    }
    return timestamp_ns;
}

// Find the first block with a timestamp >= the specified timestamp. If a block group containing the specified timestamp
// is found, it will be returned. If no blocks are found, a pointer to EOF will be returned, or nullptr if an error
// occurs.
std::shared_ptr<block_info_t> find_block(k4a_playback_context_t *context, track_reader_t *reader, uint64_t timestamp_ns)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);
    RETURN_VALUE_IF_ARG(nullptr, reader == NULL);
    RETURN_VALUE_IF_ARG(nullptr, reader->track == NULL);

    // Create a new block pointing to the start of the cluster containing timestamp_ns.
    std::shared_ptr<block_info_t> block = std::make_shared<block_info_t>();
    block->reader = reader;
    block->index = -1;
    block->sub_index = 0;
    cluster_info_t *cluster_info = find_cluster(context, timestamp_ns);
    if (cluster_info == NULL)
    {
        LOG_ERROR("Failed to find data cluster for timestamp: %llu", timestamp_ns);
        return nullptr;
    }

    block->cluster = load_cluster(context, cluster_info);
    if (block->cluster == nullptr || block->cluster->cluster == nullptr)
    {
        LOG_ERROR("Failed to load initial data cluster from disk.", 0);
        return nullptr;
    }

    // Start searching through the blocks for the timestamp we want.
    while (block)
    {
        block = next_block(context, block.get(), true);
        if (block)
        {
            // Return this block if EOF was reached, or the timestamp is >= the search timestamp.
            if (block->block == NULL || estimate_block_timestamp_ns(block) >= timestamp_ns)
            {
                return block;
            }
        }
    }

    LOG_ERROR("Failed to find block for timestamp: %llu ns.", timestamp_ns);
    return nullptr;
}

// Find the next / previous block given a current block. If there is no next block, a block pointing to EOF will be
// returned, or nullptr if an error occurs.
std::shared_ptr<block_info_t> next_block(k4a_playback_context_t *context, block_info_t *current, bool next)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);
    RETURN_VALUE_IF_ARG(nullptr, current == NULL);
    RETURN_VALUE_IF_ARG(nullptr, current->reader == NULL);
    RETURN_VALUE_IF_ARG(nullptr, current->cluster == nullptr);
    RETURN_VALUE_IF_ARG(nullptr, current->cluster->cluster == nullptr);
    RETURN_VALUE_IF_ARG(nullptr, current->cluster->cluster_info == nullptr);

    // Get the track number of the current block.
    uint64_t track_number = current->reader->track->TrackNumber().GetValue();
    assert(track_number <= UINT16_MAX);
    uint16_t search_number = static_cast<uint16_t>(track_number);

    // Copy the current block and start the search at the next index / sub-index.
    std::shared_ptr<block_info_t> next_block = std::shared_ptr<block_info_t>(new block_info_t(*current));
    if (next_block->block != NULL)
    {
        next_block->sub_index += next ? 1 : -1;
        if (next_block->sub_index >= 0 && next_block->sub_index < (int)next_block->block->NumberFrames())
        {
            return next_block;
        }
    }
    next_block->index += next ? 1 : -1;

    std::shared_ptr<loaded_cluster_t> search_cluster = next_block->cluster;
    while (search_cluster != nullptr && search_cluster->cluster != nullptr)
    {
        // Search through the current cluster for the next valid block.
        std::vector<EbmlElement *> elements = next_block->cluster->cluster->GetElementList();
        KaxSimpleBlock *simple_block = NULL;
        KaxBlockGroup *block_group = NULL;
        while (next_block->index < (int)elements.size() && next_block->index >= 0)
        {
            // We need to support both SimpleBlocks and BlockGroups, check to see if the current element is either of
            // these types.
            next_block->block = NULL;
            if (check_element_type(elements[(size_t)next_block->index], &simple_block))
            {
                if (simple_block->TrackNum() == search_number)
                {
                    simple_block->SetParent(*next_block->cluster->cluster);
                    next_block->block = simple_block;
                    next_block->block_duration_ns = 0;
                }
            }
            else if (check_element_type(elements[(size_t)next_block->index], &block_group))
            {
                if (block_group->TrackNumber() == search_number)
                {
                    block_group->SetParent(*next_block->cluster->cluster);
                    block_group->SetParentTrack(*current->reader->track);
                    next_block->block = &GetChild<KaxBlock>(*block_group);
                    if (!block_group->GetBlockDuration(next_block->block_duration_ns))
                    {
                        next_block->block_duration_ns = 0;
                    }
                }
            }
            if (next_block->block != NULL)
            {
                // We found a valid block for this track, update the timestamp and return it.
                next_block->timestamp_ns = next_block->block->GlobalTimecode();
                next_block->sync_timestamp_ns = next_block->timestamp_ns + current->reader->sync_delay_ns;
                next_block->sub_index = next ? 0 : ((int)next_block->block->NumberFrames() - 1);

                return next_block;
            }
            next_block->index += next ? 1 : -1;
        }

        // The next block wasn't found in this cluster, go to the next cluster.
        search_cluster = load_next_cluster(context, next_block->cluster.get(), next);
        if (search_cluster != nullptr && search_cluster->cluster != nullptr)
        {
            next_block->cluster = search_cluster;
            next_block->index = next ? 0 : ((int)search_cluster->cluster->ListSize() - 1);
        }
    }

    // There are no more clusters, end of file was reached.
    // The cluster and index are kept so that reading in the opposite direction returns a valid block.
    next_block->timestamp_ns = 0;
    next_block->sync_timestamp_ns = 0;
    next_block->block = NULL;
    return next_block;
}

static void free_vector_buffer(void *buffer, void *context)
{
    (void)buffer;
    assert(context != nullptr);
    std::vector<uint8_t> *vector = static_cast<std::vector<uint8_t> *>(context);
    delete vector;
}

// Allocates a new image in the specified format from in_block
k4a_result_t convert_block_to_image(k4a_playback_context_t *context,
                                    block_info_t *in_block,
                                    k4a_image_t *image_out,
                                    k4a_image_format_t target_format)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, in_block == nullptr);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, image_out == nullptr);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, in_block->reader == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, in_block->block == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, in_block->block->NumberFrames() != 1);

    DataBuffer &data_buffer = in_block->block->GetBuffer(0);

    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    std::vector<uint8_t> *buffer = NULL;
    assert(in_block->reader->width <= INT_MAX);
    assert(in_block->reader->height <= INT_MAX);
    assert(in_block->reader->stride <= INT_MAX);
    int out_width = (int)in_block->reader->width;
    int out_height = (int)in_block->reader->height;
    int out_stride = (int)in_block->reader->stride;
    assert(out_height >= 0 && out_width >= 0);

    switch (target_format)
    {
    case K4A_IMAGE_FORMAT_DEPTH16:
    case K4A_IMAGE_FORMAT_IR16:
        buffer = new std::vector<uint8_t>(data_buffer.Buffer(), data_buffer.Buffer() + data_buffer.Size());
        if (in_block->reader->format == K4A_IMAGE_FORMAT_DEPTH16 || in_block->reader->format == K4A_IMAGE_FORMAT_IR16)
        {
            // 16 bit grayscale needs to be converted from big-endian back to little-endian.
            assert(buffer->size() % sizeof(uint16_t) == 0);
            uint16_t *buffer_raw = reinterpret_cast<uint16_t *>(buffer->data());
            size_t buffer_size = buffer->size() / sizeof(uint16_t);
            for (size_t i = 0; i < buffer_size; i++)
            {
                buffer_raw[i] = swap_bytes_16(buffer_raw[i]);
            }
        }
        else if (in_block->reader->format == K4A_IMAGE_FORMAT_COLOR_YUY2)
        {
            // For backward compatibility with early recordings, the YUY2 format was used. The actual data buffer is
            // 16-bit little-endian, so we can just use the buffer as-is.
        }
        else
        {
            LOG_ERROR("Unsupported image format conversion: %d to %d", in_block->reader->format, target_format);
            result = K4A_RESULT_FAILED;
        }
        break;
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
    case K4A_IMAGE_FORMAT_COLOR_NV12:
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        if (in_block->reader->format == target_format)
        {
            // No format conversion is required, just copy the buffer.
            buffer = new std::vector<uint8_t>(data_buffer.Buffer(), data_buffer.Buffer() + data_buffer.Size());
        }
        else
        {
            // Convert the buffer to BGRA format first
            out_stride = out_width * 4 * (int)sizeof(uint8_t);
            buffer = new std::vector<uint8_t>((size_t)(out_height * out_stride));

            if (in_block->reader->format == K4A_IMAGE_FORMAT_COLOR_MJPG)
            {
                tjhandle turbojpeg_handle = tjInitDecompress();
                if (tjDecompress2(turbojpeg_handle,
                                  data_buffer.Buffer(),
                                  data_buffer.Size(),
                                  buffer->data(),
                                  out_width,
                                  0, // pitch
                                  out_height,
                                  TJPF_BGRA,
                                  TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0)
                {
                    LOG_ERROR("Failed to decompress jpeg image to BGRA format.", 0);
                    result = K4A_RESULT_FAILED;
                }
                (void)tjDestroy(turbojpeg_handle);
            }
            else if (in_block->reader->format == K4A_IMAGE_FORMAT_COLOR_NV12)
            {
                // The endianness of libyuv's ARGB is opposite our BGRA format. They are the same byte order.
                if (libyuv::NV12ToARGB(data_buffer.Buffer(),
                                       (int)in_block->reader->stride,
                                       data_buffer.Buffer() + (out_height * (int)in_block->reader->stride),
                                       (int)in_block->reader->stride,
                                       buffer->data(),
                                       out_stride,
                                       out_width,
                                       out_height) != 0)
                {
                    LOG_ERROR("Failed to convert NV12 image to BGRA format.", 0);
                    result = K4A_RESULT_FAILED;
                }
            }
            else if (in_block->reader->format == K4A_IMAGE_FORMAT_COLOR_YUY2)
            {
                // The endianness of libyuv's ARGB is opposite our BGRA format. They are the same byte order.
                if (libyuv::YUY2ToARGB(data_buffer.Buffer(),
                                       (int)in_block->reader->stride,
                                       buffer->data(),
                                       out_stride,
                                       out_width,
                                       out_height) != 0)
                {
                    LOG_ERROR("Failed to convert YUY2 image to BGRA format.", 0);
                    result = K4A_RESULT_FAILED;
                }
            }
            else
            {
                LOG_ERROR("Unsupported image format conversion: %d to %d", in_block->reader->format, target_format);
                result = K4A_RESULT_FAILED;
            }

            if (K4A_SUCCEEDED(result) && target_format != K4A_IMAGE_FORMAT_COLOR_BGRA32)
            {
                auto bgra_buffer = buffer;
                buffer = NULL;
                int bgra_stride = out_stride;

                if (target_format == K4A_IMAGE_FORMAT_COLOR_NV12)
                {
                    out_stride = out_width;
                    size_t y_plane_size = (size_t)(out_height * out_stride);
                    // Round up the size of the UV plane in case the resolution is odd.
                    size_t uv_plane_size = (size_t)(out_height * out_stride + 1) / 2;
                    buffer = new std::vector<uint8_t>(y_plane_size + uv_plane_size);

                    if (libyuv::ARGBToNV12(bgra_buffer->data(),
                                           bgra_stride,
                                           buffer->data(),
                                           out_stride,
                                           buffer->data() + y_plane_size,
                                           out_stride,
                                           out_width,
                                           out_height) != 0)
                    {
                        LOG_ERROR("Failed to convert BGRA image to NV12 format.", 0);
                        result = K4A_RESULT_FAILED;
                    }
                }
                else if (target_format == K4A_IMAGE_FORMAT_COLOR_YUY2)
                {
                    out_stride = out_width * 2;
                    buffer = new std::vector<uint8_t>((size_t)(out_height * out_stride));

                    if (libyuv::ARGBToYUY2(
                            bgra_buffer->data(), bgra_stride, buffer->data(), out_stride, out_width, out_height) != 0)
                    {
                        LOG_ERROR("Failed to convert BGRA image to YUY2 format.", 0);
                        result = K4A_RESULT_FAILED;
                    }
                }
                else
                {
                    LOG_ERROR("Unsupported image format conversion: %d to %d", in_block->reader->format, target_format);
                    result = K4A_RESULT_FAILED;
                }

                if (bgra_buffer != NULL)
                {
                    delete bgra_buffer;
                }
            }
        }
        break;
    default:
        LOG_ERROR("Unknown target image format: %d", target_format);
        result = K4A_RESULT_FAILED;
    }

    if (K4A_SUCCEEDED(result) && buffer != NULL)
    {
        result = TRACE_CALL(k4a_image_create_from_buffer(target_format,
                                                         out_width,
                                                         out_height,
                                                         out_stride,
                                                         buffer->data(),
                                                         buffer->size(),
                                                         &free_vector_buffer,
                                                         buffer,
                                                         image_out));
        uint64_t device_timestamp_usec = in_block->timestamp_ns / 1000 +
                                         (uint64_t)context->record_config.start_timestamp_offset_usec;
        k4a_image_set_device_timestamp_usec(*image_out, device_timestamp_usec);
    }

    if (K4A_FAILED(result) && buffer != NULL)
    {
        delete buffer;
    }

    return result;
}

k4a_result_t new_capture(k4a_playback_context_t *context, block_info_t *block, k4a_capture_t *capture_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, capture_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, block == nullptr);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, block->reader == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, block->block == NULL);

    if (*capture_handle == 0)
    {
        RETURN_IF_ERROR(k4a_capture_create(capture_handle));
    }

    k4a_image_t image_handle = NULL;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    if (block->reader == context->color_track)
    {
        result = TRACE_CALL(convert_block_to_image(context, block, &image_handle, context->color_format_conversion));
        k4a_capture_set_color_image(*capture_handle, image_handle);
    }
    else if (block->reader == context->depth_track)
    {
        result = TRACE_CALL(convert_block_to_image(context, block, &image_handle, K4A_IMAGE_FORMAT_DEPTH16));
        k4a_capture_set_depth_image(*capture_handle, image_handle);
    }
    else if (block->reader == context->ir_track)
    {
        result = TRACE_CALL(convert_block_to_image(context, block, &image_handle, K4A_IMAGE_FORMAT_IR16));
        k4a_capture_set_ir_image(*capture_handle, image_handle);
    }
    else
    {
        LOG_ERROR("Capture being created from unknown track!", 0);
        result = K4A_RESULT_FAILED;
    }

    if (image_handle != NULL)
    {
        k4a_image_release(image_handle);
    }
    return result;
}

k4a_stream_result_t get_capture(k4a_playback_context_t *context, k4a_capture_t *capture_handle, bool next)
{
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, capture_handle == NULL);

    track_reader_t *blocks[] = { context->color_track, context->depth_track, context->ir_track };
    std::shared_ptr<block_info_t> next_blocks[arraysize(blocks)];

    uint64_t timestamp_start_ns = UINT64_MAX;
    uint64_t timestamp_end_ns = 0;

    // Find the next block for each track
    int enabled_tracks = 0;
    for (size_t i = 0; i < arraysize(blocks); i++)
    {
        if (blocks[i] != NULL)
        {
            enabled_tracks++;

            // If the current block is NULL, find the next block before/after the seek timestamp.
            if (blocks[i]->current_block == nullptr)
            {
                next_blocks[i] = find_block(context, blocks[i], context->seek_timestamp_ns);
                if (!next && next_blocks[i])
                {
                    next_blocks[i] = next_block(context, next_blocks[i].get(), false);
                }
            }
            else
            {
                next_blocks[i] = next_block(context, blocks[i]->current_block.get(), next);
            }
            if (next_blocks[i] && next_blocks[i]->block)
            {
                // Find the lowest and highest timestamp out of the next blocks
                if (next_blocks[i]->sync_timestamp_ns < timestamp_start_ns)
                {
                    timestamp_start_ns = next_blocks[i]->sync_timestamp_ns;
                }
                if (next_blocks[i]->sync_timestamp_ns > timestamp_end_ns)
                {
                    timestamp_end_ns = next_blocks[i]->sync_timestamp_ns;
                }
            }
            else
            {
                LOG_TRACE("%s of recording reached", next ? "End" : "Beginning");
                blocks[i]->current_block = next_blocks[i];
            }
        }
    }

    // Count how many of the blocks are within the sync window
    int valid_blocks = 0;
    if (enabled_tracks > 0)
    {
        if (next)
        {
            timestamp_end_ns = timestamp_start_ns;
        }
        else
        {
            timestamp_start_ns = timestamp_end_ns;
        }

        for (size_t i = 0; i < arraysize(blocks); i++)
        {
            if (next_blocks[i] && next_blocks[i]->block)
            {
                // If we're seeking forward, check the start timestamp, otherwise check the end timestamp
                if (next && (next_blocks[i]->sync_timestamp_ns - timestamp_start_ns < context->sync_period_ns / 2))
                {
                    valid_blocks++;
                    if (next_blocks[i]->sync_timestamp_ns > timestamp_end_ns)
                    {
                        timestamp_end_ns = next_blocks[i]->sync_timestamp_ns;
                    }
                }
                else if (!next && (timestamp_end_ns - next_blocks[i]->sync_timestamp_ns < context->sync_period_ns / 2))
                {
                    valid_blocks++;
                    if (next_blocks[i]->sync_timestamp_ns < timestamp_start_ns)
                    {
                        timestamp_start_ns = next_blocks[i]->sync_timestamp_ns;
                    }
                }
                else
                {
                    // Don't use this block as part of the capture
                    next_blocks[i] = nullptr;
                }
            }
        }

        if (valid_blocks < enabled_tracks)
        {
            // Try filling in any blocks that were missed due to a seek
            bool filled = false;
            for (size_t i = 0; i < arraysize(blocks); i++)
            {
                if (blocks[i] != NULL && next_blocks[i] == nullptr && blocks[i]->current_block == nullptr)
                {
                    std::shared_ptr<block_info_t> test_block = find_block(context,
                                                                          blocks[i],
                                                                          context->seek_timestamp_ns);
                    if (next)
                    {
                        test_block = next_block(context, test_block.get(), false);
                    }
                    if (test_block && test_block->block)
                    {
                        if (next && (timestamp_end_ns - test_block->sync_timestamp_ns < context->sync_period_ns / 2))
                        {
                            valid_blocks++;
                            next_blocks[i] = test_block;
                            filled = true;
                        }
                        else if (!next &&
                                 (test_block->sync_timestamp_ns - timestamp_start_ns < context->sync_period_ns / 2))
                        {
                            valid_blocks++;
                            next_blocks[i] = test_block;
                            filled = true;
                        }
                    }
                }
            }
            if (!next && filled)
            {
                // We seeked to the middle of a capture and then called previous capture, the current state is actually
                // for next_capture. Save the state and make a call to previous capture.
                for (size_t i = 0; i < arraysize(blocks); i++)
                {
                    if (next_blocks[i])
                    {
                        blocks[i]->current_block = next_blocks[i];
                    }
                }

                return get_capture(context, capture_handle, false);
            }
        }
    }

    LOG_TRACE("Valid blocks: %d/%d, Start: %llu ms, End: %llu ms",
              valid_blocks,
              enabled_tracks,
              timestamp_start_ns / 1_ms,
              timestamp_end_ns / 1_ms);

    *capture_handle = NULL;
    for (size_t i = 0; i < arraysize(blocks); i++)
    {
        if (next_blocks[i] && next_blocks[i]->block)
        {
            blocks[i]->current_block = next_blocks[i];
            k4a_result_t result = TRACE_CALL(new_capture(context, blocks[i]->current_block.get(), capture_handle));
            if (K4A_FAILED(result))
            {
                if (*capture_handle != NULL)
                {
                    k4a_capture_release(*capture_handle);
                    *capture_handle = NULL;
                }
                return K4A_STREAM_RESULT_FAILED;
            }
        }
    }
    return valid_blocks == 0 ? K4A_STREAM_RESULT_EOF : K4A_STREAM_RESULT_SUCCEEDED;
}

// Returns NULL if the buffer is invalid.
static matroska_imu_sample_t *parse_imu_sample_buffer(DataBuffer &data_buffer)
{
    uint32_t buffer_size = data_buffer.Size();
    binary *buffer = data_buffer.Buffer();
    if (buffer_size != sizeof(matroska_imu_sample_t))
    {
        LOG_ERROR("Unsupported IMU sample size: %u", buffer_size);
        return NULL;
    }
    else if (buffer == NULL)
    {
        LOG_ERROR("IMU sample buffer is NULL.", 0);
        return NULL;
    }
    else
    {
        return reinterpret_cast<matroska_imu_sample_t *>(buffer);
    }
}

k4a_stream_result_t get_imu_sample(k4a_playback_context_t *context, k4a_imu_sample_t *imu_sample, bool next)
{
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, imu_sample == NULL);

    if (context->imu_track == NULL)
    {
        LOG_WARNING("Recording has no IMU track.", 0);
        *imu_sample = { 0 };
        return K4A_STREAM_RESULT_EOF;
    }

    std::shared_ptr<block_info_t> block_info = context->imu_track->current_block;

    if (block_info == nullptr)
    {
        // There is no current IMU sample, find the next/previous sample based on seek_timestamp.
        block_info = find_block(context, context->imu_track, context->seek_timestamp_ns);
        if (block_info && !block_info->block)
        {
            // The seek timestamp is past the end of the file, get the last block instead.
            block_info = next_block(context, block_info.get(), false);
        }

        if (block_info && block_info->block)
        {
            // The returned block will not have an accurate sub_index due to timestamp estimation, select the correct
            // sub_index based on the real timestamp stored in the sample.
            size_t sample_count = block_info->block->NumberFrames();
            if (block_info->sync_timestamp_ns > context->seek_timestamp_ns)
            {
                // The timestamp we're looking for is before the found block.
                block_info->sub_index = next ? 0 : -1;
            }
            else if (block_info->sync_timestamp_ns + block_info->block_duration_ns <= context->seek_timestamp_ns)
            {
                // The timestamp we're looking for is after the found block.
                block_info->sub_index = (int)sample_count + (next ? 0 : -1);
            }
            else
            {
                // The timestamp we're looking for is within the found block.
                // IMU timestamps within the sample buffer are device timestamps, not relative to start of file.
                // The seek timestamp needs to be converted to a device timestamp when comparing.
                uint64_t seek_device_timestamp_ns = context->seek_timestamp_ns +
                                                    ((uint64_t)context->record_config.start_timestamp_offset_usec *
                                                     1000);
                block_info->sub_index = -1;
                for (size_t i = 0; i < sample_count; i++)
                {
                    matroska_imu_sample_t *sample = parse_imu_sample_buffer(
                        block_info->block->GetBuffer((unsigned int)i));
                    if (sample == NULL)
                    {
                        *imu_sample = { 0 };
                        return K4A_STREAM_RESULT_FAILED;
                    }
                    else if (sample->acc_timestamp_ns >= seek_device_timestamp_ns)
                    {
                        block_info->sub_index = next ? (int)i : (int)i - 1;
                        break;
                    }
                }
            }
            if (block_info->sub_index < 0 || block_info->sub_index >= (int)sample_count)
            {
                block_info = next_block(context, block_info.get(), next);
            }
        }
    }
    else
    {
        block_info = next_block(context, block_info.get(), next);
    }

    context->imu_track->current_block = block_info;

    if (block_info && block_info->block && block_info->sub_index >= 0 &&
        block_info->sub_index < (int)block_info->block->NumberFrames())
    {
        matroska_imu_sample_t *sample = parse_imu_sample_buffer(
            block_info->block->GetBuffer((unsigned int)block_info->sub_index));
        if (sample == NULL)
        {
            *imu_sample = { 0 };
            return K4A_STREAM_RESULT_FAILED;
        }
        else
        {
            imu_sample->acc_timestamp_usec = sample->acc_timestamp_ns / 1000;
            imu_sample->gyro_timestamp_usec = sample->gyro_timestamp_ns / 1000;
            imu_sample->temperature = std::numeric_limits<float>::quiet_NaN();
            for (size_t i = 0; i < 3; i++)
            {
                imu_sample->acc_sample.v[i] = sample->acc_data[i];
                imu_sample->gyro_sample.v[i] = sample->gyro_data[i];
            }
            return K4A_STREAM_RESULT_SUCCEEDED;
        }
    }

    LOG_TRACE("%s of recording reached", next ? "End" : "Beginning");
    *imu_sample = { 0 };
    return K4A_STREAM_RESULT_EOF;
}

k4a_stream_result_t get_data_block(k4a_playback_context_t *context,
                                   track_reader_t *track_reader,
                                   k4a_playback_data_block_t *data_block_handle,
                                   bool next)
{
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, track_reader == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, data_block_handle == NULL);

    std::shared_ptr<block_info_t> read_block = track_reader->current_block;
    if (read_block == nullptr)
    {
        // If the track current block is nullptr, it means we just performed a seek frame operation.
        // find_block() always finds the block with timestamp >= seek_timestamp.
        read_block = find_block(context, track_reader, context->seek_timestamp_ns);
        if (!next && read_block)
        {
            // In order to find the first timestamp < seek_timestamp, we need to query the previous block.
            read_block = next_block(context, read_block.get(), false);
        }
    }
    else
    {
        read_block = next_block(context, read_block.get(), next);
    }

    if (read_block == nullptr)
    {
        return K4A_STREAM_RESULT_FAILED;
    }

    track_reader->current_block = read_block;

    // Reach EOF
    if (read_block->block == nullptr)
    {
        return K4A_STREAM_RESULT_EOF;
    }

    k4a_playback_data_block_context_t *data_block_context = k4a_playback_data_block_t_create(data_block_handle);
    if (data_block_context == nullptr)
    {
        LOG_ERROR("Creating data block failed.", 0);
        return K4A_STREAM_RESULT_FAILED;
    }

    DataBuffer &data_buffer = track_reader->current_block->block->GetBuffer(
        (unsigned int)track_reader->current_block->sub_index);

    data_block_context->device_timestamp_usec = estimate_block_timestamp_ns(track_reader->current_block) / 1000 +
                                                context->record_config.start_timestamp_offset_usec;
    data_block_context->data_block.assign(data_buffer.Buffer(), data_buffer.Buffer() + data_buffer.Size());

    return K4A_STREAM_RESULT_SUCCEEDED;
}

} // namespace k4arecord
