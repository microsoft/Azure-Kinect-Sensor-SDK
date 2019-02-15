/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4ADEVICEDOCKCONTROL_H
#define K4ADEVICEDOCKCONTROL_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//
#include "ik4adockcontrol.h"
#include "k4adatasource.h"
#include "k4adevice.h"
#include "k4amicrophone.h"
#include "k4apollingthread.h"
#include "k4aviewersettingsmanager.h"
#include "k4awindowset.h"

namespace k4aviewer
{
class K4ADeviceDockControl : public IK4ADockControl
{
public:
    explicit K4ADeviceDockControl(std::shared_ptr<K4ADevice> device);
    K4ADeviceDockControl(K4ADeviceDockControl &other) = delete;
    K4ADeviceDockControl(K4ADeviceDockControl &&other) = delete;
    K4ADeviceDockControl operator=(K4ADeviceDockControl &other) = delete;
    K4ADeviceDockControl operator=(K4ADeviceDockControl &&other) = delete;

    ~K4ADeviceDockControl() override;

    void Show() override;

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

    void ApplyDefaultConfiguration();
    void SaveDefaultConfiguration();
    void ResetDefaultConfiguration();

    void RefreshSyncCableStatus();

    K4AWindowSet::ViewType m_currentViewType = K4AWindowSet::ViewType::Normal;

    void SetViewType(K4AWindowSet::ViewType viewType);

    K4ADeviceConfiguration m_pendingDeviceConfiguration;

    std::shared_ptr<K4ADevice> m_device;
    std::shared_ptr<K4AMicrophone> m_microphone;

    K4ADataSource<std::shared_ptr<K4ACapture>> m_cameraDataSource;
    K4ADataSource<k4a_imu_sample_t> m_imuDataSource;

    bool m_firstRun = true;

    bool m_syncInConnected = false;
    bool m_syncOutConnected = false;

    bool m_paused = false;

    std::string m_windowTitle;

    std::unique_ptr<K4APollingThread<std::shared_ptr<K4ACapture>>> m_cameraPollingThread;
    std::unique_ptr<K4APollingThread<k4a_imu_sample_t>> m_imuPollingThread;
};
} // namespace k4aviewer

#endif
