// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aviewersettingsmanager.h"

// System headers
//
#include <cstdio>
#include <sstream>
#include <fstream>

// Library headers
//

// Project headers
//
#include "filesystem17.h"
#include "k4atypeoperators.h"

using namespace k4aviewer;

namespace
{
constexpr char Separator[] = "    ";
} // namespace

namespace k4aviewer
{
constexpr char BeginDeviceConfigurationTag[] = "BeginDeviceConfiguration";
constexpr char EndDeviceConfigurationTag[] = "EndDeviceConfiguration";
constexpr char EnableColorCameraTag[] = "EnableColorCamera";
constexpr char EnableDepthCameraTag[] = "EnableDepthCamera";
constexpr char ColorFormatTag[] = "ColorFormat";
constexpr char ColorResolutionTag[] = "ColorResolution";
constexpr char DepthModeTag[] = "DepthMode";
constexpr char FramerateTag[] = "Framerate";
constexpr char DepthDelayOffColorUsecTag[] = "DepthDelayOffColorUsec";
constexpr char WiredSyncModeTag[] = "WiredSyncMode";
constexpr char SubordinateDelayOffMasterUsecTag[] = "SubordinateDelayOffMasterUsec";
constexpr char DisableStreamingIndicatorTag[] = "DisableStreamingIndicator";
constexpr char SynchronizedImagesOnlyTag[] = "SynchronizedImagesOnly";
constexpr char EnableImuTag[] = "EnableImu";
constexpr char EnableMicrophoneTag[] = "EnableMicrophone";

std::ostream &operator<<(std::ostream &s, const K4ADeviceConfiguration &val)
{
    static_assert(sizeof(k4a_device_configuration_t) == 36, "Need to add a new setting");
    s << BeginDeviceConfigurationTag << std::endl;
    s << Separator << EnableColorCameraTag << Separator << val.EnableColorCamera << std::endl;
    s << Separator << EnableDepthCameraTag << Separator << val.EnableDepthCamera << std::endl;
    s << Separator << ColorFormatTag << Separator << val.ColorFormat << std::endl;
    s << Separator << ColorResolutionTag << Separator << val.ColorResolution << std::endl;
    s << Separator << DepthModeTag << Separator << val.DepthMode << std::endl;
    s << Separator << FramerateTag << Separator << val.Framerate << std::endl;
    s << Separator << DepthDelayOffColorUsecTag << Separator << val.DepthDelayOffColorUsec << std::endl;
    s << Separator << WiredSyncModeTag << Separator << val.WiredSyncMode << std::endl;
    s << Separator << SubordinateDelayOffMasterUsecTag << Separator << val.SubordinateDelayOffMasterUsec << std::endl;
    s << Separator << DisableStreamingIndicatorTag << Separator << val.DisableStreamingIndicator << std::endl;
    s << Separator << SynchronizedImagesOnlyTag << Separator << val.SynchronizedImagesOnly << std::endl;

    s << Separator << EnableImuTag << Separator << val.EnableImu << std::endl;
    s << Separator << EnableMicrophoneTag << Separator << val.EnableMicrophone << std::endl;
    s << EndDeviceConfigurationTag << std::endl;

    return s;
}

std::istream &operator>>(std::istream &s, K4ADeviceConfiguration &val)
{
    std::string variableTag;
    s >> variableTag;
    if (variableTag != BeginDeviceConfigurationTag)
    {
        s.setstate(std::ios::failbit);
        return s;
    }

    while (variableTag != EndDeviceConfigurationTag && s)
    {
        static_assert(sizeof(K4ADeviceConfiguration) == 36, "Need to add a new setting");

        variableTag.clear();
        s >> variableTag;

        if (variableTag == EnableColorCameraTag)
        {
            s >> val.EnableColorCamera;
        }
        else if (variableTag == EnableDepthCameraTag)
        {
            s >> val.EnableDepthCamera;
        }
        else if (variableTag == ColorFormatTag)
        {
            s >> val.ColorFormat;
        }
        else if (variableTag == ColorResolutionTag)
        {
            s >> val.ColorResolution;
        }
        else if (variableTag == DepthModeTag)
        {
            s >> val.DepthMode;
        }
        else if (variableTag == FramerateTag)
        {
            s >> val.Framerate;
        }
        else if (variableTag == DepthDelayOffColorUsecTag)
        {
            s >> val.DepthDelayOffColorUsec;
        }
        else if (variableTag == WiredSyncModeTag)
        {
            s >> val.WiredSyncMode;
        }
        else if (variableTag == SubordinateDelayOffMasterUsecTag)
        {
            s >> val.SubordinateDelayOffMasterUsec;
        }
        else if (variableTag == DisableStreamingIndicatorTag)
        {
            s >> val.DisableStreamingIndicator;
        }
        else if (variableTag == SynchronizedImagesOnlyTag)
        {
            s >> val.SynchronizedImagesOnly;
        }
        else if (variableTag == EnableImuTag)
        {
            s >> val.EnableImu;
        }
        else if (variableTag == EnableMicrophoneTag)
        {
            s >> val.EnableMicrophone;
        }
        else if (variableTag == EndDeviceConfigurationTag)
        {
            break;
        }
        else
        {
            s.setstate(std::ios::failbit);
            break;
        }
    }

    return s;
}

constexpr char BeginViewerOptionsTag[] = "BeginViewerOptions";
constexpr char EndViewerOptionsTag[] = "EndViewerOptions";

std::istream &operator>>(std::istream &s, K4AViewerOptions &val)
{
    std::string variableTag;
    s >> variableTag;
    if (variableTag != BeginViewerOptionsTag)
    {
        s.setstate(std::ios::failbit);
        return s;
    }

    while (variableTag != EndDeviceConfigurationTag && s)
    {
        s >> variableTag;
        if (variableTag == EndViewerOptionsTag)
        {
            break;
        }

        ViewerOption option;
        std::stringstream converter;
        converter << variableTag;
        converter >> option;
        if (!converter)
        {
            s.setstate(std::ios::failbit);
            break;
        }

        bool value;
        s >> value;
        if (!s)
        {
            break;
        }
        val.Options[static_cast<size_t>(option)] = value;
    }

    return s;
}

std::ostream &operator<<(std::ostream &s, const K4AViewerOptions &val)
{
    s << BeginViewerOptionsTag << std::endl;

    for (size_t i = 0; i < val.Options.size(); ++i)
    {
        s << static_cast<ViewerOption>(i) << Separator << val.Options[i] << std::endl;
    }

    s << EndViewerOptionsTag << std::endl;

    return s;
}

constexpr char ShowFrameRateInfoTag[] = "ShowFrameRateInfo";
constexpr char ShowInfoPaneTag[] = "ShowInfoPane";
constexpr char ShowLogDockTag[] = "ShowLogDock";
constexpr char ShowDeveloperOptionsTag[] = "ShowDeveloperOptions";

std::istream &operator>>(std::istream &s, ViewerOption &val)
{
    static_assert(static_cast<size_t>(ViewerOption::MAX) == 4, "Need to add a new viewer option conversion");
    std::string tag;
    s >> tag;

    if (tag == ShowFrameRateInfoTag)
    {
        val = ViewerOption::ShowFrameRateInfo;
    }
    else if (tag == ShowInfoPaneTag)
    {
        val = ViewerOption::ShowInfoPane;
    }
    else if (tag == ShowLogDockTag)
    {
        val = ViewerOption::ShowLogDock;
    }
    else if (tag == ShowDeveloperOptionsTag)
    {
        val = ViewerOption::ShowDeveloperOptions;
    }
    else
    {
        s.setstate(std::ios::failbit);
    }

    return s;
}

std::ostream &operator<<(std::ostream &s, const ViewerOption &val)
{
    static_assert(static_cast<size_t>(ViewerOption::MAX) == 4, "Need to add a new viewer option conversion");

    switch (val)
    {
    case ViewerOption::ShowFrameRateInfo:
        s << ShowFrameRateInfoTag;
        break;

    case ViewerOption::ShowInfoPane:
        s << ShowInfoPaneTag;
        break;

    case ViewerOption::ShowLogDock:
        s << ShowLogDockTag;
        break;

    case ViewerOption::ShowDeveloperOptions:
        s << ShowDeveloperOptionsTag;
        break;

    default:
        s.setstate(std::ios::failbit);
        break;
    }

    return s;
}
} // namespace k4aviewer

