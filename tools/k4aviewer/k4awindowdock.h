// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AWINDOWDOCK_H
#define K4AWINDOWDOCK_H

// Associated header
//

// System headers
//
#include <memory>
#include <stack>
#include <string>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "ik4adockcontrol.h"

namespace k4aviewer
{

class K4AWindowDock
{
public:
    enum class Edge
    {
        Left,
        Right,
        Top,
        Bottom
    };

    K4AWindowDock(Edge edge);
    void PushDockControl(std::unique_ptr<IK4ADockControl> &&dockControl);
    void Show(ImVec2 regionPosition, ImVec2 regionSize);
    ImVec2 GetSize();

private:
    void SetRegion(ImVec2 position, ImVec2 size);
    void SetSize(ImVec2 size);

    static constexpr ImGuiWindowFlags DockWindowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                                        ImGuiWindowFlags_AlwaysAutoResize |
                                                        ImGuiWindowFlags_NoTitleBar |
                                                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                                                        ImGuiWindowFlags_HorizontalScrollbar;

    std::stack<std::unique_ptr<IK4ADockControl>> m_dockControls;

    Edge m_edge;
    std::string m_windowName;

    // The region into which the dock is allowed to draw
    //
    ImVec2 m_regionPosition = ImVec2(0.f, 0.f);
    ImVec2 m_regionSize = ImVec2(0.f, 0.f);

    // The actual size/location of the dock window, in absolute window coordinates.
    // Must be within by m_region*
    //
    ImVec2 m_size = ImVec2(0.f, 0.f);

    bool m_isResizing = false;
    bool m_userHasResized = false;
};

} // namespace k4aviewer

#endif
