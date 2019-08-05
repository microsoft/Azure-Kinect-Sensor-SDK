// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <ctime>
#include <iostream>
#include <sstream>

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <k4ainternal/matroska_read.h>
#include <k4ainternal/common.h>

using namespace k4arecord;
using namespace LIBMATROSKA_NAMESPACE;

k4a_result_t k4a_playback_open(const char *path, k4a_playback_t *playback_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, path == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, playback_handle == NULL);
    k4a_playback_context_t *context = NULL;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    context = k4a_playback_t_create(playback_handle);
    result = K4A_RESULT_FROM_BOOL(context != NULL);

    if (K4A_SUCCEEDED(result))
    {
        context->file_path = path;
        context->file_closing = false;

        try
        {
            context->ebml_file = make_unique<LargeFileIOCallback>(path, MODE_READ);
            context->stream = make_unique<libebml::EbmlStream>(*context->ebml_file);
        }
        catch (std::ios_base::failure &e)
        {
            LOG_ERROR("Unable to open file '%s': %s", path, e.what());
            result = K4A_RESULT_FAILED;
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(parse_mkv(context));
    }

    if (K4A_SUCCEEDED(result))
    {
        // Seek to the first cluster
        cluster_info_t *seek_cluster_info = find_cluster(context, 0);
        if (seek_cluster_info == NULL)
        {
            LOG_ERROR("Failed to parse recording, recording is empty.", 0);
            result = K4A_RESULT_FAILED;
        }
        else
        {
            context->seek_cluster = load_cluster(context, seek_cluster_info);
            if (context->seek_cluster == nullptr)
            {
                LOG_ERROR("Failed to load first data cluster of recording.", 0);
                result = K4A_RESULT_FAILED;
            }
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        reset_seek_pointers(context, 0);
    }
    else
    {
        if (context && context->ebml_file)
        {
            try
            {
                context->ebml_file->close();
            }
            catch (std::ios_base::failure &)
            {
                // The file was opened as read-only, ignore any close failures.
            }
        }

        k4a_playback_t_destroy(*playback_handle);
        *playback_handle = NULL;
    }

    return result;
}

k4a_buffer_result_t k4a_playback_get_raw_calibration(k4a_playback_t playback_handle, uint8_t *data, size_t *data_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, data_size == NULL);

    if (context->calibration_attachment == NULL)
    {
        LOG_ERROR("The device calibration is missing from the recording.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    KaxFileData &file_data = GetChild<KaxFileData>(*context->calibration_attachment);
    // Attachment is stored in binary, not a string, so null termination is not guaranteed.
    // Check if the binary data is null terminated, and if not append a zero.
    size_t append_zero = 0;
    assert(file_data.GetSize() > 0 && file_data.GetSize() <= SIZE_MAX);
    if (file_data.GetBuffer()[file_data.GetSize() - 1] != '\0')
    {
        append_zero = 1;
    }
    if (data != NULL && *data_size >= (file_data.GetSize() + append_zero))
    {
        memcpy(data, static_cast<uint8_t *>(file_data.GetBuffer()), (size_t)file_data.GetSize());
        if (append_zero)
        {
            data[file_data.GetSize()] = '\0';
        }
        *data_size = (size_t)file_data.GetSize() + append_zero;
        return K4A_BUFFER_RESULT_SUCCEEDED;
    }
    else
    {
        *data_size = (size_t)file_data.GetSize() + append_zero;
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }
}

k4a_result_t k4a_playback_get_calibration(k4a_playback_t playback_handle, k4a_calibration_t *calibration)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, calibration == NULL);

    if (context->calibration_attachment == NULL)
    {
        LOG_ERROR("The device calibration is missing from the recording.", 0);
        return K4A_RESULT_FAILED;
    }

    if (context->device_calibration == nullptr)
    {
        context->device_calibration = make_unique<k4a_calibration_t>();
        KaxFileData &file_data = GetChild<KaxFileData>(*context->calibration_attachment);
        // Attachment is stored in binary, not a string, so null termination is not guaranteed.
        assert(file_data.GetSize() <= SIZE_MAX);
        std::vector<char> buffer = std::vector<char>((size_t)file_data.GetSize() + 1);
        memcpy(&buffer[0], file_data.GetBuffer(), (size_t)file_data.GetSize());
        buffer[buffer.size() - 1] = '\0';
        k4a_result_t result = k4a_calibration_get_from_raw(buffer.data(),
                                                           buffer.size(),
                                                           context->record_config.depth_mode,
                                                           context->record_config.color_resolution,
                                                           context->device_calibration.get());
        if (K4A_FAILED(result))
        {
            context->device_calibration.reset();
            return result;
        }
    }
    *calibration = *context->device_calibration;

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t k4a_playback_get_record_configuration(k4a_playback_t playback_handle, k4a_record_configuration_t *config)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);

    *config = context->record_config;
    return K4A_RESULT_SUCCEEDED;
}

