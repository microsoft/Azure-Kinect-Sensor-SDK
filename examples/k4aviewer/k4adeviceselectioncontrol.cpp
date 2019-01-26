/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4adeviceselectioncontrol.h"

// System headers
//
#include <sstream>

// Library headers
//
#include "k4aimgui_all.h"
#include <k4a/k4a.h>

// Project headers
//
#include "k4aaudiomanager.h"
#include "k4aimguiextensions.h"
#include "k4aviewererrormanager.h"

using namespace k4aviewer;

K4ADeviceSelectionControl::K4ADeviceSelectionControl()
{
    RefreshDevices();
}

void K4ADeviceSelectionControl::Show()
{
    if (!m_deviceSettingsControl)
    {
        ImGuiExtensions::K4AComboBox("Device S/N",
                                     "(No available devices)",
                                     ImGuiComboFlags_None,
                                     m_connectedDevices,
                                     &m_selectedDevice);

        if (ImGui::Button("Refresh Devices"))
        {
            RefreshDevices();
        }

        ImGui::SameLine();

        const bool openAvailable = !m_connectedDevices.empty();
        ImGuiExtensions::ButtonColorChanger colorChanger(ImGuiExtensions::ButtonColor::Green, openAvailable);
        if (ImGuiExtensions::K4AButton("Open Device", openAvailable))
        {
            OpenDevice();
        }
    }
    else
    {
        std::stringstream labelBuilder;
        labelBuilder << "Device S/N: " << m_connectedDevices[size_t(m_selectedDevice)].second;
        ImGui::Text("%s", labelBuilder.str().c_str());
        ImGui::SameLine();

        ImGuiExtensions::ButtonColorChanger cc(ImGuiExtensions::ButtonColor::Red);
        if (ImGui::SmallButton("Close device"))
        {
            m_deviceSettingsControl.reset();
        }
    }

    if (m_deviceSettingsControl)
    {
        ImGui::Separator();
        m_deviceSettingsControl->Show();
    }
}

void K4ADeviceSelectionControl::RefreshDevices()
{
    m_selectedDevice = -1;

    const uint32_t installedDevices = k4a_device_get_installed_count();

    m_connectedDevices.clear();

    for (uint8_t i = 0; i < installedDevices; i++)
    {
        std::shared_ptr<K4ADevice> device;
        const k4a_result_t openResult = K4ADeviceFactory::OpenDevice(device, i);

        // We can't have 2 handles to the same device, and we need to open a device handle to check
        // its serial number, so we expect devices we already have open to fail here.  Ignore those.
        //
        if (openResult != K4A_RESULT_SUCCEEDED)
        {
            continue;
        }

        m_connectedDevices.emplace_back(std::make_pair(i, device->GetSerialNumber()));
    }

    if (!m_connectedDevices.empty())
    {
        m_selectedDevice = 0;
    }

    const int audioRefreshStatus = K4AAudioManager::Instance().RefreshDevices();
    if (audioRefreshStatus != SoundIoErrorNone)
    {
        std::stringstream errorBuilder;
        errorBuilder << "Failed to refresh audio devices: " << soundio_strerror(audioRefreshStatus) << "!" << std::endl
                     << "Attempting to open microphones may fail!";

        K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());
    }
}

void K4ADeviceSelectionControl::OpenDevice()
{
    if (m_selectedDevice < 0)
    {
        K4AViewerErrorManager::Instance().SetErrorStatus("No device selected!");
        return;
    }

    std::shared_ptr<K4ADevice> device;
    const k4a_result_t result = K4ADeviceFactory::OpenDevice(device, static_cast<uint8_t>(m_selectedDevice));

    if (result != K4A_RESULT_SUCCEEDED)
    {
        K4AViewerErrorManager::Instance().SetErrorStatus("Failed to open device!");
        return;
    }

    m_deviceSettingsControl.reset(new K4ADeviceSettingsControl(std::move(device)));
}
