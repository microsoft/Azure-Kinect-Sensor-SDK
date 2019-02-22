// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4ainternal/transformation.h>
#include <k4ainternal/logging.h>

#include <stdlib.h>
#include <limits.h>
#include <math.h>

typedef struct _k4a_transformation_input_image_t
{
    const k4a_transformation_image_descriptor_t *descriptor;
    const uint8_t *data_uint8;
    const uint16_t *data_uint16;
} k4a_transformation_input_image_t;

typedef struct _k4a_transformation_output_image_t
{
    k4a_transformation_image_descriptor_t *descriptor;
    uint8_t *data_uint8;
    uint16_t *data_uint16;
} k4a_transformation_output_image_t;

typedef struct _k4a_transformation_rgbz_context_t
{
    const k4a_calibration_t *calibration;
    const k4a_transformation_xy_tables_t *xy_tables;
    k4a_transformation_input_image_t depth_image;
    k4a_transformation_input_image_t color_image;
    k4a_transformation_output_image_t transformed_image;
} k4a_transformation_rgbz_context_t;

typedef struct _k4a_correspondence_t
{
    k4a_float2_t point2d;
    float depth;
    int valid;
} k4a_correspondence_t;

typedef struct _k4a_bounding_box_t
{
    int top_left[2];
    int bottom_right[2];
} k4a_bounding_box_t;

static k4a_transformation_image_descriptor_t transformation_init_image_descriptor(int width, int height, int stride)
{
    k4a_transformation_image_descriptor_t descriptor;
    descriptor.width_pixels = width;
    descriptor.height_pixels = height;
    descriptor.stride_bytes = stride;
    return descriptor;
}

static bool transformation_compare_image_descriptors(const k4a_transformation_image_descriptor_t *descriptor1,
                                                     const k4a_transformation_image_descriptor_t *descriptor2)
{
    if (descriptor1->width_pixels != descriptor2->width_pixels ||
        descriptor1->height_pixels != descriptor2->height_pixels ||
        descriptor1->stride_bytes != descriptor2->stride_bytes)
    {
        return false;
    }
    return true;
}

static k4a_transformation_input_image_t
transformation_init_input_image(const k4a_transformation_image_descriptor_t *descriptor, const uint8_t *data)
{
    k4a_transformation_input_image_t image;
    image.descriptor = descriptor;
    image.data_uint8 = data;
    image.data_uint16 = (const uint16_t *)(const void *)data;
    return image;
}

static k4a_transformation_output_image_t
transformation_init_output_image(k4a_transformation_image_descriptor_t *descriptor, uint8_t *data)
{
    k4a_transformation_output_image_t image;
    image.descriptor = descriptor;
    image.data_uint8 = data;
    image.data_uint16 = (uint16_t *)(void *)data;
    return image;
}

static k4a_result_t transformation_compute_correspondence(const int depth_index,
                                                          const uint16_t depth,
                                                          const k4a_transformation_rgbz_context_t *context,
                                                          k4a_correspondence_t *correspondence)
{
    if (depth == 0)
    {
        memset(correspondence, 0, sizeof(k4a_correspondence_t));
        return K4A_RESULT_SUCCEEDED;
    }

    k4a_float3_t depth_point3d;
    depth_point3d.xyz.z = (float)depth;
    depth_point3d.xyz.x = context->xy_tables->x_table[depth_index] * depth_point3d.xyz.z;
    depth_point3d.xyz.y = context->xy_tables->y_table[depth_index] * depth_point3d.xyz.z;

    k4a_float3_t color_point3d;
    if (K4A_FAILED(TRACE_CALL(transformation_3d_to_3d(context->calibration,
                                                      depth_point3d.v,
                                                      K4A_CALIBRATION_TYPE_DEPTH,
                                                      K4A_CALIBRATION_TYPE_COLOR,
                                                      color_point3d.v))))
    {
        return K4A_RESULT_FAILED;
    }
    correspondence->depth = color_point3d.xyz.z;

    if (K4A_FAILED(TRACE_CALL(transformation_3d_to_2d(context->calibration,
                                                      color_point3d.v,
                                                      K4A_CALIBRATION_TYPE_COLOR,
                                                      K4A_CALIBRATION_TYPE_COLOR,
                                                      correspondence->point2d.v,
                                                      &correspondence->valid))))
    {
        return K4A_RESULT_FAILED;
    }
    return K4A_RESULT_SUCCEEDED;
}

