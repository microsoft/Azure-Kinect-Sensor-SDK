/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4ARECORDINGDOCKCONTROL_H
#define K4ARECORDINGDOCKCONTROL_H

// System headers
//
#include <chrono>
#include <memory>

// Library headers
//

// Project headers
//
#include "ik4adockcontrol.h"
#include "k4adatasource.h"
#include "k4arecording.h"
#include "k4awindowset.h"

namespace k4aviewer
{

class K4ARecordingDockControl : public IK4ADockControl
{
public:
    explicit K4ARecordingDockControl(std::unique_ptr<K4ARecording> &&recording);

    void Show() override;

private:
    uint64_t GetCaptureTimestamp(const std::shared_ptr<K4ACapture> &capture);
    void SetViewType(K4AWindowSet::ViewType viewType);

    void ReadNext(bool force);
    void Step(bool backward);

    std::unique_ptr<K4ARecordingDockControl> m_dockControl;

    std::shared_ptr<K4ARecording> m_recording;

    std::string m_filenameLabel;
    std::string m_fpsLabel;
    std::string m_depthModeLabel;
    std::string m_colorFormatLabel;
    std::string m_colorResolutionLabel;

    int32_t m_depthDelayOffColorUsec;
    std::string m_wiredSyncModeLabel;
    uint32_t m_subordinateDelayOffMasterUsec;
    uint32_t m_startTimestampOffsetUsec;

    std::string m_deviceSerialNumber;
    std::string m_colorFirmwareVersion;
    std::string m_depthFirmwareVersion;

    bool m_recordingHasColor = false;
    bool m_recordingHasDepth = false;

    uint64_t m_currentTimestamp = 0;

    K4ADataSource<std::shared_ptr<K4ACapture>> m_cameraDataSource;

    std::chrono::high_resolution_clock::time_point m_lastFrameShownTime;
    std::chrono::microseconds m_timePerFrame;
    std::shared_ptr<K4ACapture> m_nextCapture;

    bool m_paused = false;
    K4AWindowSet::ViewType m_viewType = K4AWindowSet::ViewType::Normal;
};

} // namespace k4aviewer

#endif
