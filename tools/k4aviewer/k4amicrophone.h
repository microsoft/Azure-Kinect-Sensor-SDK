// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AMICROPHONE_H
#define K4AMICROPHONE_H

// System headers
//
#include <memory>
#include <mutex>
#include <vector>

// Library headers
//
#include "k4asoundio_util.h"

// Project headers
//

namespace k4aviewer
{
class K4AMicrophoneListener;

class K4AMicrophone : public std::enable_shared_from_this<K4AMicrophone>
{
public:
    // Returns a libsoundio exit code
    //
    int Start();

    void Stop();

    // SoundIO status code
    //
    int GetStatusCode() const
    {
        return m_statusCode;
    }

    void ClearStatusCode()
    {
        m_statusCode = SoundIoErrorNone;
    }

    bool IsStarted() const
    {
        return m_started;
    }

    std::shared_ptr<K4AMicrophoneListener> CreateListener();

    K4AMicrophone(const K4AMicrophone &) = delete;
    K4AMicrophone(const K4AMicrophone &&) = delete;
    K4AMicrophone &operator=(const K4AMicrophone &) = delete;
    K4AMicrophone &operator=(const K4AMicrophone &&) = delete;

    ~K4AMicrophone() = default;

private:
    friend class K4AAudioManager;
    explicit K4AMicrophone(std::shared_ptr<SoundIoDevice> device);

    void SetFailed(int errorCode);

    // These are callbacks that we give to libsoundio.
    // inStream->userdata is a void* that will point to a K4AMicrophone instance.
    //
    static void ReadCallback(SoundIoInStream *inStream, int frameCountMin, int frameCountMax);
    static void ErrorCallback(SoundIoInStream *inStream, int errorCode);
    static void OverflowCallback(SoundIoInStream *inStream);

    std::mutex m_listenersMutex;
    std::vector<std::weak_ptr<K4AMicrophoneListener>> m_listeners;

    SoundIoInStreamUniquePtr m_inStream = nullptr;
    std::shared_ptr<SoundIoDevice> m_device = nullptr;
    bool m_started = false;
    int m_statusCode = SoundIoErrorNone;
};
} // namespace k4aviewer

#endif