static inline int transformation_min2(const int v1, const int v2)
{
    return (v1 < v2) ? v1 : v2;
}

static inline int transformation_max2(const int v1, const int v2)
{
    return (v1 > v2) ? v1 : v2;
}

static inline float transformation_min2f(const float v1, const float v2)
{
    return (v1 < v2) ? v1 : v2;
}

static inline float transformation_max2f(const float v1, const float v2)
{
    return (v1 > v2) ? v1 : v2;
}

static inline float transformation_min4f(const float v1, const float v2, const float v3, const float v4)
{
    return transformation_min2f(transformation_min2f(v1, v2), transformation_min2f(v3, v4));
}

static inline float transformation_max4f(const float v1, const float v2, const float v3, const float v4)
{
    return transformation_max2f(transformation_max2f(v1, v2), transformation_max2f(v3, v4));
}

static k4a_bounding_box_t transformation_compute_bounding_box(const k4a_correspondence_t *v1,
                                                              const k4a_correspondence_t *v2,
                                                              const k4a_correspondence_t *v3,
                                                              const k4a_correspondence_t *v4,
                                                              int width,
                                                              int height)
{
    k4a_bounding_box_t bounding_box;

    float x_min = transformation_min4f(v1->point2d.xy.x, v2->point2d.xy.x, v3->point2d.xy.x, v4->point2d.xy.x);
    float y_min = transformation_min4f(v1->point2d.xy.y, v2->point2d.xy.y, v3->point2d.xy.y, v4->point2d.xy.y);
    float x_max = transformation_max4f(v1->point2d.xy.x, v2->point2d.xy.x, v3->point2d.xy.x, v4->point2d.xy.x);
    float y_max = transformation_max4f(v1->point2d.xy.y, v2->point2d.xy.y, v3->point2d.xy.y, v4->point2d.xy.y);

    bounding_box.top_left[0] = transformation_max2((int)(ceilf(x_min)), 0);
    bounding_box.top_left[1] = transformation_max2((int)(ceilf(y_min)), 0);
    bounding_box.bottom_right[0] = transformation_min2((int)(ceilf(x_max)), width);
    bounding_box.bottom_right[1] = transformation_min2((int)(ceilf(y_max)), height);

    return bounding_box;
}

static inline k4a_correspondence_t transformation_interpolate_correspondences(const k4a_correspondence_t *v1,
                                                                              const k4a_correspondence_t *v2)
{
    k4a_correspondence_t result;

    result.point2d.xy.x = (v1->point2d.xy.x + v2->point2d.xy.x) * 0.5f;
    result.point2d.xy.y = (v1->point2d.xy.y + v2->point2d.xy.y) * 0.5f;
    result.depth = (v1->depth + v2->depth) * 0.5f;
    result.valid = v1->valid & v2->valid;

    return result;
}