// The UI doesn't quite line up with the struct we actually need to give to the K4A API, so
// we have to do a bit of conversion.
//
k4a_device_configuration_t K4ADeviceConfiguration::ToK4ADeviceConfiguration() const
{
    k4a_device_configuration_t deviceConfig;

    deviceConfig.color_format = ColorFormat;
    deviceConfig.color_resolution = EnableColorCamera ? ColorResolution : K4A_COLOR_RESOLUTION_OFF;
    deviceConfig.depth_mode = EnableDepthCamera ? DepthMode : K4A_DEPTH_MODE_OFF;
    deviceConfig.camera_fps = Framerate;

    deviceConfig.depth_delay_off_color_usec = DepthDelayOffColorUsec;
    deviceConfig.wired_sync_mode = WiredSyncMode;
    deviceConfig.subordinate_delay_off_master_usec = SubordinateDelayOffMasterUsec;

    deviceConfig.disable_streaming_indicator = DisableStreamingIndicator;
    deviceConfig.synchronized_images_only = SynchronizedImagesOnly;

    return deviceConfig;
}

K4AViewerOptions::K4AViewerOptions()
{
    static_assert(static_cast<size_t>(ViewerOption::MAX) == 4, "Need to add a new viewer option default");

    Options[static_cast<size_t>(ViewerOption::ShowFrameRateInfo)] = false;
    Options[static_cast<size_t>(ViewerOption::ShowInfoPane)] = true;
    Options[static_cast<size_t>(ViewerOption::ShowLogDock)] = false;
    Options[static_cast<size_t>(ViewerOption::ShowDeveloperOptions)] = false;
}

