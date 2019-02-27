// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AFRAMERATETRACKER_H
#define K4AFRAMERATETRACKER_H

// System headers
//
#include <array>
#include <chrono>
#include <memory>
#include <mutex>

// Library headers
//

// Project headers
//

namespace k4aviewer
{

class K4AFramerateTracker
{
public:
    inline double GetFramerate() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_framerate;
    }

    inline void NotifyFrame()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        const auto newSampleTime = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> frameDurationSeconds = newSampleTime - m_lastSampleTime;

        m_framerateSampleAccumulator += frameDurationSeconds.count();
        m_framerateSampleAccumulator -= m_framerateSamples[m_framerateSampleCurrentIndex];
        m_framerateSamples[m_framerateSampleCurrentIndex] = frameDurationSeconds.count();
        m_framerateSampleCurrentIndex = (m_framerateSampleCurrentIndex + 1) % m_framerateSamples.size();
        const double secondsPerFrame = m_framerateSampleAccumulator / m_framerateSamples.size();

        if (secondsPerFrame <= 0)
        {
            m_framerate = std::numeric_limits<double>::max();
        }
        else
        {
            m_framerate = 1.0 / secondsPerFrame;
        }

        m_lastSampleTime = newSampleTime;
    }

private:
    static constexpr int FramerateSampleCount = 30;
    std::array<double, FramerateSampleCount> m_framerateSamples{};
    size_t m_framerateSampleCurrentIndex = 0;
    double m_framerateSampleAccumulator = 0;
    double m_framerate = 0;
    std::chrono::high_resolution_clock::time_point m_lastSampleTime;

    mutable std::mutex m_mutex;
};

} // namespace k4aviewer

#endif
