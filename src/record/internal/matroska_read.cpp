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

using namespace LIBMATROSKA_NAMESPACE;

namespace k4arecord
{

// The upperLevel value shows the relationship of the element to the parent element
// -1 : global element
//  0 : child
//  1 : same level
//  + : further parent
std::unique_ptr<EbmlElement> next_element(k4a_playback_context_t *context, EbmlElement *parent, int *upper_level)
{
    try
    {
        int upper_level_value = 0;
        EbmlElement *element =
            context->stream->FindNextElement(parent->Generic().Context, upper_level_value, parent->GetSize(), false, 0);

        if (upper_level != nullptr)
        {
            *upper_level = upper_level_value;
        }
        return std::unique_ptr<EbmlElement>(element);
    }
    catch (std::ios_base::failure e)
    {
        logger_error(LOGGER_RECORD,
                     "Failed to get next child (parent id %x) in recording '%s': %s",
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
    catch (std::ios_base::failure e)
    {
        logger_error(LOGGER_RECORD,
                     "Failed seek past element (id %x) in recording '%s': %s",
                     EbmlId(*element).GetValue(),
                     context->file_path,
                     e.what());
        return K4A_RESULT_FAILED;
    }
}

void match_ebml_id(k4a_playback_context_t *context, EbmlId &id, uint64_t offset)
{
    RETURN_VALUE_IF_ARG(VOID_VALUE, context == NULL);

    logger_trace(LOGGER_RECORD, "Matching seek location: %x -> %llu", id.GetValue(), offset);

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
        logger_warn(LOGGER_RECORD, "Unknown element being matched: %x at %llu", id.GetValue(), offset);
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
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->stream == NULL);

    // Read and verify the EBML head information from the file
    auto ebmlHead = find_next<EbmlHead>(context);
    if (ebmlHead)
    {
        EDocType &eDocType = GetChild<EDocType>(*ebmlHead);
        EDocTypeVersion &eDocTypeVersion = GetChild<EDocTypeVersion>(*ebmlHead);
        EDocTypeReadVersion &eDocTypeReadVersion = GetChild<EDocTypeReadVersion>(*ebmlHead);
        if (eDocType.GetValue() != std::string("matroska"))
        {
            logger_error(LOGGER_RECORD, "DocType is not matroska: %s", eDocType.GetValue().c_str());
            return K4A_RESULT_FAILED;
        }

        if (eDocTypeReadVersion.GetValue() > 2)
        {
            logger_error(LOGGER_RECORD,
                         "DocTypeReadVersion (%d) > 2 is not supported.",
                         eDocTypeReadVersion.GetValue());
            return K4A_RESULT_FAILED;
        }

        if (eDocTypeVersion.GetValue() < eDocTypeReadVersion.GetValue())
        {
            logger_error(LOGGER_RECORD,
                         "DocTypeVersion (%d) is great than DocTypeReadVersion (%d)",
                         eDocTypeVersion.GetValue(),
                         eDocTypeReadVersion.GetValue());
            return K4A_RESULT_FAILED;
        }
    }

    // Find the offsets for each section of the file
    context->segment = find_next<KaxSegment>(context, true, false);
    if (context->segment)
    {
        auto element = next_element(context, context->segment.get());

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
                        EbmlId ebml_id(seek_id.GetBuffer(), (const unsigned int)seek_id.GetSize());
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

            element = next_element(context, context->segment.get());
        }
    }

    if (context->first_cluster_offset == 0)
    {
        logger_error(LOGGER_RECORD, "Recording file does not contain any frames!");
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

    // Find the last timestamp in the file
    context->last_timestamp_ns = 0;
    std::shared_ptr<KaxCluster> cluster = seek_timestamp(context, UINT64_MAX);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, cluster == nullptr);
    KaxSimpleBlock *simple_block = NULL;
    KaxBlockGroup *block_group = NULL;
    for (EbmlElement *e : cluster->GetElementList())
    {
        if (check_element_type(e, &simple_block))
        {
            simple_block->SetParent(*cluster);
            uint64_t block_timestamp_ns = simple_block->GlobalTimecode();
            if (block_timestamp_ns > context->last_timestamp_ns)
            {
                context->last_timestamp_ns = block_timestamp_ns;
            }
        }
        else if (check_element_type(e, &block_group))
        {
            block_group->SetParent(*cluster);
            uint64_t block_timestamp_ns = block_group->GlobalTimecode();
            if (block_timestamp_ns > context->last_timestamp_ns)
            {
                context->last_timestamp_ns = block_timestamp_ns;
            }
        }
    }
    logger_trace(LOGGER_RECORD, "Found last timestamp: %llu", context->last_timestamp_ns);

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t parse_recording_config(k4a_playback_context_t *context)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->segment_info == nullptr);

    context->timecode_scale = GetChild<KaxTimecodeScale>(*context->segment_info).GetValue();

