// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aaudiochanneldatagraph.h"

// System headers
//
#include <cmath>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//

using namespace k4aviewer;

namespace
{
ImVec2 operator+(const ImVec2 &lhs, const ImVec2 &rhs)
{
    return { lhs.x + rhs.x, lhs.y + rhs.y };
}

ImVec2 operator-(const ImVec2 &lhs, const ImVec2 &rhs)
{
    return { lhs.x - rhs.x, lhs.y - rhs.y };
}
} // namespace

K4AAudioChannelDataGraph::K4AAudioChannelDataGraph(const char *name) : m_name(name) {}

void K4AAudioChannelDataGraph::AddSample(const float sample)
{
    // We're computing the root-mean-square for visualization
    //
    if (sample > 0)
    {
        m_positiveDataAccumulator.AddSample(sample);
    }
    else if (sample < 0)
    {
        m_negativeDataAccumulator.AddSample(sample);
    }
    else
    {
        m_positiveDataAccumulator.AddSample(sample);
        m_negativeDataAccumulator.AddSample(sample);
    }

    const size_t totalSamples = m_positiveDataAccumulator.GetSampleCount() + m_negativeDataAccumulator.GetSampleCount();
    if (totalSamples >= AudioSamplesPerGraphSample)
    {
        // Update graph data
        //
        m_graphData[m_nextGraphPointIndex] = DataPoint(m_positiveDataAccumulator.GetAbsMax(),
                                                       m_positiveDataAccumulator.GetRms(),
                                                       -m_negativeDataAccumulator.GetRms(),
                                                       -m_negativeDataAccumulator.GetAbsMax());

        // Advance graph point
        //
        m_nextGraphPointIndex = (m_nextGraphPointIndex + 1) % AudioChannelGraphSampleCount;

        // Reset accumulators
        //
        m_positiveDataAccumulator.Reset();
        m_negativeDataAccumulator.Reset();
    }
}

void K4AAudioChannelDataGraph::Show(ImVec2 graphSize, const float scale)
{

    const float scaleMin = -scale;
    const float scaleMax = scale;

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return;
    }

    const ImGuiStyle &style = ImGui::GetCurrentContext()->Style;

    if (graphSize.x == 0.0f)
        graphSize.x = ImGui::CalcItemWidth();
    if (graphSize.y == 0.0f)
        graphSize.y = style.FramePadding.y * 2;

    const ImRect frameBoundingBox(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graphSize.x, graphSize.y));
    const ImRect innerBoundingBox(frameBoundingBox.Min + style.FramePadding, frameBoundingBox.Max - style.FramePadding);
    const ImRect totalBoundingBox(frameBoundingBox.Min, frameBoundingBox.Max);
    ImGui::ItemSize(totalBoundingBox, style.FramePadding.y);

    if (!ImGui::ItemAdd(totalBoundingBox, 0, &frameBoundingBox))
    {
        return;
    }

    ImGui::RenderFrame(frameBoundingBox.Min,
                       frameBoundingBox.Max,
                       ImGui::GetColorU32(ImGuiCol_FrameBg),
                       true,
                       style.FrameRounding);

    const auto numValues = static_cast<int>(m_graphData.size());
    if (numValues > 0)
    {
        const int sampleCount = ImMin(static_cast<int>(graphSize.x), numValues);

        const float timeStep = 1.0f / static_cast<float>(sampleCount);
        const float scaleRatio = 1.0f / (scaleMax - scaleMin);

        const auto startOffset = static_cast<int>(m_nextGraphPointIndex);

        const ImU32 colorMinMax = ImGui::GetColorU32({ 0.2f, 0.2f, 0.8f, 1.0f });
        const ImU32 colorRms = ImGui::GetColorU32({ 0.4f, 0.4f, 0.85f, 1.0f });

        float t0 = 0.0f;
        for (int n = 0; n < sampleCount; n++)
        {
            const float t1 = t0 + timeStep;

            auto drawDataPoint = [&](const float min, const float max, const ImU32 color) {
                const auto normalizeValue = [scaleMin, scaleRatio](const float value) {
                    return 1.0f - ImSaturate((value - scaleMin) * scaleRatio);
                };

                // Points in relative coordinates of graph area
                //
                const ImVec2 relativeUpperLeft = ImVec2(t0, normalizeValue(max));
                const ImVec2 relativeLowerRight = ImVec2(t1, normalizeValue(min));

                // Points in absolute/window coordinates
                //
                const ImVec2 absoluteUpperLeft = ImLerp(innerBoundingBox.Min, innerBoundingBox.Max, relativeUpperLeft);
                ImVec2 absoluteLowerRight = ImLerp(innerBoundingBox.Min, innerBoundingBox.Max, relativeLowerRight);

                // Make sure floating point error in normalization doesn't cause us
                // to make an extra-wide bar
                //
                if (absoluteLowerRight.x >= absoluteUpperLeft.x + 2.0f)
                    absoluteLowerRight.x -= 1.0f;

                window->DrawList->AddRectFilled(absoluteUpperLeft, absoluteLowerRight, color);
            };

            const auto offset = static_cast<size_t>((std::lround(t0 * numValues + 0.5f) + startOffset + 1) % numValues);
            DataPoint &currentDataPoint = m_graphData[offset];
            drawDataPoint(currentDataPoint.Min, currentDataPoint.Max, colorMinMax);

            // RMS graph will always be smaller, so draw it on top of the min/max graph
            //
            drawDataPoint(currentDataPoint.NegativeRms, currentDataPoint.PositiveRms, colorRms);

            t0 = t1;
        }
    }

    // Show channel label overlay
    //
    ImGui::RenderTextClipped(ImVec2(frameBoundingBox.Min.x, frameBoundingBox.Min.y + style.FramePadding.y),
                             frameBoundingBox.Max,
                             m_name.c_str(),
                             nullptr,
                             nullptr,
                             ImVec2(0.5f, 0.0f));
}

void K4AAudioChannelDataGraph::SignedAudioDataAccumulator::AddSample(const float sample)
{
    m_sampleCount++;
    m_rmsAccumulator += sample * sample;
    m_absMax = std::max(std::fabs(sample), m_absMax);
}

void K4AAudioChannelDataGraph::SignedAudioDataAccumulator::Reset()
{
    m_sampleCount = 0;
    m_rmsAccumulator = 0;
    m_absMax = 0;
}

float K4AAudioChannelDataGraph::SignedAudioDataAccumulator::GetAbsMax() const
{
    return m_absMax;
}

float K4AAudioChannelDataGraph::SignedAudioDataAccumulator::GetRms() const
{
    if (m_sampleCount == 0)
    {
        return 0;
    }

    return std::sqrt(m_rmsAccumulator / m_sampleCount);
}

size_t K4AAudioChannelDataGraph::SignedAudioDataAccumulator::GetSampleCount() const
{
    return m_sampleCount;
}