bool k4a_playback_check_track_exists(k4a_playback_t playback_handle, const char *track_name)
{
    RETURN_VALUE_IF_HANDLE_INVALID(false, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(false, context == NULL);
    RETURN_VALUE_IF_ARG(false, track_name == NULL);

    track_reader_t *track_reader = get_track_reader_by_name(context, track_name);
    return track_reader != nullptr;
}

size_t k4a_playback_get_track_count(k4a_playback_t playback_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(0, context == NULL);

    return context->track_map.size();
}

k4a_buffer_result_t k4a_playback_get_track_name(k4a_playback_t playback_handle,
                                                size_t track_index,
                                                char *track_name,
                                                size_t *track_name_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, context->track_map.empty());
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, track_name_size == NULL);

    size_t index = 0;
    for (auto &itr : context->track_map)
    {
        if (index == track_index)
        {
            auto track_reader = &itr.second;

            // std::string doesn't include the appending zero at the end when counting size
            const size_t append_zero = 1;
            if (track_name != NULL && *track_name_size >= track_reader->track_name.size() + append_zero)
            {
                memcpy(track_name, track_reader->track_name.c_str(), track_reader->track_name.size() + append_zero);
                *track_name_size = track_reader->track_name.size() + append_zero;
                return K4A_BUFFER_RESULT_SUCCEEDED;
            }
            else
            {
                *track_name_size = track_reader->track_name.size() + append_zero;
                return K4A_BUFFER_RESULT_TOO_SMALL;
            }
        }
        index++;
    }

    LOG_ERROR("Track index out of bounds: %u", track_index);
    return K4A_BUFFER_RESULT_FAILED;
}

bool k4a_playback_track_is_builtin(k4a_playback_t playback_handle, const char *track_name)
{
    RETURN_VALUE_IF_HANDLE_INVALID(false, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(false, context == NULL);
    RETURN_VALUE_IF_ARG(false, track_name == NULL);

    track_reader_t *track_reader = get_track_reader_by_name(context, track_name);
    return track_reader != nullptr && check_track_reader_is_builtin(context, track_reader);
}

k4a_result_t k4a_playback_track_get_video_settings(k4a_playback_t playback_handle,
                                                   const char *track_name,
                                                   k4a_record_video_settings_t *video_settings)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, track_name == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, video_settings == NULL);

    track_reader_t *track_reader = get_track_reader_by_name(context, track_name);
    if (track_reader == nullptr)
    {
        LOG_ERROR("Track name cannot be found: %s", track_name);
        return K4A_RESULT_FAILED;
    }

    if (track_reader->type != track_type::track_video)
    {
        LOG_ERROR("Track is not a video track: %s", track_name);
        return K4A_RESULT_FAILED;
    }

    video_settings->width = track_reader->width;
    video_settings->height = track_reader->height;
    video_settings->frame_rate = static_cast<uint64_t>(1_s / track_reader->frame_period_ns);

    return K4A_RESULT_SUCCEEDED;
}