    context->color_track.track = get_track_by_tag(context, "K4A_COLOR_MODE");
    // Temporary disable this check to unblock internal use.
    // TODO: further discuss this issue to find out a proper solution.
    // context->depth_track.track = get_track_by_tag(context, "K4A_DEPTH_MODE");
    context->ir_track.track = get_track_by_tag(context, "K4A_IR_MODE");
    context->imu_track.track = get_track_by_tag(context, "K4A_IMU_MODE");
    if (context->color_track.track == NULL)
    {
        context->color_track.track = get_track_by_name(context, "COLOR");
    }
    if (context->depth_track.track == NULL)
    {
        context->depth_track.track = get_track_by_name(context, "DEPTH");
    }
    if (context->ir_track.track == NULL)
    {
        context->ir_track.track = get_track_by_name(context, "IR");
    }
    if (context->imu_track.track == NULL)
    {
        context->imu_track.track = get_track_by_name(context, "IMU");
    }

    if (K4A_FAILED(parse_custom_tracks(context)))
    {
        // The rest of the recording can still be read if read custom track failed
        logger_warn(LOGGER_RECORD, "Read custom track data failed.");
    }

    // Read device calibration attachment
    context->calibration_attachment = get_attachment_by_tag(context, "K4A_CALIBRATION_FILE");
    if (context->calibration_attachment == NULL)
    {
        context->calibration_attachment = get_attachment_by_name(context, "calibration.json");
    }
    if (context->calibration_attachment == NULL)
    {
        // The rest of the recording can still be read if no device calibration blob exists.
        logger_warn(LOGGER_RECORD, "Device calibration is missing from recording.");
    }

    uint64_t frame_period_ns = 0;
    if (context->color_track.track)
    {
        context->track_number_name_map.insert(
            std::pair<uint64_t, std::string>(GetChild<KaxTrackNumber>(*context->color_track.track).GetValue(),
                                             color_track_name));
        context->color_track.type = static_cast<track_type>(
            GetChild<KaxTrackType>(*context->color_track.track).GetValue());

        if (context->color_track.type != track_video)
        {
            logger_error(LOGGER_RECORD, "Color track is not a video track.");
            return K4A_RESULT_FAILED;
        }

        KaxTrackVideo &video_track = GetChild<KaxTrackVideo>(*context->color_track.track);
        context->color_track.width = static_cast<uint32_t>(GetChild<KaxVideoPixelWidth>(video_track).GetValue());
        context->color_track.height = static_cast<uint32_t>(GetChild<KaxVideoPixelHeight>(video_track).GetValue());

        frame_period_ns = GetChild<KaxTrackDefaultDuration>(*context->color_track.track).GetValue();
        context->color_track.frame_period_ns = frame_period_ns;

        RETURN_IF_ERROR(read_bitmap_info_header(&context->color_track));
        context->record_config.color_resolution = K4A_COLOR_RESOLUTION_OFF;
        for (size_t i = 0; i < arraysize(color_resolutions); i++)
        {
            uint32_t width, height;
            if (k4a_convert_resolution_to_width_height(color_resolutions[i], &width, &height))
            {
                if (context->color_track.width == width && context->color_track.height == height)
                {
                    context->record_config.color_resolution = color_resolutions[i];
                    break;
                }
            }
        }

        if (context->record_config.color_resolution == K4A_COLOR_RESOLUTION_OFF)
        {
            logger_warn(LOGGER_RECORD,
                        "The color resolution is not officially supported: %dx%d. You cannot get the calibration "
                        "information for this color resolution",
                        context->color_track.width,
                        context->color_track.height);
        }

        context->record_config.color_track_enabled = true;
        context->record_config.color_format = context->color_track.format;
    }
    else
    {
        context->record_config.color_resolution = K4A_COLOR_RESOLUTION_OFF;
        // Set to a default color format if color track is disabled.
        context->record_config.color_format = K4A_IMAGE_FORMAT_CUSTOM;
    }

