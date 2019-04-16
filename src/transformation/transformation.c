// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4ainternal/logging.h>
#include <k4ainternal/deloader.h>
#include <k4ainternal/tewrapper.h>

// System dependencies
#include <stdlib.h>
#include <math.h>

k4a_result_t transformation_get_mode_specific_calibration(const k4a_calibration_camera_t *depth_camera_calibration,
                                                          const k4a_calibration_camera_t *color_camera_calibration,
                                                          const k4a_calibration_extrinsics_t *gyro_extrinsics,
                                                          const k4a_calibration_extrinsics_t *accel_extrinsics,
                                                          const k4a_depth_mode_t depth_mode,
                                                          const k4a_color_resolution_t color_resolution,
                                                          k4a_calibration_t *calibration)
{
    memset(&calibration->color_camera_calibration, 0, sizeof(k4a_calibration_camera_t));
    memset(&calibration->depth_camera_calibration, 0, sizeof(k4a_calibration_camera_t));

    if (K4A_FAILED(
            K4A_RESULT_FROM_BOOL(color_resolution != K4A_COLOR_RESOLUTION_OFF || depth_mode != K4A_DEPTH_MODE_OFF)))
    {
        LOG_ERROR("Expect color or depth camera is running.", 0);
        return K4A_RESULT_FAILED;
    }

    if (depth_mode != K4A_DEPTH_MODE_OFF)
    {
        if (K4A_FAILED(TRACE_CALL(transformation_get_mode_specific_depth_camera_calibration(
                depth_camera_calibration, depth_mode, &calibration->depth_camera_calibration))))
        {
            return K4A_RESULT_FAILED;
        }
    }

    if (color_resolution != K4A_COLOR_RESOLUTION_OFF)
    {
        if (K4A_FAILED(TRACE_CALL(transformation_get_mode_specific_color_camera_calibration(
                color_camera_calibration, color_resolution, &calibration->color_camera_calibration))))
        {
            return K4A_RESULT_FAILED;
        }
    }

    const k4a_calibration_extrinsics_t *extrinsics[K4A_CALIBRATION_TYPE_NUM];
    extrinsics[K4A_CALIBRATION_TYPE_DEPTH] = &calibration->depth_camera_calibration.extrinsics;
    extrinsics[K4A_CALIBRATION_TYPE_COLOR] = &calibration->color_camera_calibration.extrinsics;
    extrinsics[K4A_CALIBRATION_TYPE_GYRO] = gyro_extrinsics;
    extrinsics[K4A_CALIBRATION_TYPE_ACCEL] = accel_extrinsics;

    for (int source = 0; source < (int)K4A_CALIBRATION_TYPE_NUM; source++)
    {
        for (int target = 0; target < (int)K4A_CALIBRATION_TYPE_NUM; target++)
        {
            if (K4A_FAILED(
                    TRACE_CALL(transformation_get_extrinsic_transformation(extrinsics[source],
                                                                           extrinsics[target],
                                                                           &calibration->extrinsics[source][target]))))
            {
                return K4A_RESULT_FAILED;
            }
        }
    }

    calibration->depth_mode = depth_mode;
    calibration->color_resolution = color_resolution;

    return K4A_RESULT_SUCCEEDED;
}