static bool transformation_check_valid_correspondences(const k4a_correspondence_t *top_left,
                                                       const k4a_correspondence_t *top_right,
                                                       const k4a_correspondence_t *bottom_right,
                                                       const k4a_correspondence_t *bottom_left,
                                                       k4a_correspondence_t *valid_top_left,
                                                       k4a_correspondence_t *valid_top_right,
                                                       k4a_correspondence_t *valid_bottom_right,
                                                       k4a_correspondence_t *valid_bottom_left)
{
    *valid_top_left = *top_left;
    *valid_top_right = *top_right;
    *valid_bottom_right = *bottom_right;
    *valid_bottom_left = *bottom_left;

    // Check if a vertex is invalid and replace invalid ones with either existing
    // or interpolated vertices. Make sure the winding order of vertices stays clockwise.
    int num_invalid = 0;

    if (top_left->valid == 0)
    {
        num_invalid++;
        *valid_top_left = transformation_interpolate_correspondences(top_right, bottom_left);
    }
    if (top_right->valid == 0)
    {
        num_invalid++;
        *valid_top_right = *bottom_right;
        *valid_bottom_right = transformation_interpolate_correspondences(bottom_right, bottom_left);
    }
    if (bottom_right->valid == 0)
    {
        num_invalid++;
        *valid_bottom_right = transformation_interpolate_correspondences(top_right, bottom_left);
    }
    if (bottom_left->valid == 0)
    {
        num_invalid++;
        *valid_bottom_left = *bottom_right;
        *valid_bottom_right = transformation_interpolate_correspondences(top_right, bottom_right);
    }

    // If two or more vertices are invalid then we can't create a valid triangle
    return num_invalid < 2;
}

static inline float transformation_area_function(const k4a_float2_t *a, const k4a_float2_t *b, const k4a_float2_t *c)
{
    // Calculate area of parallelogram defined by vectors (ab) and (ac).
    // Result will be negative if vertex c is on the left side of vector (ab).
    return (c->xy.y - a->xy.y) * (b->xy.x - a->xy.x) - (c->xy.x - a->xy.x) * (b->xy.y - a->xy.y);
}

static bool transformation_point_inside_triangle(const k4a_correspondence_t *valid_top_left,
                                                 const k4a_correspondence_t *valid_intermediate,
                                                 const k4a_correspondence_t *valid_bottom_right,
                                                 const k4a_float2_t *point,
                                                 float area_intermediate,
                                                 bool counter_clockwise,
                                                 float *depth)
{
    // Calculate sub triangle areas
    float area_top_left = transformation_area_function(&valid_intermediate->point2d, &valid_top_left->point2d, point);
    float area_bottom_right = transformation_area_function(&valid_bottom_right->point2d,
                                                           &valid_intermediate->point2d,
                                                           point);

    // Check if point is inside the triangle (area is positive).
    // If counter_clockwise order is not set then we need to negate the areas.
    // Top/left edge is inclusive (>= 0) while bottom/right edge is exclusive (> 0).
    if (((counter_clockwise ? area_top_left : -area_top_left) >= 0.0f) &&
        ((counter_clockwise ? area_bottom_right : -area_bottom_right) > 0.0f))
    {
        // Calculate sum of areas and check divide by zero
        float sum_weights = area_top_left + area_intermediate + area_bottom_right;
        if (sum_weights != 0.0f)
        {
            sum_weights = 1.0f / sum_weights;
        }

        // Linear interpolatation of depth using area_top_left, area_intermediate, area_bottom_right
        *depth = (area_top_left * valid_bottom_right->depth + area_intermediate * valid_intermediate->depth +
                  area_bottom_right * valid_top_left->depth) *
                 sum_weights;

        return true;
    }

    return false;
}

static bool transformation_point_inside_quad(const k4a_correspondence_t *valid_top_left,
                                             const k4a_correspondence_t *valid_top_right,
                                             const k4a_correspondence_t *valid_bottom_right,
                                             const k4a_correspondence_t *valid_bottom_left,
                                             const k4a_float2_t *point,
                                             float *depth)
{
    // Calculate area to see if point is to the left or right of vector (valid_top_left - valid_bottom_right).
    // Set counter_clockwise flag true for all positions to the right of the aforementioned vector.
    float area_intermediate = transformation_area_function(&valid_top_left->point2d,
                                                           &valid_bottom_right->point2d,
                                                           point);
    bool counter_clockwise = (area_intermediate >= 0.0f);

    // Interpolate depth using either the right or left triangle
    return transformation_point_inside_triangle(valid_top_left,
                                                counter_clockwise ? valid_bottom_left : valid_top_right,
                                                valid_bottom_right,
                                                point,
                                                area_intermediate,
                                                counter_clockwise,
                                                depth);
}