k4a_buffer_result_t k4a_playback_track_get_codec_id(k4a_playback_t playback_handle,
                                                    const char *track_name,
                                                    char *codec_id,
                                                    size_t *codec_id_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, track_name == NULL);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, codec_id_size == NULL);

    track_reader_t *track_reader = get_track_reader_by_name(context, track_name);
    if (track_reader == nullptr)
    {
        LOG_ERROR("Track name cannot be found: %s", track_name);
        return K4A_BUFFER_RESULT_FAILED;
    }

    // std::string doesn't include the appending zero at the end when counting size
    const size_t append_zero = 1;
    if (codec_id != NULL && *codec_id_size >= track_reader->codec_id.size() + append_zero)
    {
        memcpy(codec_id, track_reader->codec_id.c_str(), track_reader->codec_id.size() + append_zero);
        *codec_id_size = track_reader->codec_id.size() + append_zero;
        return K4A_BUFFER_RESULT_SUCCEEDED;
    }
    else
    {
        *codec_id_size = track_reader->codec_id.size() + append_zero;
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }
}

k4a_buffer_result_t k4a_playback_track_get_codec_context(k4a_playback_t playback_handle,
                                                         const char *track_name,
                                                         uint8_t *codec_context,
                                                         size_t *codec_context_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, track_name == NULL);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, codec_context_size == NULL);

    track_reader_t *track_reader = get_track_reader_by_name(context, track_name);
    if (track_reader == nullptr)
    {
        LOG_ERROR("Track name cannot be found: %s", track_name);
        return K4A_BUFFER_RESULT_FAILED;
    }

    if (codec_context != NULL && *codec_context_size >= track_reader->codec_private.size())
    {
        memcpy(codec_context, track_reader->codec_private.data(), track_reader->codec_private.size());
        *codec_context_size = track_reader->codec_private.size();
        return K4A_BUFFER_RESULT_SUCCEEDED;
    }
    else
    {
        *codec_context_size = track_reader->codec_private.size();
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }
}

k4a_buffer_result_t
k4a_playback_get_tag(k4a_playback_t playback_handle, const char *name, char *value, size_t *value_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, value_size == NULL);

    KaxTag *tag = get_tag(context, name);
    if (tag != NULL)
    {
        std::string tag_str = get_tag_string(tag);

        size_t input_buffer_size = *value_size;
        *value_size = tag_str.size() + 1;
        if (value == NULL || input_buffer_size < *value_size)
        {
            return K4A_BUFFER_RESULT_TOO_SMALL;
        }
        else
        {
            memset(value, '\0', input_buffer_size);
            memcpy(value, tag_str.c_str(), tag_str.size());
            return K4A_BUFFER_RESULT_SUCCEEDED;
        }
    }
    else
    {
        return K4A_BUFFER_RESULT_FAILED;
    }
}

k4a_result_t k4a_playback_set_color_conversion(k4a_playback_t playback_handle, k4a_image_format_t target_format)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);

    if (context->color_track == NULL)
    {
        LOG_ERROR("The color track is not enabled in this recording. The color conversion format cannot be set.", 0);
        return K4A_RESULT_FAILED;
    }

    switch (target_format)
    {
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
        if (context->color_track->format == K4A_IMAGE_FORMAT_COLOR_MJPG)
        {
            context->color_format_conversion = target_format;
        }
        else
        {
            LOG_ERROR("Converting color images to K4A_IMAGE_FORMAT_COLOR_MJPG is not supported.", 0);
            return K4A_RESULT_FAILED;
        }
        break;
    case K4A_IMAGE_FORMAT_COLOR_NV12:
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        context->color_format_conversion = target_format;
        break;
    default:
        LOG_ERROR("Unsupported target_format specified for format conversion: %d", target_format);
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_buffer_result_t
k4a_playback_get_attachment(k4a_playback_t playback_handle, const char *file_name, uint8_t *data, size_t *data_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, file_name == NULL);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, data_size == NULL);

    KaxAttached *attachment = get_attachment_by_name(context, file_name);
    if (attachment != NULL)
    {
        KaxFileData &file_data = GetChild<KaxFileData>(*attachment);
        if (data != NULL && *data_size >= file_data.GetSize())
        {
            memcpy(data, static_cast<uint8_t *>(file_data.GetBuffer()), (size_t)file_data.GetSize());
            *data_size = (size_t)file_data.GetSize();
            return K4A_BUFFER_RESULT_SUCCEEDED;
        }
        else
        {
            *data_size = (size_t)file_data.GetSize();
            return K4A_BUFFER_RESULT_TOO_SMALL;
        }
    }
    else
    {
        LOG_ERROR("Attachment file name cannot be found: %s", file_name);
        return K4A_BUFFER_RESULT_FAILED;
    }
}

