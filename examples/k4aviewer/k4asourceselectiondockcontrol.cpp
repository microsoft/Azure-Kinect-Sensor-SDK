/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4asourceselectiondockcontrol.h"

// System headers
//
#include <sstream>

// Library headers
//
#include "k4aimgui_all.h"
#include <k4a/k4a.h>

// Project headers
//
#include "filesystem17.h"
#include "k4aaudiomanager.h"
#include "k4aimguiextensions.h"
#include "k4arecording.h"
#include "k4aviewererrormanager.h"
#include "k4arecordingdockcontrol.h"
#include "k4aviewerutil.h"
#include "k4awindowmanager.h"

using namespace k4aviewer;

K4ASourceSelectionDockControl::K4ASourceSelectionDockControl()
{
    RefreshDevices();
}

void K4ASourceSelectionDockControl::Show()
{
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Open Device"))
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
        {
            ImGuiExtensions::ButtonColorChanger colorChanger(ImGuiExtensions::ButtonColor::Green, openAvailable);
            if (ImGuiExtensions::K4AButton("Open Device", openAvailable))
            {
                OpenDevice();
            }
        }

        ImGui::TreePop();
    }
    else
    {
        ImGuiExtensions::ButtonColorChanger cc(ImGuiExtensions::ButtonColor::Red);
        if (ImGui::SmallButton("Close device"))
        {
            OpenRecording(m_filePicker.GetPath());
        }

        ImGui::TreePop();
    }
}

void K4ASourceSelectionDockControl::RefreshDevices()
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
        m_selectedDevice = m_connectedDevices[0].first;
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

void K4ASourceSelectionDockControl::OpenDevice()
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

    K4AWindowManager::Instance().PushDockControl(std14::make_unique<K4ADeviceDockControl>(std::move(device)));
}

void K4ASourceSelectionDockControl::OpenRecording(const std17::filesystem::path &path)
{
    std::unique_ptr<K4ARecording> recording = K4ARecording::Open(path.c_str());
    if (!recording)
    {
        K4AViewerErrorManager::Instance().SetErrorStatus("Failed to open recording!");
        return;
    }

    K4AWindowManager::Instance().PushDockControl(std14::make_unique<K4ARecordingDockControl>(std::move(recording)));
}
