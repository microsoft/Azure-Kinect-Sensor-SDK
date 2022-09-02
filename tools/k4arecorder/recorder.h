// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef RECORDER_H
#define RECORDER_H

#include <atomic>
#include <k4a/k4a.h>

extern std::atomic_bool exiting;

static const int32_t defaultExposureAuto = -12;
static const int32_t defaultWhiteBalance = -1;
static const int32_t defaultBrightness = -1;
static const int32_t defaultContrast = -1;
static const int32_t defaultSaturation = -1;
static const int32_t defaultSharpness = -1;
static const int32_t defaultGainAuto = -1;

int do_recording(uint8_t device_index,
                 char *recording_filename,
                 int recording_length,
                 k4a_device_configuration_t *device_config,
                 bool record_imu,
                 int32_t absoluteExposureValue,
                 int32_t whiteBalance,
                 int32_t brightness,
                 int32_t contrast,
                 int32_t saturation,
                 int32_t sharpness,
                 int32_t gain);

#endif /* RECORDER_H */