static void transformation_draw_rectangle(const k4a_bounding_box_t *bounding_box,
                                          const k4a_correspondence_t *valid_top_left,
                                          const k4a_correspondence_t *valid_top_right,
                                          const k4a_correspondence_t *valid_bottom_right,
                                          const k4a_correspondence_t *valid_bottom_left,
                                          k4a_transformation_output_image_t *image)
{
    k4a_float2_t point;
    for (int y = bounding_box->top_left[1]; y < bounding_box->bottom_right[1]; y++)
    {
        uint16_t *row = image->data_uint16 + y * image->descriptor->width_pixels;
        point.xy.y = (float)y;

        for (int x = bounding_box->top_left[0]; x < bounding_box->bottom_right[0]; x++)
        {
            point.xy.x = (float)x;

            float interpolated_depth = 0.0f;
            if (transformation_point_inside_quad(valid_top_left,
                                                 valid_top_right,
                                                 valid_bottom_right,
                                                 valid_bottom_left,
                                                 &point,
                                                 &interpolated_depth))
            {
                uint16_t depth = (uint16_t)(interpolated_depth + 0.5f);

                // handle occlusions
                if (row[x] == 0 || (depth < row[x]))
                {
                    row[x] = depth;
                }
            }
        }
    }
}

static k4a_result_t transformation_depth_to_color(k4a_transformation_rgbz_context_t *context)
{
    memset(context->transformed_image.data_uint8,
           0,
           (size_t)(context->transformed_image.descriptor->stride_bytes *
                    context->transformed_image.descriptor->height_pixels));

    k4a_correspondence_t *vertex_row = (k4a_correspondence_t *)malloc(
        (size_t)context->depth_image.descriptor->width_pixels * sizeof(k4a_correspondence_t));

    int idx = 0;
    for (; idx < context->depth_image.descriptor->width_pixels; idx++)
    {
        if (K4A_FAILED(TRACE_CALL(transformation_compute_correspondence(
                idx, context->depth_image.data_uint16[idx], context, vertex_row + idx))))
        {
            free(vertex_row);
            return K4A_RESULT_FAILED;
        }
    }

    for (int y = 1; y < context->depth_image.descriptor->height_pixels; y++)
    {
        k4a_correspondence_t top_left = vertex_row[0];
        k4a_correspondence_t bottom_left;
        if (K4A_FAILED(TRACE_CALL(transformation_compute_correspondence(
                idx, context->depth_image.data_uint16[idx], context, &bottom_left))))
        {
            free(vertex_row);
            return K4A_RESULT_FAILED;
        }
        idx++;
        vertex_row[0] = bottom_left;

        for (int x = 1; x < context->depth_image.descriptor->width_pixels; x++, idx++)
        {
            k4a_correspondence_t top_right = vertex_row[x];
            k4a_correspondence_t bottom_right;
            if (K4A_FAILED(TRACE_CALL(transformation_compute_correspondence(
                    idx, context->depth_image.data_uint16[idx], context, &bottom_right))))
            {
                free(vertex_row);
                return K4A_RESULT_FAILED;
            }

            k4a_correspondence_t valid_top_left, valid_top_right, valid_bottom_right, valid_bottom_left;
            if (transformation_check_valid_correspondences(&top_left,
                                                           &top_right,
                                                           &bottom_right,
                                                           &bottom_left,
                                                           &valid_top_left,
                                                           &valid_top_right,
                                                           &valid_bottom_right,
                                                           &valid_bottom_left))
            {
                k4a_bounding_box_t bounding_box =
                    transformation_compute_bounding_box(&valid_top_left,
                                                        &valid_top_right,
                                                        &valid_bottom_right,
                                                        &valid_bottom_left,
                                                        context->transformed_image.descriptor->width_pixels,
                                                        context->transformed_image.descriptor->height_pixels);

                transformation_draw_rectangle(&bounding_box,
                                              &valid_top_left,
                                              &valid_top_right,
                                              &valid_bottom_right,
                                              &valid_bottom_left,
                                              &context->transformed_image);
            }

            vertex_row[x] = bottom_right;
            top_left = top_right;
            bottom_left = bottom_right;
        }
    }
    free(vertex_row);
    return K4A_RESULT_SUCCEEDED;
}

