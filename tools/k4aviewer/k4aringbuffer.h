// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ARINGBUFFER_H
#define K4ARINGBUFFER_H

// System headers
//
#include <array>
#include <functional>
#include <mutex>

// Library headers
//

// Project headers
//

namespace k4aviewer
{

// Thread-safe ring buffer
//
template<typename T, size_t size> class K4ARingBuffer
{
public:
    static_assert(size >= 2, "Ring buffer must be size 2 or greater");

    // Initializes all the elements in the ring buffer by calling initFn on them.
    // This function is not thread-safe and should be called before making any
    // other calls
    //
    void Initialize(const std::function<void(T *)> &initFn)
    {
        for (T &element : m_buffer)
        {
            initFn(&element);
        }
    }

    bool Empty()
    {
        return m_count == 0;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_count = 0;
        m_buffer.fill(T());
    }

    bool Full()
    {
        return m_buffer.size() == m_count;
    }

    // Returns a pointer to the first item in the buffer.
    // If the buffer is empty, behavior is undefined.
    // Using the result of CurrentItem() after calling AdvanceRead() is undefined.
    //
    T *CurrentItem()
    {
        return &m_buffer[m_readIndex];
    }

    // Attempt to advance the item referenced by CurrentItem().
    // Returns true if successful, false if the buffer was empty.
    //
    bool AdvanceRead()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!Empty())
        {
            m_readIndex = AdvanceIndex(m_readIndex);
            m_count--;
            return true;
        }

        return false;
    }

    // Attempts to start an insert operation.
    // Returns true on success, false on failure.
    // When done, call EndInsert() to complete the insert operation
    // (or AbortInsert() to cancel the insert operation)
    //
    bool BeginInsert()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_inserting)
        {
            return false;
        }
        if (Full())
        {
            return false;
        }

        m_inserting = true;
        return true;
    }

    // Returns a pointer to the item the insert is to update.
    // The behavior of calling InsertionItem when you have not
    //      A) previously made a successful call to BeginInsert(), and
    //      B) not yet made a corresponding call to EndInsert() or AbortInsert()
    // is undefined.
    //
    T *InsertionItem()
    {
        return &m_buffer[m_writeIndex];
    }

    // Ends the insertion operation and commits the write operation.
    //
    void EndInsert()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_writeIndex = AdvanceIndex(m_writeIndex);
        m_inserting = false;
        ++m_count;
    }

    // Ends the insertion operation and aborts the write operation (i.e. does not advance the write pointer).
    //
    void AbortInsert()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_inserting = false;
    }

private:
    static size_t AdvanceIndex(size_t idx)
    {
        return (idx + 1) % size;
    }

    std::array<T, size> m_buffer;
    size_t m_readIndex = 0;
    size_t m_writeIndex = 0;
    size_t m_count = 0;
    bool m_inserting = false;

    std::mutex m_mutex;
};
} // namespace k4aviewer

#endif
