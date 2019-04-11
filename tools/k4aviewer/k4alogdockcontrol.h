// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ALOGDOCKCONTROL_H
#define K4ALOGDOCKCONTROL_H

// System headers
//
#include <array>
#include <list>
#include <mutex>
#include <string>

// Library headers
//

// Project headers
//
#include "ik4adockcontrol.h"
#include "k4aviewerlogmanager.h"

namespace k4aviewer
{

class K4ALogDockControl : public IK4ADockControl
{
public:
    K4ALogDockControl();
    ~K4ALogDockControl() override = default;

    K4ALogDockControl(K4ALogDockControl &) = delete;
    K4ALogDockControl(K4ALogDockControl &&) = delete;
    K4ALogDockControl operator=(K4ALogDockControl &) = delete;
    K4ALogDockControl operator=(K4ALogDockControl &&) = delete;

    K4ADockControlStatus Show() override;

private:
    struct LogEntry
    {
        LogEntry() = default;
        LogEntry(k4a_log_level_t severity, const char *file, int line, const char *msg) :
            Severity(severity),
            File(file),
            Line(line),
            Msg(msg)
        {
        }

        k4a_log_level_t Severity;
        std::string File;
        int Line;
        std::string Msg;
    };

    struct LogListener : public IK4AViewerLogListener
    {
        void Log(k4a_log_level_t severity, const char *file, int line, const char *msg) override;
        ~LogListener() override = default;

        std::list<LogEntry> m_entries;
        bool m_updated = false;
        std::mutex m_mutex;
        k4a_log_level_t m_minSeverity = K4A_LOG_LEVEL_WARNING;
    };

    std::shared_ptr<LogListener> m_logListener;
    std::array<char, 100> m_filterString = { { '\0' } };
    bool m_showLineInfo = false;
};

} // namespace k4aviewer

#endif
