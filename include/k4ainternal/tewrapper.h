/** \file TEWRAPPER.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef TEWRAPPER_H
#define TEWRAPPER_H

#include <k4a/k4atypes.h>
#include <k4ainternal/handle.h>
#include <k4ainternal/transformation.h>
#include <k4ainternal/k4aplugin.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handle to the tewrapper device.
 *
 * Handles are created with \ref tewrapper_create and closed
 * with \ref tewrapper_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(tewrapper_t);

tewrapper_t tewrapper_create(k4a_transform_engine_calibration_t *transform_engine_calibration);
void tewrapper_destroy(tewrapper_t tewrapper_handle);
k4a_result_t tewrapper_process_frame(tewrapper_t tewrapper_handle,
                                     k4a_transform_engine_type_t type,
                                     const void *depth_image_data,
                                     size_t depth_image_size,
                                     const void *image2_data,
                                     size_t image2_size,
                                     void *transformed_image_data,
                                     size_t transformed_image_size,
                                     void *transformed_image2_data,
                                     size_t transformed_image2_size,
                                     k4a_transform_engine_interpolation_t interpolation,
                                     uint32_t invalid_value);

#ifdef __cplusplus
}
#endif

#endif /* TEWRAPPER_H */
