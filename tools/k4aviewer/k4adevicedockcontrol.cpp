// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4adevicedockcontrol.h"

// System headers
//
#include <memory>
#include <ratio>
#include <sstream>
#include <utility>

// Library headers
//

// Project headers
//
#include "k4aaudiomanager.h"
#include "k4aimguiextensions.h"
#include "k4atypeoperators.h"
#include "k4aviewererrormanager.h"
#include "k4aviewerutil.h"
#include "k4awindowmanager.h"

using namespace k4aviewer;

namespace
{
constexpr std::chrono::milliseconds CameraPollingTimeout(2000);
constexpr std::chrono::milliseconds ImuPollingTimeout(2000);

constexpr std::chrono::minutes SubordinateModeStartupTimeout(5);

constexpr std::chrono::milliseconds PollingThreadCleanShutdownTimeout = std::chrono::milliseconds(1000 / 5);

template<typename T>
void StopSensor(k4a::device *device,
                std::function<void(k4a::device *)> stopFn,
                K4ADataSource<T> *dataSource,
                bool *started)
{
    if (*started)
    {
        stopFn(device);
    }
    dataSource->NotifyTermination();
    *started = false;
}

template<typename T>
bool PollSensor(const char *sensorFriendlyName,
                k4a::device *device,
                K4ADataSource<T> *dataSource,
                bool *paused,
                bool *started,
                bool *abortInProgress,
                std::function<bool(k4a::device *, T *, std::chrono::milliseconds)> pollFn,
                std::function<void(k4a::device *)> stopFn,
                std::chrono::milliseconds timeout)
{
    std::string errorMessage;

    try
    {
        T data;
        const bool succeeded = pollFn(device, &data, timeout);
        if (succeeded)
        {
            if (!*paused)
            {
                dataSource->NotifyObservers(data);
            }
            return true;
        }

        errorMessage = "timed out!";
    }
    catch (const k4a::error &e)
    {
        errorMessage = e.what();
    }

    StopSensor(device, stopFn, dataSource, started);

    if (!*abortInProgress)
    {
        std::stringstream errorBuilder;
        errorBuilder << sensorFriendlyName << " failed: " << errorMessage;
        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());
    }

    return false;
}

template<typename T>
void StopPollingThread(std::unique_ptr<K4APollingThread> *pollingThread,
                       k4a::device *device,
                       std::function<void(k4a::device *)> stopFn,
                       K4ADataSource<T> *dataSource,
                       bool *started,
                       bool *abortInProgress)
{
    *abortInProgress = true;
    if (*pollingThread)
    {
        (*pollingThread)->StopAsync();

        // Attempt graceful shutdown of the polling thread to reduce noise.
        // If this doesn't work out, we'll stop the device manually, which will
        // make the polling thread's blocking call to get the next data sample abort.
        //
        auto startTime = std::chrono::high_resolution_clock::now();
        while (*started)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto now = std::chrono::high_resolution_clock::now();
            if (now - startTime > PollingThreadCleanShutdownTimeout)
            {
                break;
            }
        }
    }

    StopSensor(device, stopFn, dataSource, started);
    pollingThread->reset();
    *abortInProgress = false;
}

} // namespace

void K4ADeviceDockControl::ShowColorControl(k4a_color_control_command_t command,
                                            ColorSetting *cacheEntry,
                                            const std::function<ColorControlAction(ColorSetting *)> &showControl)
{
    const ColorControlAction action = showControl(cacheEntry);
    if (action == ColorControlAction::None)
    {
        return;
    }

    if (action == ColorControlAction::SetManual)
    {
        cacheEntry->Mode = K4A_COLOR_CONTROL_MODE_MANUAL;
    }
    else if (action == ColorControlAction::SetAutomatic)
    {
        cacheEntry->Mode = K4A_COLOR_CONTROL_MODE_AUTO;
    }

    ApplyColorSetting(command, cacheEntry);
}

void K4ADeviceDockControl::ShowColorControlAutoButton(k4a_color_control_mode_t currentMode,
                                                      ColorControlAction *actionToUpdate,
                                                      const char *id)
{
    ImGui::PushID(id);
    if (currentMode == K4A_COLOR_CONTROL_MODE_MANUAL)
    {
        if (ImGui::Button("M"))
        {
            *actionToUpdate = ColorControlAction::SetAutomatic;
        }
    }
    else
    {
        if (ImGui::Button("A"))
        {
            *actionToUpdate = ColorControlAction::SetManual;
        }
    }
    ImGui::PopID();
}

void K4ADeviceDockControl::ApplyColorSetting(k4a_color_control_command_t command, ColorSetting *cacheEntry)
{
    try
    {
        m_device.set_color_control(command, cacheEntry->Mode, cacheEntry->Value);

        // The camera can decide to set a different value than the one we give it, so rather than just saving off
        // the mode we set, we read it back from the camera and cache that instead.
        //
        ReadColorSetting(command, cacheEntry);
    }
    catch (const k4a::error &e)
    {
        K4AViewerErrorManager::Instance().SetErrorStatus(e.what());
    }
}

