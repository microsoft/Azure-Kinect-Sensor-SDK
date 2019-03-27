// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ADEVICECORRELATOR_H
#define K4ADEVICECORRELATOR_H

// System headers
//
#include <map>
#include <string>

// Library headers
//

// Project headers
//
#include "k4asoundio_util.h"

namespace k4aviewer
{
// Populates result with a map from libsoundio backend ID to USB container ID.
// Implementation of this function is platform-specific.
// Returns true if successful, false otherwise.
//
class K4ADeviceCorrelator
{
public:
    static bool GetSoundIoBackendIdToSerialNumberMapping(SoundIo *soundIo, std::map<std::string, std::string> *result);

    K4ADeviceCorrelator() = delete;
};
} // namespace k4aviewer
#endif // K4ADEVICECORRELATOR_H
