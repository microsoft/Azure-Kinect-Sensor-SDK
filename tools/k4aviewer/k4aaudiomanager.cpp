// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aaudiomanager.h"

// System headers
//
#include <regex>

// Library headers
//
#include <libusb.h>

// Project headers
//
#include "k4aviewererrormanager.h"

#include "k4adevicecorrelator.h"

using namespace k4aviewer;

K4AAudioManager &K4AAudioManager::Instance()
{
    static K4AAudioManager instance;
    return instance;
}

int K4AAudioManager::Initialize(SoundIoBackend backend)
{
    return InitializeImpl([backend](SoundIo *soundIo) { return soundio_connect_backend(soundIo, backend); });
}

int K4AAudioManager::Initialize()
{
    return InitializeImpl(soundio_connect);
}

int K4AAudioManager::InitializeImpl(const std::function<int(SoundIo *)> &initFn)
{
    m_io.reset(soundio_create());
    const int status = initFn(m_io.get());

    if (status != SoundIoErrorNone)
    {
        return status;
    }

    return RefreshDevices();
}

int K4AAudioManager::RefreshDevices()
{
    if (!m_io)
    {
        return SoundIoErrorInvalid;
    }

    soundio_flush_events(m_io.get());
    m_inputDevices.clear();

    std::map<std::string, std::string> soundIoToSerialNumberMapping;

    if (!K4ADeviceCorrelator::GetSoundIoBackendIdToSerialNumberMapping(m_io.get(), &soundIoToSerialNumberMapping))
    {
        return SoundIoErrorIncompatibleDevice;
    }

    const int inputCount = soundio_input_device_count(m_io.get());

    for (int i = 0; i < inputCount; i++)
    {
        std::shared_ptr<SoundIoDevice> device(soundio_get_input_device(m_io.get(), i), SoundIoDeviceDeleter());
        if (device)
        {
            // Each device is listed twice - a 'raw' device and a not-'raw' device.
            // We only want the non-raw ones.
            //
            if (device->is_raw)
            {
                continue;
            }

            auto soundIoToSerialNumber = soundIoToSerialNumberMapping.find(device->id);

            // Exclude non-K4A devices
            //
            if (soundIoToSerialNumber == soundIoToSerialNumberMapping.end())
            {
                continue;
            }

            m_inputDevices[soundIoToSerialNumber->second] = std::move(device);
        }
    }

    return SoundIoErrorNone;
}

std::shared_ptr<K4AMicrophone> K4AAudioManager::GetMicrophoneForDevice(const std::string &deviceSerialNumber)
{
    const auto soundIoDevice = m_inputDevices.find(deviceSerialNumber);
    if (soundIoDevice == m_inputDevices.end())
    {
        return nullptr;
    }

    return std::shared_ptr<K4AMicrophone>(new K4AMicrophone(soundIoDevice->second));
}