k4a_stream_result_t k4a_playback_get_next_capture(k4a_playback_t playback_handle, k4a_capture_t *capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_STREAM_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, capture_handle == NULL);

    return get_capture(context, capture_handle, true);
}

k4a_stream_result_t k4a_playback_get_previous_capture(k4a_playback_t playback_handle, k4a_capture_t *capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_STREAM_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, capture_handle == NULL);

    return get_capture(context, capture_handle, false);
}

k4a_stream_result_t k4a_playback_get_next_imu_sample(k4a_playback_t playback_handle, k4a_imu_sample_t *imu_sample)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_STREAM_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, imu_sample == NULL);

    return get_imu_sample(context, imu_sample, true);
}

k4a_stream_result_t k4a_playback_get_previous_imu_sample(k4a_playback_t playback_handle, k4a_imu_sample_t *imu_sample)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_STREAM_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, imu_sample == NULL);

    return get_imu_sample(context, imu_sample, false);
}

k4a_stream_result_t k4a_playback_get_next_data_block(k4a_playback_t playback_handle,
                                                     const char *track_name,
                                                     k4a_playback_data_block_t *data_block_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_STREAM_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, track_name == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, data_block_handle == NULL);

    track_reader_t *track_reader = get_track_reader_by_name(context, track_name);
    if (track_reader == nullptr)
    {
        LOG_ERROR("Track name cannot be found: %s", track_name);
        return K4A_STREAM_RESULT_FAILED;
    }

    if (check_track_reader_is_builtin(context, track_reader))
    {
        LOG_ERROR("k4a_playback_get_next_data_block cannot be used with the built-in track: %s", track_name);
        return K4A_STREAM_RESULT_FAILED;
    }

    return get_data_block(context, track_reader, data_block_handle, true);
}

k4a_stream_result_t k4a_playback_get_previous_data_block(k4a_playback_t playback_handle,
                                                         const char *track_name,
                                                         k4a_playback_data_block_t *data_block_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_STREAM_RESULT_FAILED, k4a_playback_t, playback_handle);
    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, track_name == NULL);
    RETURN_VALUE_IF_ARG(K4A_STREAM_RESULT_FAILED, data_block_handle == NULL);

    track_reader_t *track_reader = get_track_reader_by_name(context, track_name);
    if (track_reader == nullptr)
    {
        LOG_ERROR("Track name cannot be found: %s", track_name);
        return K4A_STREAM_RESULT_FAILED;
    }

    if (check_track_reader_is_builtin(context, track_reader))
    {
        LOG_ERROR("k4a_playback_get_previous_data_block cannot be used with the built-in track: %s", track_name);
        return K4A_STREAM_RESULT_FAILED;
    }

    return get_data_block(context, track_reader, data_block_handle, false);
}

uint64_t k4a_playback_data_block_get_device_timestamp_usec(k4a_playback_data_block_t data_block_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_playback_data_block_t, data_block_handle);
    k4a_playback_data_block_context_t *data_block_context = k4a_playback_data_block_t_get_context(data_block_handle);
    RETURN_VALUE_IF_ARG(0, data_block_context == NULL);
    return data_block_context->device_timestamp_usec;
}

size_t k4a_playback_data_block_get_buffer_size(k4a_playback_data_block_t data_block_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_playback_data_block_t, data_block_handle);
    k4a_playback_data_block_context_t *data_block_context = k4a_playback_data_block_t_get_context(data_block_handle);
    RETURN_VALUE_IF_ARG(0, data_block_context == NULL);
    return data_block_context->data_block.size();
}

uint8_t *k4a_playback_data_block_get_buffer(k4a_playback_data_block_t data_block_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(nullptr, k4a_playback_data_block_t, data_block_handle);
    k4a_playback_data_block_context_t *data_block_context = k4a_playback_data_block_t_get_context(data_block_handle);
    RETURN_VALUE_IF_ARG(nullptr, data_block_context == NULL);
    return data_block_context->data_block.data();
}