K4AViewerSettingsManager::K4AViewerSettingsManager()
{
    std17::filesystem::path settingsPath = "";

#ifdef WIN32
    constexpr size_t bufferSize = 250;
    char buffer[bufferSize];
    size_t len;

    // While not technically part of the C++ spec, MSVC provides the
    // C function getenv_s in the C++ standard library headers and
    // complains if you use getenv, so we use getenv_s here.
    //
    errno_t err = getenv_s(&len, buffer, bufferSize, "LOCALAPPDATA");
    if (err == 0)
    {
        settingsPath = buffer;
    }

#else
    // The Linux compilers, on the other hand, don't provide the 'safe'
    // C functions, but they do provide a nonstandard function to do it,
    // so we do that instead.
    //
    const char *home = secure_getenv("HOME");
    if (home)
    {
        settingsPath = home;
    }

#endif

    settingsPath.append(".k4aviewer");
    m_settingsFilePath = settingsPath.string();

    LoadSettings();
}

void K4AViewerSettingsManager::SaveSettings()
{
    std::ofstream fileWriter(m_settingsFilePath.c_str());

    fileWriter << m_settingsPayload.SavedDeviceConfiguration << std::endl;
    fileWriter << m_settingsPayload.Options << std::endl;
}

void K4AViewerSettingsManager::LoadSettings()
{
    std::ifstream fileReader(m_settingsFilePath.c_str());
    if (!fileReader)
    {
        SetDefaults();
        return;
    }

    SettingsPayload newSettingsPayload;

    fileReader >> newSettingsPayload.SavedDeviceConfiguration;
    fileReader >> newSettingsPayload.Options;

    if (fileReader.fail())
    {
        // File is corrupt, try to delete it
        //
        fileReader.close();
        SetDefaults();
    }
    else
    {
        m_settingsPayload = newSettingsPayload;
    }
}

void K4AViewerSettingsManager::SetDefaults()
{
    if (!m_settingsFilePath.empty())
    {
        (void)std::remove(m_settingsFilePath.c_str());
    }

    m_settingsPayload = SettingsPayload();
}