void K4ADeviceDockControl::ApplyDefaultColorSettings()
{
    // The color settings get persisted in the camera's firmware, so there isn't a way
    // to know if the setting's value at the time we started K4AViewer is the default.
    // However, the default settings are the same for all devices, so we just hardcode
    // them here.
    //
    m_colorSettingsCache.ExposureTimeUs = { K4A_COLOR_CONTROL_MODE_AUTO, 15625 };
    ApplyColorSetting(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &m_colorSettingsCache.ExposureTimeUs);

    m_colorSettingsCache.WhiteBalance = { K4A_COLOR_CONTROL_MODE_AUTO, 4500 };
    ApplyColorSetting(K4A_COLOR_CONTROL_WHITEBALANCE, &m_colorSettingsCache.WhiteBalance);

    m_colorSettingsCache.Brightness = { K4A_COLOR_CONTROL_MODE_MANUAL, 128 };
    ApplyColorSetting(K4A_COLOR_CONTROL_BRIGHTNESS, &m_colorSettingsCache.Brightness);

    m_colorSettingsCache.Contrast = { K4A_COLOR_CONTROL_MODE_MANUAL, 5 };
    ApplyColorSetting(K4A_COLOR_CONTROL_CONTRAST, &m_colorSettingsCache.Contrast);

    m_colorSettingsCache.Saturation = { K4A_COLOR_CONTROL_MODE_MANUAL, 32 };
    ApplyColorSetting(K4A_COLOR_CONTROL_SATURATION, &m_colorSettingsCache.Saturation);

    m_colorSettingsCache.Sharpness = { K4A_COLOR_CONTROL_MODE_MANUAL, 2 };
    ApplyColorSetting(K4A_COLOR_CONTROL_SHARPNESS, &m_colorSettingsCache.Sharpness);

    m_colorSettingsCache.BacklightCompensation = { K4A_COLOR_CONTROL_MODE_MANUAL, 0 };
    ApplyColorSetting(K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION, &m_colorSettingsCache.BacklightCompensation);

    m_colorSettingsCache.Gain = { K4A_COLOR_CONTROL_MODE_MANUAL, 0 };
    ApplyColorSetting(K4A_COLOR_CONTROL_GAIN, &m_colorSettingsCache.Gain);

    m_colorSettingsCache.PowerlineFrequency = { K4A_COLOR_CONTROL_MODE_MANUAL, 2 };
    ApplyColorSetting(K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, &m_colorSettingsCache.PowerlineFrequency);
}

void K4ADeviceDockControl::ReadColorSetting(k4a_color_control_command_t command, ColorSetting *cacheEntry)
{
    try
    {
        m_device.get_color_control(command, &cacheEntry->Mode, &cacheEntry->Value);
    }
    catch (const k4a::error &e)
    {
        K4AViewerErrorManager::Instance().SetErrorStatus(e.what());
    }
}

void K4ADeviceDockControl::LoadColorSettingsCache()
{
    // If more color controls are added, they need to be initialized here
    //
    static_assert(sizeof(m_colorSettingsCache) == sizeof(ColorSetting) * 9,
                  "Missing color setting in LoadColorSettingsCache()");

    ReadColorSetting(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &m_colorSettingsCache.ExposureTimeUs);
    ReadColorSetting(K4A_COLOR_CONTROL_WHITEBALANCE, &m_colorSettingsCache.WhiteBalance);
    ReadColorSetting(K4A_COLOR_CONTROL_BRIGHTNESS, &m_colorSettingsCache.Brightness);
    ReadColorSetting(K4A_COLOR_CONTROL_CONTRAST, &m_colorSettingsCache.Contrast);
    ReadColorSetting(K4A_COLOR_CONTROL_SATURATION, &m_colorSettingsCache.Saturation);
    ReadColorSetting(K4A_COLOR_CONTROL_SHARPNESS, &m_colorSettingsCache.Sharpness);
    ReadColorSetting(K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION, &m_colorSettingsCache.BacklightCompensation);
    ReadColorSetting(K4A_COLOR_CONTROL_GAIN, &m_colorSettingsCache.Gain);
    ReadColorSetting(K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, &m_colorSettingsCache.PowerlineFrequency);
}

void K4ADeviceDockControl::RefreshSyncCableStatus()
{
    try
    {
        m_syncInConnected = m_device.is_sync_in_connected();
        m_syncOutConnected = m_device.is_sync_out_connected();
    }
    catch (const k4a::error &e)
    {
        K4AViewerErrorManager::Instance().SetErrorStatus(e.what());
    }
}

bool K4ADeviceDockControl::DeviceIsStarted() const
{
    return m_camerasStarted || m_imuStarted || (m_microphone && m_microphone->IsStarted());
}

K4ADeviceDockControl::K4ADeviceDockControl(k4a::device &&device) : m_device(std::move(device))
{
    ApplyDefaultConfiguration();

    m_deviceSerialNumber = m_device.get_serialnum();
    m_windowTitle = m_deviceSerialNumber + ": Configuration";

    m_microphone = K4AAudioManager::Instance().GetMicrophoneForDevice(m_deviceSerialNumber);

    LoadColorSettingsCache();
    RefreshSyncCableStatus();
}

K4ADeviceDockControl::~K4ADeviceDockControl()
{
    Stop();
}

