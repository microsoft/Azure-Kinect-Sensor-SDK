/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef ASSERTIONEXCEPTION_H
#define ASSERTIONEXCEPTION_H

// System headers
//
#include <exception>
#include <string>

// Library headers
//

// Project headers
//

namespace k4aviewer
{
// Intended to be thrown if an assertion fails.
// Used instead of actual assert to dodge compiler warnings related to execution paths not returning a value if an
// assertion fails.
//
class AssertionException : public std::exception
{
public:
    explicit AssertionException(const char *msg) : m_what(msg) {}

    char const *what() const noexcept override
    {
        return m_what.c_str();
    }

private:
    std::string m_what;
};
} // namespace k4aviewer

#endif
