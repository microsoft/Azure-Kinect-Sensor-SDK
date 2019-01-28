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
#include <memory>

// Library headers
//
#include <k4a/k4a.h>

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

// Reimplementations of utility functions from C++14 that were missed from C++11
// (i.e. make_shared).
//
// Put in the std14 namespace because the MSVC compiler doesn't support targeting
// any version of C++ older than C++14, so we actually target C++14 on Windows and
// C++11 on Linux.
//
// Because we have to build on Linux targeting C++11, we can't use std::make_unique,
// but if we just reimplement it, unqualified calls to make_unique on any type that
// takes a type from the std:: namespace as an argument to its constructor triggers
// argument-dependent lookup and searches the std:: namespace for a function called
// make_unique, which results in a conflict on MSVC between std::make_unique and our
// reimplementation here.
//
// Therefore, we need to qualify all calls to these functions with some namespace.
//
// If we move to C++14 (or later), migration will consist of deleting these implementations
// and changing all references to "std14" to "std".
//
namespace std14
{
template<typename T, typename... Args> std::unique_ptr<T> make_unique(Args &&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}; // namespace std14

#endif
