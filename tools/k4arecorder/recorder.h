// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef RECORDER_H
#define RECORDER_H

#include <atomic>
#include <k4a/k4a.h>

extern std::atomic_bool exiting;

static const int32_t defaultExposureAuto = -12;
static const int32_t defaultGainAuto = -1;

int do_recording(uint8_t device_index,
                 char *recording_filename,
                 int recording_length,
                 k4a_device_configuration_t *device_config,
                 bool record_imu,
                 int32_t absoluteExposureValue,
                 int32_t gain);

#endif /* RECORDER_H */
