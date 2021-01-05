// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AVIEWERSETTINGSMANAGER_H
#define K4AVIEWERSETTINGSMANAGER_H

// System headers
//
#include <array>
#include <istream>
#include <ostream>

// Library headers
//
#include <k4a/k4a.hpp>

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
    k4a_image_format_t ColorFormat = K4A_IMAGE_FORMAT_COLOR_BGRA32;

    //k4a_color_mode_info_t color_mode_info = { sizeof(k4a_color_mode_info_t), K4A_ABI_VERSION, 1, 1280, 720, K4A_IMAGE_FORMAT_COLOR_MJPG, 90.0f, 59.0f, 5, 30  }; // K4A_COLOR_RESOLUTION_720P
    //k4a_depth_mode_info_t depth_mode_info = { sizeof(k4a_depth_mode_info_t), K4A_ABI_VERSION, 2, false, 640, 576, K4A_IMAGE_FORMAT_DEPTH16, 75.0f, 65.0f, 5, 30, 500, 4000 }; // K4A_DEPTH_MODE_NFOV_UNBINNED
    //k4a_fps_mode_info_t fps_mode_info = { sizeof(k4a_fps_mode_info_t), K4A_ABI_VERSION, 2, 30 }; // K4A_FRAMES_PER_SECOND_30

    uint32_t color_mode_id = 1;
    uint32_t depth_mode_id = 2;
    uint32_t fps_mode_id = 2;

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
    k4a_device_configuration_t ToK4ADeviceConfiguration(k4a::device * device) const;
};

std::istream &operator>>(std::istream &s, K4ADeviceConfiguration &val);
std::ostream &operator<<(std::ostream &s, const K4ADeviceConfiguration &val);

enum class ViewerOption
{
    ShowFrameRateInfo,
    ShowInfoPane,
    ShowLogDock,
    ShowDeveloperOptions,

    // Insert new settings here

    MAX
};

std::istream &operator>>(std::istream &s, ViewerOption &val);
std::ostream &operator<<(std::ostream &s, const ViewerOption &val);

struct K4AViewerOptions
{
    K4AViewerOptions();

    std::array<bool, static_cast<size_t>(ViewerOption::MAX)> Options;
};

std::istream &operator>>(std::istream &s, K4AViewerOptions &val);
std::ostream &operator<<(std::ostream &s, const K4AViewerOptions &val);

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

    void SetDefaults();

    void SetViewerOption(ViewerOption option, bool value)
    {
        if (option == ViewerOption::MAX)
        {
            throw std::logic_error("Invalid viewer option!");
        }

        m_settingsPayload.Options.Options[static_cast<size_t>(option)] = value;
        SaveSettings();
    }
    bool GetViewerOption(ViewerOption option) const
    {
        if (option == ViewerOption::MAX)
        {
            throw std::logic_error("Invalid viewer option!");
        }

        return m_settingsPayload.Options.Options[static_cast<size_t>(option)];
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
        K4AViewerOptions Options;
        K4ADeviceConfiguration SavedDeviceConfiguration;
    };

    std::string m_settingsFilePath;
    SettingsPayload m_settingsPayload;
};

} // namespace k4aviewer

#endif
