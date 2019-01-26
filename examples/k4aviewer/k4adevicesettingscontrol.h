/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4ADEVICEWINDOW_H
#define K4ADEVICEWINDOW_H

// System headers
//
#include <list>
#include <memory>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//
#include "k4avideowindow.h"
#include "k4adevice.h"
#include "k4amicrophone.h"
#include "k4acolorframevisualizer.h"
#include "k4apointcloudwindow.h"
#include "k4aviewersettingsmanager.h"
#include "k4awindowmanager.h"

namespace k4aviewer
{
class K4ADeviceSettingsControl
{
public:
    explicit K4ADeviceSettingsControl(std::shared_ptr<K4ADevice> device);
    K4ADeviceSettingsControl(K4ADeviceSettingsControl &other) = delete;
    K4ADeviceSettingsControl(K4ADeviceSettingsControl &&other) = delete;
    K4ADeviceSettingsControl operator=(K4ADeviceSettingsControl &other) = delete;
    K4ADeviceSettingsControl operator=(K4ADeviceSettingsControl &&other) = delete;

    ~K4ADeviceSettingsControl();

    void Show();
    void PollDevice();

private:
    struct ColorSetting
    {
        k4a_color_control_mode_t Mode;
        int32_t Value;
    };

    struct
    {
        ColorSetting ExposureTimeUs;
        ColorSetting WhiteBalance;
        ColorSetting AutoExposurePriority;
        ColorSetting Brightness;
        ColorSetting Contrast;
        ColorSetting Saturation;
        ColorSetting Sharpness;
        ColorSetting BacklightCompensation;
        ColorSetting Gain;
        ColorSetting PowerlineFrequency;
    } m_colorSettingsCache;

    template<k4a_image_format_t ImageFormat>
    void CreateVideoWindow(const char *windowTitle, std::unique_ptr<IK4AFrameVisualizer<ImageFormat>> &&frameVisualizer)
    {
        std::string title = m_device->GetSerialNumber() + ": " + windowTitle;

        const auto frameSource = std::make_shared<K4ANonBufferingFrameSource<ImageFormat>>();
        m_device->RegisterCaptureObserver(frameSource);

        std::unique_ptr<IK4AVisualizationWindow> window(
            new K4AVideoWindow<ImageFormat>(std::move(title), std::move(frameVisualizer), frameSource));

        K4AWindowManager::Instance().AddWindow(std::move(window));
    }

    void CheckFirmwareVersion(k4a_version_t actualVersion, k4a_version_t minVersion, const char *type) const;

    // What type of change to the color control, if any, should be taken based on user input to a widget
    //
    enum class ColorControlAction
    {
        None,
        SetAutomatic,
        SetManual
    };

    void ShowColorControl(k4a_color_control_command_t command,
                          ColorSetting &cacheEntry,
                          const std::function<ColorControlAction(ColorSetting &)> &showControl);
    static void ShowColorControlAutoButton(k4a_color_control_mode_t currentMode,
                                           ColorControlAction &actionToUpdate,
                                           const char *id);
    void ApplyColorSetting(k4a_color_control_command_t command, ColorSetting &cacheEntry);
    void ApplyDefaultColorSettings();

    void ReadColorSetting(k4a_color_control_command_t command, ColorSetting &cacheEntry);
    void LoadColorSettingsCache();

    void Start();
    void Stop();
    bool DeviceIsStarted() const;

    bool StartCameras();
    void StopCameras();

    bool StartMicrophone();
    void StopMicrophone();

    bool StartImu();
    void StopImu();

    void CloseDevice();

    void ApplyDefaultConfiguration();
    void SaveDefaultConfiguration();
    void ResetDefaultConfiguration();

    void RefreshSyncCableStatus();

    enum class ViewType
    {
        Normal,
        PointCloudViewer
    };
    ViewType m_currentViewType = ViewType::Normal;

    void SetViewType(ViewType);
    void StartPointCloudViewer();
    void StartNormalView();

    K4ADeviceConfiguration m_pendingDeviceConfiguration;

    std::shared_ptr<K4ADevice> m_device;
    std::shared_ptr<K4AMicrophone> m_microphone;

    bool m_firstRun = true;

    bool m_syncInConnected = false;
    bool m_syncOutConnected = false;

    bool m_paused = false;

    std::string m_windowTitle;
};
} // namespace k4aviewer

#endif
