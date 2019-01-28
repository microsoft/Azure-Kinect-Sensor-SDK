#/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

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

std::unique_ptr<K4ACapture> K4ARecording::GetNextCapture()
{
    return GetCapture(false);
}

std::unique_ptr<K4ACapture> K4ARecording::GetPreviousCapture()
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

k4a_result_t K4ARecording::GetCalibrationTransformData(std::unique_ptr<K4ACalibrationTransformData> &calibrationData)
{
    calibrationData.reset(new K4ACalibrationTransformData);
    return calibrationData->Initialize(m_playback);
}

K4ARecording::K4ARecording(k4a_playback_t playback,
                           std17::filesystem::path path,
                           const k4a_record_configuration_t &recordConfiguration) :
    m_playback(playback),
    m_path(std::move(path)),
    m_recordConfiguration(recordConfiguration)
{
}

std::unique_ptr<K4ACapture> K4ARecording::GetCapture(bool backward)
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

    return std14::make_unique<K4ACapture>(nextCapture);
}
