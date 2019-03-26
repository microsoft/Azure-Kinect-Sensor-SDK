// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4arecordingdockcontrol.h"

// System headers
//
#include <memory>
#include <ratio>
#include <sstream>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4aimguiextensions.h"
#include "k4atypeoperators.h"
#include "k4aviewerutil.h"
#include "k4awindowmanager.h"

using namespace k4aviewer;

K4ARecordingDockControl::K4ARecordingDockControl(std::unique_ptr<K4ARecording> &&recording)
{
    m_filenameLabel = recording->GetPath().filename().c_str();

    // Recording config
    //
    m_recordConfiguration = recording->GetRecordConfiguation();
    std::stringstream fpsSS;
    fpsSS << m_recordConfiguration.camera_fps;
    m_fpsLabel = fpsSS.str();

    switch (m_recordConfiguration.camera_fps)
    {
    case K4A_FRAMES_PER_SECOND_5:
        m_playbackThreadState.TimePerFrame = std::chrono::microseconds(std::micro::den / (std::micro::num * 5));
        break;

    case K4A_FRAMES_PER_SECOND_15:
        m_playbackThreadState.TimePerFrame = std::chrono::microseconds(std::micro::den / (std::micro::num * 15));
        break;

    case K4A_FRAMES_PER_SECOND_30:
    default:
        m_playbackThreadState.TimePerFrame = std::chrono::microseconds(std::micro::den / (std::micro::num * 30));
        break;
    }

    constexpr char noneStr[] = "(None)";

    // We don't record a depth track if the camera is started in passive IR mode
    //
    m_recordingHasDepth = m_recordConfiguration.depth_track_enabled;
    std::stringstream depthSS;
    if (m_recordingHasDepth)
    {
        depthSS << m_recordConfiguration.depth_mode;
    }
    else
    {
        depthSS << noneStr;
    }
    m_depthModeLabel = depthSS.str();

    m_recordingHasColor = m_recordConfiguration.color_track_enabled;
    std::stringstream colorResolutionSS;
    std::stringstream colorFormatSS;
    if (m_recordingHasColor)
    {
        colorFormatSS << m_recordConfiguration.color_format;
        colorResolutionSS << m_recordConfiguration.color_resolution;
    }
    else
    {
        colorFormatSS << noneStr;
        colorResolutionSS << noneStr;
    }

    m_colorFormatLabel = colorFormatSS.str();
    m_colorResolutionLabel = colorResolutionSS.str();

    // Sync info
    //
    m_depthDelayOffColorUsec = m_recordConfiguration.depth_delay_off_color_usec;

    std::stringstream syncModeSS;
    syncModeSS << m_recordConfiguration.wired_sync_mode;
    m_wiredSyncModeLabel = syncModeSS.str();

    m_subordinateDelayOffMasterUsec = m_recordConfiguration.subordinate_delay_off_master_usec;
    m_startTimestampOffsetUsec = m_recordConfiguration.start_timestamp_offset_usec;
    m_playbackThreadState.TimestampOffset = std::chrono::microseconds(m_startTimestampOffsetUsec);
    m_recordingLengthUsec = recording->GetRecordingLength();

    // Device info
    //
    if (K4A_BUFFER_RESULT_SUCCEEDED != recording->GetTag("K4A_DEVICE_SERIAL_NUMBER", &m_deviceSerialNumber))
    {
        m_deviceSerialNumber = noneStr;
    }
    if (K4A_BUFFER_RESULT_SUCCEEDED != recording->GetTag("K4A_COLOR_FIRMWARE_VERSION", &m_colorFirmwareVersion))
    {
        m_colorFirmwareVersion = noneStr;
    }
    if (K4A_BUFFER_RESULT_SUCCEEDED != recording->GetTag("K4A_DEPTH_FIRMWARE_VERSION", &m_depthFirmwareVersion))
    {
        m_depthFirmwareVersion = noneStr;
    }

    m_playbackThreadState.Recording = std::move(recording);
    PlaybackThreadState *pThreadState = &m_playbackThreadState;
    m_playbackThread = std::unique_ptr<K4APollingThread>(
        new K4APollingThread([pThreadState]() { return PlaybackThreadFn(pThreadState); }));

    SetViewType(K4AWindowSet::ViewType::Normal);
}