K4ADockControlStatus K4ADeviceDockControl::Show()
{
    std::stringstream labelBuilder;
    labelBuilder << "Device S/N: " << m_deviceSerialNumber;
    ImGui::Text("%s", labelBuilder.str().c_str());
    ImGui::SameLine();
    {
        ImGuiExtensions::ButtonColorChanger cc(ImGuiExtensions::ButtonColor::Red);
        if (ImGui::SmallButton("Close device"))
        {
            return K4ADockControlStatus::ShouldClose;
        }
    }

    ImGui::Separator();

    const bool deviceIsStarted = DeviceIsStarted();

    // Check microphone health
    //
    if (m_microphone && m_microphone->GetStatusCode() != SoundIoErrorNone)
    {
        std::stringstream errorBuilder;
        errorBuilder << "Microphone on device " << m_deviceSerialNumber << " failed!";
        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());
        StopMicrophone();
        m_microphone->ClearStatusCode();
    }

    // Draw controls
    //
    // InputScalars are a bit wider than we want them by default.
    //
    constexpr float InputScalarScaleFactor = 0.5f;

    bool depthEnabledStateChanged = ImGuiExtensions::K4ACheckbox("Enable Depth Camera",
                                                                 &m_config.EnableDepthCamera,
                                                                 !deviceIsStarted);

    if (m_firstRun || depthEnabledStateChanged)
    {
        ImGui::SetNextTreeNodeOpen(m_config.EnableDepthCamera);
    }

    ImGui::Indent();
    bool depthModeUpdated = depthEnabledStateChanged;
    if (ImGui::TreeNode("Depth Configuration"))
    {
        const bool depthSettingsEditable = !deviceIsStarted && m_config.EnableDepthCamera;
        auto *pDepthMode = reinterpret_cast<int *>(&m_config.DepthMode);
        ImGui::Text("Depth mode");
        depthModeUpdated |= ImGuiExtensions::K4ARadioButton("NFOV Binned",
                                                            pDepthMode,
                                                            K4A_DEPTH_MODE_NFOV_2X2BINNED,
                                                            depthSettingsEditable);
        ImGui::SameLine();
        depthModeUpdated |= ImGuiExtensions::K4ARadioButton("NFOV Unbinned  ",
                                                            pDepthMode,
                                                            K4A_DEPTH_MODE_NFOV_UNBINNED,
                                                            depthSettingsEditable);
        // New line
        depthModeUpdated |= ImGuiExtensions::K4ARadioButton("WFOV Binned",
                                                            pDepthMode,
                                                            K4A_DEPTH_MODE_WFOV_2X2BINNED,
                                                            depthSettingsEditable);
        ImGui::SameLine();
        depthModeUpdated |= ImGuiExtensions::K4ARadioButton("WFOV Unbinned  ",
                                                            pDepthMode,
                                                            K4A_DEPTH_MODE_WFOV_UNBINNED,
                                                            depthSettingsEditable);
        // New line
        depthModeUpdated |=
            ImGuiExtensions::K4ARadioButton("Passive IR", pDepthMode, K4A_DEPTH_MODE_PASSIVE_IR, depthSettingsEditable);

        ImGui::TreePop();
    }
    ImGui::Unindent();

    bool colorEnableStateChanged = ImGuiExtensions::K4ACheckbox("Enable Color Camera",
                                                                &m_config.EnableColorCamera,
                                                                !deviceIsStarted);

    if (m_firstRun || colorEnableStateChanged)
    {
        ImGui::SetNextTreeNodeOpen(m_config.EnableColorCamera);
    }

    ImGui::Indent();
    bool colorResolutionUpdated = colorEnableStateChanged;
    if (ImGui::TreeNode("Color Configuration"))
    {
        const bool colorSettingsEditable = !deviceIsStarted && m_config.EnableColorCamera;

        bool colorFormatUpdated = false;
        auto *pColorFormat = reinterpret_cast<int *>(&m_config.ColorFormat);
        ImGui::Text("Format");
        colorFormatUpdated |=
            ImGuiExtensions::K4ARadioButton("BGRA", pColorFormat, K4A_IMAGE_FORMAT_COLOR_BGRA32, colorSettingsEditable);
        ImGui::SameLine();
        colorFormatUpdated |=
            ImGuiExtensions::K4ARadioButton("MJPG", pColorFormat, K4A_IMAGE_FORMAT_COLOR_MJPG, colorSettingsEditable);
        ImGui::SameLine();
        colorFormatUpdated |=
            ImGuiExtensions::K4ARadioButton("NV12", pColorFormat, K4A_IMAGE_FORMAT_COLOR_NV12, colorSettingsEditable);
        ImGui::SameLine();
        colorFormatUpdated |=
            ImGuiExtensions::K4ARadioButton("YUY2", pColorFormat, K4A_IMAGE_FORMAT_COLOR_YUY2, colorSettingsEditable);

        // Uncompressed formats are only supported at 720p.
        //
        const char *imageFormatHelpMessage = "Not supported in NV12 or YUY2 mode!";
        const bool imageFormatSupportsHighResolution = m_config.ColorFormat != K4A_IMAGE_FORMAT_COLOR_NV12 &&
                                                       m_config.ColorFormat != K4A_IMAGE_FORMAT_COLOR_YUY2;
        if (colorFormatUpdated || m_firstRun)
        {
            if (!imageFormatSupportsHighResolution)
            {
                m_config.ColorResolution = K4A_COLOR_RESOLUTION_720P;
            }
        }

        auto *pColorResolution = reinterpret_cast<int *>(&m_config.ColorResolution);

        ImGui::Text("Resolution");
        ImGui::Indent();
        ImGui::Text("16:9");
        ImGui::Indent();
        colorResolutionUpdated |= ImGuiExtensions::K4ARadioButton(" 720p",
                                                                  pColorResolution,
                                                                  K4A_COLOR_RESOLUTION_720P,
                                                                  colorSettingsEditable);
        ImGui::SameLine();
        colorResolutionUpdated |= ImGuiExtensions::K4ARadioButton("1080p",
                                                                  pColorResolution,
                                                                  K4A_COLOR_RESOLUTION_1080P,
                                                                  colorSettingsEditable &&
                                                                      imageFormatSupportsHighResolution);
        ImGuiExtensions::K4AShowTooltip(imageFormatHelpMessage, !imageFormatSupportsHighResolution);
        // New line
        colorResolutionUpdated |= ImGuiExtensions::K4ARadioButton("1440p",
                                                                  pColorResolution,
                                                                  K4A_COLOR_RESOLUTION_1440P,
                                                                  colorSettingsEditable &&
                                                                      imageFormatSupportsHighResolution);
        ImGuiExtensions::K4AShowTooltip(imageFormatHelpMessage, !imageFormatSupportsHighResolution);
        ImGui::SameLine();
        colorResolutionUpdated |= ImGuiExtensions::K4ARadioButton("2160p",
                                                                  pColorResolution,
                                                                  K4A_COLOR_RESOLUTION_2160P,
                                                                  colorSettingsEditable &&
                                                                      imageFormatSupportsHighResolution);
        ImGuiExtensions::K4AShowTooltip(imageFormatHelpMessage, !imageFormatSupportsHighResolution);
        ImGui::Unindent();
        ImGui::Text("4:3");
        ImGui::Indent();

        colorResolutionUpdated |= ImGuiExtensions::K4ARadioButton("1536p",
                                                                  pColorResolution,
                                                                  K4A_COLOR_RESOLUTION_1536P,
                                                                  colorSettingsEditable &&
                                                                      imageFormatSupportsHighResolution);
        ImGuiExtensions::K4AShowTooltip(imageFormatHelpMessage, !imageFormatSupportsHighResolution);

        ImGui::SameLine();
        colorResolutionUpdated |= ImGuiExtensions::K4ARadioButton("3072p",
                                                                  pColorResolution,
                                                                  K4A_COLOR_RESOLUTION_3072P,
                                                                  colorSettingsEditable &&
                                                                      imageFormatSupportsHighResolution);
        ImGuiExtensions::K4AShowTooltip(imageFormatHelpMessage, !imageFormatSupportsHighResolution);

        ImGui::Unindent();
        ImGui::Unindent();
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Color Controls"))
    {
        // Some of the variable names in here are just long enough to cause clang-format to force a line-wrap,
        // which causes half of these (very similar) calls to look radically different than the rest, which
        // makes it harder to pick out similarities/differences at a glance, so turn off clang-format for this
        // bit.
        //
        // clang-format off

        const float SliderScaleFactor = 0.5f;

        ShowColorControl(K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &m_colorSettingsCache.ExposureTimeUs,
            [SliderScaleFactor](ColorSetting *cacheEntry) {
                ColorControlAction result = ColorControlAction::None;

                // Exposure time supported values are factors off 1,000,000 / 2, so we need an exponential control.
                // There isn't one for ints, so we use the float control and make it look like an int control.
                //
                auto valueFloat = static_cast<float>(cacheEntry->Value);
                ImGui::PushItemWidth(ImGui::CalcItemWidth() * SliderScaleFactor);
                if (ImGuiExtensions::K4ASliderFloat("Exposure Time",
                                                    &valueFloat,
                                                    488.f,
                                                    1000000.f,
                                                    "%.0f us",
                                                    8.0f,
                                                    cacheEntry->Mode == K4A_COLOR_CONTROL_MODE_MANUAL))
                {
                    result = ColorControlAction::SetManual;
                    cacheEntry->Value = static_cast<int32_t>(valueFloat);
                }
                ImGui::PopItemWidth();

                ImGui::SameLine();
                ShowColorControlAutoButton(cacheEntry->Mode, &result, "exposure");
                return result;
        });

        ShowColorControl(K4A_COLOR_CONTROL_WHITEBALANCE, &m_colorSettingsCache.WhiteBalance,
            [SliderScaleFactor](ColorSetting *cacheEntry) {
                ColorControlAction result = ColorControlAction::None;
                ImGui::PushItemWidth(ImGui::CalcItemWidth() * SliderScaleFactor);
                if (ImGuiExtensions::K4ASliderInt("White Balance",
                                                  &cacheEntry->Value,
                                                  2500,
                                                  12500,
                                                  "%d K",
                                                  cacheEntry->Mode == K4A_COLOR_CONTROL_MODE_MANUAL))
                {
                    result = ColorControlAction::SetManual;

                    // White balance must be stepped in units of 10 or the call to update the setting fails.
                    //
                    cacheEntry->Value -= cacheEntry->Value % 10;
                }
                ImGui::PopItemWidth();

                ImGui::SameLine();
                ShowColorControlAutoButton(cacheEntry->Mode, &result, "whitebalance");
                return result;
        });

        ImGui::PushItemWidth(ImGui::CalcItemWidth() * SliderScaleFactor);

        ShowColorControl(K4A_COLOR_CONTROL_BRIGHTNESS, &m_colorSettingsCache.Brightness,
            [](ColorSetting *cacheEntry) {
                return ImGui::SliderInt("Brightness", &cacheEntry->Value, 0, 255) ?
                    ColorControlAction::SetManual :
                    ColorControlAction::None;
        });

        ShowColorControl(K4A_COLOR_CONTROL_CONTRAST, &m_colorSettingsCache.Contrast,
            [](ColorSetting *cacheEntry) {
                return ImGui::SliderInt("Contrast", &cacheEntry->Value, 0, 10) ?
                    ColorControlAction::SetManual :
                    ColorControlAction::None;
        });

        ShowColorControl(K4A_COLOR_CONTROL_SATURATION, &m_colorSettingsCache.Saturation,
            [](ColorSetting *cacheEntry) {
                return ImGui::SliderInt("Saturation", &cacheEntry->Value, 0, 63) ?
                    ColorControlAction::SetManual :
                    ColorControlAction::None;
        });

        ShowColorControl(K4A_COLOR_CONTROL_SHARPNESS, &m_colorSettingsCache.Sharpness,
            [](ColorSetting *cacheEntry) {
                return ImGui::SliderInt("Sharpness", &cacheEntry->Value, 0, 4) ?
                    ColorControlAction::SetManual :
                    ColorControlAction::None;
        });

        ShowColorControl(K4A_COLOR_CONTROL_GAIN, &m_colorSettingsCache.Gain,
            [](ColorSetting *cacheEntry) {
                return ImGui::SliderInt("Gain", &cacheEntry->Value, 0, 255) ?
                    ColorControlAction::SetManual :
                    ColorControlAction::None;
        });

        ImGui::PopItemWidth();

        ShowColorControl(K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION, &m_colorSettingsCache.BacklightCompensation,
            [](ColorSetting *cacheEntry) {
                return ImGui::Checkbox("Backlight Compensation", reinterpret_cast<bool *>(&cacheEntry->Value)) ?
                    ColorControlAction::SetManual :
                    ColorControlAction::None;
         });

        ShowColorControl(K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, &m_colorSettingsCache.PowerlineFrequency,
            [](ColorSetting *cacheEntry) {
                ImGui::Text("Power Frequency");
                ImGui::SameLine();
                bool updated = false;
                updated |= ImGui::RadioButton("50Hz", &cacheEntry->Value, 1);
                ImGui::SameLine();
                updated |= ImGui::RadioButton("60Hz", &cacheEntry->Value, 2);
                return updated ? ColorControlAction::SetManual : ColorControlAction::None;
         });

        // clang-format on

        if (ImGui::Button("Refresh"))
        {
            LoadColorSettingsCache();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset to default##RGB"))
        {
            ApplyDefaultColorSettings();
        }

        ImGui::TreePop();
    }
    ImGui::Unindent();

    if (colorResolutionUpdated || m_firstRun)
    {
        if (m_config.ColorResolution == K4A_COLOR_RESOLUTION_3072P)
        {
            // 4K supports up to 15FPS
            //
            m_config.Framerate = K4A_FRAMES_PER_SECOND_15;
        }
    }
    if (depthModeUpdated || m_firstRun)
    {
        if (m_config.DepthMode == K4A_DEPTH_MODE_WFOV_UNBINNED)
        {
            m_config.Framerate = K4A_FRAMES_PER_SECOND_15;
        }
    }

    const bool supports30fps = !(m_config.EnableColorCamera &&
                                 m_config.ColorResolution == K4A_COLOR_RESOLUTION_3072P) &&
                               !(m_config.EnableDepthCamera && m_config.DepthMode == K4A_DEPTH_MODE_WFOV_UNBINNED);

    const bool enableFramerate = !deviceIsStarted && (m_config.EnableColorCamera || m_config.EnableDepthCamera);

    ImGui::Text("Framerate");
    auto *pFramerate = reinterpret_cast<int *>(&m_config.Framerate);
    bool framerateUpdated = false;
    framerateUpdated |= ImGuiExtensions::K4ARadioButton("30 FPS",
                                                        pFramerate,
                                                        K4A_FRAMES_PER_SECOND_30,
                                                        enableFramerate && supports30fps);
    ImGuiExtensions::K4AShowTooltip("Not supported with WFOV Unbinned or 3072p!", !supports30fps);
    ImGui::SameLine();
    framerateUpdated |=
        ImGuiExtensions::K4ARadioButton("15 FPS", pFramerate, K4A_FRAMES_PER_SECOND_15, enableFramerate);
    ImGui::SameLine();
    framerateUpdated |= ImGuiExtensions::K4ARadioButton(" 5 FPS", pFramerate, K4A_FRAMES_PER_SECOND_5, enableFramerate);

    ImGuiExtensions::K4ACheckbox("Disable streaming LED", &m_config.DisableStreamingIndicator, !deviceIsStarted);

    ImGui::Separator();

    const bool imuSupported = m_config.EnableColorCamera || m_config.EnableDepthCamera;
    m_config.EnableImu &= imuSupported;
    ImGuiExtensions::K4ACheckbox("Enable IMU", &m_config.EnableImu, !deviceIsStarted && imuSupported);
    ImGuiExtensions::K4AShowTooltip("Not supported without at least one camera!", !imuSupported);

    const bool synchronizedImagesAvailable = m_config.EnableColorCamera && m_config.EnableDepthCamera;
    m_config.SynchronizedImagesOnly &= synchronizedImagesAvailable;

    if (m_microphone)
    {
        ImGuiExtensions::K4ACheckbox("Enable Microphone", &m_config.EnableMicrophone, !deviceIsStarted);
    }
    else
    {
        m_config.EnableMicrophone = false;
        ImGui::Text("Microphone not detected!");
    }

    ImGui::Separator();

    if (ImGui::TreeNode("Internal Sync"))
    {
        ImGuiExtensions::K4ACheckbox("Synchronized images only",
                                     &m_config.SynchronizedImagesOnly,
                                     !deviceIsStarted && synchronizedImagesAvailable);

        ImGui::PushItemWidth(ImGui::CalcItemWidth() * InputScalarScaleFactor);
        constexpr int stepSize = 1;
        const bool depthDelayUpdated = ImGuiExtensions::K4AInputScalar("Depth delay (us)",
                                                                       ImGuiDataType_S32,
                                                                       &m_config.DepthDelayOffColorUsec,
                                                                       &stepSize,
                                                                       nullptr,
                                                                       "%d",
                                                                       !deviceIsStarted);
        if (framerateUpdated || depthDelayUpdated)
        {
            // InputScalar doesn't do bounds-checks, so we have to do it ourselves whenever
            // the user interacts with the control
            //
            int maxDepthDelay = 0;
            switch (m_config.Framerate)
            {
            case K4A_FRAMES_PER_SECOND_30:
                maxDepthDelay = std::micro::den / 30;
                break;
            case K4A_FRAMES_PER_SECOND_15:
                maxDepthDelay = std::micro::den / 15;
                break;
            case K4A_FRAMES_PER_SECOND_5:
                maxDepthDelay = std::micro::den / 5;
                break;
            default:
                throw std::logic_error("Invalid framerate!");
            }
            m_config.DepthDelayOffColorUsec = std::max(m_config.DepthDelayOffColorUsec, -maxDepthDelay);
            m_config.DepthDelayOffColorUsec = std::min(m_config.DepthDelayOffColorUsec, maxDepthDelay);
        }

        ImGui::PopItemWidth();
        ImGui::TreePop();
    }

    if (m_firstRun && (m_syncInConnected || m_syncOutConnected))
    {
        ImGui::SetNextTreeNodeOpen(true);
    }
    if (ImGui::TreeNode("External Sync"))
    {
        ImGui::Text("Sync cable state");
        ImGuiExtensions::K4ARadioButton("In", m_syncInConnected, false);
        ImGui::SameLine();
        ImGuiExtensions::K4ARadioButton("Out", m_syncOutConnected, false);
        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
        {
            RefreshSyncCableStatus();
        }

        const char *syncModesSupportedTooltip = "Requires at least one camera and a connected sync cable!";
        const bool syncModesSupported = (m_syncInConnected || m_syncOutConnected) &&
                                        (m_config.EnableColorCamera || m_config.EnableDepthCamera);
        if (!syncModesSupported)
        {
            m_config.WiredSyncMode = K4A_WIRED_SYNC_MODE_STANDALONE;
        }

        auto *pSyncMode = reinterpret_cast<int *>(&m_config.WiredSyncMode);
        ImGuiExtensions::K4ARadioButton("Standalone", pSyncMode, K4A_WIRED_SYNC_MODE_STANDALONE, !deviceIsStarted);
        ImGui::SameLine();
        ImGuiExtensions::K4ARadioButton("Master",
                                        pSyncMode,
                                        K4A_WIRED_SYNC_MODE_MASTER,
                                        !deviceIsStarted && syncModesSupported);
        ImGuiExtensions::K4AShowTooltip(syncModesSupportedTooltip, !syncModesSupported);
        ImGui::SameLine();
        ImGuiExtensions::K4ARadioButton("Sub",
                                        pSyncMode,
                                        K4A_WIRED_SYNC_MODE_SUBORDINATE,
                                        !deviceIsStarted && syncModesSupported);
        ImGuiExtensions::K4AShowTooltip(syncModesSupportedTooltip, !syncModesSupported);

        constexpr int stepSize = 1;
        ImGui::PushItemWidth(ImGui::CalcItemWidth() * InputScalarScaleFactor);
        ImGuiExtensions::K4AInputScalar("Delay off master (us)",
                                        ImGuiDataType_U32,
                                        &m_config.SubordinateDelayOffMasterUsec,
                                        &stepSize,
                                        nullptr,
                                        "%d",
                                        !deviceIsStarted);
        ImGui::PopItemWidth();

        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::TreeNode("Device Firmware Version Info"))
    {
        k4a_hardware_version_t versionInfo = m_device.get_version();
        ImGui::Text("RGB camera: %u.%u.%u", versionInfo.rgb.major, versionInfo.rgb.minor, versionInfo.rgb.iteration);
        ImGui::Text("Depth camera: %u.%u.%u",
                    versionInfo.depth.major,
                    versionInfo.depth.minor,
                    versionInfo.depth.iteration);
        ImGui::Text("Audio: %u.%u.%u", versionInfo.audio.major, versionInfo.audio.minor, versionInfo.audio.iteration);

        ImGui::Text("Build Config: %s", versionInfo.firmware_build == K4A_FIRMWARE_BUILD_RELEASE ? "Release" : "Debug");
        ImGui::Text("Signature type: %s",
                    versionInfo.firmware_signature == K4A_FIRMWARE_SIGNATURE_MSFT ?
                        "Microsoft" :
                        versionInfo.firmware_signature == K4A_FIRMWARE_SIGNATURE_TEST ? "Test" : "Unsigned");

        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGuiExtensions::K4AButton("Restore", !deviceIsStarted))
        ApplyDefaultConfiguration();
    ImGui::SameLine();
    if (ImGuiExtensions::K4AButton("Save", !deviceIsStarted))
        SaveDefaultConfiguration();
    ImGui::SameLine();
    if (ImGuiExtensions::K4AButton("Reset", !deviceIsStarted))
        ResetDefaultConfiguration();

    const bool enableCameras = m_config.EnableColorCamera || m_config.EnableDepthCamera;

    const ImVec2 buttonSize{ 275, 0 };
    if (!deviceIsStarted)
    {
        ImGuiExtensions::ButtonColorChanger colorChanger(ImGuiExtensions::ButtonColor::Green);
        const bool validStartMode = enableCameras || m_config.EnableMicrophone || m_config.EnableImu;

        if (m_config.WiredSyncMode == K4A_WIRED_SYNC_MODE_SUBORDINATE)
        {
            ImGuiExtensions::TextColorChanger cc(ImGuiExtensions::TextColor::Warning);
            ImGui::TextUnformatted("You are starting in subordinate mode.");
            ImGui::TextUnformatted("The camera will not start until it");
            ImGui::TextUnformatted("receives a start signal from the");
            ImGui::TextUnformatted("master device");
        }

        if (ImGuiExtensions::K4AButton("Start", buttonSize, validStartMode))
        {
            Start();
        }
    }
    else
    {
        ImGuiExtensions::ButtonColorChanger colorChanger(ImGuiExtensions::ButtonColor::Red);
        if (ImGuiExtensions::K4AButton("Stop", buttonSize))
        {
            Stop();
        }

        ImGui::Separator();

        const bool pointCloudViewerAvailable = m_config.EnableDepthCamera &&
                                               m_config.DepthMode != K4A_DEPTH_MODE_PASSIVE_IR && m_camerasStarted;

        K4AWindowSet::ShowModeSelector(&m_currentViewType,
                                       true,
                                       pointCloudViewerAvailable,
                                       [this](K4AWindowSet::ViewType t) { return this->SetViewType(t); });

        if (m_paused)
        {
            ImGuiExtensions::ButtonColorChanger cc(ImGuiExtensions::ButtonColor::Green);
            if (ImGui::Button("Resume", buttonSize))
            {
                m_paused = false;
            }
        }
        else
        {
            ImGuiExtensions::ButtonColorChanger cc(ImGuiExtensions::ButtonColor::Yellow);
            if (ImGui::Button("Pause", buttonSize))
            {
                m_paused = true;
            }
        }
    }

    m_firstRun = false;
    return K4ADockControlStatus::Ok;
}

void K4ADeviceDockControl::Start()
{
    const bool enableCameras = m_config.EnableColorCamera || m_config.EnableDepthCamera;
    if (enableCameras)
    {
        bool camerasStarted = StartCameras();
        if (camerasStarted && m_config.EnableImu)
        {
            StartImu();
        }
    }
    if (m_config.EnableMicrophone)
    {
        StartMicrophone();
    }

    SetViewType(K4AWindowSet::ViewType::Normal);
    m_paused = false;
}

void K4ADeviceDockControl::Stop()
{
    K4AWindowManager::Instance().ClearWindows();

    StopCameras();
    StopImu();
    StopMicrophone();
}

bool K4ADeviceDockControl::StartCameras()
{
    if (m_camerasStarted)
    {
        return false;
    }

    k4a_device_configuration_t deviceConfig = m_config.ToK4ADeviceConfiguration();

    try
    {
        m_device.start_cameras(&deviceConfig);
    }
    catch (const k4a::error &)
    {
        K4AViewerErrorManager::Instance().SetErrorStatus(
            "Failed to start device!\nIf you unplugged the device, you must close and reopen the device.");
        return false;
    }

    m_camerasStarted = true;

    k4a::device *pDevice = &m_device;
    K4ADataSource<k4a::capture> *pCameraDataSource = &m_cameraDataSource;
    bool *pPaused = &m_paused;
    bool *pCamerasStarted = &m_camerasStarted;
    bool *pAbortInProgress = &m_camerasAbortInProgress;
    bool isSubordinate = m_config.WiredSyncMode == K4A_WIRED_SYNC_MODE_SUBORDINATE;

    m_cameraPollingThread = std14::make_unique<K4APollingThread>(
        [pDevice, pCameraDataSource, pPaused, pCamerasStarted, pAbortInProgress, isSubordinate](bool firstRun) {
            std::chrono::milliseconds pollingTimeout = CameraPollingTimeout;
            if (firstRun && isSubordinate)
            {
                // If we're starting in subordinate mode, we need to give the user time to start the
                // master device, so we wait for longer.
                //
                pollingTimeout = SubordinateModeStartupTimeout;
            }
            return PollSensor<k4a::capture>("Cameras",
                                            pDevice,
                                            pCameraDataSource,
                                            pPaused,
                                            pCamerasStarted,
                                            pAbortInProgress,
                                            [](k4a::device *device,
                                               k4a::capture *capture,
                                               std::chrono::milliseconds timeout) {
                                                return device->get_capture(capture, timeout);
                                            },
                                            [](k4a::device *device) { device->stop_cameras(); },
                                            pollingTimeout);
        });

    return true;
}

void K4ADeviceDockControl::StopCameras()
{
    StopPollingThread(&m_cameraPollingThread,
                      &m_device,
                      [](k4a::device *device) { device->stop_cameras(); },
                      &m_cameraDataSource,
                      &m_camerasStarted,
                      &m_camerasAbortInProgress);
}

bool K4ADeviceDockControl::StartMicrophone()
{
    if (!m_microphone)
    {
        std::stringstream errorBuilder;
        errorBuilder << "Failed to find microphone for device: " << m_deviceSerialNumber << "!";
        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());
        return false;
    }

    if (m_microphone->IsStarted())
    {
        return false;
    }

    const int startResult = m_microphone->Start();
    if (startResult != SoundIoErrorNone)
    {
        std::stringstream errorBuilder;
        errorBuilder << "Failed to start microphone: " << soundio_strerror(startResult) << "!";
        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());
        return false;
    }

    return true;
}

