// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "filesystem17.h"

// System headers
//
#include <cstring>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// Library headers
//

// Project headers
//

using namespace std17::filesystem;

struct std17::filesystem::directory_iterator_impl
{
    directory_iterator_impl(const path &p)
    {
        m_end = false;
        m_dir = opendir(p.c_str());
        if (!m_dir)
        {
            m_end = true;
            return;
        }

        advance();
    }

    directory_iterator_impl &advance()
    {
        m_dirent = readdir(m_dir);

        // readdir includes the '.' and '..' special directories, but std::filesystem
        // omits them, so we want to mimic that behavior.  Unlike Windows, they're not
        // always the first two entries, so we have to check every file we look at
        //
        while (m_dirent != nullptr && (!strcmp(".", m_dirent->d_name) || !strcmp("..", m_dirent->d_name)))
        {
            m_dirent = readdir(m_dir);
        }

        if (m_dirent == nullptr)
        {
            m_end = true;
        }

        return *this;
    }

    ~directory_iterator_impl()
    {
        if (m_dir)
        {
            closedir(m_dir);
        }
    }

    dirent *m_dirent;
    DIR *m_dir = nullptr;
    bool m_end = false;
};

path::path(const char *p)
{
    m_path = p;
}

path &path::append(const char *p)
{
    if (m_path.size() != 0 && m_path[m_path.size() - 1] != '/')
    {
        m_path += "/";
    }
    m_path += p;
    return *this;
}

path &path::concat(const char *p)
{
    m_path += p;
    return *this;
}

const char *path::c_str() const
{
    return m_path.c_str();
}

std::string path::string() const
{
    return m_path;
}

bool path::exists(const path &path)
{
    struct stat buffer;
    const int status = stat(path.c_str(), &buffer);
    return status == 0;
}

bool path::is_directory(const path &path)
{
    struct stat buffer;
    const int status = stat(path.c_str(), &buffer);
    if (status != 0)
    {
        return false;
    }

    return S_ISDIR(buffer.st_mode);
}

path path::filename() const
{
    std::string filename = m_path;
    return path(basename(&filename[0]));
}

path path::extension() const
{
    std::string filenameStr = filename().string();
    size_t lastPeriod = filenameStr.rfind('.');
    if (lastPeriod == std::string::npos || lastPeriod == 0 || filenameStr == "..")
    {
        return path("");
    }

    return path(filenameStr.substr(lastPeriod).c_str());
}

path path::parent_path() const
{
    std::string filename = m_path;
    return path(dirname(&filename[0]));
}

bool directory_entry::exists() const
{
    return path::exists(m_path);
}

bool directory_entry::is_directory() const
{
    return path::is_directory(m_path);
}

path std17::filesystem::current_path()
{
    std::vector<char> buffer;
    buffer.resize(PATH_MAX, '\0');
    if (!getcwd(&buffer[0], PATH_MAX))
    {
        return path("");
    }
    return path(&buffer[0]);
}

directory_iterator::directory_iterator() : m_impl(nullptr) {}

directory_iterator::directory_iterator(const path &p) : m_impl(std::make_shared<directory_iterator_impl>(p))
{
    if (m_impl->m_end)
    {
        m_impl.reset();
        return;
    }

    m_directory = p;

    update_entry_path();
}

directory_iterator directory_iterator::end(const directory_iterator &)
{
    return directory_iterator();
}

directory_iterator &directory_iterator::operator++()
{
    if (m_impl)
    {
        m_impl->advance();
    }
    if (m_impl->m_end)
    {
        m_impl.reset();
    }
    else
    {
        update_entry_path();
    }

    return *this;
}

bool directory_iterator::operator!=(const directory_iterator &other)
{
    // This is not strictly true, but we only need operator!= to support iterator loops
    // (i.e. they only need to return equal if they're both the end iterator)
    //
    return !(m_impl == nullptr && other.m_impl == nullptr);
}

const directory_entry &directory_iterator::operator*() const
{
    if (!m_impl)
    {
        // then they're trying to deference an invalid iterator - segfault
        //
        directory_entry *e = nullptr;
        return *e;
    }

    return m_current;
}

const directory_entry *directory_iterator::operator->() const
{
    if (!m_impl)
    {
        return nullptr;
    }

    return &m_current;
}

void directory_iterator::update_entry_path()
{
    m_current.m_path = m_directory;
    m_current.m_path.append(m_impl->m_dirent->d_name);
}
