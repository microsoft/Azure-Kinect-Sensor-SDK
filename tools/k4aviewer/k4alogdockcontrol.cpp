// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4alogdockcontrol.h"

// System headers
//
#include <sstream>

// Library headers
//

// Project headers
//
#include "k4aimguiextensions.h"

using namespace k4aviewer;

namespace
{
// Maximum number of log entries to keep in memory
//
constexpr static size_t MaxLines = 10000;

// clang-format off

// Get a string representation of a log level suitable for printing
// in the log box (fields are fixed-width)
//
const char *LogLevelToString(k4a_log_level_t logLevel)
{
    switch(logLevel)
    {
    case K4A_LOG_LEVEL_CRITICAL: return "critical";
    case K4A_LOG_LEVEL_ERROR:    return "error   ";
    case K4A_LOG_LEVEL_WARNING:  return "warning ";
    case K4A_LOG_LEVEL_INFO:     return "info    ";
    case K4A_LOG_LEVEL_TRACE:    return "trace   ";

    default:                     return "[unknown]";
    }
}

const ImVec4 &LogLevelToColor(k4a_log_level_t logLevel)
{
    static const ImVec4 critical = ImVec4(1.f, 0.f, 0.f, 1.f);
    static const ImVec4 error    = ImVec4(1.f, .3f, 0.f, 1.f);
    static const ImVec4 warning  = ImVec4(1.f, 1.f, 0.f, 1.f);
    static const ImVec4 info     = ImVec4(1.f, 1.f, 1.f, 1.f);
    static const ImVec4 trace    = ImVec4(.5f, .5f, .5f, 1.f);

    switch(logLevel)
    {
    case K4A_LOG_LEVEL_CRITICAL: return critical;
    case K4A_LOG_LEVEL_ERROR:    return error;
    case K4A_LOG_LEVEL_WARNING:  return warning;
    case K4A_LOG_LEVEL_INFO:     return info;
    case K4A_LOG_LEVEL_TRACE:    return trace;

    default:                     return warning;
    }
}

// String mappings used for the combo box used to select error levels
//
const std::vector<std::pair<k4a_log_level_t, std::string>> LogLevelLabels = {
    {K4A_LOG_LEVEL_CRITICAL, "Critical"},
    {K4A_LOG_LEVEL_ERROR,    "Error"},
    {K4A_LOG_LEVEL_WARNING,  "Warning"},
    {K4A_LOG_LEVEL_INFO,     "Info"},
    {K4A_LOG_LEVEL_TRACE,    "Trace"}
};

// clang-format on
} // namespace

K4ALogDockControl::K4ALogDockControl() : m_logListener(std::make_shared<LogListener>())
{
    K4AViewerLogManager::Instance().RegisterListener(m_logListener);
}

void K4ALogDockControl::LogListener::Log(k4a_log_level_t severity, const char *file, int line, const char *msg)
{
    if (severity > m_minSeverity)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_entries.emplace_back(severity, file, line, msg);
    if (m_entries.size() > MaxLines)
    {
        m_entries.pop_front();
    }

    m_updated = true;
}

K4ADockControlStatus K4ALogDockControl::Show()
{
    ImGui::BeginGroup();
    if (ImGui::Button("Clear Log"))
    {
        std::lock_guard<std::mutex> lock(m_logListener->m_mutex);
        m_logListener->m_entries.clear();
    }
    ImGui::SameLine();
    const bool copy = ImGui::Button("Copy Log to Clipboard");

    m_logListener->m_updated |= ImGuiExtensions::K4AComboBox("Severity",
                                                             "",
                                                             ImGuiComboFlags_None,
                                                             LogLevelLabels,
                                                             &m_logListener->m_minSeverity);
    m_logListener->m_updated |= ImGui::InputText("Search", &m_filterString[0], m_filterString.size());
    m_logListener->m_updated |= ImGui::Checkbox("Show line info", &m_showLineInfo);
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginChild("LogTextScrollArea", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (copy)
    {
        ImGui::LogToClipboard();
    }

    bool updated = false;
    {
        std::lock_guard<std::mutex> lock(m_logListener->m_mutex);
        for (const LogEntry &entry : m_logListener->m_entries)
        {
            std::stringstream lineBuilder;
            lineBuilder << "[ " << LogLevelToString(entry.Severity) << " ] ";

            if (m_showLineInfo)
            {
                lineBuilder << "( " << entry.File << ":" << entry.Line << " ) ";
            }

            lineBuilder << ": " << entry.Msg.c_str();
            std::string lineStr = lineBuilder.str();
            if (m_filterString[0] != '\0' && lineStr.find(&m_filterString[0]) == std::string::npos)
            {
                continue;
            }
            ImGui::TextColored(LogLevelToColor(entry.Severity), "%s", lineStr.c_str());
        }

        updated = m_logListener->m_updated;
        m_logListener->m_updated = false;
    }

    if (copy)
    {
        ImGui::LogFinish();
    }

    if (updated)
    {
        ImGui::SetScrollHere(1.0f);
    }

    ImGui::EndChild();

    return K4ADockControlStatus::Ok;
}
