// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4awindowmanager.h"

// System headers
//
#include <algorithm>
#include <limits>
#include <string>

// Library headers
//
#include "k4aimgui_all.h"
#include "k4aviewersettingsmanager.h"
#include "k4awindowsizehelpers.h"

// Project headers
//

using namespace k4aviewer;

K4AWindowManager &K4AWindowManager::Instance()
{
    static K4AWindowManager instance;
    return instance;
}

void K4AWindowManager::SetGLWindowSize(ImVec2 glWindowSize)
{
    m_glWindowSize = glWindowSize;
}

void K4AWindowManager::SetMenuBarHeight(float menuBarHeight)
{
    m_menuBarHeight = menuBarHeight;
}

void K4AWindowManager::AddWindow(std::unique_ptr<IK4AVisualizationWindow> &&window)
{
    m_windows.WindowGroup.emplace_back(WindowListEntry(std::move(window)));
}

void K4AWindowManager::AddWindowGroup(std::vector<std::unique_ptr<IK4AVisualizationWindow>> &&windowGroup)
{
    m_windows.WindowGroup.emplace_back(std::move(windowGroup));
}

void K4AWindowManager::ClearFullscreenWindow()
{
    m_maximizedWindow = nullptr;
}

void K4AWindowManager::ClearWindows()
{
    assert(m_windows.IsWindowGroup);
    m_windows.WindowGroup.clear();
    ClearFullscreenWindow();
}

void K4AWindowManager::PushLeftDockControl(std::unique_ptr<IK4ADockControl> &&dockControl)
{
    m_leftDock.PushDockControl(std::move(dockControl));
}

void K4AWindowManager::PushBottomDockControl(std::unique_ptr<IK4ADockControl> &&dockControl)
{
    m_bottomDock.PushDockControl(std::move(dockControl));
}

void K4AWindowManager::ShowAll()
{
    const ImVec2 leftDockRegionPos(0.f, m_menuBarHeight);
    const ImVec2 leftDockRegionSize(m_glWindowSize.x, m_glWindowSize.y - leftDockRegionPos.y);
    m_leftDock.Show(leftDockRegionPos, leftDockRegionSize);

    const ImVec2 windowAreaPosition(m_leftDock.GetSize().x, m_menuBarHeight);
    ImVec2 windowAreaSize(m_glWindowSize.x - windowAreaPosition.x, m_glWindowSize.y - windowAreaPosition.y);

    if (K4AViewerSettingsManager::Instance().GetViewerOption(ViewerOption::ShowLogDock))
    {
        const ImVec2 bottomDockRegionPos(m_leftDock.GetSize().x, m_menuBarHeight);
        const ImVec2 bottomDockRegionSize(m_glWindowSize.x - bottomDockRegionPos.x,
                                          m_glWindowSize.y - bottomDockRegionPos.y);
        m_bottomDock.Show(bottomDockRegionPos, bottomDockRegionSize);

        windowAreaSize.y -= m_bottomDock.GetSize().y;
    }

    if (m_maximizedWindow != nullptr)
    {
        ShowWindow(windowAreaPosition, windowAreaSize, m_maximizedWindow, true);
    }
    else
    {
        ShowWindowArea(windowAreaPosition, windowAreaSize, &m_windows);
    }
}

