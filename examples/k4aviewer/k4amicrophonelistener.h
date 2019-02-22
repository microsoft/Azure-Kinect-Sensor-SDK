// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AMICROPHONELISTENER_H
#define K4AMICROPHONELISTENER_H

// System headers
//
#include <functional>
#include <memory>

// Library headers
//
#include "k4asoundio_util.h"

// Project headers
//
#include "k4amicrophone.h"

namespace k4aviewer
{
constexpr size_t K4AMicrophoneSampleRate = 48000;

struct K4AMicrophoneFrame
{
    static constexpr size_t ChannelCount = 7;

    float Channel[ChannelCount];
};

class K4AMicrophoneListener
{
public:
    // processor takes a pointer to array of frames and the number of frames available to read.  It returns
    // the number of frames that it processed (i.e. wants removed from the buffer).  It must return a number
    // that is <= the number of frames it received.
    //
    size_t ProcessFrames(const std::function<size_t(K4AMicrophoneFrame *, size_t)> &processor);

    int GetStatus() const
    {
        return m_statusCode;
    }

    bool Overflowed() const
    {
        return m_overflowed;
    }

    void ClearOverflowed()
    {
        m_overflowed = false;
    }

    ~K4AMicrophoneListener() = default;

    K4AMicrophoneListener(const K4AMicrophoneListener &) = delete;
    K4AMicrophoneListener(const K4AMicrophoneListener &&) = delete;
    K4AMicrophoneListener &operator=(const K4AMicrophoneListener &) = delete;
    K4AMicrophoneListener &operator=(const K4AMicrophoneListener &&) = delete;

private:
    friend class K4AMicrophone;

    K4AMicrophoneListener(std::shared_ptr<K4AMicrophone> backingDevice, size_t bufferSize);

    SoundIoRingBufferUniquePtr m_buffer;
    std::shared_ptr<K4AMicrophone> m_backingDevice;
    int m_statusCode = SoundIoErrorNone;
    bool m_overflowed = false;
};
} // namespace k4aviewer

#endif
