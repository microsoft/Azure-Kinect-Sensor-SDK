// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ARECORDINGDOCKCONTROL_H
#define K4ARECORDINGDOCKCONTROL_H

// System headers
//
#include <chrono>
#include <memory>

// Library headers
//
#include <k4arecord/playback.hpp>

// Project headers
//
#include "ik4adockcontrol.h"
#include "k4adatasource.h"
#include "k4aimugraphdatagenerator.h"
#include "k4apollingthread.h"
#include "k4awindowset.h"

namespace k4aviewer
{

class K4ARecordingDockControl : public IK4ADockControl
{
public:
    explicit K4ARecordingDockControl(std::string &&path, k4a::playback &&recording);

    K4ADockControlStatus Show() override;

private:
    enum class StepDirection
    {
        None,
        Forward,
        Backward
    };

    struct PlaybackThreadState
    {
        std::mutex Mutex;

        // UI control signals
        //
        bool Paused = false;
        bool RecordingAtEnd = false;
        StepDirection Step = StepDirection::None;
        std::chrono::microseconds SeekTimestamp;
        std::chrono::microseconds CurrentCaptureTimestamp;

        // Constant state (expected to be set once, accessible without synchronization)
        //
        std::chrono::microseconds TimePerFrame;

        // Recording state
        //
        k4a::playback Recording;
        K4ADataSource<k4a::capture> CaptureDataSource;
        K4ADataSource<k4a_imu_sample_t> ImuDataSource;

        bool ImuPlaybackEnabled;
    } m_playbackThreadState;

    static bool PlaybackThreadFn(PlaybackThreadState *state);

    static std::chrono::microseconds GetCaptureTimestamp(const k4a::capture &capture);

    void SetViewType(K4AWindowSet::ViewType viewType);

    // Labels / static UI state
    //
    k4a_record_configuration_t m_recordConfiguration;

    std::string m_filenameLabel;
    std::string m_fpsLabel;
    std::string m_depthModeLabel;
    std::string m_colorFormatLabel;
    std::string m_colorResolutionLabel;

    int32_t m_depthDelayOffColorUsec;
    std::string m_wiredSyncModeLabel;
    uint32_t m_subordinateDelayOffMasterUsec;
    uint32_t m_startTimestampOffsetUsec;
    uint64_t m_recordingLengthUsec;

    std::string m_deviceSerialNumber;
    std::string m_colorFirmwareVersion;
    std::string m_depthFirmwareVersion;

    bool m_recordingHasColor = false;
    bool m_recordingHasDepth = false;
    bool m_recordingHasIR = false;

    K4AWindowSet::ViewType m_viewType = K4AWindowSet::ViewType::Normal;

    std::unique_ptr<K4APollingThread> m_playbackThread;
};

} // namespace k4aviewer

#endif
