/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

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
#include "k4atypeoperators.h"

using namespace k4aviewer;

namespace
{
constexpr char SettingsFileName[] = "k4aviewersettings.txt";
constexpr char Separator[] = "    ";

constexpr char ShowFramerateTag[] = "ShowFramerate";
constexpr char ShowInfoPaneTag[] = "ShowInfoPane";

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

K4AViewerSettingsManager::K4AViewerSettingsManager()
{
    LoadSettings();
}

void K4AViewerSettingsManager::SaveSettings()
{
    std::ofstream fileWriter(SettingsFileName);

    fileWriter << m_settingsPayload.SavedDeviceConfiguration << std::endl;
    fileWriter << ShowFramerateTag << Separator << m_settingsPayload.ShowFrameRateInfo << std::endl;
    fileWriter << ShowInfoPaneTag << Separator << m_settingsPayload.ShowInfoPane << std::endl;
}

void K4AViewerSettingsManager::LoadSettings()
{
    std::ifstream fileReader(SettingsFileName);
    if (!fileReader)
    {
        return;
    }

    bool readSettingsFailed = false;
    SettingsPayload newSettingsPayload;

    fileReader >> newSettingsPayload.SavedDeviceConfiguration;
    bool readFailed = fileReader.fail();
    while (!readFailed && !fileReader.eof())
    {
        std::string variableTag;
        fileReader >> variableTag;

        if (variableTag == ShowFramerateTag)
        {
            fileReader >> newSettingsPayload.ShowFrameRateInfo;
        }
        if (variableTag == ShowInfoPaneTag)
        {
            fileReader >> newSettingsPayload.ShowInfoPane;
        }
        else if (variableTag.empty())
        {
            continue;
        }
        else
        {
            // Mark the file as corrupt
            //
            readFailed = true;
            break;
        }
    }

    if (readFailed)
    {
        // File is corrupt, try to delete it
        //
        readSettingsFailed = true;
        fileReader.close();
        (void)std::remove(SettingsFileName);
    }

    if (!readSettingsFailed)
    {
        m_settingsPayload = newSettingsPayload;
    }
}
