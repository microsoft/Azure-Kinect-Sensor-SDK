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