    KaxTag *depth_mode_tag = get_tag(context, "K4A_DEPTH_MODE");
    if (depth_mode_tag == NULL && (context->depth_track.track || context->ir_track.track))
    {
        logger_error(LOGGER_RECORD, "K4A_DEPTH_MODE tag is missing.");
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
                    if (k4a_convert_depth_mode_to_width_height(legacy_depth_modes[i].first, &depth_width, &depth_height))
                    {
                        context->record_config.depth_mode = legacy_depth_modes[i].first;
                        break;
                    }
                }
            }
        }
        if (context->record_config.depth_mode == K4A_DEPTH_MODE_OFF)
        {
            logger_error(LOGGER_RECORD, "Unsupported depth mode: %s", depth_mode_str.c_str());
            return K4A_RESULT_FAILED;
        }
    }

    if (context->depth_track.track)
    {
        context->track_number_name_map.insert(
            std::pair<uint64_t, std::string>(GetChild<KaxTrackNumber>(*context->depth_track.track).GetValue(),
                                             depth_track_name));
        context->depth_track.type = static_cast<track_type>(
            GetChild<KaxTrackType>(*context->depth_track.track).GetValue());

        KaxTrackVideo &video_track = GetChild<KaxTrackVideo>(*context->depth_track.track);
        context->depth_track.width = static_cast<uint32_t>(GetChild<KaxVideoPixelWidth>(video_track).GetValue());
        context->depth_track.height = static_cast<uint32_t>(GetChild<KaxVideoPixelHeight>(video_track).GetValue());

        uint64_t depth_period_ns = GetChild<KaxTrackDefaultDuration>(*context->depth_track.track).GetValue();
        context->depth_track.frame_period_ns = depth_period_ns;
        if (frame_period_ns == 0)
        {
            frame_period_ns = depth_period_ns;
        }
        else if (frame_period_ns != depth_period_ns)
        {
            logger_error(LOGGER_RECORD,
                         "Track frame durations don't match (Depth): %llu ns != %llu ns",
                         frame_period_ns,
                         depth_period_ns);
            return K4A_RESULT_FAILED;
        }

        if (context->depth_track.width != depth_width || context->depth_track.height != depth_height)
        {
            logger_error(LOGGER_RECORD,
                         "Unsupported depth resolution / mode: %dx%d (%s)",
                         context->depth_track.width,
                         context->depth_track.height,
                         depth_mode_str.c_str());
            return K4A_RESULT_FAILED;
        }

        RETURN_IF_ERROR(read_bitmap_info_header(&context->depth_track));

        context->record_config.depth_track_enabled = true;
    }

    if (context->ir_track.track)
    {
        context->track_number_name_map.insert(
            std::pair<uint64_t, std::string>(GetChild<KaxTrackNumber>(*context->ir_track.track).GetValue(),
                                             ir_track_name));
        context->ir_track.type = static_cast<track_type>(GetChild<KaxTrackType>(*context->ir_track.track).GetValue());
        KaxTrackVideo &video_track = GetChild<KaxTrackVideo>(*context->ir_track.track);
        context->ir_track.width = static_cast<uint32_t>(GetChild<KaxVideoPixelWidth>(video_track).GetValue());
        context->ir_track.height = static_cast<uint32_t>(GetChild<KaxVideoPixelHeight>(video_track).GetValue());

        uint64_t ir_period_ns = GetChild<KaxTrackDefaultDuration>(*context->ir_track.track).GetValue();
        context->ir_track.frame_period_ns = ir_period_ns;
        if (frame_period_ns == 0)
        {
            frame_period_ns = ir_period_ns;
        }
        else if (frame_period_ns != ir_period_ns)
        {
            logger_error(LOGGER_RECORD,
                         "Track frame durations don't match (IR): %llu ns != %llu ns",
                         frame_period_ns,
                         ir_period_ns);
            return K4A_RESULT_FAILED;
        }

        if (context->depth_track.track)
        {
            if (context->ir_track.width != context->depth_track.width ||
                context->ir_track.height != context->depth_track.height)
            {
                logger_error(LOGGER_RECORD,
                             "Depth and IR track have different resolutions: Depth %dx%d, IR %dx%d",
                             context->depth_track.width,
                             context->depth_track.height,
                             context->ir_track.width,
                             context->ir_track.height);
                return K4A_RESULT_FAILED;
            }
        }
        else if (context->ir_track.width != depth_width || context->ir_track.height != depth_height)
        {
            logger_error(LOGGER_RECORD,
                         "Unsupported IR resolution / depth mode: %dx%d (%s)",
                         context->ir_track.width,
                         context->ir_track.height,
                         depth_mode_str.c_str());
            return K4A_RESULT_FAILED;
        }

        RETURN_IF_ERROR(read_bitmap_info_header(&context->ir_track));

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
            logger_error(LOGGER_RECORD,
                         "Unsupported recording frame period: %llu ns (%llu fps)",
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

    // Read depth_delay_off_color_usec and set offsets for each default track accordingly.
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
            if (depth_delay_ns > 0)
            {
                context->color_track.sync_delay_ns = (uint64_t)depth_delay_ns;
            }
            else if (depth_delay_ns < 0)
            {
                context->depth_track.sync_delay_ns = (uint64_t)(-depth_delay_ns);
                context->ir_track.sync_delay_ns = (uint64_t)(-depth_delay_ns);
            }
        }
        else
        {
            logger_error(LOGGER_RECORD,
                         "Tag K4A_DEPTH_DELAY_NS contains invalid value: %s",
                         get_tag_string(depth_delay_tag).c_str());
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        context->record_config.depth_delay_off_color_usec = 0;
    }

    if (context->imu_track.track)
    {
        context->record_config.imu_track_enabled = true;
        context->imu_track.type = static_cast<track_type>(GetChild<KaxTrackType>(*context->imu_track.track).GetValue());

        context->track_number_name_map.insert(
            std::pair<uint64_t, std::string>(GetChild<KaxTrackNumber>(*context->imu_track.track).GetValue(),
                                             imu_track_name));
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
            logger_error(LOGGER_RECORD, "Unsupported wired sync mode: %s", sync_mode_str.c_str());
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
                    logger_error(LOGGER_RECORD,
                                 "Tag K4A_SUBORDINATE_DELAY_NS contains invalid value: %s",
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
            logger_error(LOGGER_RECORD,
                         "Tag K4A_START_OFFSET_NS contains invalid value: %s",
                         get_tag_string(start_offset_tag).c_str());
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        context->record_config.start_timestamp_offset_usec = 0;
    }

    if (K4A_FAILED(parse_all_timestamps(context)))
    {
        // The rest of the recording can still be read if parse track timestamps failed
        logger_warn(LOGGER_RECORD, "Pre-parsing all track timestamp information failed.");
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t read_bitmap_info_header(track_reader_t *track)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, track == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, track->track == NULL);

    track->codec_id = GetChild<KaxCodecID>(*track->track).GetValue();
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
            logger_error(LOGGER_RECORD,
                         "Unsupported FOURCC format for track '%s': %x",
                         GetChild<KaxTrackName>(*track->track).GetValueUTF8().c_str(),
                         bitmap_header->biCompression);
            return K4A_RESULT_FAILED;
        }

        return K4A_RESULT_SUCCEEDED;
    }
    else
    {
        logger_error(LOGGER_RECORD,
                     "Unsupported codec id for track '%s': %s",
                     GetChild<KaxTrackName>(*track->track).GetValueUTF8().c_str(),
                     track->codec_id.c_str());
        return K4A_RESULT_FAILED;
    }
}

