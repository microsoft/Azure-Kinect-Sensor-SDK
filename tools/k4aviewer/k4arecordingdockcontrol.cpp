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
#include "k4aimugraphdatagenerator.h"

using namespace k4aviewer;
namespace
{
static constexpr std::chrono::microseconds InvalidSeekTime = std::chrono::microseconds(-1);

std::string SafeGetTag(const k4a::playback &recording, const char *tagName)
{
    std::string result;
    if (!recording.get_tag(tagName, &result))
    {
        result = "Failed to read tag!";
    }

    return result;
}
} // namespace

K4ARecordingDockControl::K4ARecordingDockControl(std::string &&path, k4a::playback &&recording) :
    m_filenameLabel(std::move(path))
{
    m_playbackThreadState.SeekTimestamp = InvalidSeekTime;

    // Recording config
    //
    m_recordConfiguration = recording.get_record_configuration();
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
    m_recordingHasIR = m_recordConfiguration.ir_track_enabled;
    std::stringstream depthSS;
    if (m_recordingHasDepth || m_recordingHasIR)
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

        recording.set_color_conversion(K4A_IMAGE_FORMAT_COLOR_BGRA32);
        m_recordConfiguration.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
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
    m_recordingLengthUsec = static_cast<uint64_t>(recording.get_recording_length().count());

    // Device info
    //
    m_deviceSerialNumber = SafeGetTag(recording, "K4A_DEVICE_SERIAL_NUMBER");
    m_colorFirmwareVersion = SafeGetTag(recording, "K4A_COLOR_FIRMWARE_VERSION");
    m_depthFirmwareVersion = SafeGetTag(recording, "K4A_DEPTH_FIRMWARE_VERSION");

    m_playbackThreadState.Recording = std::move(recording);
    PlaybackThreadState *pThreadState = &m_playbackThreadState;
    m_playbackThread = std14::make_unique<K4APollingThread>(
        [pThreadState](bool) { return PlaybackThreadFn(pThreadState); });

    SetViewType(K4AWindowSet::ViewType::Normal);
}

K4ADockControlStatus K4ARecordingDockControl::Show()
{
    ImGui::TextUnformatted(m_filenameLabel.c_str());
    ImGui::SameLine();
    ImGuiExtensions::ButtonColorChanger cc(ImGuiExtensions::ButtonColor::Red);
    if (ImGui::SmallButton("Close"))
    {
        K4AWindowManager::Instance().ClearWindows();
        return K4ADockControlStatus::ShouldClose;
    }
    cc.Clear();
    ImGui::Separator();

    ImGui::TextUnformatted("Recording Settings");
    ImGui::Text("FPS:              %s", m_fpsLabel.c_str());
    ImGui::Text("Depth mode:       %s", m_depthModeLabel.c_str());
    ImGui::Text("Color format:     %s", m_colorFormatLabel.c_str());
    ImGui::Text("Color resolution: %s", m_colorResolutionLabel.c_str());
    ImGui::Text("IMU enabled:      %s", m_recordConfiguration.imu_track_enabled ? "Yes" : "No");
    ImGui::Separator();

    ImGui::TextUnformatted("Sync settings");
    ImGui::Text("Depth/color delay (us): %d", m_depthDelayOffColorUsec);
    ImGui::Text("Sync mode:              %s", m_wiredSyncModeLabel.c_str());
    ImGui::Text("Subordinate delay (us): %d", m_subordinateDelayOffMasterUsec);
    ImGui::Text("Start timestamp offset: %d", m_startTimestampOffsetUsec);
    ImGui::Text("Recording Length (us):  %lu", m_recordingLengthUsec);
    ImGui::Separator();

    ImGui::TextUnformatted("Device info");
    ImGui::Text("Device S/N:      %s", m_deviceSerialNumber.c_str());
    ImGui::Text("RGB camera FW:   %s", m_colorFirmwareVersion.c_str());
    ImGui::Text("Depth camera FW: %s", m_depthFirmwareVersion.c_str());
    ImGui::Separator();

    if (!m_playbackThread->IsRunning())
    {
        ImGui::Text("Playback failed!");
        return K4ADockControlStatus::Ok;
    }

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

    return K4ADockControlStatus::Ok;
}

