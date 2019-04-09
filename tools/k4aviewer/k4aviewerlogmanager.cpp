// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aviewerlogmanager.h"

// System headers
//
#include <utility>
#include <vector>

// Library headers
//

// Project headers
//

using namespace k4aviewer;

K4AViewerLogManager &K4AViewerLogManager::Instance()
{
    static K4AViewerLogManager instance;
    return instance;
}

K4AViewerLogManager::K4AViewerLogManager()
{
    if (k4a_set_debug_message_handler(&LoggerCallback, this, K4A_LOG_LEVEL_TRACE) != K4A_RESULT_SUCCEEDED)
    {
        Log(K4A_LOG_LEVEL_ERROR, __FILE__, __LINE__, "Failed to initialize K4A logging!");
    }
}

K4AViewerLogManager::~K4AViewerLogManager()
{
    (void)k4a_set_debug_message_handler(nullptr, nullptr, K4A_LOG_LEVEL_TRACE);
}

void K4AViewerLogManager::Log(k4a_log_level_t severity, const char *file, int line, const char *msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto wpListener = m_listeners.begin(); wpListener != m_listeners.end();)
    {
        std::shared_ptr<IK4AViewerLogListener> spListener = wpListener->lock();
        if (spListener)
        {
            spListener->Log(severity, file, line, msg);
            ++wpListener;
        }
        else
        {
            auto toDelete = wpListener;
            ++wpListener;
            m_listeners.erase(toDelete);
        }
    }
}

void K4AViewerLogManager::RegisterListener(std::shared_ptr<IK4AViewerLogListener> listener)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.emplace_back(std::move(listener));
}

void K4AViewerLogManager::LoggerCallback(void *context,
                                         k4a_log_level_t level,
                                         const char *file,
                                         int line,
                                         const char *msg)
{
    K4AViewerLogManager *instance = reinterpret_cast<K4AViewerLogManager *>(context);
    instance->Log(level, file, line, msg);
}