k4a_buffer_result_t transformation_depth_image_to_color_camera_internal(
    const k4a_calibration_t *calibration,
    const k4a_transformation_xy_tables_t *xy_tables_depth_camera,
    const uint8_t *depth_image_data,
    const k4a_transformation_image_descriptor_t *depth_image_descriptor,
    uint8_t *transformed_depth_image_data,
    k4a_transformation_image_descriptor_t *transformed_depth_image_descriptor)
{
    if (transformed_depth_image_descriptor == 0 || calibration == 0)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_transformed_depth_image_descriptor =
        transformation_init_image_descriptor(calibration->color_camera_calibration.resolution_width,
                                             calibration->color_camera_calibration.resolution_height,
                                             calibration->color_camera_calibration.resolution_width *
                                                 (int)sizeof(uint16_t));

    if (transformed_depth_image_data == 0 ||
        transformation_compare_image_descriptors(transformed_depth_image_descriptor,
                                                 &expected_transformed_depth_image_descriptor) == false)
    {
        memcpy(transformed_depth_image_descriptor,
               &expected_transformed_depth_image_descriptor,
               sizeof(k4a_transformation_image_descriptor_t));
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }

    if (xy_tables_depth_camera == 0 || depth_image_data == 0 || depth_image_descriptor == 0 ||
        transformed_depth_image_data == 0)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_depth_image_descriptor =
        transformation_init_image_descriptor(calibration->depth_camera_calibration.resolution_width,
                                             calibration->depth_camera_calibration.resolution_height,
                                             calibration->depth_camera_calibration.resolution_width *
                                                 (int)sizeof(uint16_t));

    if (transformation_compare_image_descriptors(depth_image_descriptor, &expected_depth_image_descriptor) == false)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_rgbz_context_t context;
    memset(&context, 0, sizeof(k4a_transformation_rgbz_context_t));

    context.xy_tables = xy_tables_depth_camera;
    context.calibration = calibration;

    context.depth_image = transformation_init_input_image(depth_image_descriptor, depth_image_data);

    context.transformed_image = transformation_init_output_image(transformed_depth_image_descriptor,
                                                                 transformed_depth_image_data);

    if (K4A_FAILED(transformation_depth_to_color(&context)))
    {
        return K4A_BUFFER_RESULT_FAILED;
    }
    return K4A_BUFFER_RESULT_SUCCEEDED;
}

static inline int transformation_point_inside_image(int width, int height, k4a_float2_t *point2d)
{
    int point_floor[2];
    point_floor[0] = (int)(floorf(point2d->xy.x));
    point_floor[1] = (int)(floorf(point2d->xy.y));
    if (point_floor[0] < 0 || point_floor[1] < 0 || point_floor[0] + 1 >= width || point_floor[1] + 1 >= height)
    {
        return 0;
    }
    return 1;
}

static inline uint8_t transformation_bilinear_interpolation(const uint8_t *image, int stride, k4a_float2_t *point2d)
{
    int point_floor[2];
    point_floor[0] = (int)(floorf(point2d->xy.x));
    point_floor[1] = (int)(floorf(point2d->xy.y));

    float fractional[2];
    fractional[0] = point2d->xy.x - point_floor[0];
    fractional[1] = point2d->xy.y - point_floor[1];

    float vals[4];
    int idx = point_floor[1] * stride + 4 * point_floor[0];
    vals[0] = (float)image[idx];
    vals[1] = (float)image[idx + 4];
    idx += stride;
    vals[2] = (float)image[idx];
    vals[3] = (float)image[idx + 4];

    float interpol_x[2];
    interpol_x[0] = (1.f - fractional[0]) * vals[0] + fractional[0] * vals[1];
    interpol_x[1] = (1.f - fractional[0]) * vals[2] + fractional[0] * vals[3];

    float interpol_y = (1.f - fractional[1]) * interpol_x[0] + fractional[1] * interpol_x[1];
    return (uint8_t)(interpol_y + 0.5f);
}

