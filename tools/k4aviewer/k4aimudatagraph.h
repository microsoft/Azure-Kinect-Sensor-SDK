// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AIMUDATAGRAPH_H
#define K4AIMUDATAGRAPH_H

// System headers
//
#include <array>
#include <string>

// Library headers
//
#include <k4a/k4a.h>
#include "k4aimgui_all.h"

// Project headers
//

namespace k4aviewer
{
class K4AImuDataGraph
{
public:
    K4AImuDataGraph(std::string &&title,
                    std::string &&xLabel,
                    std::string &&yLabel,
                    std::string &&zLabel,
                    std::string &&units,
                    float minRange,
                    float maxRange,
                    float defaultRange,
                    float scaleFactor);

    void AddSample(const k4a_float3_t &sample, uint64_t timestampUs);
    void Show(ImVec2 maxSize);

private:
    // Number of samples to show in the graphs
    //
    static constexpr int GraphSampleCount = 150;

    // Number of data samples we average together to compute a graph sample
    //
    static constexpr int DataSamplesPerGraphSample = 20;

    void PlotGraph(const char *name, const std::array<float, GraphSampleCount> &data, ImVec2 graphSize);

    const std::string m_title;
    const std::string m_xLabel;
    const std::string m_yLabel;
    const std::string m_zLabel;
    const std::string m_units;

    const float m_minRange;
    const float m_maxRange;
    float m_currentRange;

    const float m_scaleFactor;

    uint64_t m_lastTimestamp = 0;
    size_t m_offset = 0;
    std::array<float, GraphSampleCount> m_x = {};
    std::array<float, GraphSampleCount> m_y = {};
    std::array<float, GraphSampleCount> m_z = {};

    k4a_float3_t m_nextSampleAccumulator{ { 0.f, 0.f, 0.f } };
    int m_nextSampleAccumulatorCount = 0;

    const std::string m_scaleTitle;
};
} // namespace k4aviewer

#endif
