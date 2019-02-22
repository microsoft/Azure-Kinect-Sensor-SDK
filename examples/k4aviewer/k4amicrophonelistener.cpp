// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4amicrophonelistener.h"

// System headers
//
#include <regex>

// Library headers
//

// Project headers
//

using namespace k4aviewer;

size_t K4AMicrophoneListener::ProcessFrames(const std::function<size_t(K4AMicrophoneFrame *, size_t)> &processor)
{
    if (!m_buffer)
    {
        return 0;
    }

    if (m_backingDevice->GetStatusCode() != SoundIoErrorNone)
    {
        // When our backing device fails, that's unrecoverable.  To get working again,
        // we need to recreate the microphone listener.  Clear out everything.
        //
        m_statusCode = m_backingDevice->GetStatusCode();
        m_buffer.reset();
        m_backingDevice.reset();
        return 0;
    }

    const auto readableBytes = static_cast<size_t>(soundio_ring_buffer_fill_count(m_buffer.get()));
    const size_t readableFrames = readableBytes / sizeof(K4AMicrophoneFrame);

    if (readableFrames == 0)
    {
        return 0;
    }

    char *readPoint = soundio_ring_buffer_read_ptr(m_buffer.get());
    auto *frameReadPoint = reinterpret_cast<K4AMicrophoneFrame *>(readPoint);

    const size_t readFrames = processor(frameReadPoint, readableFrames);
    const size_t readBytes = readFrames * sizeof(K4AMicrophoneFrame);

    soundio_ring_buffer_advance_read_ptr(m_buffer.get(), static_cast<int>(readBytes));

    return readFrames;
}

K4AMicrophoneListener::K4AMicrophoneListener(std::shared_ptr<K4AMicrophone> backingDevice, const size_t bufferSize) :
    m_backingDevice(std::move(backingDevice))
{
    m_buffer.reset(soundio_ring_buffer_create(nullptr, static_cast<int>(bufferSize)));
    if (m_buffer)
    {
        memset(soundio_ring_buffer_write_ptr(m_buffer.get()), 0, bufferSize);
    }
}
