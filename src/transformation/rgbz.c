// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4ainternal/transformation.h>
#include <k4ainternal/logging.h>

#include <stdlib.h>
#include <limits.h>
#include <math.h>

#if defined(__amd64__) || defined(_M_AMD64) || defined(__i386__) || defined(_M_IX86)
#define K4A_USING_SSE
#include <emmintrin.h> // SSE2
#include <tmmintrin.h> // SSE3
#include <smmintrin.h> // SSE4.1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define K4A_USING_NEON
#include <arm_neon.h>
#endif

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
    k4a_transformation_input_image_t custom_image;
    k4a_transformation_output_image_t transformed_image;
    k4a_transformation_output_image_t transformed_custom_image;
    k4a_transformation_interpolation_type_t interpolation_type;
    uint16_t invalid_value;
    bool enable_custom8;
    bool enable_custom16;
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

// g_transformation_instruction_type is set to SSE, NEON, None, or NULL
static char g_transformation_instruction_type[5] = { 0 };

// Share g_transformation_instruction_type with tests to confirm this is built correctly.
char *transformation_get_instruction_type(void);
char *transformation_get_instruction_type(void)
{
    return g_transformation_instruction_type;
}

// Set the special instruction
static void set_special_instruction_optimization(char *opt)
{
    // Only set this once
    if (g_transformation_instruction_type[0] == '\0')
    {
        size_t sz = MIN(sizeof(opt), sizeof(g_transformation_instruction_type) - 1);
        memcpy(g_transformation_instruction_type, opt, sz);
        LOG_INFO("Compiled special instruction type is: %s\n", opt);
    }
}

static k4a_transformation_image_descriptor_t
transformation_init_image_descriptor(int width, int height, int stride, k4a_image_format_t format)
{
    k4a_transformation_image_descriptor_t descriptor;
    descriptor.width_pixels = width;
    descriptor.height_pixels = height;
    descriptor.stride_bytes = stride;
    descriptor.format = format;
    return descriptor;
}

