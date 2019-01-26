/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4aimudatagraph.h"

// System headers
//
#include <iomanip>
#include <sstream>
#include <utility>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4aimguiextensions.h"
#include "k4aviewererrormanager.h"
#include "k4aviewersettingsmanager.h"
#include "k4awindowsizehelpers.h"

using namespace k4aviewer;

namespace
{
std::string GetScaleTitle(const std::string &title)
{
    // We need to have different widget names for each instance or we'll
    // get two copies of the same widget that modify the same variable.
    //
    // ImGui hides everything in the title past the ##, so this isn't user-visible,
    // but it lets us disambiguate the slider widgets.
    //
    return std::string("##") + title;
}

} // namespace

K4AImuDataGraph::K4AImuDataGraph(std::string &&title,
                                 std::string &&xLabel,
                                 std::string &&yLabel,
                                 std::string &&zLabel,
                                 std::string &&units,
                                 const float minRange,
                                 const float maxRange,
                                 const float defaultRange,
                                 const float scaleFactor) :
    m_title(std::move(title)),
    m_xLabel(std::move(xLabel)),
    m_yLabel(std::move(yLabel)),
    m_zLabel(std::move(zLabel)),
    m_units(std::move(units)),
    m_minRange(minRange),
    m_maxRange(maxRange),
    m_currentRange(-defaultRange),
    m_scaleFactor(scaleFactor),
    m_scaleTitle(GetScaleTitle(m_title))

{
}

void K4AImuDataGraph::AddSample(const k4a_float3_t &sample, const uint64_t timestampUs)
{
    m_offset = (m_offset + 1) % m_x.size();

    m_x[m_offset] = sample.xyz.x * m_scaleFactor;
    m_y[m_offset] = sample.xyz.y * m_scaleFactor;
    m_z[m_offset] = sample.xyz.z * m_scaleFactor;

    m_lastTimestamp = timestampUs;
}

void K4AImuDataGraph::Show(ImVec2 maxSize)
{
    // One line for the graph type (accelerometer/gyro), one for the timestamp
    //
    const float textHeight = 2 * ImGui::GetTextLineHeightWithSpacing();

    ImGuiStyle &style = ImGui::GetStyle();
    ImVec2 sliderSize;

    sliderSize.x = GetStandardVerticalSliderWidth();
    sliderSize.y = maxSize.y - textHeight;

    ImVec2 graphSize;
    graphSize.x = maxSize.x;
    graphSize.x -= sliderSize.x;
    graphSize.x -= 2 * style.ItemSpacing.x;

    constexpr int graphCount = 3;
    graphSize.y = sliderSize.y;
    graphSize.y -= (graphCount - 1) * style.ItemSpacing.y;

    graphSize.y /= graphCount;

    ImGui::BeginGroup();
    ImGui::Text("%s", m_title.c_str());
    ImGui::Text("Time (us): %llu", static_cast<long long unsigned int>(m_lastTimestamp));

    // We use negative min/max ranges to reverse the direction of the slider, which makes it
    // grow when you drag up, which is a bit more intuitive.
    //
    ImGuiExtensions::K4AVSliderFloat(m_scaleTitle.c_str(),
                                     sliderSize,
                                     &m_currentRange,
                                     -m_maxRange,
                                     -m_minRange,
                                     "Scale");
    ImGui::SameLine();

    ImGui::BeginGroup();
    PlotGraph(m_xLabel.c_str(), m_x, graphSize);
    PlotGraph(m_yLabel.c_str(), m_y, graphSize);
    PlotGraph(m_zLabel.c_str(), m_z, graphSize);
    ImGui::EndGroup();
    ImGui::EndGroup();
}

void K4AImuDataGraph::PlotGraph(const char *name, const std::array<float, GraphSampleCount> &data, ImVec2 graphSize)
{
    std::stringstream nameBuilder;
    nameBuilder << "##" << name;

    const float currentData = data[m_offset];

    std::string label;
    if (K4AViewerSettingsManager::Instance().GetShowInfoPane())
    {
        std::stringstream labelBuilder;
        labelBuilder << name << ": ";

        // We want to keep the same width for the number so it doesn't
        // jump around and the decimal point aligns across graphs
        //
        // Add padding if there's no negative sign
        //
        if (currentData >= 0)
        {
            labelBuilder << " ";
        }

        // Pad assuming a max of 3 digits over the decimal point
        //
        if (std::log10(std::abs(currentData)) < 1)
        {
            labelBuilder << " ";
        }
        if (std::log10(std::abs(currentData)) < 2)
        {
            labelBuilder << " ";
        }

        labelBuilder << std::fixed << std::setfill('0') << std::setw(5) << currentData << " " << m_units;
        label = labelBuilder.str();
    }

    ImGui::PlotLines(nameBuilder.str().c_str(),
                     &data[0],
                     static_cast<int>(data.size()),
                     static_cast<int>(m_offset),
                     label.c_str(),
                     m_currentRange,
                     -m_currentRange,
                     graphSize);
}
