/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4AVIEWERUTIL_H
#define K4AVIEWERUTIL_H

// System headers
//
#include <functional>

// Library headers
//

// Project headers
//
#include "assertionexception.h"

namespace k4aviewer
{

// Generic wrapper to provide one-off automatic cleanup
//
class CleanupGuard
{
public:
    explicit CleanupGuard(std::function<void()> cleanupFunction) : m_cleanupFunction(std::move(cleanupFunction)) {}

    ~CleanupGuard()
    {
        if (m_cleanupFunction)
        {
            m_cleanupFunction();
        }
    }

    void Dismiss()
    {
        m_cleanupFunction = nullptr;
    }

    CleanupGuard(const CleanupGuard &) = delete;
    CleanupGuard(const CleanupGuard &&) = delete;
    CleanupGuard &operator=(const CleanupGuard &) = delete;
    CleanupGuard &operator=(const CleanupGuard &&) = delete;

private:
    std::function<void()> m_cleanupFunction;
};

struct ExpectedValueRange
{
    uint16_t Min;
    uint16_t Max;
};

static inline ExpectedValueRange GetRangeForDepthMode(const k4a_depth_mode_t depthMode)
{
    switch (depthMode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
        return { 500, 5800 };
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        return { 500, 4000 };
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        return { 250, 3000 };
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        return { 250, 2500 };

    case K4A_DEPTH_MODE_PASSIVE_IR:
    default:
        throw AssertionException("Invalid depth mode");
    }
}

} // namespace k4aviewer

#endif
