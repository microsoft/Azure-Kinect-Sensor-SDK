/** \file deloader.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Depth Engine Loader
 * Stub layer to abstract away the dynamic loading of the depth engine and transform engine from our
 * developers usage
 */

#pragma once

#include <k4ainternal/k4aplugin.h>

#ifdef __cplusplus
extern "C" {
#endif

k4a_depth_engine_result_code_t deloader_depth_engine_create_and_initialize(k4a_depth_engine_context_t **context,
                                                                           size_t cal_block_size_in_bytes,
                                                                           void *cal_block,
                                                                           k4a_depth_engine_mode_t mode,
                                                                           k4a_depth_engine_input_type_t input_format,
                                                                           void *camera_calibration,
                                                                           k4a_processing_complete_cb_t *callback,
                                                                           void *callback_context);

k4a_depth_engine_result_code_t
deloader_depth_engine_process_frame(k4a_depth_engine_context_t *context,
                                    void *input_frame,
                                    size_t input_frame_size,
                                    k4a_depth_engine_output_type_t output_type,
                                    void *output_frame,
                                    size_t output_frame_size,
                                    k4a_depth_engine_output_frame_info_t *output_frame_info,
                                    k4a_depth_engine_input_frame_info_t *input_frame_info);

size_t deloader_depth_engine_get_output_frame_size(k4a_depth_engine_context_t *context);

void deloader_depth_engine_destroy(k4a_depth_engine_context_t **context);

k4a_depth_engine_result_code_t deloader_transform_engine_create_and_initialize(k4a_transform_engine_context_t **context,
                                                                               void *camera_calibration,
                                                                               k4a_processing_complete_cb_t *callback,
                                                                               void *callback_context);

k4a_depth_engine_result_code_t
deloader_transform_engine_process_frame(k4a_transform_engine_context_t *context,
                                        k4a_transform_engine_type_t type,
                                        const void *depth_frame,
                                        size_t depth_frame_size,
                                        const void *frame2,
                                        size_t frame2_size,
                                        void *output_frame,
                                        size_t output_frame_size,
                                        void *output_frame2,
                                        size_t output_frame2_size,
                                        k4a_transform_engine_interpolation_t interpolation,
                                        uint32_t invalid_value);

size_t deloader_transform_engine_get_output_frame_size(k4a_transform_engine_context_t *context,
                                                       k4a_transform_engine_type_t type);

void deloader_transform_engine_destroy(k4a_transform_engine_context_t **context);

#ifdef __cplusplus
}
#endif
