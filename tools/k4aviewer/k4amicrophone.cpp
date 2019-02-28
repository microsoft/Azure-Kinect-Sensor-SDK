// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4amicrophone.h"

// System headers
//
#include <algorithm>

// Library headers
//

// Project headers
//
#include "k4amicrophonelistener.h"
#include "k4aviewererrormanager.h"

using namespace k4aviewer;

K4AMicrophone::K4AMicrophone(std::shared_ptr<SoundIoDevice> device) : m_device(std::move(device)) {}

void K4AMicrophone::SetFailed(const int errorCode)
{
    // We can't stop the stream because this function can be called from the reader thread,
    // and you're not allowed to call soundio_destroy_instream from the reader thread.
    // Instead, we set the failed state and destroy the instream the next time someone tries
    // to use it.  K4AMicrophoneListeners know to check this and drop their references to the
    // K4AMicrophone if the K4AMicrophone has failed.
    //
    m_statusCode = errorCode;
    m_started = false;

    // The next time all our listeners try to pull frames and realize that the mic is dead,
    // we'll lose the last reference to the microphone and it'll get deleted.  We can't actually
    // stop the sound stream on the callback thread, though (libsoundio doesn't allow the callback
    // thread to call soundio_instream_destroy), so we can't stop the callbacks from happening.
    // To work around this, we have to null out the pointer that libsoundio has our callback can
    // know that the stream is dead and we get don't segfault.
    //
    m_inStream->userdata = nullptr;
}

int K4AMicrophone::Start()
{
    m_inStream.reset(soundio_instream_create(m_device.get()));
    if (!m_inStream)
    {
        return SoundIoErrorNoMem;
    }

    m_inStream->format = SoundIoFormatFloat32LE;
    m_inStream->sample_rate = K4AMicrophoneSampleRate;

    m_inStream->layout = *soundio_channel_layout_get_builtin(SoundIoChannelLayoutId7Point0);
    m_inStream->software_latency = 0.2;

    m_inStream->userdata = this;
    m_inStream->read_callback = K4AMicrophone::ReadCallback;
    m_inStream->overflow_callback = K4AMicrophone::OverflowCallback;
    m_inStream->error_callback = K4AMicrophone::ErrorCallback;

    int result = soundio_instream_open(m_inStream.get());
    if (result != SoundIoErrorNone)
    {
        m_inStream.reset();
        return result;
    }

    result = soundio_instream_start(m_inStream.get());
    if (result != SoundIoErrorNone)
    {
        return result;
    }

    m_started = true;

    return result;
}

void K4AMicrophone::Stop()
{
    m_started = false;
    m_inStream.reset();

    std::lock_guard<std::mutex> lock(m_listenersMutex);
    m_listeners.clear();
}

std::shared_ptr<K4AMicrophoneListener> K4AMicrophone::CreateListener()
{
    if (!m_inStream)
    {
        return nullptr;
    }

    constexpr int bufferPaddingRatio = 3;
    const auto bufferSize = static_cast<size_t>(bufferPaddingRatio * m_inStream->software_latency *
                                                m_inStream->sample_rate * m_inStream->bytes_per_frame);

    auto result = std::shared_ptr<K4AMicrophoneListener>(new K4AMicrophoneListener(shared_from_this(), bufferSize));
    if (!result->m_buffer)
    {
        // OOM
        //
        result.reset();
        return nullptr;
    }

    std::weak_ptr<K4AMicrophoneListener> newListenerWeakPtr = result;

    std::lock_guard<std::mutex> lock(m_listenersMutex);
    m_listeners.emplace_back(std::move(newListenerWeakPtr));

    return result;
}

