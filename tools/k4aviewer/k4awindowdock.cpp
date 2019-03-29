// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4awindowdock.h"

// System headers
//
#include <algorithm>
#include <string>

// Library headers
//
#include "k4aimgui_all.h"
#include "k4awindowsizehelpers.h"

// Project headers
//

using namespace k4aviewer;

namespace
{
constexpr float MaxRegionPercentage = 0.75;
constexpr float ResizeHoverTolerance = 2.f;
constexpr float MinSize = 2.f;
} // namespace

K4AWindowDock::K4AWindowDock(Edge edge)
{
    m_edge = edge;
    static int dockId(0);
    m_windowName = "Dock_";
    m_windowName += std::to_string(dockId++);
}

void K4AWindowDock::PushDockControl(std::unique_ptr<IK4ADockControl> &&dockControl)
{
    m_dockControls.emplace(std::move(dockControl));
    m_userHasResized = false;
}

void K4AWindowDock::SetRegion(ImVec2 position, ImVec2 size)
{
    m_regionPosition = position;
    m_regionSize = size;

    ImVec2 newWindowSize = m_size;
    switch (m_edge)
    {
    case Edge::Left:
    case Edge::Right:
        newWindowSize.y = m_regionSize.y;
        break;

    case Edge::Top:
    case Edge::Bottom:
        newWindowSize.x = m_regionSize.x;
        break;
    }

    SetSize(newWindowSize);
}

void K4AWindowDock::Show(ImVec2 regionPosition, ImVec2 regionSize)
{
    if (m_dockControls.empty())
    {
        // We're not going to show at all, so bypass the sizing
        // checks that are in SetSize() and hard-set to 0.
        //
        m_size = ImVec2(0.f, 0.f);
        return;
    }

    SetRegion(regionPosition, regionSize);
    ImVec2 position(m_regionPosition.x, m_regionPosition.y);
    if (m_edge == Edge::Right)
    {
        position.x += m_regionSize.x - m_size.x;
    }
    else if (m_edge == Edge::Bottom)
    {
        position.y += m_regionSize.y - m_size.y;
    }

    bool mouseOnResizeLine = false;
    if (m_userHasResized)
    {
        ImGui::SetNextWindowSize(m_size);
    }
    else
    {
        ImVec2 minSize(0.f, 0.f);
        ImVec2 maxSize(0.f, 0.f);
        switch (m_edge)
        {
        case Edge::Top:
        case Edge::Bottom:
            minSize.y = MinSize;
            minSize.x = regionSize.x;
            maxSize.y = regionSize.y;
            maxSize.x = regionSize.x;
            break;
        case Edge::Left:
        case Edge::Right:
            minSize.x = MinSize;
            minSize.y = regionSize.y;
            maxSize.x = regionSize.x;
            maxSize.y = regionSize.y;
            break;
        }

        ImGui::SetNextWindowSizeConstraints(minSize, maxSize);
    }
    ImGui::SetNextWindowPos(position);
    if (ImGui::Begin(m_windowName.c_str(), nullptr, DockWindowFlags))
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        {
            const ImVec2 mousePos = ImGui::GetIO().MousePos;
            float mousePosResizeDimension = 0.f;
            float windowEdgeResizeDimension = 0.f;
            switch (m_edge)
            {
            case Edge::Top:
                mousePosResizeDimension = mousePos.y;
                windowEdgeResizeDimension = position.y + m_size.y;
                break;
            case Edge::Bottom:
                mousePosResizeDimension = mousePos.y;
                windowEdgeResizeDimension = position.y;
                break;
            case Edge::Left:
                mousePosResizeDimension = mousePos.x;
                windowEdgeResizeDimension = position.x + m_size.x;
                break;
            case Edge::Right:
                mousePosResizeDimension = mousePos.x;
                windowEdgeResizeDimension = position.x;
                break;
            }

            if (std::abs(mousePosResizeDimension - windowEdgeResizeDimension) <= ResizeHoverTolerance)
            {
                mouseOnResizeLine = true;
            }
        }

        if (m_dockControls.top()->Show() == K4ADockControlStatus::ShouldClose)
        {
            m_dockControls.pop();
            m_userHasResized = false;
        }

        if (!m_userHasResized)
        {
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 fittedWindowSize = ImVec2(windowSize.x + ImGui::GetScrollMaxX(),
                                             windowSize.y + ImGui::GetScrollMaxY());
            SetSize(fittedWindowSize);
        }
        else
        {
            SetSize(ImGui::GetWindowSize());
        }
    }
    ImGui::End();

    // Process resize
    //
    if (mouseOnResizeLine || m_isResizing)
    {
        ImGuiMouseCursor cursorType = ImGuiMouseCursor_ResizeAll;
        switch (m_edge)
        {
        case Edge::Top:
        case Edge::Bottom:
            cursorType = ImGuiMouseCursor_ResizeNS;
            break;
        case Edge::Left:
        case Edge::Right:
            cursorType = ImGuiMouseCursor_ResizeEW;
            break;
        }
        ImGui::SetMouseCursor(cursorType);
    }

    bool mouseDown = ImGui::GetIO().MouseDown[GLFW_MOUSE_BUTTON_1];
    if (!mouseDown)
    {
        m_isResizing = false;
    }
    else if (mouseOnResizeLine && mouseDown)
    {
        m_isResizing = true;
    }

    if (m_isResizing)
    {
        m_userHasResized = true;
        const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        ImVec2 newSize = m_size;
        switch (m_edge)
        {
        case Edge::Top:
            newSize.y += mouseDelta.y;
            break;
        case Edge::Bottom:
            newSize.y -= mouseDelta.y;
            break;
        case Edge::Left:
            newSize.x += mouseDelta.x;
            break;
        case Edge::Right:
            newSize.x -= mouseDelta.x;
            break;
        }

        SetSize(newSize);
    }
}

ImVec2 K4AWindowDock::GetSize()
{
    return m_size;
}

void K4AWindowDock::SetSize(ImVec2 size)
{
    ImVec2 maxRegionScalePercentage(1.f, 1.f);
    switch (m_edge)
    {
    case Edge::Top:
    case Edge::Bottom:
        maxRegionScalePercentage.y = MaxRegionPercentage;
        break;

    case Edge::Left:
    case Edge::Right:
        maxRegionScalePercentage.x = MaxRegionPercentage;
        break;
    }
    m_size = ImVec2(std::max(MinSize, std::min(m_regionSize.x * maxRegionScalePercentage.x, size.x)),
                    std::max(MinSize, std::min(m_regionSize.y * maxRegionScalePercentage.y, size.y)));
}
