/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "perfcounter.h"

// System headers
//

// Library headers
//

// Project headers
//

using namespace k4aviewer;

void PerfCounterManager::RegisterPerfCounter(const char *name, PerfCounter *perfCounter)
{
    m_perfCounters[std::string(name)] = perfCounter;
}

void PerfCounterManager::ShowPerfWindow(bool *windowOpen)
{
    if (ImGui::Begin("Performance Counters (in ms)", windowOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        for (auto &counter : m_perfCounters)
        {
            ImGui::Text("%s: %f", counter.first.c_str(), counter.second->GetAverage());
        }

        if (ImGui::Button("Reset perf counters"))
        {
            for (auto &counter : m_perfCounters)
            {
                counter.second->Reset();
            }
        }
    }
    ImGui::End();
}

std::map<std::string, PerfCounter *> PerfCounterManager::m_perfCounters;
