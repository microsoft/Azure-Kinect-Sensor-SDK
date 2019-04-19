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
    K4APollingThread(std::function<bool(bool)> &&pollFn) : m_pollFn(std::move(pollFn))
    {
        m_thread = std::thread(&K4APollingThread::Run, this);
    }

    void Stop()
    {
        StopAsync();
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void StopAsync()
    {
        m_shouldExit = true;
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
        bool firstRun = true;
        while (!instance->m_shouldExit)
        {
            if (!instance->m_pollFn(firstRun))
            {
                instance->m_shouldExit = true;
            }
            firstRun = false;
        }
    }

    std::thread m_thread;
    volatile bool m_shouldExit = false;
    volatile bool m_isRunning = false;

    std::function<bool(bool)> m_pollFn;
};

} // namespace k4aviewer

#endif
