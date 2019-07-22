// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COLOR_PRIV_H
#define COLOR_PRIV_H

#include <k4a/k4atypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _color_control_cap_t
{
    int32_t minValue;
    int32_t maxValue;
    int32_t stepValue;
    int32_t defaultValue;
    k4a_color_control_mode_t defaultMode;
    bool supportAuto;
    bool valid;
} color_control_cap_t;

typedef struct _exposure_mapping
{
    int exponent;                  // Windows Media Foundation implementation detail
    int exposure_usec;             // Windows Media Foundation implementation detail
    int exposure_mapped_50Hz_usec; // Exposure when K4A_COLOR_CONTROL_POWERLINE_FREQUENCY == 50Hz
    int exposure_mapped_60Hz_usec; // Exposure when K4A_COLOR_CONTROL_POWERLINE_FREQUENCY == 60Hz
} color_exposure_mapping_t;

#ifdef _WIN32
#define KSELECTANY __declspec(selectany)
#else
#define KSELECTANY __attribute__((weak))
#endif

extern color_exposure_mapping_t device_exposure_mapping[];
KSELECTANY color_exposure_mapping_t device_exposure_mapping[] = {
    // clang-format off
    //exp,   2^exp,   50Hz,   60Hz,
    { -11,     488,    500,    500},
    { -10,     977,   1250,   1250},
    {  -9,    1953,   2500,   2500},
    {  -8,    3906,  10000,   8330},
    {  -7,    7813,  20000,  16670},
    {  -6,   15625,  30000,  33330},
    {  -5,   31250,  40000,  41670},
    {  -4,   62500,  50000,  50000},
    {  -3,  125000,  60000,  66670},
    {  -2,  250000,  80000,  83330},
    {  -1,  500000, 100000, 100000},
    {   0, 1000000, 120000, 116670},
    {   1, 2000000, 130000, 133330}
    // clang-format on
};

#define MAX_EXPOSURE(is_using_60hz)                                                                                    \
    (is_using_60hz ? device_exposure_mapping[COUNTOF(device_exposure_mapping) - 1].exposure_mapped_60Hz_usec :         \
                     device_exposure_mapping[COUNTOF(device_exposure_mapping) - 1].exposure_mapped_50Hz_usec)

/** Delivers a sample to the registered callback function when a capture is ready for processing.
 *
 * \param result
 * inidicates if the capture being passed in is complete
 *
 * \param capture_handle
 * Capture being read by hardware.
 *
 * \param context
 * Context for the callback function
 *
 * \remarks
 * Capture is only of one type. At this point it is not linked to other captures. Capture is safe to use durring this
 * callback as the caller ensures a ref is held. If the callback function wants the capture to exist beyond this
 * callback, a ref must be taken with capture_inc_ref().
 */
typedef void(color_cb_stream_t)(k4a_result_t result, k4a_capture_t capture_handle, void *context);

#ifdef __cplusplus
}
#endif

#endif // COLOR_PRIV_H