void K4AMicrophone::ReadCallback(SoundIoInStream *inStream, const int frameCountMin, const int frameCountMax)
{
    if (!inStream->userdata)
    {
        return;
    }

    auto instance = reinterpret_cast<K4AMicrophone *>(inStream->userdata);

    int maxFramesToWrite = 0;

    // Grab references to all the listeners and figure out how many frames we're going to read
    //
    struct ListenerInfo
    {
        std::shared_ptr<K4AMicrophoneListener> Listener = nullptr;
        int FramesToWrite = 0;
        int FramesWritten = 0;
        char *WritePtr = nullptr;
    };

    std::vector<ListenerInfo> listenerInfo;

    // We don't need the lock on the listeners for the whole function, so create a scope that just
    // lasts as long as we need the lock
    {
        std::lock_guard<std::mutex> lock(instance->m_listenersMutex);
        listenerInfo.reserve(instance->m_listeners.size());

        bool expiredReferencesFound = false;

        for (std::weak_ptr<K4AMicrophoneListener> &wpListener : instance->m_listeners)
        {
            std::shared_ptr<K4AMicrophoneListener> spListener = wpListener.lock();
            if (spListener)
            {
                const int bufferFreeBytes = soundio_ring_buffer_free_count(spListener->m_buffer.get());
                const int bufferFreeFrames = bufferFreeBytes / instance->m_inStream->bytes_per_frame;
                const int totalFramesToWrite = std::min(bufferFreeFrames, frameCountMax);

                ListenerInfo newListener;
                newListener.Listener = std::move(spListener);
                newListener.FramesToWrite = totalFramesToWrite;
                newListener.FramesWritten = 0;
                newListener.WritePtr = soundio_ring_buffer_write_ptr(newListener.Listener->m_buffer.get());
                listenerInfo.emplace_back(std::move(newListener));

                maxFramesToWrite = std::max(totalFramesToWrite, maxFramesToWrite);
            }
            else
            {
                expiredReferencesFound = true;
            }
        }

        // Clean up listeners that have been destroyed
        //
        if (expiredReferencesFound)
        {
            const auto isExpired = [](const std::weak_ptr<K4AMicrophoneListener> &wp) { return wp.expired(); };

            instance->m_listeners.erase(std::remove_if(instance->m_listeners.begin(),
                                                       instance->m_listeners.end(),
                                                       isExpired),
                                        instance->m_listeners.end());
        }
    }

    if (frameCountMin > maxFramesToWrite)
    {
        // Everyone is out of buffer space, which means something has gone badly wrong.
        //
        ErrorCallback(inStream, SoundIoErrorStreaming);
        return;
    }

    // Actually read audio data
    //
    int remainingFramesToWrite = maxFramesToWrite;
    int maxFramesWritten = 0;
    while (true)
    {
        int readFrameCount = remainingFramesToWrite;
        SoundIoChannelArea *areas;

        int err = soundio_instream_begin_read(instance->m_inStream.get(), &areas, &readFrameCount);
        if (err != SoundIoErrorNone)
        {
            ErrorCallback(inStream, err);
            return;
        }

        if (readFrameCount == 0)
        {
            break;
        }

        // Distribute audio data to each listener
        //
        for (ListenerInfo &listener : listenerInfo)
        {
            const int framesToWriteForListener = std::min(readFrameCount, listener.FramesToWrite);

            if (framesToWriteForListener < readFrameCount)
            {
                // This listener has run out of space in its buffer and is going to lose data.
                //
                listener.Listener->m_overflowed = true;
            }

            if (areas == nullptr)
            {
                // There is a hole in the buffer; we need to fill it with silence.
                // This can happen if the microphone is muted by the OS.
                //
                memset(listener.WritePtr,
                       0,
                       static_cast<size_t>(readFrameCount * instance->m_inStream->bytes_per_frame));
            }
            else
            {
                for (int frame = 0; frame < readFrameCount; ++frame)
                {
                    for (int channel = 0; channel < instance->m_inStream->layout.channel_count; channel++)
                    {
                        memcpy(listener.WritePtr,
                               areas[channel].ptr,
                               static_cast<size_t>(instance->m_inStream->bytes_per_sample));
                        areas[channel].ptr += areas[channel].step;
                        listener.WritePtr += instance->m_inStream->bytes_per_sample;
                    }
                }
            }

            listener.FramesToWrite -= framesToWriteForListener;
            listener.FramesWritten += framesToWriteForListener;

            maxFramesWritten = std::max(listener.FramesWritten, maxFramesWritten);
        }

        err = soundio_instream_end_read(instance->m_inStream.get());
        if (err != SoundIoErrorNone)
        {
            ErrorCallback(inStream, err);
            return;
        }

        remainingFramesToWrite -= readFrameCount;
        if (remainingFramesToWrite <= 0)
        {
            break;
        }
    }

    for (ListenerInfo &listener : listenerInfo)
    {
        const int bytesWritten = listener.FramesWritten * instance->m_inStream->bytes_per_frame;
        soundio_ring_buffer_advance_write_ptr(listener.Listener->m_buffer.get(), bytesWritten);

        // This listener fell behind and lost some data; notify it that this happened
        //
        if (listener.FramesWritten < maxFramesWritten)
        {
            listener.Listener->m_overflowed = true;
        }
    }
}

void K4AMicrophone::ErrorCallback(SoundIoInStream *inStream, const int errorCode)
{
    auto instance = reinterpret_cast<K4AMicrophone *>(inStream->userdata);

    instance->SetFailed(errorCode);
}

void K4AMicrophone::OverflowCallback(SoundIoInStream *inStream)
{
    ErrorCallback(inStream, SoundIoErrorStreaming);
}
