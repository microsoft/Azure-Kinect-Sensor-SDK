/** \file transformation.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef K4ATRANSFORMATION_H
#define K4ATRANSFORMATION_H

#include <k4a/k4atypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _k4a_camera_calibration_mode_info_t
{
    unsigned int calibration_image_binned_resolution[2];
    int crop_offset[2];
    unsigned int output_image_resolution[2];
} k4a_camera_calibration_mode_info_t;

typedef struct _k4a_transformation_image_descriptor_t
{
    int width_pixels;          // image width in pixels
    int height_pixels;         // image height in pixels
    int stride_bytes;          // image stride in bytes
    k4a_image_format_t format; // image format
} k4a_transformation_image_descriptor_t;

typedef struct _k4a_transformation_xy_tables_t
{
    float *x_table; // table used to compute X coordinate
    float *y_table; // table used to compute Y coordinate
    int width;      // width of x and y tables
    int height;     // height of x and y tables
} k4a_transformation_xy_tables_t;

typedef struct _k4a_transformation_pinhole_t
{
    float px;
    float py;
    float fx;
    float fy;
    int width;
    int height;
} k4a_transformation_pinhole_t;

typedef struct _k4a_transform_engine_calibration_t
{
    k4a_calibration_camera_t depth_camera_calibration;                    // depth camera calibration
    k4a_calibration_camera_t color_camera_calibration;                    // color camera calibration
    k4a_calibration_extrinsics_t depth_camera_to_color_camera_extrinsics; // depth to color extrinsics
    k4a_calibration_extrinsics_t color_camera_to_depth_camera_extrinsics; // color to depth extrinsics
    k4a_transformation_xy_tables_t depth_camera_xy_tables;                // depth camera xy tables
} k4a_transform_engine_calibration_t;

k4a_result_t transformation_get_mode_specific_calibration(const k4a_calibration_camera_t *depth_camera_calibration,
                                                          const k4a_calibration_camera_t *color_camera_calibration,
                                                          const k4a_calibration_extrinsics_t *gyro_extrinsics,
                                                          const k4a_calibration_extrinsics_t *accel_extrinsics,
                                                          const k4a_depth_mode_t depth_mode,
                                                          const k4a_color_resolution_t color_resolution,
                                                          k4a_calibration_t *calibration);

k4a_result_t transformation_3d_to_3d(const k4a_calibration_t *calibration,
                                     const float source_point3d[3],
                                     const k4a_calibration_type_t source_camera,
                                     const k4a_calibration_type_t target_camera,
                                     float target_point3d[3]);

k4a_result_t transformation_2d_to_3d(const k4a_calibration_t *calibration,
                                     const float source_point2d[2],
                                     const float source_depth,
                                     const k4a_calibration_type_t source_camera,
                                     const k4a_calibration_type_t target_camera,
                                     float target_point3d[3],
                                     int *valid);

k4a_result_t transformation_3d_to_2d(const k4a_calibration_t *calibration,
                                     const float source_point3d[3],
                                     const k4a_calibration_type_t source_camera,
                                     const k4a_calibration_type_t target_camera,
                                     float target_point2d[2],
                                     int *valid);

k4a_result_t transformation_2d_to_2d(const k4a_calibration_t *calibration,
                                     const float source_point2d[2],
                                     const float source_depth,
                                     const k4a_calibration_type_t source_camera,
                                     const k4a_calibration_type_t target_camera,
                                     float target_point2d[2],
                                     int *valid);

k4a_result_t transformation_color_2d_to_depth_2d(const k4a_calibration_t *calibration,
                                                 const float source_point2d[2],
                                                 const k4a_image_t depth_image,
                                                 float target_point2d[2],
                                                 int *valid);

k4a_transformation_t transformation_create(const k4a_calibration_t *calibration, bool gpu_optimization);

void transformation_destroy(k4a_transformation_t transformation_handle);

k4a_buffer_result_t transformation_depth_image_to_color_camera_validate_parameters(
    const k4a_calibration_t *calibration,
    const k4a_transformation_xy_tables_t *xy_tables_depth_camera,
    const uint8_t *depth_image_data,
    const k4a_transformation_image_descriptor_t *depth_image_descriptor,
    const uint8_t *custom_image_data,
    const k4a_transformation_image_descriptor_t *custom_image_descriptor,
    uint8_t *transformed_depth_image_data,
    k4a_transformation_image_descriptor_t *transformed_depth_image_descriptor,
    uint8_t *transformed_custom_image_data,
    k4a_transformation_image_descriptor_t *transformed_custom_image_descriptor);

k4a_buffer_result_t transformation_depth_image_to_color_camera_internal(
    const k4a_calibration_t *calibration,
    const k4a_transformation_xy_tables_t *xy_tables_depth_camera,
    const uint8_t *depth_image_data,
    const k4a_transformation_image_descriptor_t *depth_image_descriptor,
    const uint8_t *custom_image_data,
    const k4a_transformation_image_descriptor_t *custom_image_descriptor,
    uint8_t *transformed_depth_image_data,
    k4a_transformation_image_descriptor_t *transformed_depth_image_descriptor,
    uint8_t *transformed_custom_image_data,
    k4a_transformation_image_descriptor_t *transformed_custom_image_descriptor,
    k4a_transformation_interpolation_type_t interpolation_type,
    uint32_t invalid_custom_value);

k4a_result_t transformation_depth_image_to_color_camera_custom(
    k4a_transformation_t transformation_handle,
    const uint8_t *depth_image_data,
    const k4a_transformation_image_descriptor_t *depth_image_descriptor,
    const uint8_t *custom_image_data,
    const k4a_transformation_image_descriptor_t *custom_image_descriptor,
    uint8_t *transformed_depth_image_data,
    k4a_transformation_image_descriptor_t *transformed_depth_image_descriptor,
    uint8_t *transformed_custom_image_data,
    k4a_transformation_image_descriptor_t *transformed_custom_image_descriptor,
    k4a_transformation_interpolation_type_t interpolation_type,
    uint32_t invalid_custom_value);

k4a_buffer_result_t transformation_color_image_to_depth_camera_validate_parameters(
    const k4a_calibration_t *calibration,
    const k4a_transformation_xy_tables_t *xy_tables_depth_camera,
    const uint8_t *depth_image_data,
    const k4a_transformation_image_descriptor_t *depth_image_descriptor,
    const uint8_t *color_image_data,
    const k4a_transformation_image_descriptor_t *color_image_descriptor,
    uint8_t *transformed_color_image_data,
    k4a_transformation_image_descriptor_t *transformed_color_image_descriptor);

k4a_buffer_result_t transformation_color_image_to_depth_camera_internal(
    const k4a_calibration_t *calibration,
    const k4a_transformation_xy_tables_t *xy_tables_depth_camera,
    const uint8_t *depth_image_data,
    const k4a_transformation_image_descriptor_t *depth_image_descriptor,
    const uint8_t *color_image_data,
    const k4a_transformation_image_descriptor_t *color_image_descriptor,
    uint8_t *transformed_color_image_data,
    k4a_transformation_image_descriptor_t *transformed_color_image_descriptor);

k4a_result_t
transformation_color_image_to_depth_camera(k4a_transformation_t transformation_handle,
                                           const uint8_t *depth_image_data,
                                           const k4a_transformation_image_descriptor_t *depth_image_descriptor,
                                           const uint8_t *color_image_data,
                                           const k4a_transformation_image_descriptor_t *color_image_descriptor,
                                           uint8_t *transformed_color_image_data,
                                           k4a_transformation_image_descriptor_t *transformed_color_image_descriptor);

k4a_buffer_result_t
transformation_depth_image_to_point_cloud_internal(k4a_transformation_xy_tables_t *xy_tables,
                                                   const uint8_t *depth_image_data,
                                                   const k4a_transformation_image_descriptor_t *depth_image_descriptor,
                                                   uint8_t *xyz_image_data,
                                                   k4a_transformation_image_descriptor_t *xyz_image_descriptor);

k4a_result_t
transformation_depth_image_to_point_cloud(k4a_transformation_t transformation_handle,
                                          const uint8_t *depth_image_data,
                                          const k4a_transformation_image_descriptor_t *depth_image_descriptor,
                                          const k4a_calibration_type_t camera,
                                          uint8_t *xyz_image_data,
                                          k4a_transformation_image_descriptor_t *xyz_image_descriptor);

// Mode specific calibration
k4a_result_t
transformation_get_mode_specific_depth_camera_calibration(const k4a_calibration_camera_t *raw_camera_calibration,
                                                          const k4a_depth_mode_t depth_mode,
                                                          k4a_calibration_camera_t *mode_specific_camera_calibration);

k4a_result_t
transformation_get_mode_specific_color_camera_calibration(const k4a_calibration_camera_t *raw_camera_calibration,
                                                          const k4a_color_resolution_t color_resolution,
                                                          k4a_calibration_camera_t *mode_specific_camera_calibration);

k4a_result_t
transformation_get_mode_specific_camera_calibration(const k4a_calibration_camera_t *raw_camera_calibration,
                                                    const k4a_camera_calibration_mode_info_t *mode_info,
                                                    k4a_calibration_camera_t *mode_specific_camera_calibration,
                                                    bool pixelized_zero_centered_output);

// Intrinsic transformations
k4a_result_t transformation_unproject(const k4a_calibration_camera_t *camera_calibration,
                                      const float point2d[2],
                                      const float depth,
                                      float point3d[3],
                                      int *valid);

k4a_result_t transformation_project(const k4a_calibration_camera_t *camera_calibration,
                                    const float point3d[3],
                                    float point2d[2],
                                    int *valid);

// Extrinsic transformations
k4a_result_t transformation_get_extrinsic_transformation(const k4a_calibration_extrinsics_t *source_camera_calibration,
                                                         const k4a_calibration_extrinsics_t *target_camera_calibration,
                                                         k4a_calibration_extrinsics_t *source_to_target);

k4a_result_t transformation_apply_extrinsic_transformation(const k4a_calibration_extrinsics_t *source_to_target,
                                                           const float source_point3d[3],
                                                           float target_point3d[3]);

#ifdef __cplusplus
}
#endif

#endif /* K4ATRANSFORMATION_H */