bool K4ARecordingDockControl::PlaybackThreadFn(PlaybackThreadState *state)
{
    try
    {
        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

        std::unique_lock<std::mutex> lock(state->Mutex);

        bool forceRefreshImuData = false;
        if (state->SeekTimestamp != InvalidSeekTime)
        {
            // We need to read back a few seconds from before the time we seeked to.
            //
            forceRefreshImuData = true;

            state->Recording.seek_timestamp(state->SeekTimestamp, K4A_PLAYBACK_SEEK_BEGIN);
            state->SeekTimestamp = InvalidSeekTime;

            // Force-read the next frame
            //
            state->Step = StepDirection::Forward;
        }

        bool backward = false;
        if (state->Step != StepDirection::None)
        {
            backward = state->Step == StepDirection::Backward;
            state->Step = StepDirection::None;

            // Stepping backwards is closer to a seek - we can't just add
            // new samples on the end, we need to regenerate the graph
            //
            forceRefreshImuData |= backward;

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
            state->Recording.seek_timestamp(std::chrono::microseconds(0), K4A_PLAYBACK_SEEK_BEGIN);
            state->RecordingAtEnd = false;
            forceRefreshImuData = true;
        }

        k4a::capture nextCapture;
        const bool seekSuccessful = backward ? state->Recording.get_previous_capture(&nextCapture) :
                                               state->Recording.get_next_capture(&nextCapture);
        if (!seekSuccessful)
        {
            // We're at the end of the file.
            //
            state->RecordingAtEnd = true;
            state->Paused = true;

            // Attempt to show the last capture in the file.
            // We need to do this rather than just leaving the last-posted capture to handle
            // cases where we did a seek to EOF.
            //
            const bool backseekSuccessful = state->Recording.get_previous_capture(&nextCapture);
            if (!backseekSuccessful)
            {
                // Couldn't read back the last capture, so continue showing the last one
                //
                return true;
            }
        }

        state->CurrentCaptureTimestamp = GetCaptureTimestamp(nextCapture);

        // Read IMU data up to the next timestamp, if applicable
        //
        if (state->ImuPlaybackEnabled)
        {
            try
            {
                // On seek operations, we need to load historic data or the graph will be wrong.
                // Move the IMU read pointer back enough samples to populate the entire graph (if available).
                //
                if (forceRefreshImuData)
                {
                    state->ImuDataSource.ClearData();

                    k4a_imu_sample_t sample;

                    // Seek to the first IMU sample that was before the camera frame we're trying to show
                    //
                    while (state->Recording.get_previous_imu_sample(&sample))
                    {
                        if (sample.acc_timestamp_usec < static_cast<uint64_t>(state->CurrentCaptureTimestamp.count()))
                        {
                            break;
                        }
                    }

                    // Then seek back the length of the graph
                    //
                    for (int i = 0; i < K4AImuGraphDataGenerator::SamplesPerGraph; ++i)
                    {
                        if (!state->Recording.get_previous_imu_sample(&sample))
                        {
                            break;
                        }
                    }
                }

                // Read enough samples to catch up to the images that we're about to show
                //
                k4a_imu_sample_t nextImuSample;
                nextImuSample.acc_timestamp_usec = 0;

                while (nextImuSample.acc_timestamp_usec < static_cast<uint64_t>(state->CurrentCaptureTimestamp.count()))
                {
                    if (!state->Recording.get_next_imu_sample(&nextImuSample))
                    {
                        break;
                    }

                    state->ImuDataSource.NotifyObservers(nextImuSample);
                }
            }
            catch (const k4a::error &e)
            {
                // If something went wrong while reading the IMU data, mark the IMU failed, but allow
                // the camera playback to continue.
                //
                K4AViewerErrorManager::Instance().SetErrorStatus(e.what());
                state->ImuDataSource.NotifyTermination();
                state->ImuPlaybackEnabled = false;
            }
        }

        // Update the timestamps on the images using the timing data embedded in the recording
        // so we show comparable timestamps when playing back synchronized recordings
        //
        k4a::image images[] = { nextCapture.get_color_image(),
                                nextCapture.get_depth_image(),
                                nextCapture.get_ir_image() };

        for (k4a::image &image : images)
        {
            if (image)
            {
                image.set_timestamp(image.get_device_timestamp());
            }
        }

        state->CaptureDataSource.NotifyObservers(nextCapture);
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
    catch (const k4a::error &e)
    {
        K4AViewerErrorManager::Instance().SetErrorStatus(e.what());
        return false;
    }
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
        return irImage.get_device_timestamp();
    }

    const auto depthImage = capture.get_depth_image();
    if (depthImage != nullptr)
    {
        return depthImage.get_device_timestamp();
    }

    const auto colorImage = capture.get_color_image();
    if (colorImage != nullptr)
    {
        return colorImage.get_device_timestamp();
    }

    return std::chrono::microseconds::zero();
}

void K4ARecordingDockControl::SetViewType(K4AWindowSet::ViewType viewType)
{
    K4AWindowManager::Instance().ClearWindows();

    std::lock_guard<std::mutex> lock(m_playbackThreadState.Mutex);

    K4ADataSource<k4a_imu_sample_t> *imuDataSource = nullptr;
    switch (viewType)
    {
    case K4AWindowSet::ViewType::Normal:
        if (m_recordConfiguration.imu_track_enabled)
        {
            m_playbackThreadState.ImuPlaybackEnabled = true;
            imuDataSource = &m_playbackThreadState.ImuDataSource;
        }
        K4AWindowSet::StartNormalWindows(m_filenameLabel.c_str(),
                                         &m_playbackThreadState.CaptureDataSource,
                                         imuDataSource,
                                         nullptr, // Audio source - sound is not supported in recordings
                                         m_recordingHasDepth || m_recordingHasIR,
                                         m_recordConfiguration.depth_mode,
                                         m_recordingHasColor,
                                         m_recordConfiguration.color_format,
                                         m_recordConfiguration.color_resolution);
        break;

    case K4AWindowSet::ViewType::PointCloudViewer:
        try
        {
            k4a::calibration calibration = m_playbackThreadState.Recording.get_calibration();
            K4AWindowSet::StartPointCloudWindow(m_filenameLabel.c_str(),
                                                std::move(calibration),
                                                &m_playbackThreadState.CaptureDataSource,
                                                m_recordConfiguration.color_track_enabled);
        }
        catch (const k4a::error &e)
        {
            K4AViewerErrorManager::Instance().SetErrorStatus(e.what());
        }

        break;
    }

    m_viewType = viewType;
}