void reset_seek_pointers(k4a_playback_context_t *context, uint64_t seek_timestamp_ns)
{
    RETURN_VALUE_IF_ARG(VOID_VALUE, context == NULL);

    context->seek_timestamp_ns = seek_timestamp_ns;
    context->color_track.current_block.reset();
    context->depth_track.current_block.reset();
    context->ir_track.current_block.reset();
    context->imu_track.current_block.reset();
    context->imu_sample_index = -1;

    for (auto &itr : context->custom_track_map)
    {
        auto &track_reader = itr.second;
        track_reader.current_block.reset();
    }
}

KaxTrackEntry *get_track_by_name(k4a_playback_context_t *context, const char *name)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, context->tracks == NULL);
    RETURN_VALUE_IF_ARG(NULL, name == NULL);

    std::string search(name);
    KaxTrackEntry *track = NULL;
    for (EbmlElement *e : context->tracks->GetElementList())
    {
        if (check_element_type(e, &track))
        {
            if (GetChild<KaxTrackName>(*track).GetValueUTF8() == search)
            {
                return track;
            }
        }
    }

    return NULL;
}

bool check_track_name_default_track(std::string track_name)
{
    return track_name == depth_track_name || track_name == ir_track_name || track_name == color_track_name ||
           track_name == imu_track_name;
}

track_reader_t *get_track_reader_by_name(k4a_playback_context_t *context, std::string track_name)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);
    if (track_name == depth_track_name)
    {
        return context->record_config.depth_track_enabled ? &context->depth_track : nullptr;
    }
    else if (track_name == ir_track_name)
    {
        return context->record_config.ir_track_enabled ? &context->ir_track : nullptr;
    }
    else if (track_name == color_track_name)
    {
        return context->record_config.color_track_enabled ? &context->color_track : nullptr;
    }
    else if (track_name == imu_track_name)
    {
        return context->record_config.imu_track_enabled ? &context->imu_track : nullptr;
    }

    auto itr = context->custom_track_map.find(track_name);
    if (itr != context->custom_track_map.end())
    {
        return &itr->second;
    }

    return nullptr;
}

k4a_result_t parse_custom_tracks(k4a_playback_context_t *context)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->tracks == NULL);

    KaxTrackEntry *track = NULL;
    for (EbmlElement *e : context->tracks->GetElementList())
    {
        if (check_element_type(e, &track))
        {
            std::string track_name = GetChild<KaxTrackName>(*track).GetValueUTF8();
            if (!check_track_name_default_track(track_name))
            {
                track_reader_t track_reader;
                track_reader.track = track;

                // Read custom tracks information
                track_reader.codec_id = GetChild<KaxCodecID>(*track).GetValue();
                KaxCodecPrivate &codec_private = GetChild<KaxCodecPrivate>(*track);
                track_reader.codec_private.assign(codec_private.GetBuffer(),
                                                  codec_private.GetBuffer() + codec_private.GetSize());

                track_reader.frame_period_ns = GetChild<KaxTrackDefaultDuration>(*track).GetValue();

                track_reader.type = static_cast<track_type>(GetChild<KaxTrackType>(*track).GetValue());
                if (track_reader.type == track_video)
                {
                    KaxTrackVideo &video_track = GetChild<KaxTrackVideo>(*track);
                    track_reader.width = static_cast<uint32_t>(GetChild<KaxVideoPixelWidth>(video_track).GetValue());
                    track_reader.height = static_cast<uint32_t>(GetChild<KaxVideoPixelHeight>(video_track).GetValue());
                }

                context->track_number_name_map.insert(
                    std::pair<uint64_t, std::string>(GetChild<KaxTrackNumber>(*track).GetValue(), track_name));

                context->custom_track_map.insert(std::pair<std::string, track_reader_t>(track_name, track_reader));
            }
        }
    }

    return K4A_RESULT_SUCCEEDED;
}