void K4ARecordingDockControl::Show()
{
    ImGui::Text("%s", m_filenameLabel.c_str());

    ImGuiExtensions::ButtonColorChanger cc(ImGuiExtensions::ButtonColor::Red);
    if (ImGui::SmallButton("Close"))
    {
        K4AWindowManager::Instance().ClearWindows();
        K4AWindowManager::Instance().PopDockControl();
        return;
    }
    cc.Clear();
    ImGui::Separator();

    ImGui::Text("%s", "Image formats");
    ImGui::Text("FPS:              %s", m_fpsLabel.c_str());
    ImGui::Text("Depth mode:       %s", m_depthModeLabel.c_str());
    ImGui::Text("Color format:     %s", m_colorFormatLabel.c_str());
    ImGui::Text("Color resolution: %s", m_colorResolutionLabel.c_str());
    ImGui::Separator();

    ImGui::Text("%s", "Sync settings");
    ImGui::Text("Depth/color delay (us): %d", m_depthDelayOffColorUsec);
    ImGui::Text("Sync mode:              %s", m_wiredSyncModeLabel.c_str());
    ImGui::Text("Subordinate delay (us): %d", m_subordinateDelayOffMasterUsec);
    ImGui::Text("Start timestamp offset: %d", m_startTimestampOffsetUsec);
    ImGui::Text("Recording Length (us):  %lu", m_recordingLengthUsec);
    ImGui::Separator();

    ImGui::Text("%s", "Device info");
    ImGui::Text("Device S/N:      %s", m_deviceSerialNumber.c_str());
    ImGui::Text("RGB camera FW:   %s", m_colorFirmwareVersion.c_str());
    ImGui::Text("Depth camera FW: %s", m_depthFirmwareVersion.c_str());
    ImGui::Separator();

    if (ImGui::Button("<|"))
    {
        std::lock_guard<std::mutex> lock(m_playbackThreadState.Mutex);
        m_playbackThreadState.Step = StepDirection::Backward;
        m_playbackThreadState.Paused = true;
    }
    ImGui::SameLine();

    const uint64_t seekMin = 0;
    const uint64_t seekMax = m_recordingLengthUsec;

    uint64_t currentTimestampUs;
    bool paused;
    {
        std::lock_guard<std::mutex> lock(m_playbackThreadState.Mutex);
        currentTimestampUs = static_cast<uint64_t>(m_playbackThreadState.CurrentCaptureTimestamp.count());
        paused = m_playbackThreadState.Paused;
    }

    if (ImGui::SliderScalar("##seek", ImGuiDataType_U64, &currentTimestampUs, &seekMin, &seekMax, ""))
    {
        std::lock_guard<std::mutex> lock(m_playbackThreadState.Mutex);
        m_playbackThreadState.SeekTimestamp = std::chrono::microseconds(currentTimestampUs);
        m_playbackThreadState.Paused = true;
    }
    ImGui::SameLine();

    if (ImGui::Button("|>"))
    {
        std::lock_guard<std::mutex> lock(m_playbackThreadState.Mutex);
        m_playbackThreadState.Step = StepDirection::Forward;
        m_playbackThreadState.Paused = true;
    }

    if (ImGui::Button("<<"))
    {
        std::lock_guard<std::mutex> lock(m_playbackThreadState.Mutex);
        m_playbackThreadState.SeekTimestamp = std::chrono::microseconds(0);
        m_playbackThreadState.Step = StepDirection::Forward;
        m_playbackThreadState.Paused = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(paused ? ">" : "||"))
    {
        std::lock_guard<std::mutex> lock(m_playbackThreadState.Mutex);
        m_playbackThreadState.Paused = !m_playbackThreadState.Paused;
    }
    ImGui::SameLine();
    if (ImGui::Button(">>"))
    {
        std::lock_guard<std::mutex> lock(m_playbackThreadState.Mutex);
        m_playbackThreadState.SeekTimestamp = std::chrono::microseconds(m_recordingLengthUsec + 1);
        m_playbackThreadState.Step = StepDirection::Forward;
        m_playbackThreadState.Paused = true;
    }

    K4AWindowSet::ShowModeSelector(&m_viewType, true, m_recordingHasDepth, [this](K4AWindowSet::ViewType t) {
        return this->SetViewType(t);
    });
}

