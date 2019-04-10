// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ANONBUFFERINGCAPTURESOURCE_H
#define K4ANONBUFFERINGCAPTURESOURCE_H

// System headers
//
#include <array>
#include <memory>
#include <mutex>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//
#include "ik4aobserver.h"
#include "k4aimageextractor.h"

namespace k4aviewer
{
class K4ANonBufferingCaptureSource : public IK4ACaptureObserver
{
public:
    inline k4a::capture GetLastCapture()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_lastCapture;
    }

    bool IsFailed() const
    {
        return m_failed;
    }

    bool HasData() const
    {
        return m_lastCapture != nullptr;
    }

    void NotifyData(const k4a::capture &capture) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (capture)
        {
            m_lastCapture = capture;
        }
    }

    void ClearData() override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastCapture.reset();
    }

    void NotifyTermination() override
    {
        m_failed = true;
    }

    ~K4ANonBufferingCaptureSource() override = default;

    K4ANonBufferingCaptureSource() = default;
    K4ANonBufferingCaptureSource(K4ANonBufferingCaptureSource &) = delete;
    K4ANonBufferingCaptureSource(K4ANonBufferingCaptureSource &&) = delete;
    K4ANonBufferingCaptureSource &operator=(K4ANonBufferingCaptureSource &) = delete;
    K4ANonBufferingCaptureSource &operator=(K4ANonBufferingCaptureSource &&) = delete;

private:
    k4a::capture m_lastCapture;

    bool m_failed = false;

    std::mutex m_mutex;
};

} // namespace k4aviewer

#endif
