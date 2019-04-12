// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4APOLLINGTHREAD_H
#define K4APOLLINGTHREAD_H

// System headers
//
#include <functional>
#include <thread>

// Library headers
//

// Project headers
//
#include "k4aviewerutil.h"

namespace k4aviewer
{
class K4APollingThread
{
public:
    K4APollingThread(std::function<bool()> &&pollFn) : m_pollFn(std::move(pollFn))
    {
        m_thread = std::thread(&K4APollingThread::Run, this);
    }

    void Stop()
    {
        m_shouldExit = true;
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    bool IsRunning() const
    {
        return m_isRunning;
    }

    ~K4APollingThread()
    {
        Stop();
    }

private:
    static void Run(K4APollingThread *instance)
    {
        instance->m_isRunning = true;
        CleanupGuard runGuard([instance]() { instance->m_isRunning = false; });

        while (!instance->m_shouldExit)
        {
            if (!instance->m_pollFn())
            {
                instance->m_shouldExit = true;
            }
        }
    }

    std::thread m_thread;
    volatile bool m_shouldExit = false;
    volatile bool m_isRunning = false;

    std::function<bool()> m_pollFn;
};

} // namespace k4aviewer

#endif
