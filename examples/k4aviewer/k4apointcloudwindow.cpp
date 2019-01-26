/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4apointcloudwindow.h"

// System headers
//
#include <utility>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4aviewererrormanager.h"
#include "k4awindowsizehelpers.h"

using namespace k4aviewer;

void K4APointCloudWindow::Show(K4AWindowPlacementInfo placementInfo)
{
    if (m_failed)
    {
        ImGui::Text("Frame source failed!");
        return;
    }

    if (m_depthFrameSource->IsFailed())
    {
        K4AViewerErrorManager::Instance().SetErrorStatus(m_title + ": frame source failed!");
        m_failed = true;
        return;
    }

    if (m_depthFrameSource->HasData())
    {
        m_pointCloudVisualizer.UpdateTexture(m_texture, *(m_depthFrameSource->GetLastFrame()));
    }

    ImVec2 availableSize = placementInfo.Size;
    availableSize.y -= 3 * ImGui::GetTextLineHeightWithSpacing(); // Instructions text
    availableSize.y -= GetDefaultButtonHeight();                  // Reset button

    const ImVec2 sourceImageSize = ImVec2(static_cast<float>(m_texture->GetDimensions().Width),
                                          static_cast<float>(m_texture->GetDimensions().Height));
    const ImVec2 textureSize = GetImageSize(sourceImageSize, availableSize);

    ImGui::Image(static_cast<ImTextureID>(*m_texture), textureSize);

    ImGui::Text("Movement: W/S/A/D/[Ctrl]/[Space]");
    ImGui::Text("Look: [Right Mouse] + Drag");
    ImGui::Text("Zoom: Mouse wheel");

    if (ImGui::Button("Reset position"))
    {
        m_pointCloudVisualizer.ResetPosition();
    }

    ProcessInput();
}

const char *K4APointCloudWindow::GetTitle() const
{
    return m_title.c_str();
}

K4APointCloudWindow::K4APointCloudWindow(
    std::string &&windowTitle,
    const k4a_depth_mode_t depthMode,
    std::shared_ptr<K4ANonBufferingFrameSource<K4A_IMAGE_FORMAT_DEPTH16>> &&depthFrameSource,
    std::unique_ptr<K4ACalibrationTransformData> &&calibrationData) :
    m_title(std::move(windowTitle)),
    m_pointCloudVisualizer(depthMode, std::move(calibrationData)),
    m_depthFrameSource(std::move(depthFrameSource))
{
    m_pointCloudVisualizer.InitializeTexture(m_texture);
    m_lastTime = glfwGetTime();
}

void K4APointCloudWindow::ProcessInput()
{
    const double currentTime = glfwGetTime();
    const auto timeDelta = static_cast<float>(currentTime - m_lastTime);
    m_lastTime = currentTime;

    // TODO consider redoing controls to use the mouse to rotate around a point rather than 'video game controls'
    //
    if (ImGui::IsWindowFocused())
    {
        ImGuiIO &io = ImGui::GetIO();
        if (io.KeysDown[GLFW_KEY_W])
        {
            m_pointCloudVisualizer.ProcessPositionalMovement(ViewMovement::Forward, timeDelta);
        }
        if (io.KeysDown[GLFW_KEY_A])
        {
            m_pointCloudVisualizer.ProcessPositionalMovement(ViewMovement::Left, timeDelta);
        }
        if (io.KeysDown[GLFW_KEY_D])
        {
            m_pointCloudVisualizer.ProcessPositionalMovement(ViewMovement::Right, timeDelta);
        }
        if (io.KeysDown[GLFW_KEY_S])
        {
            m_pointCloudVisualizer.ProcessPositionalMovement(ViewMovement::Backward, timeDelta);
        }
        if (io.KeysDown[GLFW_KEY_SPACE])
        {
            m_pointCloudVisualizer.ProcessPositionalMovement(ViewMovement::Down, timeDelta);
        }
        if (io.KeysDown[GLFW_KEY_LEFT_CONTROL])
        {
            m_pointCloudVisualizer.ProcessPositionalMovement(ViewMovement::Up, timeDelta);
        }

        if (io.MouseDown[GLFW_MOUSE_BUTTON_2]) // right-click
        {
            m_pointCloudVisualizer.ProcessMouseMovement(io.MouseDelta.x, io.MouseDelta.y);
        }

        m_pointCloudVisualizer.ProcessMouseScroll(io.MouseWheel);
    }
}
