// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4arecording.h"

// System headers
//
#include <utility>

// Library headers
//

// Project headers
//
#include "k4aviewerutil.h"
#include "perfcounter.h"

using namespace k4aviewer;

std::unique_ptr<K4ARecording> K4ARecording::Open(const char *path)
{
    k4a_playback_t playback = nullptr;
    k4a_result_t result = k4a_playback_open(path, &playback);

    if (result != K4A_RESULT_SUCCEEDED || playback == nullptr)
    {
        return nullptr;
    }

    k4a_record_configuration_t recordConfiguration;
    result = k4a_playback_get_record_configuration(playback, &recordConfiguration);

    if (result != K4A_RESULT_SUCCEEDED)
    {
        k4a_playback_close(playback);
        return nullptr;
    }

    return std::unique_ptr<K4ARecording>(
        new K4ARecording(playback, std17::filesystem::path(path), recordConfiguration));
}

K4ARecording::~K4ARecording()
{
    if (m_playback)
    {
        (void)k4a_playback_close(m_playback);
    }
}

k4a::capture K4ARecording::GetNextCapture()
{
    return GetCapture(false);
}

k4a::capture K4ARecording::GetPreviousCapture()
{
    return GetCapture(true);
}

uint64_t K4ARecording::GetRecordingLength()
{
    return k4a_playback_get_last_timestamp_usec(m_playback);
}

k4a_result_t K4ARecording::SeekTimestamp(int64_t offsetUsec)
{
    return k4a_playback_seek_timestamp(m_playback, offsetUsec, K4A_PLAYBACK_SEEK_BEGIN);
}

const std17::filesystem::path &K4ARecording::GetPath() const
{
    return m_path;
}

k4a_result_t K4ARecording::GetCalibration(k4a::calibration &calibration)
{
    return k4a_playback_get_calibration(m_playback, &calibration);
}

k4a_buffer_result_t K4ARecording::GetTag(const char *name, std::string &out) const
{
    size_t size = 0;
    k4a_buffer_result_t result = k4a_playback_get_tag(m_playback, name, nullptr, &size);
    if (result != K4A_BUFFER_RESULT_TOO_SMALL)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    out.resize(size);
    result = k4a_playback_get_tag(m_playback, name, &out[0], &size);

    // Drop the null terminator since std::string 'hides' it
    //
    out.resize(std::max(size_t(0), size - 1));
    return result;
}

K4ARecording::K4ARecording(k4a_playback_t playback,
                           std17::filesystem::path path,
                           const k4a_record_configuration_t &recordConfiguration) :
    m_playback(playback),
    m_path(std::move(path)),
    m_recordConfiguration(recordConfiguration)
{
}

k4a::capture K4ARecording::GetCapture(bool backward)
{
    static PerfCounter getNextCapturePerfCounter("Playback: Get Next Capture");
    PerfSample ps(&getNextCapturePerfCounter);

    k4a_capture_t nextCapture;
    const k4a_stream_result_t result = backward ? k4a_playback_get_previous_capture(m_playback, &nextCapture) :
                                                  k4a_playback_get_next_capture(m_playback, &nextCapture);
    if (result != K4A_STREAM_RESULT_SUCCEEDED || nextCapture == nullptr)
    {
        return nullptr;
    }

    return nextCapture;
}
