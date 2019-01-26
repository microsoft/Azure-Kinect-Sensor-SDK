
/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef PERFCOUNTER_H
#define PERFCOUNTER_H

// System headers
//
#include <chrono>
#include <ratio>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include <map>

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
    static std::map<std::string, PerfCounter *> m_perfCounters;
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
    PerfCounter(const char *name)
    {
        PerfCounterManager::RegisterPerfCounter(name, this);
    }

    PerfCounter(const std::string &name) : PerfCounter(name.c_str()) {}

    inline double GetAverage()
    {
        return Total / SampleCount;
    }

    inline void EndSample(std::chrono::high_resolution_clock::time_point startTime)
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        Total += std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        ++SampleCount;
    }

    inline void Reset()
    {
        Total = 0;
        SampleCount = 0;
    }

private:
    double Total = 0;
    double SampleCount = 0;
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