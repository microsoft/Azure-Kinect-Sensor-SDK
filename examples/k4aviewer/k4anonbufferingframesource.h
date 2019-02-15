/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4ANONBUFFERINGFRAMESOURCE_H
#define K4ANONBUFFERINGFRAMESOURCE_H

// System headers
//
#include <array>
#include <chrono>
#include <memory>
#include <mutex>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//
#include "assertionexception.h"
#include "ik4aobserver.h"
#include "k4aimageextractor.h"

namespace k4aviewer
{

template<k4a_image_format_t ImageFormat> class K4ANonBufferingFrameSourceImpl : public IK4ACaptureObserver
{
public:
    inline std::shared_ptr<K4AImage<ImageFormat>> GetLastFrame()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return GetLastFrameImpl();
    }

    double GetFrameRate() const
    {
        return m_frameRate;
    }

    bool IsFailed() const
    {
        return m_failed;
    }

    bool HasData() const
    {
        return m_lastImage != nullptr;
    }

    void NotifyData(const std::shared_ptr<K4ACapture> &data) override
    {
        NotifyDataImpl(data);
    }

    void NotifyTermination() override
    {
        m_failed = true;
    }

    ~K4ANonBufferingFrameSourceImpl() override = default;

    K4ANonBufferingFrameSourceImpl() = default;
    K4ANonBufferingFrameSourceImpl(K4ANonBufferingFrameSourceImpl &) = delete;
    K4ANonBufferingFrameSourceImpl(K4ANonBufferingFrameSourceImpl &&) = delete;
    K4ANonBufferingFrameSourceImpl operator=(K4ANonBufferingFrameSourceImpl &) = delete;
    K4ANonBufferingFrameSourceImpl operator=(K4ANonBufferingFrameSourceImpl &&) = delete;

protected:
    void NotifyDataImpl(const std::shared_ptr<K4ACapture> &data)
    {
        // If the capture we're being notified of doesn't contain data
        // from the mode we're listening for, we don't want to update
        // our data.  If GetTimestamp() == 0, it means the capture doesn't
        // have data for the mode we gave it.
        //
        std::shared_ptr<K4AImage<ImageFormat>> image = K4AImageExtractor::GetImageFromCapture<ImageFormat>(data);
        if (image != nullptr)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastImage = std::move(image);
            UpdateFrameRate();
        }
    }

private:
    void UpdateFrameRate()
    {
        const auto newSampleTime = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> frameDurationSeconds = newSampleTime - m_lastSampleTime;

        m_frameRateSampleAccumulator += frameDurationSeconds.count();
        m_frameRateSampleAccumulator -= m_frameRateSamples[m_frameRateSampleCurrentIndex];
        m_frameRateSamples[m_frameRateSampleCurrentIndex] = frameDurationSeconds.count();
        m_frameRateSampleCurrentIndex = (m_frameRateSampleCurrentIndex + 1) % m_frameRateSamples.size();
        const double secondsPerFrame = m_frameRateSampleAccumulator / m_frameRateSamples.size();

        if (secondsPerFrame <= 0)
        {
            m_frameRate = std::numeric_limits<double>::max();
        }
        else
        {
            m_frameRate = 1.0 / secondsPerFrame;
        }

        m_lastSampleTime = newSampleTime;
    }

    std::shared_ptr<K4AImage<ImageFormat>> GetLastFrameImpl() const
    {
        return m_lastImage;
    }

    std::shared_ptr<K4AImage<ImageFormat>> m_lastImage;

    bool m_failed = false;

    // Framerate bookkeeping
    //
    static constexpr int FrameRateSampleCount = 30;
    std::array<double, FrameRateSampleCount> m_frameRateSamples{};
    size_t m_frameRateSampleCurrentIndex = 0;
    double m_frameRateSampleAccumulator = 0;
    double m_frameRate = 0;
    std::chrono::high_resolution_clock::time_point m_lastSampleTime;

    std::mutex m_mutex;
};

template<k4a_image_format_t ImageFormat>
class K4ANonBufferingFrameSource : public K4ANonBufferingFrameSourceImpl<ImageFormat>
{
};

// On depth captures, we also want to track the temperature
//
template<>
class K4ANonBufferingFrameSource<K4A_IMAGE_FORMAT_DEPTH16>
    : public K4ANonBufferingFrameSourceImpl<K4A_IMAGE_FORMAT_DEPTH16>
{
public:
    float GetLastSensorTemperature()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_lastSensorTemperature;
    }

    void NotifyData(const std::shared_ptr<K4ACapture> &data) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastSensorTemperature = data->GetTemperature();
        NotifyDataImpl(data);
    }

private:
    float m_lastSensorTemperature = 0.0f;

    std::mutex m_mutex;
};

} // namespace k4aviewer

#endif