static bool transformation_compare_image_descriptors(const k4a_transformation_image_descriptor_t *descriptor1,
                                                     const k4a_transformation_image_descriptor_t *descriptor2)
{
    if (descriptor1->width_pixels != descriptor2->width_pixels ||
        descriptor1->height_pixels != descriptor2->height_pixels ||
        descriptor1->stride_bytes != descriptor2->stride_bytes || descriptor1->format != descriptor2->format)
    {
        LOG_ERROR("Unexpected image descriptor. "
                  "Descriptor 1: width_pixels: %d, height_pixels: %d, stride_bytes: %d, format: %d. "
                  "Descriptor 2: width_pixels: %d, height_pixels: %d, stride_bytes: %d, format: %d. ",
                  descriptor1->width_pixels,
                  descriptor1->height_pixels,
                  descriptor1->stride_bytes,
                  descriptor1->format,
                  descriptor2->width_pixels,
                  descriptor2->height_pixels,
                  descriptor2->stride_bytes,
                  descriptor2->format);
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
    if (depth == 0 || isnan(context->xy_tables->x_table[depth_index]))
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

static inline uint16_t transformation_interpolate_custom(const uint16_t *c1,
                                                         const uint16_t *c2,
                                                         const uint16_t *nearest,
                                                         bool use_linear_interpolation)
{
    uint16_t result = *nearest;

    if (use_linear_interpolation)
    {
        result = (*c1 + *c2) / 2;
    }

    return result;
}

static bool transformation_check_valid_correspondences(const k4a_correspondence_t *top_left,
                                                       const k4a_correspondence_t *top_right,
                                                       const k4a_correspondence_t *bottom_right,
                                                       const k4a_correspondence_t *bottom_left,
                                                       k4a_correspondence_t *valid_top_left,
                                                       k4a_correspondence_t *valid_top_right,
                                                       k4a_correspondence_t *valid_bottom_right,
                                                       k4a_correspondence_t *valid_bottom_left,
                                                       uint16_t *custom_top_left,
                                                       uint16_t *custom_top_right,
                                                       uint16_t *custom_bottom_right,
                                                       uint16_t *custom_bottom_left,
                                                       bool use_linear_interpolation)
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
        *custom_top_left = transformation_interpolate_custom(custom_top_right,
                                                             custom_bottom_left,
                                                             custom_bottom_right,
                                                             use_linear_interpolation);
    }
    if (top_right->valid == 0)
    {
        num_invalid++;
        *valid_top_right = *bottom_right;
        *valid_bottom_right = transformation_interpolate_correspondences(bottom_right, bottom_left);
        *custom_top_right = *custom_bottom_right;
        *custom_bottom_right = transformation_interpolate_custom(custom_bottom_right,
                                                                 custom_bottom_left,
                                                                 custom_bottom_left,
                                                                 use_linear_interpolation);
    }
    if (bottom_right->valid == 0)
    {
        num_invalid++;
        *valid_bottom_right = transformation_interpolate_correspondences(top_right, bottom_left);
        *custom_bottom_right = transformation_interpolate_custom(custom_top_right,
                                                                 custom_bottom_left,
                                                                 custom_top_left,
                                                                 use_linear_interpolation);
    }
    if (bottom_left->valid == 0)
    {
        num_invalid++;
        *valid_bottom_left = *bottom_right;
        *valid_bottom_right = transformation_interpolate_correspondences(top_right, bottom_right);
        *custom_bottom_left = *custom_bottom_right;
        *custom_bottom_right = transformation_interpolate_custom(custom_top_right,
                                                                 custom_bottom_right,
                                                                 custom_top_right,
                                                                 use_linear_interpolation);
    }

    // If two or more vertices are invalid then we can't create a valid triangle
    bool valid = num_invalid < 2;

    // Ignore interpolation at large depth discontinuity without disrupting slanted surface
    // Skip interpolation threshold is estimated based on the following logic:
    // - angle between two pixels is: theta = 0.234375 degree (120 degree / 512) in binning resolution mode
    // - distance between two pixels at same depth approximately is: A ~= sin(theta) * depth
    // - distance between two pixels at highly slanted surface (e.g. alpha = 85 degree) is: B = A / cos(alpha)
    // - skip_interpolation_ratio ~= sin(theta) / cos(alpha)
    // We use B as the threshold that to skip interpolation if the depth difference in the triangle is larger
    // than B. This is a conservative threshold to estimate largest distance on a highly slanted surface at given depth,
    // in reality, given distortion, distance, resolution difference, B can be smaller
    const float skip_interpolation_ratio = 0.04693441759f;
    float d1 = valid_top_left->depth;
    float d2 = valid_top_right->depth;
    float d3 = valid_bottom_right->depth;
    float d4 = valid_bottom_left->depth;
    float depth_min = transformation_min2f(transformation_min2f(d1, d2), transformation_min2f(d3, d4));
    float depth_max = transformation_max2f(transformation_max2f(d1, d2), transformation_max2f(d3, d4));
    float depth_delta = depth_max - depth_min;
    float skip_interpolation_threshold = skip_interpolation_ratio * depth_min;
    if (depth_delta > skip_interpolation_threshold)
    {
        valid = false;
    }

    return valid;
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
                                                 uint16_t custom_top_left,
                                                 uint16_t custom_intermediate,
                                                 uint16_t custom_bottom_right,
                                                 const k4a_float2_t *point,
                                                 float area_intermediate,
                                                 bool counter_clockwise,
                                                 float *depth,
                                                 float *custom,
                                                 bool use_linear_interpolation)
{
    // Calculate sub triangle areas
    float area_top_left = transformation_area_function(&valid_intermediate->point2d, &valid_top_left->point2d, point);
    float area_bottom_right = transformation_area_function(&valid_bottom_right->point2d,
                                                           &valid_intermediate->point2d,
                                                           point);

    // If counter_clockwise order is not set then we need to negate the areas.
    if (counter_clockwise == false)
    {
        area_top_left = -area_top_left;
        area_bottom_right = -area_bottom_right;
        area_intermediate = -area_intermediate;
    }

    // Check if point is inside the triangle (area is positive).
    // Top/left edge is inclusive (>= 0) while bottom/right edge is exclusive (> 0).
    if ((area_top_left >= 0.0f) && (area_bottom_right > 0.0f))
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

        if (use_linear_interpolation)
        {
            *custom = (area_top_left * (float)custom_bottom_right + area_intermediate * (float)custom_intermediate +
                       area_bottom_right * (float)custom_top_left) *
                      sum_weights;
        }
        else
        {
            // Select custom based on highest weight (nearest neighbor)
            if (area_top_left > area_intermediate)
            {
                *custom = area_top_left > area_bottom_right ? (float)custom_bottom_right : (float)custom_top_left;
            }
            else
            {
                *custom = area_intermediate > area_bottom_right ? (float)custom_intermediate : (float)custom_top_left;
            }
        }

        return true;
    }

    return false;
}