void K4ADeviceDockControl::StopMicrophone()
{
    if (m_microphone)
    {
        m_microphone->Stop();
    }
}

bool K4ADeviceDockControl::StartImu()
{
    if (m_imuStarted)
    {
        return false;
    }

    try
    {
        m_device.start_imu();
    }
    catch (const k4a::error &e)
    {
        K4AViewerErrorManager::Instance().SetErrorStatus(e.what());
        return false;
    }

    m_imuStarted = true;

    k4a::device *pDevice = &m_device;
    K4ADataSource<k4a_imu_sample_t> *pImuDataSource = &m_imuDataSource;
    bool *pPaused = &m_paused;
    bool *pImuStarted = &m_imuStarted;
    bool *pAbortInProgress = &m_imuAbortInProgress;
    bool isSubordinate = m_config.WiredSyncMode == K4A_WIRED_SYNC_MODE_SUBORDINATE;

    m_imuPollingThread = std14::make_unique<K4APollingThread>(
        [pDevice, pImuDataSource, pPaused, pImuStarted, pAbortInProgress, isSubordinate](bool firstRun) {
            std::chrono::milliseconds pollingTimeout = ImuPollingTimeout;
            if (firstRun && isSubordinate)
            {
                // If we're starting in subordinate mode, we need to give the user time to start the
                // master device, so we wait for longer.
                //
                pollingTimeout = SubordinateModeStartupTimeout;
            }
            return PollSensor<k4a_imu_sample_t>("IMU",
                                                pDevice,
                                                pImuDataSource,
                                                pPaused,
                                                pImuStarted,
                                                pAbortInProgress,
                                                [](k4a::device *device,
                                                   k4a_imu_sample_t *sample,
                                                   std::chrono::milliseconds timeout) {
                                                    return device->get_imu_sample(sample, timeout);
                                                },
                                                [](k4a::device *device) { device->stop_imu(); },
                                                pollingTimeout);
        });

    return true;
}