KaxTrackEntry *get_track_by_tag(k4a_playback_context_t *context, const char *tag_name)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, context->tracks == NULL);
    RETURN_VALUE_IF_ARG(NULL, tag_name == NULL);

    KaxTag *track_tag = get_tag(context, tag_name);
    if (track_tag)
    {
        KaxTagTargets &tagTargets = GetChild<KaxTagTargets>(*track_tag);
        if (GetChild<KaxTagTargetType>(tagTargets).GetValue() != "TRACK")
        {
            return NULL;
        }
        uint64_t search_uid = GetChild<KaxTagTrackUID>(tagTargets).GetValue();

        KaxTrackEntry *track = NULL;
        for (EbmlElement *e : context->tracks->GetElementList())
        {
            if (check_element_type(e, &track))
            {
                if (GetChild<KaxTrackUID>(*track).GetValue() == search_uid)
                {
                    return track;
                }
            }
        }
    }

    return NULL;
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

k4a_result_t parse_all_timestamps(k4a_playback_context_t *context)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL || context->segment == NULL);

    if (K4A_FAILED(seek_offset(context, context->first_cluster_offset)))
    {
        return K4A_RESULT_FAILED;
    }
    std::shared_ptr<KaxCluster> cluster = find_next<KaxCluster>(context, false, false);

    while (cluster != nullptr)
    {
        std::shared_ptr<EbmlElement> element = next_element(context, cluster.get());
        bool cluster_start_found = false;
        int upper_level = 0;

        while (element != nullptr)
        {
            uint64_t block_timestamp_ns = 0;
            uint16_t track_number = 0;
            bool is_data_block = false;

            EbmlId element_id(*element);
            if (element_id == KaxClusterTimecode::ClassInfos.GlobalId)
            {
                KaxClusterTimecode *cluster_timecode = read_element<KaxClusterTimecode>(context, element.get());
                cluster->InitTimecode(cluster_timecode->GetValue(), (int64_t)context->timecode_scale);
                cluster_start_found = true;
            }
            else if (element_id == KaxSimpleBlock::ClassInfos.GlobalId)
            {
                assert(cluster_start_found);
                KaxSimpleBlock *simple_block = read_element<KaxSimpleBlock>(context, element.get(), SCOPE_PARTIAL_DATA);

                simple_block->SetParent(*cluster);
                block_timestamp_ns = simple_block->GlobalTimecode();
                track_number = simple_block->TrackNum();
                is_data_block = true;
            }
            else if (element_id == KaxBlockGroup::ClassInfos.GlobalId)
            {
                assert(cluster_start_found);
                KaxBlockGroup *block_group = read_element<KaxBlockGroup>(context, element.get(), SCOPE_PARTIAL_DATA);

                block_group->SetParent(*cluster);
                block_timestamp_ns = block_group->GlobalTimecode();
                track_number = block_group->TrackNumber();
                is_data_block = true;
            }

            if (is_data_block)
            {
                auto itr = context->track_number_name_map.find(track_number);
                if (itr != context->track_number_name_map.end())
                {
                    track_reader_t *track_reader = get_track_reader_by_name(context, itr->second);
                    track_reader->block_index_timestamp_usec_map.push_back(block_timestamp_ns / 1000);
                }
            }

            skip_element(context, element.get());
            element = next_element(context, cluster.get(), &upper_level);

            // The next element is no longer the child, but is same level as cluster
            if (upper_level > 0)
            {
                break;
            }
        }

        // There is no more element left
        if (element == nullptr)
        {
            break;
        }
        else if (static_cast<EbmlId>(*element) == KaxCluster::ClassInfos.GlobalId)
        {
            // The next element happen to be another cluster. Use this as the next cluster pointer
            cluster = std::static_pointer_cast<KaxCluster, EbmlElement>(element);
        }
        else
        {
            // The next element is other types. Try to find the next cluster type
            cluster = find_next<KaxCluster>(context, true, false);
        }
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t seek_offset(k4a_playback_context_t *context, uint64_t offset)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->segment == NULL);

    uint64_t file_offset = context->segment->GetGlobalPosition(offset);
    try
    {
        logger_trace(LOGGER_RECORD, "Seeking to file position: %llu", file_offset);
        assert(file_offset <= INT64_MAX);
        context->ebml_file->setFilePointer((int64_t)file_offset);
        return K4A_RESULT_SUCCEEDED;
    }
    catch (std::ios_base::failure e)
    {
        logger_error(LOGGER_RECORD,
                     "Failed to seek file to %llu (relative %llu) '%s': %s",
                     file_offset,
                     offset,
                     context->file_path,
                     e.what());
        return K4A_RESULT_FAILED;
    }
}