static k4a_result_t transformation_color_to_depth(k4a_transformation_rgbz_context_t *context)
{
    memset(context->transformed_image.data_uint8,
           0,
           (size_t)(context->transformed_image.descriptor->stride_bytes *
                    context->transformed_image.descriptor->height_pixels));

    for (int idx = 0;
         idx < context->depth_image.descriptor->width_pixels * context->depth_image.descriptor->height_pixels;
         idx++)
    {
        k4a_correspondence_t correspondence;
        if (K4A_FAILED(TRACE_CALL(transformation_compute_correspondence(
                idx, context->depth_image.data_uint16[idx], context, &correspondence))))
        {
            return K4A_RESULT_FAILED;
        }

        if (correspondence.valid && transformation_point_inside_image(context->color_image.descriptor->width_pixels,
                                                                      context->color_image.descriptor->height_pixels,
                                                                      &correspondence.point2d))
        {
            uint8_t b = transformation_bilinear_interpolation(context->color_image.data_uint8,
                                                              context->color_image.descriptor->stride_bytes,
                                                              &correspondence.point2d);

            uint8_t g = transformation_bilinear_interpolation(context->color_image.data_uint8 + 1,
                                                              context->color_image.descriptor->stride_bytes,
                                                              &correspondence.point2d);

            uint8_t r = transformation_bilinear_interpolation(context->color_image.data_uint8 + 2,
                                                              context->color_image.descriptor->stride_bytes,
                                                              &correspondence.point2d);

            uint8_t alpha = transformation_bilinear_interpolation(context->color_image.data_uint8 + 3,
                                                                  context->color_image.descriptor->stride_bytes,
                                                                  &correspondence.point2d);

            // bgra = (0,0,0,0) is used to indicate that the bgra pixel is invalid. A valid bgra pixel with values
            // (0,0,0,0) is mapped to (1,0,0,0) to express that it is valid and very close to black.
            if (b == 0 && g == 0 && r == 0 && alpha == 0)
            {
                b++;
            }

            context->transformed_image.data_uint8[4 * idx + 0] = b;
            context->transformed_image.data_uint8[4 * idx + 1] = g;
            context->transformed_image.data_uint8[4 * idx + 2] = r;
            context->transformed_image.data_uint8[4 * idx + 3] = alpha;
        }
    }
    return K4A_RESULT_SUCCEEDED;
}