void K4AWindowManager::ShowWindowArea(ImVec2 windowAreaPosition, ImVec2 windowAreaSize, WindowListEntry *windowList)
{
    if (!windowList->IsWindowGroup)
    {
        ShowWindow(windowAreaPosition, windowAreaSize, windowList->Window.get(), false);
        return;
    }

    ImVec2 individualWindowSize = windowAreaSize;

    int totalRows = 1;
    int totalColumns = 1;

    bool nextDivisionHorizontal = false;

    size_t divisionsRemaining = windowList->WindowGroup.size();
    while (divisionsRemaining > 1)
    {
        if (nextDivisionHorizontal)
        {
            totalRows++;
        }
        else
        {
            totalColumns++;
        }

        divisionsRemaining = (divisionsRemaining / 2) + (divisionsRemaining % 2);
        nextDivisionHorizontal = !nextDivisionHorizontal;
    }

    individualWindowSize.x /= totalColumns;
    individualWindowSize.y /= totalRows;

    int currentRow = 0;
    int currentColumn = 0;
    for (auto &listEntry : windowList->WindowGroup)
    {
        ImVec2 entryPosition = { windowAreaPosition.x + currentColumn * individualWindowSize.x,
                                 windowAreaPosition.y + currentRow * individualWindowSize.y };

        ImVec2 entrySize = individualWindowSize;

        currentColumn = (currentColumn + 1) % totalColumns;
        currentRow = currentRow + (currentColumn == 0 ? 1 : 0);

        if (listEntry.IsWindowGroup)
        {
            ShowWindowArea(entryPosition, entrySize, &listEntry);
        }
        else
        {
            ShowWindow(entryPosition, entrySize, listEntry.Window.get(), false);
        }
    }
}

void K4AWindowManager::ShowWindow(const ImVec2 windowAreaPosition,
                                  const ImVec2 windowAreaSize,
                                  IK4AVisualizationWindow *window,
                                  bool isMaximized)
{
    K4AWindowPlacementInfo placementInfo;
    placementInfo.Position = windowAreaPosition;
    placementInfo.Size = windowAreaSize;
    placementInfo.Size.y -= GetTitleBarHeight();
    placementInfo.Size.x = std::max(1.f, placementInfo.Size.x);
    placementInfo.Size.y = std::max(1.f, placementInfo.Size.y);
    ImGui::SetNextWindowPos(windowAreaPosition);
    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), windowAreaSize);

    static constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                                                    ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));

    if (ImGui::Begin(window->GetTitle(), nullptr, windowFlags))
    {
        window->Show(placementInfo);

        // Draw minimize/maximize button
        //
        if (m_windows.WindowGroup.size() != 1)
        {
            if (ShowMinMaxButton("-", "+", isMaximized))
            {
                if (m_maximizedWindow == nullptr)
                {
                    m_maximizedWindow = window;
                }
                else
                {
                    ClearFullscreenWindow();
                }
            }
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
}

bool K4AWindowManager::ShowMinMaxButton(const char *minimizeLabel, const char *maximizeLabel, bool isMaximized)
{
    bool result = false;

    const char *label = isMaximized ? minimizeLabel : maximizeLabel;

    const char *parentWindowTitle = ImGui::GetCurrentWindow()->Name;
    const ImVec2 currentWindowSize = ImGui::GetWindowSize();
    const ImVec2 currentWindowPosition = ImGui::GetWindowPos();
    // Make the button fit inside the border of the parent window
    //
    const float windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    const float buttonSize = GetTitleBarHeight() - (2 * windowBorderSize);
    ImVec2 minMaxButtonSize(buttonSize, buttonSize);
    minMaxButtonSize.x = std::min(minMaxButtonSize.x, currentWindowSize.x);
    minMaxButtonSize.y = std::min(minMaxButtonSize.y, currentWindowSize.y);

    const ImVec2 minMaxPosition(currentWindowPosition.x + currentWindowSize.x - minMaxButtonSize.x - windowBorderSize,
                                currentWindowPosition.y + windowBorderSize);

    constexpr ImGuiWindowFlags minMaxButtonFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                                   ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    const std::string minMaxButtonTitle = std::string(parentWindowTitle) + "##minmax";

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::SetNextWindowPos(minMaxPosition, ImGuiCond_Always);
    ImGui::SetNextWindowSize(minMaxButtonSize);
    if (ImGui::Begin(minMaxButtonTitle.c_str(), nullptr, minMaxButtonFlags))
    {
        if (ImGui::Button(label, minMaxButtonSize))
        {
            result = true;
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);

    return result;
}