bool K4ARecordingDockControl::PlaybackThreadFn(PlaybackThreadState *state)
{
    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

    std::unique_lock<std::mutex> lock(state->Mutex);

    k4a::capture backseekCapture;
    if (state->SeekTimestamp != PlaybackThreadState::InvalidSeekTime)
    {
        state->Recording->SeekTimestamp(state->SeekTimestamp.count());

        // The seek timestamp may end up in the middle of a capture, read backwards and forwards
        // again to get a full capture.
        //
        // If the read-forward fails after this, it means we seeked to the end of the file, and
        // this capture is the last capture in the file, so we actually do want to use this
        // capture, so we need to keep it until we determine if that happened.
        //
        backseekCapture = state->Recording->GetPreviousCapture();

        state->SeekTimestamp = PlaybackThreadState::InvalidSeekTime;

        // Force-read the next frame
        //
        state->Step = StepDirection::Forward;
    }

    bool backward = false;
    if (state->Step != StepDirection::None)
    {
        backward = state->Step == StepDirection::Backward;
        state->Step = StepDirection::None;

        // We don't want to restart from the beginning after stepping
        // under most circumstances.  If the user stepped to the last
        // capture, we'll pick up on that when we try to read it and
        // re-set this flag.
        //
        state->RecordingAtEnd = false;
    }
    else if (state->Paused)
    {
        return true;
    }
    else if (!state->Paused && state->RecordingAtEnd)
    {
        // Someone hit 'play' after the recording ended, so we need
        // to restart from the beginning
        //
        state->Recording->SeekTimestamp(0);
        state->RecordingAtEnd = false;
    }

    k4a::capture nextCapture = backward ? state->Recording->GetPreviousCapture() : state->Recording->GetNextCapture();
    if (nextCapture == nullptr)
    {
        // We're at the end of the file.
        //
        state->RecordingAtEnd = true;
        state->Paused = true;

        if (backseekCapture != nullptr)
        {
            // We reached EOF as a result of an explicit seek operation, and
            // the backseek capture is the last capture in the file.
            //
            nextCapture = std::move(backseekCapture);
        }
        else
        {
            // Recording ended
            //
            return true;
        }
    }

    state->CurrentCaptureTimestamp = GetCaptureTimestamp(nextCapture);

    // Update the images' timestamps using the timing data embedded in the recording
    // so we show comparable timestamps whenplaying back synchronized recordings
    //

    k4a::image images[] = { nextCapture.get_color_image(), nextCapture.get_depth_image(), nextCapture.get_ir_image() };

    for (k4a::image &image : images)
    {
        if (image)
        {
            image.set_timestamp(image.get_timestamp() + state->TimestampOffset);
        }
    }

    state->DataSource.NotifyObservers(nextCapture);
    lock.unlock();

    // Account for the time we spent getting captures and such when figuring out how long to wait
    // before starting the next frame
    //
    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::duration processingTime = endTime - startTime;
    auto processingTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(processingTime);
    std::chrono::microseconds sleepTime = state->TimePerFrame - processingTimeUs;
    if (sleepTime > std::chrono::microseconds(0))
    {
        std::this_thread::sleep_for(sleepTime);
    }
    return true;
}

std::chrono::microseconds K4ARecordingDockControl::GetCaptureTimestamp(const k4a::capture &capture)
{
    // Captures don't actually have timestamps, images do, so we have to look at all the images
    // associated with the capture.  We only need an approximate timestamp for seeking, so we just
    // return the first one we get back (we don't have to care if a capture has multiple images but
    // the timestamps are slightly off).
    //
    // We check the IR capture instead of the depth capture because if the depth camera is started
    // in passive IR mode, it only has an IR image (i.e. no depth image), but there is no mode
    // where a capture will have a depth image but not an IR image.
    //
    const auto irImage = capture.get_ir_image();
    if (irImage != nullptr)
    {
        return irImage.get_timestamp();
    }

    const auto depthImage = capture.get_depth_image();
    if (depthImage != nullptr)
    {
        return depthImage.get_timestamp();
    }

    const auto colorImage = capture.get_color_image();
    if (colorImage != nullptr)
    {
        return colorImage.get_timestamp();
    }

    return std::chrono::microseconds::zero();
}

void K4ARecordingDockControl::SetViewType(K4AWindowSet::ViewType viewType)
{
    K4AWindowManager::Instance().ClearWindows();

    std::lock_guard<std::mutex> lock(m_playbackThreadState.Mutex);
    switch (viewType)
    {
    case K4AWindowSet::ViewType::Normal:
        K4AWindowSet::StartNormalWindows(m_filenameLabel.c_str(),
                                         &m_playbackThreadState.DataSource,
                                         nullptr, // IMU playback not supported yet
                                         nullptr, // Audio source - sound is not supported in recordings
                                         m_recordingHasDepth,
                                         m_recordConfiguration.depth_mode,
                                         m_recordingHasColor,
                                         m_recordConfiguration.color_format,
                                         m_recordConfiguration.color_resolution);
        break;

    case K4AWindowSet::ViewType::PointCloudViewer:
        k4a::calibration calibration;
        const k4a_result_t result = m_playbackThreadState.Recording->GetCalibration(&calibration);
        if (result != K4A_RESULT_SUCCEEDED)
        {
            return;
        }

        bool colorPointCloudAvailable = m_recordConfiguration.color_track_enabled &&
                                        m_recordConfiguration.color_format == K4A_IMAGE_FORMAT_COLOR_BGRA32;
        K4AWindowSet::StartPointCloudWindow(m_filenameLabel.c_str(),
                                            std::move(calibration),
                                            &m_playbackThreadState.DataSource,
                                            colorPointCloudAvailable);
        break;
    }

    m_viewType = viewType;
}
