/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4ADEVICESELECTIONCONTROL_H
#define K4ADEVICESELECTIONCONTROL_H

// System headers
//
#include <memory>
#include <string>
#include <vector>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4adevicesettingscontrol.h"

namespace k4aviewer
{
class K4ADeviceSelectionControl
{
public:
    K4ADeviceSelectionControl();
    ~K4ADeviceSelectionControl() = default;

    void Show();

    K4ADeviceSelectionControl(const K4ADeviceSelectionControl &) = delete;
    K4ADeviceSelectionControl(const K4ADeviceSelectionControl &&) = delete;
    K4ADeviceSelectionControl operator=(const K4ADeviceSelectionControl &) = delete;
    K4ADeviceSelectionControl operator=(const K4ADeviceSelectionControl &&) = delete;

private:
    void RefreshDevices();

    void OpenDevice();

    int m_selectedDevice = -1;
    std::vector<std::pair<int, std::string>> m_connectedDevices;

    std::unique_ptr<K4ADeviceSettingsControl> m_deviceSettingsControl;
};
} // namespace k4aviewer

#endif
