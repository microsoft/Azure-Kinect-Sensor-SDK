// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef PERFCOUNTER_H
#define PERFCOUNTER_H

// System headers
//
#include <algorithm>
#include <array>
#include <chrono>
#include <map>
#include <mutex>
#include <numeric>
#include <ratio>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//

namespace k4aviewer
{

// Rudimentary performance counter for tracking down performance problems.
//
// Perf counters must last forever once declared (usually by being declared static).
//
class PerfCounter;
class PerfCounterManager
{
public:
    static void RegisterPerfCounter(const char *name, PerfCounter *perfCounter);
    static void ShowPerfWindow(bool *windowOpen);

private:
    static PerfCounterManager &Instance();
    std::map<std::string, PerfCounter *> m_perfCounters;
    std::mutex m_mutex;
};

// A wrapper for taking a single perf measurement.  Timing starts when the sample is created.
//
struct PerfSample
{
    inline PerfSample(PerfCounter *counter) :
        m_counter(counter),
        m_currentSampleStart(std::chrono::high_resolution_clock::now())
    {
    }

    // End the sample early
    //
    inline void End();

    inline ~PerfSample()
    {
        End();
    }

private:
    PerfCounter *m_counter;
    std::chrono::high_resolution_clock::time_point m_currentSampleStart;
};

class PerfCounter
{
public:
    using SampleData = std::array<float, 100>;

    PerfCounter(const char *name)
    {
        PerfCounterManager::RegisterPerfCounter(name, this);
    }

    PerfCounter(const std::string &name) : PerfCounter(name.c_str()) {}

    inline float GetMax() const
    {
        return m_max;
    }

    inline float GetAverage() const
    {
        return static_cast<float>(std::accumulate(m_samples.begin(), m_samples.end(), 0.0) / m_samples.size());
    }

    inline const SampleData &GetSampleData() const
    {
        return m_samples;
    }

    inline size_t GetCurrentSampleId() const
    {
        return m_currentSample;
    }

    inline void EndSample(std::chrono::high_resolution_clock::time_point startTime)
    {
        const auto endTime = std::chrono::high_resolution_clock::now();
        const long long durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();

        const float durationMs = durationNs * 1.0f * std::milli::den / std::milli::num * std::nano::num /
                                 std::nano::den;
        m_max = std::max(m_max, durationMs);

        m_currentSample = (m_currentSample + 1) % m_samples.size();
        m_samples[m_currentSample] = durationMs;
    }

    inline void Reset()
    {
        m_max = 0;
        std::fill(m_samples.begin(), m_samples.end(), 0.0f);
    }

private:
    float m_max = 0;
    size_t m_currentSample = 0;
    SampleData m_samples;
};

inline void PerfSample::End()
{
    if (m_counter)
    {
        m_counter->EndSample(m_currentSampleStart);
        m_counter = nullptr;
    }
}

} // namespace k4aviewer

#endif
