/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4ARECORDING_H
#define K4ARECORDING_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a_cpp.h>
#include <k4arecord/playback.h>

// Project headers
//
#include "filesystem17.h"
#include "k4acalibrationtransformdata.h"

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

    k4a::capture GetNextCapture();
    k4a::capture GetPreviousCapture();

    k4a_result_t SeekTimestamp(int64_t offsetUsec);

    uint64_t GetRecordingLength();

    const std17::filesystem::path &GetPath() const;

    k4a_result_t GetCalibrationTransformData(std::unique_ptr<K4ACalibrationTransformData> &calibrationData);

private:
    K4ARecording(k4a_playback_t playback,
                 std17::filesystem::path path,
                 const k4a_record_configuration_t &recordConfiguration);

    k4a::capture GetCapture(bool backward);

    k4a_playback_t m_playback = nullptr;
    const std17::filesystem::path m_path;
    k4a_record_configuration_t m_recordConfiguration;
};

} // namespace k4aviewer

#endif
