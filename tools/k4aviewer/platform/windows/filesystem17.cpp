// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "filesystem17.h"

// System headers
//

// Library headers
//
#define NOMINMAX
#include "Windows.h"

#include <locale>
#include <codecvt>

#include "PathCch.h"
#include "Shlwapi.h"

// Project headers
//
#include "k4aviewerutil.h"

using namespace std17::filesystem;

struct std17::filesystem::directory_iterator_impl
{
    directory_iterator_impl(const path &p)
    {
        end = false;

        path searchPath = p;
        searchPath.append("*");
        hFindFile = FindFirstFile(searchPath.c_str(), &findData);

        if (hFindFile == INVALID_HANDLE_VALUE)
        {
            end = true;
            return;
        }

        // FindFirstFile/FindNextFile return the '.' and '..' special directories,
        // but std::filesystem omits them, so we want to mimic that behavior
        //
        while (!end && (!strcmp(".", findData.cFileName) || !strcmp("..", findData.cFileName)))
        {
            advance();
        }
    }

    directory_iterator_impl &advance()
    {
        end = !FindNextFile(hFindFile, &findData);
        return *this;
    }

    ~directory_iterator_impl()
    {
        if (hFindFile)
        {
            FindClose(hFindFile);
        }
    }
    WIN32_FIND_DATAA findData{};
    HANDLE hFindFile = nullptr;
    bool end = false;
};

path::path(const char *p)
{
    m_path = p;
}

path &path::append(const char *p)
{
    if (m_path.size() != 0 && m_path[m_path.size() - 1] != '\\')
    {
        m_path += "\\";
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
    const DWORD attributes = GetFileAttributes(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES;
}

bool path::is_directory(const path &path)
{
    const DWORD attributes = GetFileAttributes(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && attributes & FILE_ATTRIBUTE_DIRECTORY;
}

path path::filename() const
{
    std::string filename = m_path;
    PathStripPath(&filename[0]);
    return path(filename.c_str());
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
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wsParentPath = converter.from_bytes(m_path);
    PathCchRemoveFileSpec(&wsParentPath[0], wsParentPath.size());

    std::string resultPath = converter.to_bytes(wsParentPath);

    return path(resultPath.c_str());
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
    const DWORD size = GetCurrentDirectory(0, nullptr);
    std::string pathStr;
    pathStr.resize(size);
    GetCurrentDirectory(size, &pathStr[0]);
    return path(pathStr.c_str());
}

directory_iterator::directory_iterator() : m_impl(nullptr) {}

directory_iterator::directory_iterator(const path &p) : m_impl(std::make_shared<directory_iterator_impl>(p))
{
    if (m_impl->end)
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
    if (m_impl->end)
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
    m_current.m_path.append(m_impl->findData.cFileName);
}