void K4ADeviceDockControl::StopImu()
{
    StopPollingThread(&m_imuPollingThread,
                      &m_device,
                      [](k4a::device *device) { device->stop_imu(); },
                      &m_imuDataSource,
                      &m_imuStarted,
                      &m_imuAbortInProgress);
}

void K4ADeviceDockControl::SetViewType(K4AWindowSet::ViewType viewType)
{
    K4AWindowManager::Instance().ClearWindows();

    std::shared_ptr<K4AMicrophoneListener> micListener = nullptr;
    if (m_config.EnableMicrophone)
    {
        micListener = m_microphone->CreateListener();
        if (micListener == nullptr)
        {
            std::stringstream errorBuilder;
            errorBuilder << "Failed to create microphone listener: " << soundio_strerror(m_microphone->GetStatusCode());
            K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());
        }
    }

    switch (viewType)
    {
    case K4AWindowSet::ViewType::Normal:
        K4AWindowSet::StartNormalWindows(m_deviceSerialNumber.c_str(),
                                         &m_cameraDataSource,
                                         m_config.EnableImu ? &m_imuDataSource : nullptr,
                                         std::move(micListener),
                                         m_config.EnableDepthCamera,
                                         m_config.DepthMode,
                                         m_config.EnableColorCamera,
                                         m_config.ColorFormat,
                                         m_config.ColorResolution);
        break;

    case K4AWindowSet::ViewType::PointCloudViewer:
        try
        {
            k4a::calibration calib = m_device.get_calibration(m_config.DepthMode, m_config.ColorResolution);
            bool rgbPointCloudAvailable = m_config.EnableColorCamera &&
                                          m_config.ColorFormat == K4A_IMAGE_FORMAT_COLOR_BGRA32;
            K4AWindowSet::StartPointCloudWindow(m_deviceSerialNumber.c_str(),
                                                calib,
                                                &m_cameraDataSource,
                                                rgbPointCloudAvailable);
        }
        catch (const k4a::error &e)
        {
            K4AViewerErrorManager::Instance().SetErrorStatus(e.what());
        }

        break;
    }

    m_currentViewType = viewType;
}

void K4ADeviceDockControl::ApplyDefaultConfiguration()
{
    m_config = K4AViewerSettingsManager::Instance().GetSavedDeviceConfiguration();
}

void K4ADeviceDockControl::SaveDefaultConfiguration()
{
    K4AViewerSettingsManager::Instance().SetSavedDeviceConfiguration(m_config);
}

void K4ADeviceDockControl::ResetDefaultConfiguration()
{
    m_config = K4ADeviceConfiguration();
    SaveDefaultConfiguration();
}
