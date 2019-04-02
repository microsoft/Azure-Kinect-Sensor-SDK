// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AVIEWERERRORMANAGER_H
#define K4AVIEWERERRORMANAGER_H

// System headers
//
#include <mutex>
#include <queue>
#include <string>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//

namespace k4aviewer
{
// Singleton that holds info on the last error detected
//
class K4AViewerErrorManager
{
public:
    static K4AViewerErrorManager &Instance();

    void SetErrorStatus(const char *msg);
    void SetErrorStatus(const std::string &msg);
    void SetErrorStatus(std::string &&msg);

    void SetErrorStatus(const char *msg, k4a_buffer_result_t result);
    void SetErrorStatus(const std::string &msg, k4a_buffer_result_t result);

    void SetErrorStatus(const char *msg, k4a_wait_result_t result);
    void SetErrorStatus(const std::string &msg, k4a_wait_result_t result);

    void PopError();

    bool IsErrorSet() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_errors.empty();
    }

    const std::string &GetErrorMessage() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_errors.front();
    }

private:
    K4AViewerErrorManager() = default;

    std::queue<std::string> m_errors;
    mutable std::mutex m_mutex;
};

} // namespace k4aviewer

#endif