k4a_buffer_result_t transformation_color_image_to_depth_camera_internal(
    const k4a_calibration_t *calibration,
    const k4a_transformation_xy_tables_t *xy_tables_depth_camera,
    const uint8_t *depth_image_data,
    const k4a_transformation_image_descriptor_t *depth_image_descriptor,
    const uint8_t *color_image_data,
    const k4a_transformation_image_descriptor_t *color_image_descriptor,
    uint8_t *transformed_color_image_data,
    k4a_transformation_image_descriptor_t *transformed_color_image_descriptor)
{
    if (transformed_color_image_descriptor == 0 || calibration == 0)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_transformed_color_image_descriptor =
        transformation_init_image_descriptor(calibration->depth_camera_calibration.resolution_width,
                                             calibration->depth_camera_calibration.resolution_height,
                                             calibration->depth_camera_calibration.resolution_width * 4 *
                                                 (int)sizeof(uint8_t));

    if (transformed_color_image_data == 0 ||
        transformation_compare_image_descriptors(transformed_color_image_descriptor,
                                                 &expected_transformed_color_image_descriptor) == false)
    {
        memcpy(transformed_color_image_descriptor,
               &expected_transformed_color_image_descriptor,
               sizeof(k4a_transformation_image_descriptor_t));
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }

    if (xy_tables_depth_camera == 0 || depth_image_data == 0 || depth_image_descriptor == 0 || color_image_data == 0 ||
        color_image_descriptor == 0 || transformed_color_image_data == 0)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_depth_image_descriptor =
        transformation_init_image_descriptor(calibration->depth_camera_calibration.resolution_width,
                                             calibration->depth_camera_calibration.resolution_height,
                                             calibration->depth_camera_calibration.resolution_width *
                                                 (int)sizeof(uint16_t));

    if (transformation_compare_image_descriptors(depth_image_descriptor, &expected_depth_image_descriptor) == false)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_color_image_descriptor =
        transformation_init_image_descriptor(calibration->color_camera_calibration.resolution_width,
                                             calibration->color_camera_calibration.resolution_height,
                                             calibration->color_camera_calibration.resolution_width * 4 *
                                                 (int)sizeof(uint8_t));

    if (transformation_compare_image_descriptors(color_image_descriptor, &expected_color_image_descriptor) == false)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_rgbz_context_t context;
    memset(&context, 0, sizeof(k4a_transformation_rgbz_context_t));

    context.xy_tables = xy_tables_depth_camera;
    context.calibration = calibration;

    context.depth_image = transformation_init_input_image(depth_image_descriptor, depth_image_data);

    context.color_image = transformation_init_input_image(color_image_descriptor, color_image_data);

    context.transformed_image = transformation_init_output_image(transformed_color_image_descriptor,
                                                                 transformed_color_image_data);

    if (K4A_FAILED(TRACE_CALL(transformation_color_to_depth(&context))))
    {
        return K4A_BUFFER_RESULT_FAILED;
    }
    return K4A_BUFFER_RESULT_SUCCEEDED;
}

k4a_buffer_result_t
transformation_depth_image_to_point_cloud_internal(k4a_transformation_xy_tables_t *xy_tables,
                                                   const uint8_t *depth_image_data,
                                                   const k4a_transformation_image_descriptor_t *depth_image_descriptor,
                                                   uint8_t *xyz_image_data,
                                                   k4a_transformation_image_descriptor_t *xyz_image_descriptor)
{
    if (xyz_image_descriptor == 0)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_xyz_image_descriptor =
        transformation_init_image_descriptor(xy_tables->width,
                                             xy_tables->height,
                                             xy_tables->width * 3 * (int)sizeof(int16_t));

    if (xyz_image_data == 0 ||
        transformation_compare_image_descriptors(xyz_image_descriptor, &expected_xyz_image_descriptor) == false)
    {
        memcpy(xyz_image_descriptor, &expected_xyz_image_descriptor, sizeof(k4a_transformation_image_descriptor_t));
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }

    if (depth_image_data == 0 || depth_image_descriptor == 0)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_depth_image_descriptor =
        transformation_init_image_descriptor(xy_tables->width,
                                             xy_tables->height,
                                             xy_tables->width * (int)sizeof(uint16_t));

    if (transformation_compare_image_descriptors(depth_image_descriptor, &expected_depth_image_descriptor) == false)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    const uint16_t *depth_image_data_uint16 = (const uint16_t *)(const void *)depth_image_data;
    int16_t *xyz_data_int16 = (int16_t *)(void *)xyz_image_data;

    for (int i = 0; i < xy_tables->width * xy_tables->height; i++)
    {
        int16_t z = (int16_t)depth_image_data_uint16[i];
        int16_t x = (int16_t)(floorf(xy_tables->x_table[i] * (float)z + 0.5f));
        int16_t y = (int16_t)(floorf(xy_tables->y_table[i] * (float)z + 0.5f));

        xyz_data_int16[3 * i + 0] = x;
        xyz_data_int16[3 * i + 1] = y;
        xyz_data_int16[3 * i + 2] = z;
    }

    return K4A_BUFFER_RESULT_SUCCEEDED;
}
