// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
    std::lock_guard<std::mutex> lockGuard(Instance().m_mutex);
    Instance().m_perfCounters[std::string(name)] = perfCounter;
}

void PerfCounterManager::ShowPerfWindow(bool *windowOpen)
{
    std::lock_guard<std::mutex> lockGuard(Instance().m_mutex);

    if (ImGui::Begin("Performance Counters (in ms)", windowOpen, ImGuiWindowFlags_AlwaysAutoResize))
    {
        for (auto &counter : Instance().m_perfCounters)
        {
            ImGui::Text("%s", counter.first.c_str());
            ImGui::Text("avg: %f", double(counter.second->GetAverage()));
            ImGui::Text("max: %f", double(counter.second->GetMax()));

            const PerfCounter::SampleData &data = counter.second->GetSampleData();
            ImGui::PlotLines("",
                             &data[0],
                             static_cast<int>(data.size()),
                             static_cast<int>(counter.second->GetCurrentSampleId()),
                             "");
            ImGui::Separator();
        }

        if (ImGui::Button("Reset perf counters"))
        {
            for (auto &counter : Instance().m_perfCounters)
            {
                counter.second->Reset();
            }
        }
    }
    ImGui::End();
}

PerfCounterManager &PerfCounterManager::Instance()
{
    static PerfCounterManager instance;
    return instance;
}
