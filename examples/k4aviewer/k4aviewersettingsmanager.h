/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4AVIEWERSETTINGSMANAGER_H
#define K4AVIEWERSETTINGSMANAGER_H

// System headers
//
#include <istream>
#include <ostream>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//

namespace k4aviewer
{

struct K4ADeviceConfiguration
{
    // Fields that convert to k4a_device_configuration_t
    //
    bool EnableColorCamera = true;
    bool EnableDepthCamera = true;
    k4a_image_format_t ColorFormat = K4A_IMAGE_FORMAT_COLOR_MJPG;
    k4a_color_resolution_t ColorResolution = K4A_COLOR_RESOLUTION_720P;
    k4a_depth_mode_t DepthMode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    k4a_fps_t Framerate = K4A_FRAMES_PER_SECOND_30;

    int32_t DepthDelayOffColorUsec = 0;
    k4a_wired_sync_mode_t WiredSyncMode = K4A_WIRED_SYNC_MODE_STANDALONE;
    uint32_t SubordinateDelayOffMasterUsec = 0;
    bool DisableStreamingIndicator = false;
    bool SynchronizedImagesOnly = true;

    // UI-only fields that do not map to k4a_device_configuration_t
    //
    bool EnableImu = true;
    bool EnableMicrophone = true;

    // Convert to a k4a_device_configuration_t suitable for passing to the K4A API
    //
    k4a_device_configuration_t ToK4ADeviceConfiguration() const;
};

std::istream &operator>>(std::istream &s, K4ADeviceConfiguration &val);
std::ostream &operator<<(std::ostream &s, const K4ADeviceConfiguration &val);

// Singleton that holds viewer settings
//
class K4AViewerSettingsManager
{
public:
    static K4AViewerSettingsManager &Instance()
    {
        static K4AViewerSettingsManager instance;
        return instance;
    }

    void SetShowFrameRateInfo(bool showFrameRateInfo)
    {
        m_settingsPayload.ShowFrameRateInfo = showFrameRateInfo;
        SaveSettings();
    }
    bool GetShowFrameRateInfo() const
    {
        return m_settingsPayload.ShowFrameRateInfo;
    }

    void SetShowInfoPane(bool showInfoPane)
    {
        m_settingsPayload.ShowInfoPane = showInfoPane;
        SaveSettings();
    }
    bool GetShowInfoPane() const
    {
        return m_settingsPayload.ShowInfoPane;
    }

    const K4ADeviceConfiguration &GetSavedDeviceConfiguration() const
    {
        return m_settingsPayload.SavedDeviceConfiguration;
    }
    void SetSavedDeviceConfiguration(const K4ADeviceConfiguration &configuration)
    {
        m_settingsPayload.SavedDeviceConfiguration = configuration;
        SaveSettings();
    }

private:
    K4AViewerSettingsManager();
    void SaveSettings();
    void LoadSettings();

    struct SettingsPayload
    {
        bool ShowFrameRateInfo = false;
        bool ShowInfoPane = true;
        K4ADeviceConfiguration SavedDeviceConfiguration;
    };

    SettingsPayload m_settingsPayload;
};

} // namespace k4aviewer

#endif