std::shared_ptr<KaxCluster> seek_timestamp(k4a_playback_context_t *context, uint64_t timestamp_ns)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);

    uint64_t target_offset = context->first_cluster_offset;
    KaxCuePoint *cue = find_closest_cue(context, timestamp_ns);
    if (cue)
    {
        const KaxCueTrackPositions *positions = cue->GetSeekPosition();
        if (positions)
        {
            target_offset = positions->ClusterPosition();
        }
    }

    std::shared_ptr<KaxCluster> cluster = find_cluster(context, target_offset, timestamp_ns);
    if (!cluster)
    {
        logger_error(LOGGER_RECORD, "Failed to seek to timestamp %llu ns: Cluster not found", timestamp_ns);
        return nullptr;
    }

    return cluster;
}

// Returns the Cue entry with the closest timestamp less than the given timestamp.
// Returns NULL if time is before the first entry.
KaxCuePoint *find_closest_cue(k4a_playback_context_t *context, uint64_t timestamp_ns)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);

    // Only search for the cue if the parent element exists
    if (context->cues)
    {
        uint64_t search_time = timestamp_ns / context->timecode_scale;
        KaxCuePoint *last = NULL;
        KaxCuePoint *cue = NULL;
        for (EbmlElement *e : context->cues->GetElementList())
        {
            if (check_element_type(e, &cue))
            {
                if (GetChild<KaxCueTime>(*cue).GetValue() > search_time)
                {
                    return last;
                }

                last = cue;
            }
        }
        return last;
    }

    return NULL;
}

// Starts at the specified search offset in the file and searches forward to find the specified timestamp
std::shared_ptr<KaxCluster> find_cluster(k4a_playback_context_t *context, uint64_t search_offset, uint64_t timestamp_ns)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);
    RETURN_VALUE_IF_ARG(nullptr, search_offset == 0);

    uint64_t search_time = timestamp_ns / context->timecode_scale;
    if (K4A_FAILED(seek_offset(context, search_offset)))
    {
        return nullptr;
    }
    std::shared_ptr<KaxCluster> previous;
    uint64_t previous_start = 0;
    std::shared_ptr<KaxCluster> current = find_next<KaxCluster>(context, false, false);
    while (current != nullptr)
    {
        auto element = next_element(context, current.get());
        uint64_t current_start = previous_start;
        bool cluster_start_found = false;

        while (element != nullptr)
        {
            EbmlId element_id(*element);
            if (element_id == KaxClusterTimecode::ClassInfos.GlobalId)
            {
                KaxClusterTimecode *cluster_timecode = read_element<KaxClusterTimecode>(context, element.get());
                current_start = cluster_timecode->GetValue();
                cluster_start_found = true;
                break;
            }
            else
            {
                skip_element(context, element.get());
            }

            element = next_element(context, current.get());
        }

        if (cluster_start_found)
        {
            if (current_start > search_time)
            {
                logger_trace(LOGGER_RECORD,
                             "Found Cluster for timestamp %llu: %llu to %llu",
                             search_time,
                             previous_start,
                             current_start);
                break;
            }
        }
        else
        {
            logger_warn(LOGGER_RECORD, "Recording cluster is missing a start timestamp");
        }
        previous = std::move(current);
        previous_start = current_start;
        current = find_next<KaxCluster>(context, true, false);
    }

    if (previous != nullptr)
    {
        // If the timestamp is before the first cluster, return the first cluster.
        current = std::move(previous);
    }
    if (current != nullptr)
    {
        if (read_element<KaxCluster>(context, current.get()) == NULL)
        {
            logger_error(LOGGER_RECORD, "Failed to read cluster");
            return nullptr;
        }

        uint64_t timecode = GetChild<KaxClusterTimecode>(*current).GetValue();
        assert(context->timecode_scale <= INT64_MAX);
        current->InitTimecode(timecode, (int64_t)context->timecode_scale);
    }
    return current;
}

// Finds the next or previous cluster given a current cluster.
// Previous cluster uses the file Cues to search for a close timestamp, and then walks forward.
std::shared_ptr<libmatroska::KaxCluster> next_cluster(k4a_playback_context_t *context,
                                                      libmatroska::KaxCluster *current_cluster,
                                                      bool next)
{
    RETURN_VALUE_IF_ARG(nullptr, context == NULL);
    RETURN_VALUE_IF_ARG(nullptr, context->segment == NULL);
    RETURN_VALUE_IF_ARG(nullptr, current_cluster == NULL);

    uint64_t current_offset = context->segment->GetRelativePosition(*current_cluster);
    if (next)
    {
        if (K4A_FAILED(seek_offset(context, current_offset)))
        {
            return nullptr;
        }
        if (K4A_FAILED(skip_element(context, current_cluster)))
        {
            return nullptr;
        }

        std::shared_ptr<KaxCluster> next_cluster = find_next<KaxCluster>(context, true, true);
        if (next_cluster)
        {
            uint64_t timecode = GetChild<KaxClusterTimecode>(*next_cluster).GetValue();
            assert(context->timecode_scale <= INT64_MAX);
            next_cluster->InitTimecode(timecode, (int64_t)context->timecode_scale);
        }
        return next_cluster;
    }
    else
    {
        uint64_t timestamp_ns = current_cluster->GlobalTimecode();
        if (timestamp_ns == 0)
        {
            return nullptr;
        }
        std::shared_ptr<KaxCluster> previous_cluster = seek_timestamp(context, timestamp_ns - 1);
        if (previous_cluster && previous_cluster->GetElementPosition() == current_cluster->GetElementPosition())
        {
            return nullptr;
        }
        return previous_cluster;
    }
}

