// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "k4ainternal/matroska_common.h"

using namespace k4arecord;

static_assert(sizeof(std::streamoff) == sizeof(int64), "64-bit seeking is not supported on this architecture");
static_assert(sizeof(std::streamsize) == sizeof(int64), "64-bit seeking is not supported on this architecture");

LargeFileIOCallback::LargeFileIOCallback(const char *path, const open_mode mode) : m_owner(std::this_thread::get_id())
{
    assert(path);
    std::ios::openmode om = std::ios::binary;

    switch (mode)
    {
    case MODE_READ:
        om |= std::ios::in;
        break;
    case MODE_SAFE:
        om |= std::ios::in | std::ios::out;
        break;
    case MODE_WRITE:
        om |= std::ios::in | std::ios::out;
        break;
    case MODE_CREATE:
        om |= std::ios::in | std::ios::out | std::ios::trunc;
        break;
    default:
        throw std::invalid_argument("Unknown file mode specified");
    }

    m_stream.exceptions(std::ios::failbit | std::ios::badbit);
    m_stream.open(path, om);
    m_stream.exceptions(std::ios::badbit); // Don't throw exceptions for EOF errors
}

LargeFileIOCallback::~LargeFileIOCallback()
{
    close();
}

uint32 LargeFileIOCallback::read(void *buffer, size_t size)
{
    assert(size <= UINT32_MAX); // can't properly return > uint32
    assert(m_owner == std::this_thread::get_id());

    m_stream.read((char *)buffer, (std::streamsize)size);
    return (uint32)m_stream.gcount();
}

void LargeFileIOCallback::setFilePointer(int64 offset, libebml::seek_mode mode)
{
    assert(mode == SEEK_SET || mode == SEEK_CUR || mode == SEEK_END);
    assert(m_owner == std::this_thread::get_id());

    switch (mode)
    {
    case SEEK_SET:
        m_stream.clear();
        m_stream.seekg(offset, std::ios::beg);
        break;
    case SEEK_CUR:
        m_stream.seekg(offset, std::ios::cur);
        break;
    case SEEK_END:
        m_stream.clear();
        m_stream.seekg(offset, std::ios::end);
        break;
    }
}

size_t LargeFileIOCallback::write(const void *buffer, size_t size)
{
    assert(size <= INT64_MAX); // m_stream.write() takes a signed long input
    assert(m_owner == std::this_thread::get_id());

    m_stream.write((const char *)buffer, (std::streamsize)size);
    return size;
}

uint64 LargeFileIOCallback::getFilePointer()
{
    assert(m_owner == std::this_thread::get_id());
    std::streampos pos = m_stream.tellg();
    assert(pos >= 0); // tellg() should have thrown an exception if this happens
    return (uint64)pos;
}

void LargeFileIOCallback::close()
{
    // LargeFileIOCallback::close() can be called more than once, only close the underlying stream the first time.
    if (m_stream.is_open())
    {
        // Due to the definition of libebml::IOCallback, the only way for us to return a close error is with an
        // exception. Enable failbit exceptions again for file close.
        m_stream.clear();
        m_stream.exceptions(std::ios::failbit | std::ios::badbit);
        m_stream.close();
    }
}

void LargeFileIOCallback::setOwnerThread()
{
    m_owner = std::this_thread::get_id();
}
