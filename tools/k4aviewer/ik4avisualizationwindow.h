// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef IK4AVISUALIZATIONWINDOW_H
#define IK4AVISUALIZATIONWINDOW_H

// System headers
//

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//

namespace k4aviewer
{
struct K4AWindowPlacementInfo
{
    ImVec2 Size;
    ImVec2 Position;
};

class IK4AVisualizationWindow
{
public:
    // Draw widgets to fill your window (ImGui::Begin()/ImGui::End() will be called for you).
    // Size constraints will be automatically applied to your window, but if you want to know
    // how much space you have access to, placementInfo has that.
    //
    virtual void Show(K4AWindowPlacementInfo placementInfo) = 0;

    virtual const char *GetTitle() const = 0;

    IK4AVisualizationWindow() = default;
    virtual ~IK4AVisualizationWindow() = default;

    IK4AVisualizationWindow(const IK4AVisualizationWindow &) = delete;
    IK4AVisualizationWindow &operator=(const IK4AVisualizationWindow &) = delete;
    IK4AVisualizationWindow(const IK4AVisualizationWindow &&) = delete;
    IK4AVisualizationWindow &operator=(const IK4AVisualizationWindow &&) = delete;
};
} // namespace k4aviewer

#endif