void k4a_playback_data_block_release(k4a_playback_data_block_t data_block_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_playback_data_block_t, data_block_handle);
    k4a_playback_data_block_t_destroy(data_block_handle);
}

k4a_result_t k4a_playback_seek_timestamp(k4a_playback_t playback_handle,
                                         int64_t offset_usec,
                                         k4a_playback_seek_origin_t origin)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_playback_t, playback_handle);

    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, context->segment == nullptr);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED,
                        origin != K4A_PLAYBACK_SEEK_BEGIN && origin != K4A_PLAYBACK_SEEK_END &&
                            origin != K4A_PLAYBACK_SEEK_DEVICE_TIME);

    // If seeking to a device timestamp, calculate the offset relative to the start of file.
    if (origin == K4A_PLAYBACK_SEEK_DEVICE_TIME)
    {
        origin = K4A_PLAYBACK_SEEK_BEGIN;
        offset_usec -= (int64_t)context->record_config.start_timestamp_offset_usec;
    }

    // Clamp the offset timestamp so the seek direction is correct reletive to the specified origin.
    if (origin == K4A_PLAYBACK_SEEK_BEGIN && offset_usec < 0)
    {
        offset_usec = 0;
    }
    else if (origin == K4A_PLAYBACK_SEEK_END && offset_usec > 0)
    {
        offset_usec = 0;
    }

    uint64_t target_time_ns = 0;
    if (origin == K4A_PLAYBACK_SEEK_END)
    {
        uint64_t offset_ns = (uint64_t)(-offset_usec * 1000);
        if (offset_ns > context->last_file_timestamp_ns)
        {
            // If the target timestamp is negative, clamp to 0 so we don't underflow.
            target_time_ns = 0;
        }
        else
        {
            target_time_ns = context->last_file_timestamp_ns + 1 - offset_ns;
        }
    }
    else
    {
        target_time_ns = (uint64_t)offset_usec * 1000;
    }

    cluster_info_t *seek_cluster_info = find_cluster(context, target_time_ns);
    if (seek_cluster_info == NULL)
    {
        LOG_ERROR("Failed to find cluster for timestamp: %llu ns", target_time_ns);
        return K4A_RESULT_FAILED;
    }

    std::shared_ptr<loaded_cluster_t> seek_cluster = load_cluster(context, seek_cluster_info);
    if (seek_cluster == nullptr || seek_cluster->cluster == nullptr)
    {
        LOG_ERROR("Failed to load data cluster at timestamp: %llu ns", target_time_ns);
        return K4A_RESULT_FAILED;
    }

    context->seek_cluster = seek_cluster;
    reset_seek_pointers(context, target_time_ns);

    return K4A_RESULT_SUCCEEDED;
}

uint64_t k4a_playback_get_recording_length_usec(k4a_playback_t playback_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_playback_t, playback_handle);

    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(0, context == NULL);
    return context->last_file_timestamp_ns / 1000;
}

uint64_t k4a_playback_get_last_timestamp_usec(k4a_playback_t playback_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_playback_t, playback_handle);

    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    RETURN_VALUE_IF_ARG(0, context == NULL);
    return context->last_file_timestamp_ns / 1000;
}

void k4a_playback_close(const k4a_playback_t playback_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_playback_t, playback_handle);

    k4a_playback_context_t *context = k4a_playback_t_get_context(playback_handle);
    if (context != NULL)
    {
        LOG_TRACE("File reading stats:", 0);
        LOG_TRACE("  Seek count: %llu", context->seek_count);
        LOG_TRACE("  Cluster load count: %llu", context->load_count);
        LOG_TRACE("  Cluster cache hits: %llu", context->cache_hits);

        context->file_closing = true;

        try
        {
            try
            {
                context->io_lock.lock();
            }
            catch (std::system_error &)
            {
                // Lock is in a bad state, close the file anyway.
            }
            context->ebml_file->close();
        }
        catch (std::ios_base::failure &)
        {
            // The file was opened as read-only, ignore any close failures.
        }

        context->io_lock.unlock();
    }
    k4a_playback_t_destroy(playback_handle);
}
