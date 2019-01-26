/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "k4aviewererrormanager.h"

// System headers
//
#include <sstream>

// Library headers
//

// Project headers
//
#include "k4atypeoperators.h"

using namespace k4aviewer;

K4AViewerErrorManager &K4AViewerErrorManager::Instance()
{
    static K4AViewerErrorManager instance;
    return instance;
}

void K4AViewerErrorManager::SetErrorStatus(const char *msg)
{
    m_errors.emplace(msg);
}

void K4AViewerErrorManager::SetErrorStatus(const std::string &msg)
{
    SetErrorStatus(msg.c_str());
}

void K4AViewerErrorManager::SetErrorStatus(std::string &&msg)
{
    m_errors.emplace(std::move(msg));
}

void K4AViewerErrorManager::SetErrorStatus(const char *msg, const k4a_buffer_result_t result)
{
    std::stringstream errorBuilder;
    errorBuilder << msg << ": " << result << "!";
    SetErrorStatus(errorBuilder.str());
}

void K4AViewerErrorManager::SetErrorStatus(const std::string &msg, k4a_buffer_result_t result)
{
    SetErrorStatus(msg.c_str(), result);
}

void K4AViewerErrorManager::SetErrorStatus(const char *msg, const k4a_wait_result_t result)
{
    std::stringstream errorBuilder;
    errorBuilder << msg << ": " << result << "!";
    SetErrorStatus(errorBuilder.str());
}

void K4AViewerErrorManager::SetErrorStatus(const std::string &msg, k4a_wait_result_t result)
{
    SetErrorStatus(msg.c_str(), result);
}

void K4AViewerErrorManager::PopError()
{
    m_errors.pop();
}
