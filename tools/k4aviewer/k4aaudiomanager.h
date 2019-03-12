// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AAUDIOMANAGER_H
#define K4AAUDIOMANAGER_H

// System headers
//
#include <functional>
#include <map>
#include <memory>

// Library headers
//
#include "k4asoundio_util.h"

// Project headers
//
#include "k4amicrophone.h"

namespace k4aviewer
{
class K4AAudioManager
{
public:
    static K4AAudioManager &Instance();

    int Initialize(SoundIoBackend backend);
    int Initialize();

    int RefreshDevices();

    size_t GetDeviceCount() const
    {
        return m_inputDevices.size();
    }

    std::shared_ptr<K4AMicrophone> GetMicrophoneForDevice(const std::string &deviceSerialNumber);

    ~K4AAudioManager() = default;
    K4AAudioManager(const K4AAudioManager &) = delete;
    K4AAudioManager(const K4AAudioManager &&) = delete;
    K4AAudioManager &operator=(const K4AAudioManager &) = delete;
    K4AAudioManager &operator=(const K4AAudioManager &&) = delete;

private:
    K4AAudioManager() = default;

    int InitializeImpl(const std::function<int(SoundIo *)> &initFn);

    SoundIoUniquePtr m_io;

    std::map<std::string, std::shared_ptr<SoundIoDevice>> m_inputDevices;
};
} // namespace k4aviewer

#endif
