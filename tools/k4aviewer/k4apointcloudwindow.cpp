// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4apointcloudwindow.h"

// System headers
//
#include <sstream>
#include <utility>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4aimguiextensions.h"
#include "k4aviewererrormanager.h"
#include "k4aviewerlogmanager.h"
#include "k4awindowsizehelpers.h"

using namespace k4aviewer;

namespace
{
constexpr int DefaultPointSize = 2;
}

void K4APointCloudWindow::Show(K4AWindowPlacementInfo placementInfo)
{
    if (m_failed)
    {
        ImGui::Text("Data source failed!");
        return;
    }

    if (m_captureSource->IsFailed())
    {
        K4AViewerErrorManager::Instance().SetErrorStatus(m_title + ": image source failed!");
        m_failed = true;
        return;
    }

    if (m_captureSource->HasData())
    {
        const PointCloudVisualizationResult visualizationResult =
            m_pointCloudVisualizer.UpdateTexture(&m_texture, m_captureSource->GetLastCapture());
        if (!CheckVisualizationResult(visualizationResult))
        {
            // Fatal error.
            //
            return;
        }

        m_consecutiveMissingImages = 0;
    }

    ImVec2 availableSize = placementInfo.Size;
    availableSize.y -= GetDefaultButtonHeight(); // Mode radio buttons
    availableSize.y -= GetDefaultButtonHeight(); // Reset button

    const ImVec2 sourceImageSize = ImVec2(static_cast<float>(m_texture->GetDimensions().Width),
                                          static_cast<float>(m_texture->GetDimensions().Height));
    const ImVec2 textureSize = GetMaxImageSize(sourceImageSize, availableSize);

    const ImVec2 imageStartPos = ImGui::GetCursorScreenPos();
    ImGui::Image(static_cast<ImTextureID>(*m_texture), textureSize);

    int *ipColorizationStrategy = reinterpret_cast<int *>(&m_colorizationStrategy);

    bool colorizationStrategyUpdated = false;
    colorizationStrategyUpdated |=
        ImGuiExtensions::K4ARadioButton("Simple",
                                        ipColorizationStrategy,
                                        static_cast<int>(K4APointCloudVisualizer::ColorizationStrategy::Simple));
    ImGui::SameLine();
    colorizationStrategyUpdated |=
        ImGuiExtensions::K4ARadioButton("Shaded",
                                        ipColorizationStrategy,
                                        static_cast<int>(K4APointCloudVisualizer::ColorizationStrategy::Shaded));
    ImGui::SameLine();
    colorizationStrategyUpdated |=
        ImGuiExtensions::K4ARadioButton("Color",
                                        ipColorizationStrategy,
                                        static_cast<int>(K4APointCloudVisualizer::ColorizationStrategy::Color),
                                        m_enableColorPointCloud);
    if (!m_enableColorPointCloud)
    {
        ImGuiExtensions::K4AShowTooltip("Color mode must be BGRA!");
    }

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::TextDisabled("[Show Controls]");
    const char *controlsHelpMessage = "Rotate: [Left Mouse] + Drag\n"
                                      "Pan: [Right Mouse] + Drag\n"
                                      "Zoom: Mouse wheel";
    ImGuiExtensions::K4AShowTooltip(controlsHelpMessage);

    if (colorizationStrategyUpdated)
    {
        PointCloudVisualizationResult result = m_pointCloudVisualizer.SetColorizationStrategy(m_colorizationStrategy);
        if (!CheckVisualizationResult(result))
        {
            return;
        }
    }

    if (ImGui::SliderInt("", &m_pointSize, 1, 10, "Point Size: %d px"))
    {
        m_pointCloudVisualizer.SetPointSize(m_pointSize);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset view"))
    {
        m_pointCloudVisualizer.ResetPosition();
        m_pointSize = DefaultPointSize;
        m_pointCloudVisualizer.SetPointSize(m_pointSize);
    }

    ProcessInput(imageStartPos, textureSize);
}

const char *K4APointCloudWindow::GetTitle() const
{
    return m_title.c_str();
}

K4APointCloudWindow::K4APointCloudWindow(std::string &&windowTitle,
                                         bool enableColorPointCloud,
                                         std::shared_ptr<K4ANonBufferingCaptureSource> &&captureSource,
                                         const k4a::calibration &calibrationData) :
    m_title(std::move(windowTitle)),
    m_pointCloudVisualizer(enableColorPointCloud, calibrationData),
    m_captureSource(std::move(captureSource)),
    m_pointSize(DefaultPointSize),
    m_enableColorPointCloud(enableColorPointCloud)
{
    GLenum initResult = m_pointCloudVisualizer.InitializeTexture(&m_texture);
    if (initResult != GL_NO_ERROR)
    {
        CheckVisualizationResult(PointCloudVisualizationResult::OpenGlError);
    }

    m_pointCloudVisualizer.SetPointSize(m_pointSize);
    CheckVisualizationResult(m_pointCloudVisualizer.SetColorizationStrategy(m_colorizationStrategy));
}

void K4APointCloudWindow::ProcessInput(ImVec2 imageStartPos, ImVec2 displayDimensions)
{
    ImGuiIO &io = ImGui::GetIO();

    if (ImGui::IsWindowHovered())
    {
        m_pointCloudVisualizer.ProcessMouseScroll(io.MouseWheel);
    }

    MouseMovementType movementType = MouseMovementType::None;

    bool mouseDown = false;
    ImVec2 mouseDownPos(-1.f, -1.f);
    if (io.MouseDown[GLFW_MOUSE_BUTTON_1])
    {
        mouseDownPos = io.MouseClickedPos[GLFW_MOUSE_BUTTON_1];
        movementType = MouseMovementType::Rotation;
        mouseDown = true;
    }
    else if (io.MouseDown[GLFW_MOUSE_BUTTON_2])
    {
        mouseDownPos = io.MouseClickedPos[GLFW_MOUSE_BUTTON_2];
        movementType = MouseMovementType::Translation;
        mouseDown = true;
    }

    if (mouseDown)
    {
        // Normalize to the image start coordinates
        //
        mouseDownPos.x -= imageStartPos.x;
        mouseDownPos.y -= imageStartPos.y;

        const linmath::vec2 mousePos{ io.MousePos.x - imageStartPos.x, io.MousePos.y - imageStartPos.y };

        // Only count drags if they originated on the image
        //
        if (mouseDownPos.x >= 0.f && mouseDownPos.x <= displayDimensions.x && mouseDownPos.y >= 0.f &&
            mouseDownPos.y <= displayDimensions.y)
        {
            const linmath::vec2 dimensions{ displayDimensions.x, displayDimensions.y };
            const linmath::vec2 mouseDelta{ io.MouseDelta.x, io.MouseDelta.y };
            m_pointCloudVisualizer.ProcessMouseMovement(dimensions, mousePos, mouseDelta, movementType);
        }
    }
}

bool K4APointCloudWindow::CheckVisualizationResult(PointCloudVisualizationResult visualizationResult)
{
    switch (visualizationResult)
    {
    case PointCloudVisualizationResult::Success:
        return true;
    case PointCloudVisualizationResult::MissingDepthImage:
        ++m_consecutiveMissingImages;
        K4AViewerLogManager::Instance()
            .Log(K4A_LOG_LEVEL_WARNING,
                 __FILE__,
                 __LINE__,
                 "Dropped a capture due to a missing depth image - set \"Synchronized Images Only\" to avoid this");
        break;
    case PointCloudVisualizationResult::MissingColorImage:
        K4AViewerLogManager::Instance()
            .Log(K4A_LOG_LEVEL_WARNING,
                 __FILE__,
                 __LINE__,
                 "Dropped a capture due to a missing color image - set \"Synchronized Images Only\" to avoid this");
        ++m_consecutiveMissingImages;
        break;
    case PointCloudVisualizationResult::OpenGlError:
        SetFailed("OpenGL error!");
        return false;
    case PointCloudVisualizationResult::DepthToXyzTransformationFailed:
        SetFailed("Depth -> XYZ transformation failed!");
        return false;
    case PointCloudVisualizationResult::DepthToColorTransformationFailed:
        SetFailed("Depth -> Color transformation failed!");
        return false;
    }

    if (m_consecutiveMissingImages >= MaxConsecutiveMissingImages)
    {
        if (visualizationResult == PointCloudVisualizationResult::MissingDepthImage)
        {
            SetFailed("Stopped receiving depth data!");
            return false;
        }
        if (visualizationResult == PointCloudVisualizationResult::MissingColorImage)
        {
            SetFailed("Stopped receiving color data!");
            return false;
        }
    }

    return true;
}

void K4APointCloudWindow::SetFailed(const char *msg)
{
    std::stringstream errorBuilder;
    errorBuilder << m_title << ": " << msg;
    K4AViewerErrorManager::Instance().SetErrorStatus(errorBuilder.str());
    m_failed = true;
}