static bool transformation_point_inside_quad(const k4a_correspondence_t *valid_top_left,
                                             const k4a_correspondence_t *valid_top_right,
                                             const k4a_correspondence_t *valid_bottom_right,
                                             const k4a_correspondence_t *valid_bottom_left,
                                             uint16_t custom_top_left,
                                             uint16_t custom_top_right,
                                             uint16_t custom_bottom_right,
                                             uint16_t custom_bottom_left,
                                             const k4a_float2_t *point,
                                             float *depth,
                                             float *custom,
                                             bool use_linear_interpolation)
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
                                                custom_top_left,
                                                counter_clockwise ? custom_bottom_left : custom_top_right,
                                                custom_bottom_right,
                                                point,
                                                area_intermediate,
                                                counter_clockwise,
                                                depth,
                                                custom,
                                                use_linear_interpolation);
}

static void transformation_draw_rectangle(const k4a_bounding_box_t *bounding_box,
                                          const k4a_correspondence_t *valid_top_left,
                                          const k4a_correspondence_t *valid_top_right,
                                          const k4a_correspondence_t *valid_bottom_right,
                                          const k4a_correspondence_t *valid_bottom_left,
                                          uint16_t custom_top_left,
                                          uint16_t custom_top_right,
                                          uint16_t custom_bottom_right,
                                          uint16_t custom_bottom_left,
                                          bool use_linear_interpolation,
                                          bool enable_custom8,
                                          bool enable_custom16,
                                          k4a_transformation_output_image_t *depth_out,
                                          k4a_transformation_output_image_t *custom_out)
{
    k4a_float2_t point;
    for (int y = bounding_box->top_left[1]; y < bounding_box->bottom_right[1]; y++)
    {
        uint16_t *depth_row = depth_out->data_uint16 + y * depth_out->descriptor->width_pixels;

        uint8_t *custom8_row = 0;
        uint16_t *custom16_row = 0;
        if (enable_custom8)
        {
            custom8_row = custom_out->data_uint8 + y * custom_out->descriptor->width_pixels;
        }
        else if (enable_custom16)
        {
            custom16_row = custom_out->data_uint16 + y * custom_out->descriptor->width_pixels;
        }

        point.xy.y = (float)y;

        for (int x = bounding_box->top_left[0]; x < bounding_box->bottom_right[0]; x++)
        {
            point.xy.x = (float)x;

            float interpolated_depth = 0.0f;
            float interpolated_custom = 0.0f;
            if (transformation_point_inside_quad(valid_top_left,
                                                 valid_top_right,
                                                 valid_bottom_right,
                                                 valid_bottom_left,
                                                 custom_top_left,
                                                 custom_top_right,
                                                 custom_bottom_right,
                                                 custom_bottom_left,
                                                 &point,
                                                 &interpolated_depth,
                                                 &interpolated_custom,
                                                 use_linear_interpolation))
            {
                uint16_t depth = (uint16_t)(interpolated_depth + 0.5f);

                // handle occlusions
                if (depth_row[x] == 0 || (depth < depth_row[x]))
                {
                    depth_row[x] = depth;

                    if (enable_custom8)
                    {
                        custom8_row[x] = (uint8_t)(interpolated_custom + 0.5f);
                    }
                    else if (enable_custom16)
                    {
                        custom16_row[x] = (uint16_t)(interpolated_custom + 0.5f);
                    }
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

    if (context->enable_custom8)
    {
        int num_pixels = context->transformed_custom_image.descriptor->width_pixels *
                         context->transformed_custom_image.descriptor->height_pixels;
        for (int i = 0; i < num_pixels; i++)
        {
            context->transformed_custom_image.data_uint8[i] = (uint8_t)context->invalid_value;
        }
    }
    else if (context->enable_custom16)
    {
        int num_pixels = context->transformed_custom_image.descriptor->width_pixels *
                         context->transformed_custom_image.descriptor->height_pixels;
        for (int i = 0; i < num_pixels; i++)
        {
            context->transformed_custom_image.data_uint16[i] = context->invalid_value;
        }
    }

    bool use_linear_interpolation = context->interpolation_type == K4A_TRANSFORMATION_INTERPOLATION_TYPE_LINEAR;

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

            uint16_t custom_top_left = 0;
            uint16_t custom_top_right = 0;
            uint16_t custom_bottom_right = 0;
            uint16_t custom_bottom_left = 0;

            if (context->enable_custom8)
            {
                int custom_width = context->custom_image.descriptor->width_pixels;
                custom_top_left = context->custom_image.data_uint8[(y - 1) * custom_width + x - 1];
                custom_top_right = context->custom_image.data_uint8[(y - 1) * custom_width + x];
                custom_bottom_right = context->custom_image.data_uint8[y * custom_width + x];
                custom_bottom_left = context->custom_image.data_uint8[y * custom_width + x - 1];
            }
            else if (context->enable_custom16)
            {
                int custom_width = context->custom_image.descriptor->width_pixels;
                custom_top_left = context->custom_image.data_uint16[(y - 1) * custom_width + x - 1];
                custom_top_right = context->custom_image.data_uint16[(y - 1) * custom_width + x];
                custom_bottom_right = context->custom_image.data_uint16[y * custom_width + x];
                custom_bottom_left = context->custom_image.data_uint16[y * custom_width + x - 1];
            }

            k4a_correspondence_t valid_top_left, valid_top_right, valid_bottom_right, valid_bottom_left;
            if (transformation_check_valid_correspondences(&top_left,
                                                           &top_right,
                                                           &bottom_right,
                                                           &bottom_left,
                                                           &valid_top_left,
                                                           &valid_top_right,
                                                           &valid_bottom_right,
                                                           &valid_bottom_left,
                                                           &custom_top_left,
                                                           &custom_top_right,
                                                           &custom_bottom_right,
                                                           &custom_bottom_left,
                                                           use_linear_interpolation))
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
                                              custom_top_left,
                                              custom_top_right,
                                              custom_bottom_right,
                                              custom_bottom_left,
                                              use_linear_interpolation,
                                              context->enable_custom8,
                                              context->enable_custom16,
                                              &context->transformed_image,
                                              &context->transformed_custom_image);
            }

            vertex_row[x] = bottom_right;
            top_left = top_right;
            bottom_left = bottom_right;
        }
    }
    free(vertex_row);
    return K4A_RESULT_SUCCEEDED;
}

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
    k4a_transformation_image_descriptor_t *transformed_custom_image_descriptor)
{
    if (depth_image_descriptor == 0 || custom_image_descriptor == 0 || transformed_depth_image_descriptor == 0 ||
        transformed_custom_image_descriptor == 0)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    if (calibration == 0)
    {
        LOG_ERROR("Calibration is null.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    if (xy_tables_depth_camera == 0)
    {
        LOG_ERROR("Depth camera xy table is null.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    if (depth_image_data == 0)
    {
        LOG_ERROR("Depth image data is null.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    if (transformed_depth_image_data == 0)
    {
        LOG_ERROR("Transformed depth image data is null.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    if (transformed_custom_image_data != 0 && custom_image_data == 0)
    {
        LOG_ERROR("Custom image data is null.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    if (custom_image_data != 0 && transformed_custom_image_data == 0)
    {
        LOG_ERROR("Transformed custom image data is null.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_depth_image_descriptor =
        transformation_init_image_descriptor(calibration->depth_camera_calibration.resolution_width,
                                             calibration->depth_camera_calibration.resolution_height,
                                             calibration->depth_camera_calibration.resolution_width *
                                                 (int)sizeof(uint16_t),
                                             K4A_IMAGE_FORMAT_DEPTH16);

    if (transformation_compare_image_descriptors(depth_image_descriptor, &expected_depth_image_descriptor) == false)
    {
        LOG_ERROR("Unexpected depth image descriptor, see details above.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    int custom_bytes_per_pixel = 1;
    k4a_image_format_t custom_format = K4A_IMAGE_FORMAT_CUSTOM8;
    if (custom_image_descriptor->format == K4A_IMAGE_FORMAT_CUSTOM16)
    {
        custom_bytes_per_pixel = 2;
        custom_format = K4A_IMAGE_FORMAT_CUSTOM16;
    }

    k4a_transformation_image_descriptor_t expected_custom_image_descriptor =
        transformation_init_image_descriptor(calibration->depth_camera_calibration.resolution_width,
                                             calibration->depth_camera_calibration.resolution_height,
                                             calibration->depth_camera_calibration.resolution_width *
                                                 custom_bytes_per_pixel,
                                             custom_format);

    if (custom_image_data != 0 &&
        transformation_compare_image_descriptors(custom_image_descriptor, &expected_custom_image_descriptor) == false)
    {
        LOG_ERROR("Unexpected custom image descriptor, see details above.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_transformed_depth_image_descriptor =
        transformation_init_image_descriptor(calibration->color_camera_calibration.resolution_width,
                                             calibration->color_camera_calibration.resolution_height,
                                             calibration->color_camera_calibration.resolution_width *
                                                 (int)sizeof(uint16_t),
                                             K4A_IMAGE_FORMAT_DEPTH16);

    if (transformation_compare_image_descriptors(transformed_depth_image_descriptor,
                                                 &expected_transformed_depth_image_descriptor) == false)
    {
        LOG_ERROR("Unexpected transformed depth image descriptor, see details above.", 0);
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }

    k4a_transformation_image_descriptor_t expected_transformed_custom_image_descriptor =
        transformation_init_image_descriptor(calibration->color_camera_calibration.resolution_width,
                                             calibration->color_camera_calibration.resolution_height,
                                             calibration->color_camera_calibration.resolution_width *
                                                 custom_bytes_per_pixel,
                                             custom_format);

    if (transformed_custom_image_data != 0 &&
        transformation_compare_image_descriptors(transformed_custom_image_descriptor,
                                                 &expected_transformed_custom_image_descriptor) == false)
    {
        LOG_ERROR("Unexpected transformed custom image descriptor, see details above.", 0);
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }

    return K4A_BUFFER_RESULT_SUCCEEDED;
}

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
    uint32_t invalid_custom_value)
{
    if (K4A_BUFFER_RESULT_SUCCEEDED !=
        TRACE_BUFFER_CALL(
            transformation_depth_image_to_color_camera_validate_parameters(calibration,
                                                                           xy_tables_depth_camera,
                                                                           depth_image_data,
                                                                           depth_image_descriptor,
                                                                           custom_image_data,
                                                                           custom_image_descriptor,
                                                                           transformed_depth_image_data,
                                                                           transformed_depth_image_descriptor,
                                                                           transformed_custom_image_data,
                                                                           transformed_custom_image_descriptor)))
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_rgbz_context_t context;
    memset(&context, 0, sizeof(k4a_transformation_rgbz_context_t));

    context.xy_tables = xy_tables_depth_camera;
    context.calibration = calibration;

    context.depth_image = transformation_init_input_image(depth_image_descriptor, depth_image_data);

    context.custom_image = transformation_init_input_image(custom_image_descriptor, custom_image_data);

    context.transformed_image = transformation_init_output_image(transformed_depth_image_descriptor,
                                                                 transformed_depth_image_data);

    context.transformed_custom_image = transformation_init_output_image(transformed_custom_image_descriptor,
                                                                        transformed_custom_image_data);

    context.enable_custom8 = false;
    context.enable_custom16 = false;
    if (custom_image_descriptor->format == K4A_IMAGE_FORMAT_CUSTOM8)
    {
        context.enable_custom8 = true;
    }
    else if (custom_image_descriptor->format == K4A_IMAGE_FORMAT_CUSTOM16)
    {
        context.enable_custom16 = true;
    }

    context.interpolation_type = interpolation_type;
    context.invalid_value = (uint16_t)(invalid_custom_value & 0xffff);

    if (K4A_FAILED(TRACE_CALL(transformation_depth_to_color(&context))))
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

k4a_buffer_result_t transformation_color_image_to_depth_camera_validate_parameters(
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
        if (calibration == 0)
        {
            LOG_ERROR("Calibration is null.", 0);
        }
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_transformed_color_image_descriptor =
        transformation_init_image_descriptor(calibration->depth_camera_calibration.resolution_width,
                                             calibration->depth_camera_calibration.resolution_height,
                                             calibration->depth_camera_calibration.resolution_width * 4 *
                                                 (int)sizeof(uint8_t),
                                             K4A_IMAGE_FORMAT_COLOR_BGRA32);

    if (transformed_color_image_data == 0 ||
        transformation_compare_image_descriptors(transformed_color_image_descriptor,
                                                 &expected_transformed_color_image_descriptor) == false)
    {
        if (transformed_color_image_data == 0)
        {
            LOG_ERROR("Transformed color image data is null.", 0);
        }
        else
        {
            LOG_ERROR("Unexpected transformed color image descriptor, see details above.", 0);
        }
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }

    if (xy_tables_depth_camera == 0 || depth_image_data == 0 || depth_image_descriptor == 0 || color_image_data == 0 ||
        color_image_descriptor == 0 || transformed_color_image_data == 0)
    {
        if (xy_tables_depth_camera == 0)
        {
            LOG_ERROR("Depth camera xy table is null.", 0);
        }
        if (depth_image_data == 0)
        {
            LOG_ERROR("Depth image data is null.", 0);
        }
        if (color_image_data == 0)
        {
            LOG_ERROR("Color image data is null.", 0);
        }
        if (transformed_color_image_data == 0)
        {
            LOG_ERROR("Transformed color image data is null.", 0);
        }
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_depth_image_descriptor =
        transformation_init_image_descriptor(calibration->depth_camera_calibration.resolution_width,
                                             calibration->depth_camera_calibration.resolution_height,
                                             calibration->depth_camera_calibration.resolution_width *
                                                 (int)sizeof(uint16_t),
                                             K4A_IMAGE_FORMAT_DEPTH16);

    if (transformation_compare_image_descriptors(depth_image_descriptor, &expected_depth_image_descriptor) == false)
    {
        LOG_ERROR("Unexpected depth image descriptor, see details above.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_color_image_descriptor =
        transformation_init_image_descriptor(calibration->color_camera_calibration.resolution_width,
                                             calibration->color_camera_calibration.resolution_height,
                                             calibration->color_camera_calibration.resolution_width * 4 *
                                                 (int)sizeof(uint8_t),
                                             K4A_IMAGE_FORMAT_COLOR_BGRA32);

    if (transformation_compare_image_descriptors(color_image_descriptor, &expected_color_image_descriptor) == false)
    {
        LOG_ERROR("Unexpected color image descriptor, see details above.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    return K4A_BUFFER_RESULT_SUCCEEDED;
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
    if (K4A_BUFFER_RESULT_SUCCEEDED !=
        TRACE_BUFFER_CALL(
            transformation_color_image_to_depth_camera_validate_parameters(calibration,
                                                                           xy_tables_depth_camera,
                                                                           depth_image_data,
                                                                           depth_image_descriptor,
                                                                           color_image_data,
                                                                           color_image_descriptor,
                                                                           transformed_color_image_data,
                                                                           transformed_color_image_descriptor)))
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

#if !defined(K4A_USING_SSE) && !defined(K4A_USING_NEON)
// This is the same function as transformation_depth_to_xyz without the SSE
// instructions. This code is kept here for readability.
static void transformation_depth_to_xyz(k4a_transformation_xy_tables_t *xy_tables,
                                        const void *depth_image_data,
                                        void *xyz_image_data)
{
    const uint16_t *depth_image_data_uint16 = (const uint16_t *)depth_image_data;
    int16_t *xyz_data_int16 = (int16_t *)xyz_image_data;
    int16_t x, y, z;

    set_special_instruction_optimization("None");

    for (int i = 0; i < xy_tables->width * xy_tables->height; i++)
    {
        float x_tab = xy_tables->x_table[i];

        if (!isnan(x_tab))
        {
            z = (int16_t)depth_image_data_uint16[i];
            x = (int16_t)(floorf(x_tab * (float)z + 0.5f));
            y = (int16_t)(floorf(xy_tables->y_table[i] * (float)z + 0.5f));
        }
        else
        {
            x = 0;
            y = 0;
            z = 0;
        }

        xyz_data_int16[3 * i + 0] = x;
        xyz_data_int16[3 * i + 1] = y;
        xyz_data_int16[3 * i + 2] = z;
    }
}

#elif defined(K4A_USING_NEON)
// convert from float to int using NEON is round to zero
// make separate function to do floor
static inline int32x4_t neon_floor(float32x4_t v)
{
    int32x4_t v0 = vcvtq_s32_f32(v);
    int32x4_t a0 = vreinterpretq_s32_u32(vcgtq_f32(vcvtq_f32_s32(v0), v));
    return vaddq_s32(v0, a0);
}

static void transformation_depth_to_xyz(k4a_transformation_xy_tables_t *xy_tables,
                                        const void *depth_image_data,
                                        void *xyz_image_data)
{
    float *x_tab = (float *)xy_tables->x_table;
    float *y_tab = (float *)xy_tables->y_table;
    const uint16_t *depth_image_data_uint16 = (const uint16_t *)depth_image_data;
    int16_t *xyz_data_int16 = (int16_t *)xyz_image_data;
    float32x4_t half = vdupq_n_f32(0.5f);

    set_special_instruction_optimization("NEON");

    for (int i = 0; i < xy_tables->width * xy_tables->height / 8; i++)
    {
        // 8 elements in 1 loop
        int offset = i * 8;
        float32x4_t x_tab_lo = vld1q_f32(x_tab + offset);
        float32x4_t x_tab_hi = vld1q_f32(x_tab + offset + 4);
        // equivalent to isnan
        uint32x4_t valid_lo = vceqq_f32(x_tab_lo, x_tab_lo);
        uint32x4_t valid_hi = vceqq_f32(x_tab_hi, x_tab_hi);
        // each element in valid is a mask which corresponds to isnan
        uint16x8_t valid = vcombine_u16(vmovn_u32(valid_lo), vmovn_u32(valid_hi));
        uint16x8_t v_0 = vandq_u16(vld1q_u16(depth_image_data_uint16 + offset), valid);
        // v_z corresponds to z in naive code
        int16x8_t v_z = vreinterpretq_s16_u16(v_0);
        // expand v_z to compute x and y
        float32x4_t v_z_lo = vcvtq_f32_u32(vmovl_u16(vget_low_u16(v_0)));
        float32x4_t v_z_hi = vcvtq_f32_u32(vmovl_u16(vget_high_u16(v_0)));
        // load x_table and y_table
        float32x4_t t_x_lo = vld1q_f32(x_tab + offset);
        float32x4_t t_x_hi = vld1q_f32(x_tab + offset + 4);
        float32x4_t t_y_lo = vld1q_f32(y_tab + offset);
        float32x4_t t_y_hi = vld1q_f32(y_tab + offset + 4);
        // main computation of x and y
        int32x4_t v_x_lo = neon_floor(vmlaq_f32(half, v_z_lo, t_x_lo));
        int32x4_t v_x_hi = neon_floor(vmlaq_f32(half, v_z_hi, t_x_hi));
        int32x4_t v_y_lo = neon_floor(vmlaq_f32(half, v_z_lo, t_y_lo));
        int32x4_t v_y_hi = neon_floor(vmlaq_f32(half, v_z_hi, t_y_hi));
        int16x8_t v_x = vcombine_s16(vmovn_s32(v_x_lo), vmovn_s32(v_x_hi));
        int16x8_t v_y = vcombine_s16(vmovn_s32(v_y_lo), vmovn_s32(v_y_hi));
        // use scatter store instruction
        int16x8x3_t store;
        store.val[0] = v_x; // x0 x1 .. x14 x15
        store.val[1] = v_y; // y0 y1 .. y14 y15
        store.val[2] = v_z; // z0 z1 .. z14 z15
        // x0 y0 z0 x1 y1 z1 .. x15 y15 z15
        vst3q_s16(xyz_data_int16 + offset * 3, store);
    }
}

#else /* defined(K4A_USING_SSE) */

static void transformation_depth_to_xyz(k4a_transformation_xy_tables_t *xy_tables,
                                        const void *depth_image_data,
                                        void *xyz_image_data)
{
    const __m128i *depth_image_data_m128i = (const __m128i *)depth_image_data;
#if defined(__clang__) || defined(__GNUC__)
    void *x_table = __builtin_assume_aligned(xy_tables->x_table, 16);
    void *y_table = __builtin_assume_aligned(xy_tables->y_table, 16);
#else
    void *x_table = (void *)xy_tables->x_table;
    void *y_table = (void *)xy_tables->y_table;
#endif
    __m128 *x_table_m128 = (__m128 *)x_table;
    __m128 *y_table_m128 = (__m128 *)y_table;
    __m128i *xyz_data_m128i = (__m128i *)xyz_image_data;

    set_special_instruction_optimization("SSE");

    const int16_t pos0 = 0x0100;
    const int16_t pos1 = 0x0302;
    const int16_t pos2 = 0x0504;
    const int16_t pos3 = 0x0706;
    const int16_t pos4 = 0x0908;
    const int16_t pos5 = 0x0B0A;
    const int16_t pos6 = 0x0D0C;
    const int16_t pos7 = 0x0F0E;

    // x0, x3, x6, x1, x4, x7, x2, x5
    __m128i x_shuffle = _mm_setr_epi16(pos0, pos3, pos6, pos1, pos4, pos7, pos2, pos5);
    // y5, y0, y3, y6, y1, y4, y7, y2
    __m128i y_shuffle = _mm_setr_epi16(pos5, pos0, pos3, pos6, pos1, pos4, pos7, pos2);
    // z2, z5, z0, z3, z6, z1, z4, z7
    __m128i z_shuffle = _mm_setr_epi16(pos2, pos5, pos0, pos3, pos6, pos1, pos4, pos7);

    __m128i valid_shuffle = _mm_setr_epi16(pos0, pos2, pos4, pos6, pos0, pos2, pos4, pos6);

    for (int i = 0; i < xy_tables->width * xy_tables->height / 8; i++)
    {
        __m128i z = *depth_image_data_m128i++;

        __m128 x_tab_lo = *x_table_m128++;
        __m128 x_tab_hi = *x_table_m128++;
        __m128 valid_lo = _mm_cmpeq_ps(x_tab_lo, x_tab_lo);
        __m128 valid_hi = _mm_cmpeq_ps(x_tab_hi, x_tab_hi);
        __m128i valid_shuffle_lo = _mm_shuffle_epi8(*((__m128i *)&valid_lo), valid_shuffle);
        __m128i valid_shuffle_hi = _mm_shuffle_epi8(*((__m128i *)&valid_hi), valid_shuffle);
        __m128i valid = _mm_blend_epi16(valid_shuffle_lo, valid_shuffle_hi, 0xF0);
        z = _mm_blendv_epi8(_mm_setzero_si128(), z, valid);

        __m128 depth_lo = _mm_cvtepi32_ps(_mm_unpacklo_epi16(z, _mm_setzero_si128()));
        __m128 depth_hi = _mm_cvtepi32_ps(_mm_unpackhi_epi16(z, _mm_setzero_si128()));

        __m128i x_lo = _mm_cvtps_epi32(_mm_mul_ps(depth_lo, x_tab_lo));
        __m128i x_hi = _mm_cvtps_epi32(_mm_mul_ps(depth_hi, x_tab_hi));
        __m128i x = _mm_packs_epi32(x_lo, x_hi);
        x = _mm_blendv_epi8(_mm_setzero_si128(), x, valid);
        x = _mm_shuffle_epi8(x, x_shuffle);

        __m128i y_lo = _mm_cvtps_epi32(_mm_mul_ps(depth_lo, *y_table_m128++));
        __m128i y_hi = _mm_cvtps_epi32(_mm_mul_ps(depth_hi, *y_table_m128++));
        __m128i y = _mm_packs_epi32(y_lo, y_hi);
        y = _mm_shuffle_epi8(y, y_shuffle);

        z = _mm_shuffle_epi8(z, z_shuffle);

        // x0, y0, z0, x1, y1, z1, x2, y2
        *xyz_data_m128i++ = _mm_blend_epi16(_mm_blend_epi16(x, y, 0x92), z, 0x24);
        // z2, x3, y3, z3, x4, y4, z4, x5
        *xyz_data_m128i++ = _mm_blend_epi16(_mm_blend_epi16(x, y, 0x24), z, 0x49);
        // y5, z5, x6, y6, z6, x7, y7, z7
        *xyz_data_m128i++ = _mm_blend_epi16(_mm_blend_epi16(x, y, 0x49), z, 0x92);
    }
}
#endif

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

    k4a_transformation_image_descriptor_t expected_xyz_image_descriptor = transformation_init_image_descriptor(
        xy_tables->width, xy_tables->height, xy_tables->width * 3 * (int)sizeof(int16_t), xyz_image_descriptor->format);

    if (xyz_image_data == 0 ||
        transformation_compare_image_descriptors(xyz_image_descriptor, &expected_xyz_image_descriptor) == false)
    {
        if (xyz_image_data == 0)
        {
            LOG_ERROR("XYZ image data is null.", 0);
        }
        else
        {
            LOG_ERROR("Unexpected XYZ image descriptor, see details above.", 0);
        }
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }

    if (depth_image_data == 0 || depth_image_descriptor == 0)
    {
        if (depth_image_data == 0)
        {
            LOG_ERROR("Depth image data is null.", 0);
        }
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_transformation_image_descriptor_t expected_depth_image_descriptor = transformation_init_image_descriptor(
        xy_tables->width, xy_tables->height, xy_tables->width * (int)sizeof(uint16_t), K4A_IMAGE_FORMAT_DEPTH16);

    if (transformation_compare_image_descriptors(depth_image_descriptor, &expected_depth_image_descriptor) == false)
    {
        LOG_ERROR("Unexpected depth image descriptor, see details above.", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    transformation_depth_to_xyz(xy_tables, (const void *)depth_image_data, (void *)xyz_image_data);

    return K4A_BUFFER_RESULT_SUCCEEDED;
}
