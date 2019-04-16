// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AVIEWERLOGMANAGER_H
#define K4AVIEWERLOGMANAGER_H

// System headers
//
#include <list>
#include <memory>
#include <mutex>
#include <sstream>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//

namespace k4aviewer
{
class IK4AViewerLogListener
{
public:
    virtual void Log(k4a_log_level_t severity, const char *file, int line, const char *msg) = 0;
    virtual ~IK4AViewerLogListener() = default;
};

class K4AViewerLogManager
{
public:
    static K4AViewerLogManager &Instance();

    void Log(k4a_log_level_t severity, const char *file, int line, const char *msg);

    void RegisterListener(std::shared_ptr<IK4AViewerLogListener> listener);

private:
    K4AViewerLogManager();
    ~K4AViewerLogManager();

    static void LoggerCallback(void *context, k4a_log_level_t level, const char *file, int line, const char *msg);

    std::mutex m_mutex;
    std::list<std::weak_ptr<IK4AViewerLogListener>> m_listeners;
};

} // namespace k4aviewer

#endif