static k4a_result_t transformation_possible(const k4a_calibration_t *camera_calibration,
                                            const k4a_calibration_type_t camera)
{
    if (camera >= K4A_CALIBRATION_TYPE_NUM || camera <= K4A_CALIBRATION_TYPE_UNKNOWN)
    {
        LOG_ERROR("Unexpected camera calibration type %d.", camera);
        return K4A_RESULT_FAILED;
    }
    if (camera == K4A_CALIBRATION_TYPE_DEPTH && camera_calibration->depth_mode == K4A_DEPTH_MODE_OFF)
    {
        LOG_ERROR("Expect depth camera is running to perform transformation.", 0);
        return K4A_RESULT_FAILED;
    }
    if (camera == K4A_CALIBRATION_TYPE_COLOR && camera_calibration->color_resolution == K4A_COLOR_RESOLUTION_OFF)
    {
        LOG_ERROR("Expect color camera is running to perform transformation.", 0);
        return K4A_RESULT_FAILED;
    }
    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t transformation_3d_to_3d(const k4a_calibration_t *calibration,
                                     const float source_point3d[3],
                                     const k4a_calibration_type_t source_camera,
                                     const k4a_calibration_type_t target_camera,
                                     float target_point3d[3])
{
    if (K4A_FAILED(TRACE_CALL(transformation_possible(calibration, source_camera))) ||
        K4A_FAILED(TRACE_CALL(transformation_possible(calibration, target_camera))))
    {
        return K4A_RESULT_FAILED;
    }

    if (source_camera == target_camera)
    {
        target_point3d[0] = source_point3d[0];
        target_point3d[1] = source_point3d[1];
        target_point3d[2] = source_point3d[2];
        return K4A_RESULT_SUCCEEDED;
    }

    if (K4A_FAILED(TRACE_CALL(transformation_apply_extrinsic_transformation(
            &calibration->extrinsics[source_camera][target_camera], source_point3d, target_point3d))))
    {
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t transformation_2d_to_3d(const k4a_calibration_t *calibration,
                                     const float source_point2d[2],
                                     const float source_depth,
                                     const k4a_calibration_type_t source_camera,
                                     const k4a_calibration_type_t target_camera,
                                     float target_point3d[3],
                                     int *valid)
{
    if (K4A_FAILED(TRACE_CALL(transformation_possible(calibration, source_camera))))
    {
        return K4A_RESULT_FAILED;
    }

    if (source_camera == K4A_CALIBRATION_TYPE_DEPTH)
    {
        if (K4A_FAILED(TRACE_CALL(transformation_unproject(
                &calibration->depth_camera_calibration, source_point2d, source_depth, target_point3d, valid))))
        {
            return K4A_RESULT_FAILED;
        }
    }
    else if (source_camera == K4A_CALIBRATION_TYPE_COLOR)
    {
        if (K4A_FAILED(TRACE_CALL(transformation_unproject(
                &calibration->color_camera_calibration, source_point2d, source_depth, target_point3d, valid))))
        {
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        LOG_ERROR("Unexpected source camera calibration type %d, should either be K4A_CALIBRATION_TYPE_DEPTH (%d) or "
                  "K4A_CALIBRATION_TYPE_COLOR (%d).",
                  source_camera,
                  K4A_CALIBRATION_TYPE_DEPTH,
                  K4A_CALIBRATION_TYPE_COLOR);
        return K4A_RESULT_FAILED; // unproject only supported for depth and color cameras
    }

    if (source_camera == target_camera)
    {
        return K4A_RESULT_SUCCEEDED;
    }
    else
    {
        return TRACE_CALL(
            transformation_3d_to_3d(calibration, target_point3d, source_camera, target_camera, target_point3d));
    }
}

k4a_result_t transformation_3d_to_2d(const k4a_calibration_t *calibration,
                                     const float source_point3d[3],
                                     const k4a_calibration_type_t source_camera,
                                     const k4a_calibration_type_t target_camera,
                                     float target_point2d[2],
                                     int *valid)
{
    if (K4A_FAILED(TRACE_CALL(transformation_possible(calibration, target_camera))))
    {
        return K4A_RESULT_FAILED;
    }

    float target_point3d[3];
    if (source_camera == target_camera)
    {
        target_point3d[0] = source_point3d[0];
        target_point3d[1] = source_point3d[1];
        target_point3d[2] = source_point3d[2];
    }
    else
    {
        if (K4A_FAILED(TRACE_CALL(
                transformation_3d_to_3d(calibration, source_point3d, source_camera, target_camera, target_point3d))))
        {
            return K4A_RESULT_FAILED;
        }
    }

    if (target_camera == K4A_CALIBRATION_TYPE_DEPTH)
    {
        return TRACE_CALL(
            transformation_project(&calibration->depth_camera_calibration, target_point3d, target_point2d, valid));
    }
    else if (target_camera == K4A_CALIBRATION_TYPE_COLOR)
    {
        return TRACE_CALL(
            transformation_project(&calibration->color_camera_calibration, target_point3d, target_point2d, valid));
    }
    else
    {
        LOG_ERROR("Unexpected target camera calibration type %d, should either be K4A_CALIBRATION_TYPE_DEPTH (%d) or "
                  "K4A_CALIBRATION_TYPE_COLOR (%d).",
                  target_camera,
                  K4A_CALIBRATION_TYPE_DEPTH,
                  K4A_CALIBRATION_TYPE_COLOR);
        return K4A_RESULT_FAILED; // project only supported for depth and color cameras
    }
}

k4a_result_t transformation_2d_to_2d(const k4a_calibration_t *calibration,
                                     const float source_point2d[2],
                                     const float source_depth,
                                     const k4a_calibration_type_t source_camera,
                                     const k4a_calibration_type_t target_camera,
                                     float target_point2d[2],
                                     int *valid)
{
    if (source_camera == target_camera)
    {
        target_point2d[0] = source_point2d[0];
        target_point2d[1] = source_point2d[1];
        *valid = 1;
        return K4A_RESULT_SUCCEEDED;
    }

    float target_point3d[3];
    if (K4A_FAILED(TRACE_CALL(transformation_2d_to_3d(
            calibration, source_point2d, source_depth, source_camera, target_camera, target_point3d, valid))))
    {
        return K4A_RESULT_FAILED;
    }
    int valid_transformation1 = *valid;

    if (K4A_FAILED(TRACE_CALL(
            transformation_3d_to_2d(calibration, target_point3d, target_camera, target_camera, target_point2d, valid))))
    {
        return K4A_RESULT_FAILED;
    }
    *valid = *valid && valid_transformation1;

    return K4A_RESULT_SUCCEEDED;
}

static k4a_buffer_result_t transformation_init_xy_tables(const k4a_calibration_t *calibration,
                                                         const k4a_calibration_type_t camera,
                                                         float *data,
                                                         size_t *data_size,
                                                         k4a_transformation_xy_tables_t *xy_tables)
{
    int width = 0;
    int height = 0;
    switch (camera)
    {
    case K4A_CALIBRATION_TYPE_DEPTH:
        width = calibration->depth_camera_calibration.resolution_width;
        height = calibration->depth_camera_calibration.resolution_height;
        break;
    case K4A_CALIBRATION_TYPE_COLOR:
        width = calibration->color_camera_calibration.resolution_width;
        height = calibration->color_camera_calibration.resolution_height;
        break;
    default:
        LOG_ERROR("Unexpected camera calibration type %d, should either be K4A_CALIBRATION_TYPE_DEPTH (%d) or "
                  "K4A_CALIBRATION_TYPE_COLOR (%d).",
                  camera,
                  K4A_CALIBRATION_TYPE_DEPTH,
                  K4A_CALIBRATION_TYPE_COLOR);
        return K4A_BUFFER_RESULT_FAILED;
    }

    size_t table_size = (size_t)(width * height);
    if (data == NULL)
    {
        (*data_size) = 2 * table_size;
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }
    else
    {
        if (K4A_FAILED(K4A_RESULT_FROM_BOOL(*data_size >= 2 * table_size)))
        {
            LOG_ERROR("Unexpected xy table size %d, should be larger or equal than %d.", *data_size, 2 * table_size);
            return K4A_BUFFER_RESULT_FAILED;
        }

        xy_tables->width = width;
        xy_tables->height = height;
        xy_tables->x_table = data;
        xy_tables->y_table = data + table_size;

        float point2d[2], point3d[3];
        int valid = 1;

        for (int y = 0, idx = 0; y < height; y++)
        {
            point2d[1] = (float)y;
            for (int x = 0; x < width; x++, idx++)
            {
                point2d[0] = (float)x;
                if (K4A_FAILED(TRACE_CALL(
                        transformation_2d_to_3d(calibration, point2d, 1.f, camera, camera, point3d, &valid))))
                {
                    return K4A_BUFFER_RESULT_FAILED;
                }

                if (valid == 0)
                {
                    // x table value of NAN marks invalid
                    xy_tables->x_table[idx] = NAN;
                    // set y table value to 0 to speed up SSE implementation
                    xy_tables->y_table[idx] = 0.f;
                }
                else
                {
                    xy_tables->x_table[idx] = point3d[0];
                    xy_tables->y_table[idx] = point3d[1];
                }
            }
        }
        (*data_size) = 2 * table_size;
        return K4A_BUFFER_RESULT_SUCCEEDED;
    }
}

static k4a_result_t transformation_allocate_xy_tables(const k4a_calibration_t *calibration,
                                                      k4a_calibration_type_t camera,
                                                      float **buffer,
                                                      k4a_transformation_xy_tables_t *xy_tables)
{
    *buffer = 0;
    size_t xy_tables_data_size = 0;
    if (K4A_BUFFER_RESULT_TOO_SMALL !=
        TRACE_BUFFER_CALL(transformation_init_xy_tables(calibration, camera, *buffer, &xy_tables_data_size, xy_tables)))
    {
        return K4A_RESULT_FAILED;
    }

    *buffer = malloc(xy_tables_data_size * sizeof(float));

    if (K4A_BUFFER_RESULT_SUCCEEDED !=
        TRACE_BUFFER_CALL(transformation_init_xy_tables(calibration, camera, *buffer, &xy_tables_data_size, xy_tables)))
    {
        return K4A_RESULT_FAILED;
    }
    return K4A_RESULT_SUCCEEDED;
}

typedef struct _k4a_transformation_context_t
{
    k4a_calibration_t calibration;
    k4a_transformation_xy_tables_t depth_camera_xy_tables;
    float *memory_depth_camera_xy_tables;
    k4a_transformation_xy_tables_t color_camera_xy_tables;
    float *memory_color_camera_xy_tables;
    bool enable_gpu_optimization;
    bool enable_depth_color_transform;
    tewrapper_t tewrapper;
} k4a_transformation_context_t;

K4A_DECLARE_CONTEXT(k4a_transformation_t, k4a_transformation_context_t);

k4a_transformation_t transformation_create(const k4a_calibration_t *calibration, bool gpu_optimization)
{
    k4a_transformation_t transformation_handle = NULL;
    k4a_transformation_context_t *transformation_context = k4a_transformation_t_create(&transformation_handle);

    memcpy(&transformation_context->calibration, calibration, sizeof(k4a_calibration_t));

    if (K4A_FAILED(TRACE_CALL(transformation_allocate_xy_tables(&transformation_context->calibration,
                                                                K4A_CALIBRATION_TYPE_DEPTH,
                                                                &transformation_context->memory_depth_camera_xy_tables,
                                                                &transformation_context->depth_camera_xy_tables))))
    {
        transformation_destroy(transformation_handle);
        return 0;
    }

    if (K4A_FAILED(TRACE_CALL(transformation_allocate_xy_tables(&transformation_context->calibration,
                                                                K4A_CALIBRATION_TYPE_COLOR,
                                                                &transformation_context->memory_color_camera_xy_tables,
                                                                &transformation_context->color_camera_xy_tables))))
    {
        transformation_destroy(transformation_handle);
        return 0;
    }

    transformation_context->enable_gpu_optimization = gpu_optimization;
    transformation_context->enable_depth_color_transform = transformation_context->calibration.color_resolution !=
                                                               K4A_COLOR_RESOLUTION_OFF &&
                                                           transformation_context->calibration.depth_mode !=
                                                               K4A_DEPTH_MODE_OFF;
    if (transformation_context->enable_gpu_optimization && transformation_context->enable_depth_color_transform)
    {
        // Set up transform engine expected calibration struct
        k4a_transform_engine_calibration_t transform_engine_calibration;
        memcpy(&transform_engine_calibration.depth_camera_calibration,
               &transformation_context->calibration.depth_camera_calibration,
               sizeof(k4a_calibration_camera_t));
        memcpy(&transform_engine_calibration.color_camera_calibration,
               &transformation_context->calibration.color_camera_calibration,
               sizeof(k4a_calibration_camera_t));
        memcpy(&transform_engine_calibration.depth_camera_to_color_camera_extrinsics,
               &transformation_context->calibration.extrinsics[K4A_CALIBRATION_TYPE_DEPTH][K4A_CALIBRATION_TYPE_COLOR],
               sizeof(k4a_calibration_extrinsics_t));
        memcpy(&transform_engine_calibration.color_camera_to_depth_camera_extrinsics,
               &transformation_context->calibration.extrinsics[K4A_CALIBRATION_TYPE_COLOR][K4A_CALIBRATION_TYPE_DEPTH],
               sizeof(k4a_calibration_extrinsics_t));
        memcpy(&transform_engine_calibration.depth_camera_xy_tables,
               &transformation_context->depth_camera_xy_tables,
               sizeof(k4a_transformation_xy_tables_t));

        transformation_context->tewrapper = tewrapper_create(&transform_engine_calibration);
        if (K4A_FAILED(K4A_RESULT_FROM_BOOL(transformation_context->tewrapper != NULL)))
        {
            transformation_destroy(transformation_handle);
            return 0;
        }
    }

    return transformation_handle;
}

void transformation_destroy(k4a_transformation_t transformation_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_transformation_t, transformation_handle);
    k4a_transformation_context_t *transformation_context = k4a_transformation_t_get_context(transformation_handle);

    if (transformation_context->memory_depth_camera_xy_tables != 0)
    {
        free(transformation_context->memory_depth_camera_xy_tables);
    }
    if (transformation_context->memory_color_camera_xy_tables != 0)
    {
        free(transformation_context->memory_color_camera_xy_tables);
    }
    if (transformation_context->tewrapper)
    {
        tewrapper_destroy(transformation_context->tewrapper);
    }
    k4a_transformation_t_destroy(transformation_handle);
}

k4a_result_t
transformation_depth_image_to_color_camera(k4a_transformation_t transformation_handle,
                                           const uint8_t *depth_image_data,
                                           const k4a_transformation_image_descriptor_t *depth_image_descriptor,
                                           uint8_t *transformed_depth_image_data,
                                           k4a_transformation_image_descriptor_t *transformed_depth_image_descriptor)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_transformation_t, transformation_handle);
    k4a_transformation_context_t *transformation_context = k4a_transformation_t_get_context(transformation_handle);

    if (!transformation_context->enable_depth_color_transform)
    {
        LOG_ERROR("Expect both depth camera and color camera are running to transform depth image to color camera.", 0);
        return K4A_RESULT_FAILED;
    }

    if (transformation_context->enable_gpu_optimization)
    {
        if (K4A_BUFFER_RESULT_SUCCEEDED !=
            TRACE_BUFFER_CALL(transformation_depth_image_to_color_camera_validate_parameters(
                &transformation_context->calibration,
                &transformation_context->depth_camera_xy_tables,
                depth_image_data,
                depth_image_descriptor,
                transformed_depth_image_data,
                transformed_depth_image_descriptor)))
        {
            return K4A_RESULT_FAILED;
        }

        size_t depth_image_size = (size_t)(depth_image_descriptor->stride_bytes *
                                           depth_image_descriptor->height_pixels);
        size_t transformed_depth_image_size = (size_t)(transformed_depth_image_descriptor->stride_bytes *
                                                       transformed_depth_image_descriptor->height_pixels);

        if (K4A_FAILED(TRACE_CALL(tewrapper_process_frame(transformation_context->tewrapper,
                                                          K4A_TRANSFORM_ENGINE_TYPE_DEPTH_TO_COLOR,
                                                          depth_image_data,
                                                          depth_image_size,
                                                          NULL, // Color image data is not needed when transform depth
                                                                // image to the geometry of color camera
                                                          0,
                                                          transformed_depth_image_data,
                                                          transformed_depth_image_size))))
        {
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        if (K4A_BUFFER_RESULT_SUCCEEDED !=
            TRACE_BUFFER_CALL(
                transformation_depth_image_to_color_camera_internal(&transformation_context->calibration,
                                                                    &transformation_context->depth_camera_xy_tables,
                                                                    depth_image_data,
                                                                    depth_image_descriptor,
                                                                    transformed_depth_image_data,
                                                                    transformed_depth_image_descriptor)))
        {
            return K4A_RESULT_FAILED;
        }
    }
    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t
transformation_color_image_to_depth_camera(k4a_transformation_t transformation_handle,
                                           const uint8_t *depth_image_data,
                                           const k4a_transformation_image_descriptor_t *depth_image_descriptor,
                                           const uint8_t *color_image_data,
                                           const k4a_transformation_image_descriptor_t *color_image_descriptor,
                                           uint8_t *transformed_color_image_data,
                                           k4a_transformation_image_descriptor_t *transformed_color_image_descriptor)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_transformation_t, transformation_handle);
    k4a_transformation_context_t *transformation_context = k4a_transformation_t_get_context(transformation_handle);

    if (!transformation_context->enable_depth_color_transform)
    {
        LOG_ERROR("Expect both depth camera and color camera are running to transform color image to depth camera.", 0);
        return K4A_RESULT_FAILED;
    }

    if (transformation_context->enable_gpu_optimization)
    {
        if (K4A_BUFFER_RESULT_SUCCEEDED !=
            TRACE_BUFFER_CALL(transformation_color_image_to_depth_camera_validate_parameters(
                &transformation_context->calibration,
                &transformation_context->depth_camera_xy_tables,
                depth_image_data,
                depth_image_descriptor,
                color_image_data,
                color_image_descriptor,
                transformed_color_image_data,
                transformed_color_image_descriptor)))
        {
            return K4A_RESULT_FAILED;
        }

        size_t depth_image_size = (size_t)(depth_image_descriptor->stride_bytes *
                                           depth_image_descriptor->height_pixels);
        size_t color_image_size = (size_t)(color_image_descriptor->stride_bytes *
                                           color_image_descriptor->height_pixels);
        size_t transformed_color_image_size = (size_t)(transformed_color_image_descriptor->stride_bytes *
                                                       transformed_color_image_descriptor->height_pixels);

        if (K4A_FAILED(TRACE_CALL(tewrapper_process_frame(transformation_context->tewrapper,
                                                          K4A_TRANSFORM_ENGINE_TYPE_COLOR_TO_DEPTH,
                                                          depth_image_data,
                                                          depth_image_size,
                                                          color_image_data,
                                                          color_image_size,
                                                          transformed_color_image_data,
                                                          transformed_color_image_size))))
        {
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        if (K4A_BUFFER_RESULT_SUCCEEDED !=
            TRACE_BUFFER_CALL(
                transformation_color_image_to_depth_camera_internal(&transformation_context->calibration,
                                                                    &transformation_context->depth_camera_xy_tables,
                                                                    depth_image_data,
                                                                    depth_image_descriptor,
                                                                    color_image_data,
                                                                    color_image_descriptor,
                                                                    transformed_color_image_data,
                                                                    transformed_color_image_descriptor)))
        {
            return K4A_RESULT_FAILED;
        }
    }
    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t
transformation_depth_image_to_point_cloud(k4a_transformation_t transformation_handle,
                                          const uint8_t *depth_image_data,
                                          const k4a_transformation_image_descriptor_t *depth_image_descriptor,
                                          const k4a_calibration_type_t camera,
                                          uint8_t *xyz_image_data,
                                          k4a_transformation_image_descriptor_t *xyz_image_descriptor)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_transformation_t, transformation_handle);
    k4a_transformation_context_t *transformation_context = k4a_transformation_t_get_context(transformation_handle);

    k4a_transformation_xy_tables_t *xy_tables;
    if (camera == K4A_CALIBRATION_TYPE_DEPTH)
    {
        xy_tables = &transformation_context->depth_camera_xy_tables;
    }
    else if (camera == K4A_CALIBRATION_TYPE_COLOR)
    {
        xy_tables = &transformation_context->color_camera_xy_tables;
    }
    else
    {
        LOG_ERROR("Unexpected camera calibration type %d, should either be K4A_CALIBRATION_TYPE_DEPTH (%d) or "
                  "K4A_CALIBRATION_TYPE_COLOR (%d).",
                  camera,
                  K4A_CALIBRATION_TYPE_DEPTH,
                  K4A_CALIBRATION_TYPE_COLOR);
        return K4A_RESULT_FAILED;
    }

    if (K4A_BUFFER_RESULT_SUCCEEDED !=
        TRACE_BUFFER_CALL(transformation_depth_image_to_point_cloud_internal(
            xy_tables, depth_image_data, depth_image_descriptor, xyz_image_data, xyz_image_descriptor)))
    {
        return K4A_RESULT_FAILED;
    }
    return K4A_RESULT_SUCCEEDED;
}