// Search operates in 2 modes:
// - If there is already a current_block, find the next/previous block
// - If there is no current_block, find the first block before or after the seek_timestamp
std::shared_ptr<read_block_t> find_next_block(k4a_playback_context_t *context, track_reader_t *reader, bool next)
{
    RETURN_VALUE_IF_ARG(NULL, context == NULL);
    RETURN_VALUE_IF_ARG(NULL, context->seek_cluster == NULL);
    RETURN_VALUE_IF_ARG(NULL, reader == NULL);
    RETURN_VALUE_IF_ARG(NULL, reader->track == NULL);

    bool timestamp_search = reader->current_block == nullptr;
    uint64_t track_number = reader->track->TrackNumber().GetValue();
    assert(track_number <= UINT16_MAX);
    uint16_t search_number = static_cast<uint16_t>(track_number);

    std::shared_ptr<read_block_t> next_block = std::make_shared<read_block_t>();
    next_block->reader = reader;
    if (timestamp_search)
    {
        // Search the whole cluster for the correct timestamp
        next_block->index = next ? 0 : ((int)context->seek_cluster->ListSize() - 1);
        next_block->cluster = context->seek_cluster;
    }
    else
    {
        // Increment/Decrement the block index and start searching from there.
        next_block->index = reader->current_block->index + (next ? 1 : -1);
        next_block->cluster = reader->current_block->cluster;
    }
    while (next_block->cluster != nullptr)
    {
        std::vector<EbmlElement *> elements = next_block->cluster->GetElementList();
        KaxSimpleBlock *simple_block = NULL;
        KaxBlockGroup *block_group = NULL;
        while (next_block->index < (int)elements.size() && next_block->index >= 0)
        {
            if (check_element_type(elements[(size_t)next_block->index], &simple_block))
            {
                simple_block->SetParent(*next_block->cluster);
                if (simple_block->TrackNum() == search_number)
                {
                    next_block->block = simple_block;
                }
            }
            else if (check_element_type(elements[(size_t)next_block->index], &block_group))
            {
                block_group->SetParent(*next_block->cluster);
                block_group->SetParentTrack(*reader->track);
                if (block_group->TrackNumber() == search_number)
                {
                    next_block->block = &GetChild<KaxBlock>(*block_group);
                }
            }
            if (next_block->block != NULL)
            {
                next_block->timestamp_ns = next_block->block->GlobalTimecode();
                next_block->sync_timestamp_ns = next_block->timestamp_ns + reader->sync_delay_ns;
                if (timestamp_search)
                {
                    if ((next && next_block->timestamp_ns >= context->seek_timestamp_ns) ||
                        (!next && next_block->timestamp_ns < context->seek_timestamp_ns))
                    {
                        return next_block;
                    }
                }
                else
                {
                    return next_block;
                }
            }
            next_block->index += next ? 1 : -1;
        }

        // Block wasn't found in this cluster, go to the next one
        std::shared_ptr<KaxCluster> found_cluster = next_cluster(context, next_block->cluster.get(), next);
        if (found_cluster != nullptr)
        {
            next_block->index = next ? 0 : ((int)found_cluster->ListSize() - 1);
            next_block->cluster = found_cluster;
        }
        else
        {
            break;
        }
    }

    // End of file reached
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

k4a_result_t new_capture(k4a_playback_context_t *context,
                         std::shared_ptr<read_block_t> &block,
                         k4a_capture_t *capture_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, capture_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, block == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, block->reader == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, block->block == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, block->block->NumberFrames() != 1);

    if (*capture_handle == 0)
    {
        RETURN_IF_ERROR(k4a_capture_create(capture_handle));
    }

    std::shared_ptr<read_block_t> *block_ptr = new std::shared_ptr<read_block_t>(block);
    DataBuffer &data_buffer = block->block->GetBuffer(0);
    std::vector<uint8_t> *buffer = new std::vector<uint8_t>(data_buffer.Buffer(),
                                                            data_buffer.Buffer() + data_buffer.Size());

    if (block->reader->format == K4A_IMAGE_FORMAT_DEPTH16 || block->reader->format == K4A_IMAGE_FORMAT_IR16)
    {
        // 16 bit grayscale needs to be converted from big-endian back to little-endian.
        assert(buffer->size() % sizeof(uint16_t) == 0);
        uint16_t *buffer_raw = reinterpret_cast<uint16_t *>(buffer->data());
        for (size_t j = 0; j < buffer->size() / sizeof(uint16_t); j++)
        {
            buffer_raw[j] = swap_bytes_16(buffer_raw[j]);
        }
    }

    assert(block->reader->width <= INT_MAX);
    assert(block->reader->height <= INT_MAX);
    assert(block->reader->stride <= INT_MAX);
    k4a_image_t image_handle = NULL;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    if (block->reader == &context->color_track)
    {
        result = TRACE_CALL(k4a_image_create_from_buffer(context->record_config.color_format,
                                                         (int)block->reader->width,
                                                         (int)block->reader->height,
                                                         (int)block->reader->stride,
                                                         buffer->data(),
                                                         buffer->size(),
                                                         &free_vector_buffer,
                                                         buffer,
                                                         &image_handle));
        k4a_image_set_timestamp_usec(image_handle, block->timestamp_ns / 1000);
        k4a_capture_set_color_image(*capture_handle, image_handle);
    }
    else if (block->reader == &context->depth_track)
    {
        result = TRACE_CALL(k4a_image_create_from_buffer(K4A_IMAGE_FORMAT_DEPTH16,
                                                         (int)block->reader->width,
                                                         (int)block->reader->height,
                                                         (int)block->reader->stride,
                                                         buffer->data(),
                                                         buffer->size(),
                                                         &free_vector_buffer,
                                                         buffer,
                                                         &image_handle));
        k4a_image_set_timestamp_usec(image_handle, block->timestamp_ns / 1000);
        k4a_capture_set_depth_image(*capture_handle, image_handle);
    }
    else if (block->reader == &context->ir_track)
    {
        result = TRACE_CALL(k4a_image_create_from_buffer(K4A_IMAGE_FORMAT_IR16,
                                                         (int)block->reader->width,
                                                         (int)block->reader->height,
                                                         (int)block->reader->stride,
                                                         buffer->data(),
                                                         buffer->size(),
                                                         &free_vector_buffer,
                                                         buffer,
                                                         &image_handle));
        k4a_image_set_timestamp_usec(image_handle, block->timestamp_ns / 1000);
        k4a_capture_set_ir_image(*capture_handle, image_handle);
    }
    else
    {
        logger_error(LOGGER_RECORD, "Capture being created from unknown track!");
        result = K4A_RESULT_FAILED;
    }

    if (image_handle != NULL)
    {
        k4a_image_release(image_handle);
    }

    if (K4A_FAILED(result))
    {
        delete block_ptr;
    }
    return result;
}

