// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ARECORDING_H
#define K4ARECORDING_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a.h>
#include <k4arecord/playback.h>

// Project headers
//
#include "filesystem17.h"
#include "k4acalibrationtransformdata.h"
#include "k4acapture.h"

namespace k4aviewer
{

class K4ACalibrationTransformData;

class K4ARecording
{
public:
    static std::unique_ptr<K4ARecording> Open(const char *path);

    ~K4ARecording();

    const k4a_record_configuration_t &GetRecordConfiguation()
    {
        return m_recordConfiguration;
    }

    std::unique_ptr<K4ACapture> GetNextCapture();
    std::unique_ptr<K4ACapture> GetPreviousCapture();

    k4a_result_t SeekTimestamp(int64_t offsetUsec);

    uint64_t GetRecordingLength();

    const std17::filesystem::path &GetPath() const;

    k4a_result_t GetCalibrationTransformData(std::unique_ptr<K4ACalibrationTransformData> &calibrationData);

    k4a_buffer_result_t GetTag(const char *name, std::string &out) const;

private:
    K4ARecording(k4a_playback_t playback,
                 std17::filesystem::path path,
                 const k4a_record_configuration_t &recordConfiguration);

    std::unique_ptr<K4ACapture> GetCapture(bool backward);

    k4a_playback_t m_playback = nullptr;
    const std17::filesystem::path m_path;
    k4a_record_configuration_t m_recordConfiguration;
};

} // namespace k4aviewer

#endif
