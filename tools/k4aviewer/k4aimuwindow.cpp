// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aimuwindow.h"

// System headers
//
#include <cmath>
#include <utility>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4aviewererrormanager.h"
#include "k4awindowsizehelpers.h"

using namespace k4aviewer;

namespace
{
constexpr float AccelMinRange = 5.0f;
constexpr float AccelMaxRange = 100.0f;
constexpr float AccelDefaultRange = 20.0f;

constexpr float GyroMinRange = 5.0f;
constexpr float GyroMaxRange = 50.0f;
constexpr float GyroDefaultRange = 20.0f;
} // namespace

K4AImuWindow::K4AImuWindow(std::string &&title, std::shared_ptr<K4AImuGraphDataGenerator> graphDataGenerator) :
    m_graphDataGenerator(std::move(graphDataGenerator)),
    m_title(std::move(title)),
    m_accGraph("Accelerometer", "X", "Y", "Z", "m/s/s", AccelMinRange, AccelMaxRange, AccelDefaultRange),
    m_gyroGraph("Gyroscope", " Roll", "Pitch", "  Yaw", "Rad/s", GyroMinRange, GyroMaxRange, GyroDefaultRange)
{
}

void K4AImuWindow::Show(K4AWindowPlacementInfo placementInfo)
{
    if (!m_failed && m_graphDataGenerator->IsFailed())
    {
        K4AViewerErrorManager::Instance().SetErrorStatus(m_title + ": data source failed!");
        m_failed = true;
    }

    if (m_failed)
    {
        ImGui::Text("Data source failed!");
        return;
    }

    K4AImuGraphDataGenerator::GraphReader reader = m_graphDataGenerator->GetGraphData();

    // Sizing math
    //
    ImGuiStyle &style = ImGui::GetStyle();
    ImVec2 graphSize = placementInfo.Size;
    graphSize.x -= 2 * style.WindowPadding.x;

    graphSize.y -= GetTitleBarHeight();       // Title bar
    graphSize.y -= 2 * style.WindowPadding.y; // Window internal padding

    graphSize.y -= ImGui::GetTextLineHeightWithSpacing(); // Sensor temp label

    constexpr int numSeparators = 2;
    constexpr int itemSpacingPerSeparator = 2;
    graphSize.y -= numSeparators * itemSpacingPerSeparator * style.ItemSpacing.y; // Separators

    constexpr int graphCount = 2;
    graphSize.y /= graphCount;

    // Actually draw the widgets
    //
    m_accGraph.Show(graphSize, reader.Data->AccData, reader.Data->StartOffset, reader.Data->AccTimestamp);

    ImGui::Separator();

    m_gyroGraph.Show(graphSize, reader.Data->GyroData, reader.Data->StartOffset, reader.Data->GyroTimestamp);

    ImGui::Separator();

    if (!std::isnan(reader.Data->LastTemperature))
    {
        ImGui::Text("Sensor temperature: %.2f C", static_cast<double>(reader.Data->LastTemperature));
    }
}

const char *K4AImuWindow::GetTitle() const
{
    return m_title.c_str();
}
