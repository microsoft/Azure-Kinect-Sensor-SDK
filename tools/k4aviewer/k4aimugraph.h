// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AIMUGRAPH_H
#define K4AIMUGRAPH_H

// System headers
//
#include <array>
#include <string>

// Library headers
//
#include <k4a/k4a.hpp>
#include "k4aimgui_all.h"

// Project headers
//
#include "k4aimugraphdatagenerator.h"

namespace k4aviewer
{

class K4AImuGraph
{
public:
    K4AImuGraph(std::string &&title,
                std::string &&xLabel,
                std::string &&yLabel,
                std::string &&zLabel,
                std::string &&units,
                float minRange,
                float maxRange,
                float defaultRange);

    void
    Show(ImVec2 maxSize, const K4AImuGraphData::AccumulatorArray &graphData, int graphFrontIdx, uint64_t timestamp);

private:
    void PlotGraph(const char *name,
                   ImVec2 graphSize,
                   const K4AImuGraphData::AccumulatorArray &graphData,
                   int graphFrontIdx,
                   int offset);

    const std::string m_title;
    const std::string m_xLabel;
    const std::string m_yLabel;
    const std::string m_zLabel;
    const std::string m_units;

    const float m_minRange;
    const float m_maxRange;
    float m_currentRange;

    const std::string m_scaleTitle;
};
} // namespace k4aviewer

#endif
