// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4adevicecorrelator.h"

// System headers
//
#include <regex>

// Library headers
//

// Project headers
//
#include "k4aviewererrormanager.h"
#include "k4asoundio_util.h"

using namespace k4aviewer;

bool K4ADeviceCorrelator::GetSoundIoBackendIdToSerialNumberMapping(SoundIo *soundio,
                                                                   std::map<std::string, std::string> *result)
{
    const int inputCount = soundio_input_device_count(soundio);

    bool foundDevices = false;
    for (int i = 0; i < inputCount; i++)
    {
        std::unique_ptr<SoundIoDevice, SoundIoDeviceDeleter> device(soundio_get_input_device(soundio, i));
        if (device)
        {
            // Each device is listed twice - a 'raw' device and a not-'raw' device.
            // We only want the non-raw ones.
            //
            if (device->is_raw)
            {
                continue;
            }

            // On ALSA/Pulse, the device ID contains the serial number, so we can just extract it from the name
            //
            static const std::regex nameRegex(".*Kinect.*_([0-9]+)-.*");
            std::cmatch match;
            if (!std::regex_match(device->id, match, nameRegex))
            {
                continue;
            }

            foundDevices = true;

            (*result)[device->id] = match.str(1);
        }
    }

    return foundDevices;
}
