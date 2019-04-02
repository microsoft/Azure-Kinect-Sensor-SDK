// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ASOURCESELECTIONDOCKCONTROL_H
#define K4ASOURCESELECTIONDOCKCONTROL_H

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
#include "ik4adockcontrol.h"
#include "k4adevicedockcontrol.h"
#include "k4afilepicker.h"

namespace k4aviewer
{
class K4ASourceSelectionDockControl : public IK4ADockControl
{
public:
    K4ASourceSelectionDockControl();
    ~K4ASourceSelectionDockControl() override = default;

    K4ADockControlStatus Show() override;

    K4ASourceSelectionDockControl(const K4ASourceSelectionDockControl &) = delete;
    K4ASourceSelectionDockControl(const K4ASourceSelectionDockControl &&) = delete;
    K4ASourceSelectionDockControl operator=(const K4ASourceSelectionDockControl &) = delete;
    K4ASourceSelectionDockControl operator=(const K4ASourceSelectionDockControl &&) = delete;

private:
    void RefreshDevices();

    void OpenDevice();
    void OpenRecording(const std17::filesystem::path &path);

    int m_selectedDevice = -1;
    std::vector<std::pair<int, std::string>> m_connectedDevices;

    K4AFilePicker m_filePicker;
};
} // namespace k4aviewer

#endif
