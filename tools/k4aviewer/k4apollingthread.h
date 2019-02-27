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

namespace k4aviewer
{
template<typename T> class K4APollingThread
{
public:
    K4APollingThread(std::function<bool()> &&pollFn) : m_pollFn(std::move(pollFn))
    {
        m_thread = std::thread(&K4APollingThread<T>::Run, this);
    }

    void Stop()
    {
        m_shouldExit = true;
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    ~K4APollingThread()
    {
        Stop();
    }

private:
    static void Run(K4APollingThread *instance)
    {
        while (!instance->m_shouldExit)
        {
            if (!instance->m_pollFn())
            {
                instance->m_shouldExit = true;
            }
        }
    }

    std::thread m_thread;
    bool m_shouldExit = false;

    std::function<bool()> m_pollFn;
};

} // namespace k4aviewer

#endif