k4a_stream_result_t get_capture(k4a_playback_context_t *context, k4a_capture_t *capture_handle, bool next)
{
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, capture_handle == NULL);

    track_reader_t *blocks[] = { &context->color_track, &context->depth_track, &context->ir_track };
    std::shared_ptr<read_block_t> next_blocks[] = { nullptr, nullptr, nullptr };
    static_assert(arraysize(blocks) == arraysize(next_blocks), "Track / block mapping does not match");

    uint64_t timestamp_start_ns = UINT64_MAX;
    uint64_t timestamp_end_ns = 0;

    // Find the next block for each track
    int enabled_tracks = 0;
    for (size_t i = 0; i < arraysize(blocks); i++)
    {
        if (blocks[i]->track != NULL)
        {
            enabled_tracks++;

            // Only read from disk if we haven't aready found the next block for this track
            if (next_blocks[i] == nullptr)
            {
                next_blocks[i] = find_next_block(context, blocks[i], next);
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
                logger_trace(LOGGER_RECORD, "%s of recording reached", next ? "End" : "Beginning");
                blocks[i]->current_block = next_blocks[i];
            }
        }
    }

    // Count how many of the blocks are within the sync window
    int valid_blocks = 0;
    if (enabled_tracks > 0)
    {
        for (size_t i = 0; i < arraysize(blocks); i++)
        {
            if (next_blocks[i] && next_blocks[i]->block)
            {
                // If we're seeking forward, check the start timestamp, otherwise check the end timestamp
                if (next && (next_blocks[i]->sync_timestamp_ns - timestamp_start_ns < context->sync_period_ns / 2))
                {
                    valid_blocks++;
                }
                else if (!next && (timestamp_end_ns - next_blocks[i]->sync_timestamp_ns < context->sync_period_ns / 2))
                {
                    valid_blocks++;
                }
                else
                {
                    // Don't use this block as part of the capture
                    next_blocks[i] = nullptr;
                }
            }
        }
    }

    logger_trace(LOGGER_RECORD,
                 "Valid blocks: %d/%d, Start: %llu ms, End: %llu ms",
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
            k4a_result_t result = TRACE_CALL(new_capture(context, blocks[i]->current_block, capture_handle));
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

k4a_stream_result_t get_imu_sample(k4a_playback_context_t *context, k4a_imu_sample_t *imu_sample, bool next)
{
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, imu_sample == NULL);

    if (context->imu_track.track == NULL)
    {
        return K4A_STREAM_RESULT_EOF;
    }

    logger_error(LOGGER_RECORD, "IMU playback is not yet supported.");
    (void)next;

    return K4A_STREAM_RESULT_FAILED;
}

} // namespace k4arecord
