// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef FILESYSTEM17_H
#define FILESYSTEM17_H

// System headers
//
#include <memory>
#include <string>

// Library headers
//

// Project headers
//

namespace std17
{
// We're building with C++11, so we don't have access to std::filesystem from C++17, but we need
// some of the functionality it provides to do cross-platform filesystem operations for recording
// playback (e.g. file picker widgets).
//
// This is a (very minimal) partial implementation of the parts of std::filesystem that we need
// to make the viewer work.  For the parts that are implemented, the signatures should match
// std::filesystem's signatures, so switching to std::filesystem in future should just require
// #including <filesystem> instead of "filesystem17.h" and swapping std17:: for std::.
//
namespace filesystem
{
class path
{
public:
    path() = default;

    path(const char *p);

    path &append(const char *p);

    path &concat(const char *p);

    const char *c_str() const;

    std::string string() const;

    static bool exists(const path &path);
    static bool is_directory(const path &path);

    path filename() const;
    path extension() const;

    path parent_path() const;

private:
    std::string m_path;
};

class directory_entry
{
public:
    const filesystem::path &path() const
    {
        return m_path;
    }

    bool exists() const;

    bool is_directory() const;

private:
    friend class directory_iterator;
    filesystem::path m_path;
};

struct directory_iterator_impl;
class directory_iterator
{
public:
    directory_iterator();
    directory_iterator(const path &p);

    static directory_iterator end(const directory_iterator &);

    directory_iterator &operator++();

    bool operator!=(const directory_iterator &other);

    const directory_entry &operator*() const;
    const directory_entry *operator->() const;

private:
    void update_entry_path();

    // Should be unique_ptr, but we want to not put the definition for
    // directory_iterator_impl in the header because it's the piece of
    // the directory_iterator that holds platform-specific info, and
    // unique_ptr needs to know the size of the thing at compile time
    // for generating the autodeleter, so using shared_ptr
    //
    std::shared_ptr<directory_iterator_impl> m_impl;
    directory_entry m_current;
    path m_directory;
};

path current_path();

} // namespace filesystem
} // namespace std17
#endif // FILESYSTEM17_H
