// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include <ut_calibration_data.h>

// Module being tested
#include <k4a/k4a.h>
#include <k4ainternal/transformation.h>
#include <k4ainternal/common.h>
#include <k4ainternal/image.h>

using namespace testing;

class transformation_ut : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_depth_point2d_reference[0] = 256.f;
        m_depth_point2d_reference[1] = 256.f;

        m_depth_point3d_reference[0] = -2.45376134f;
        m_depth_point3d_reference[1] = -1.66107690f;
        m_depth_point3d_reference[2] = 1000.f;

        m_color_point2d_reference[0] = 1835.68555f;
        m_color_point2d_reference[1] = 1206.31128f;

        m_color_point3d_reference[0] = -37.6291695f;
        m_color_point3d_reference[1] = 63.3947754f;
        m_color_point3d_reference[2] = 1001.46521f;

        m_gyro_point3d_reference[0] = -993.876404f;
        m_gyro_point3d_reference[1] = -4.87153625f;
        m_gyro_point3d_reference[2] = 110.429428f;

        m_accel_point3d_reference[0] = -1045.06238f;
        m_accel_point3d_reference[1] = 4.92006636f;
        m_accel_point3d_reference[2] = 108.398674f;

        k4a_depth_mode_t depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
        k4a_color_resolution_t color_resolution = K4A_COLOR_RESOLUTION_2160P;

        k4a_result_t result = k4a_calibration_get_from_raw(g_test_json,
                                                           sizeof(g_test_json),
                                                           depth_mode,
                                                           color_resolution,
                                                           &m_calibration);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    };

    k4a_calibration_t m_calibration;
    float m_depth_point2d_reference[2], m_depth_point3d_reference[3];
    float m_color_point2d_reference[2], m_color_point3d_reference[3];
    float m_gyro_point3d_reference[3];
    float m_accel_point3d_reference[3];
};

#define ASSERT_EQ_FLT(A, B)                                                                                            \
    {                                                                                                                  \
        float delta = (A) - (B);                                                                                       \
        if (delta < 0)                                                                                                 \
        {                                                                                                              \
            delta *= -1;                                                                                               \
        }                                                                                                              \
        if (delta > 1e-3f)                                                                                             \
        {                                                                                                              \
            std::cout << #A << " (" << A << ") is != " << #B << " (" << B << ") \n";                                   \
            ASSERT_EQ((A), (B));                                                                                       \
        }                                                                                                              \
    }

#define ASSERT_EQ_FLT2(A, B)                                                                                           \
    {                                                                                                                  \
        ASSERT_EQ_FLT(A[0], B[0])                                                                                      \
        ASSERT_EQ_FLT(A[1], B[1])                                                                                      \
    }

#define ASSERT_EQ_FLT3(A, B)                                                                                           \
    {                                                                                                                  \
        ASSERT_EQ_FLT(A[0], B[0])                                                                                      \
        ASSERT_EQ_FLT(A[1], B[1])                                                                                      \
        ASSERT_EQ_FLT(A[2], B[2])                                                                                      \
    }

static k4a_transformation_image_descriptor_t image_get_descriptor(const k4a_image_t image)
{
    k4a_transformation_image_descriptor_t descriptor;
    descriptor.width_pixels = image_get_width_pixels(image);
    descriptor.height_pixels = image_get_height_pixels(image);
    descriptor.stride_bytes = image_get_stride_bytes(image);
    return descriptor;
}

TEST_F(transformation_ut, transformation_3d_to_3d)
{
    float dummy[3] = { 0.f, 0.f, 0.f };

    float *_points3d[K4A_CALIBRATION_TYPE_NUM + 2];
    float **points3d = &_points3d[1];
    points3d[K4A_CALIBRATION_TYPE_UNKNOWN] = dummy;
    points3d[K4A_CALIBRATION_TYPE_DEPTH] = m_depth_point3d_reference;
    points3d[K4A_CALIBRATION_TYPE_COLOR] = m_color_point3d_reference;
    points3d[K4A_CALIBRATION_TYPE_GYRO] = m_gyro_point3d_reference;
    points3d[K4A_CALIBRATION_TYPE_ACCEL] = m_accel_point3d_reference;
    points3d[K4A_CALIBRATION_TYPE_NUM] = dummy;

    for (int source = (int)K4A_CALIBRATION_TYPE_UNKNOWN; source <= (int)K4A_CALIBRATION_TYPE_NUM; source++)
    {
        for (int target = (int)K4A_CALIBRATION_TYPE_UNKNOWN; target <= (int)K4A_CALIBRATION_TYPE_NUM; target++)
        {
            float transformed_point[3] = { 0.f, 0.f, 0.f };
            k4a_result_t result = transformation_3d_to_3d(&m_calibration,
                                                          points3d[source],
                                                          (k4a_calibration_type_t)source,
                                                          (k4a_calibration_type_t)target,
                                                          transformed_point);

            if (source >= (int)K4A_CALIBRATION_TYPE_NUM || target >= (int)K4A_CALIBRATION_TYPE_NUM ||
                source <= (int)K4A_CALIBRATION_TYPE_UNKNOWN || target <= (int)K4A_CALIBRATION_TYPE_UNKNOWN)
            {
                ASSERT_EQ(result, K4A_RESULT_FAILED);
            }
            else
            {
                ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
                ASSERT_EQ_FLT3(transformed_point, points3d[target]);
            }
        }
    }
}

TEST_F(transformation_ut, transformation_2d_to_3d)
{
    float point3d[3] = { 0.f, 0.f, 0.f };
    int valid = 0;

    k4a_result_t result = transformation_2d_to_3d(&m_calibration,
                                                  m_depth_point2d_reference,
                                                  m_depth_point3d_reference[2],
                                                  K4A_CALIBRATION_TYPE_DEPTH,
                                                  K4A_CALIBRATION_TYPE_DEPTH,
                                                  point3d,
                                                  &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT3(point3d, m_depth_point3d_reference);

    result = transformation_2d_to_3d(&m_calibration,
                                     m_color_point2d_reference,
                                     m_color_point3d_reference[2],
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     point3d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT3(point3d, m_color_point3d_reference);

    result = transformation_2d_to_3d(&m_calibration,
                                     m_depth_point2d_reference,
                                     m_depth_point3d_reference[2],
                                     K4A_CALIBRATION_TYPE_DEPTH,
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     point3d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT3(point3d, m_color_point3d_reference);

    result = transformation_2d_to_3d(&m_calibration,
                                     m_color_point2d_reference,
                                     m_color_point3d_reference[2],
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     K4A_CALIBRATION_TYPE_DEPTH,
                                     point3d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT3(point3d, m_depth_point3d_reference);

    // failure case
    result = transformation_2d_to_3d(&m_calibration,
                                     m_color_point2d_reference,
                                     m_color_point3d_reference[2],
                                     K4A_CALIBRATION_TYPE_GYRO,
                                     K4A_CALIBRATION_TYPE_DEPTH,
                                     point3d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_FAILED);
}

TEST_F(transformation_ut, transformation_3d_to_2d)
{
    float point2d[2] = { 0.f, 0.f };
    int valid = 0;

    k4a_result_t result = transformation_3d_to_2d(&m_calibration,
                                                  m_depth_point3d_reference,
                                                  K4A_CALIBRATION_TYPE_DEPTH,
                                                  K4A_CALIBRATION_TYPE_DEPTH,
                                                  point2d,
                                                  &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT2(point2d, m_depth_point2d_reference);

    result = transformation_3d_to_2d(&m_calibration,
                                     m_color_point3d_reference,
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     point2d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT2(point2d, m_color_point2d_reference);

    result = transformation_3d_to_2d(&m_calibration,
                                     m_depth_point3d_reference,
                                     K4A_CALIBRATION_TYPE_DEPTH,
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     point2d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT2(point2d, m_color_point2d_reference);

    result = transformation_3d_to_2d(&m_calibration,
                                     m_color_point3d_reference,
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     K4A_CALIBRATION_TYPE_DEPTH,
                                     point2d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT2(point2d, m_depth_point2d_reference);

    // failure case
    result = transformation_3d_to_2d(&m_calibration,
                                     m_color_point3d_reference,
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     K4A_CALIBRATION_TYPE_GYRO,
                                     point2d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_FAILED);
}

TEST_F(transformation_ut, transformation_2d_to_2d)
{
    float point2d[2] = { 0.f, 0.f };
    int valid = 0;

    k4a_result_t result = transformation_2d_to_2d(&m_calibration,
                                                  m_depth_point2d_reference,
                                                  m_depth_point3d_reference[2],
                                                  K4A_CALIBRATION_TYPE_DEPTH,
                                                  K4A_CALIBRATION_TYPE_DEPTH,
                                                  point2d,
                                                  &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT2(point2d, m_depth_point2d_reference);

    result = transformation_2d_to_2d(&m_calibration,
                                     m_color_point2d_reference,
                                     m_color_point3d_reference[2],
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     point2d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT2(point2d, m_color_point2d_reference);

    result = transformation_2d_to_2d(&m_calibration,
                                     m_depth_point2d_reference,
                                     m_depth_point3d_reference[2],
                                     K4A_CALIBRATION_TYPE_DEPTH,
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     point2d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT2(point2d, m_color_point2d_reference);

    result = transformation_2d_to_2d(&m_calibration,
                                     m_color_point2d_reference,
                                     m_color_point3d_reference[2],
                                     K4A_CALIBRATION_TYPE_COLOR,
                                     K4A_CALIBRATION_TYPE_DEPTH,
                                     point2d,
                                     &valid);

    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(valid, 1);
    ASSERT_EQ_FLT2(point2d, m_depth_point2d_reference);
}

TEST_F(transformation_ut, transformation_depth_image_to_point_cloud)
{
    k4a_transformation_t transformation_handle = transformation_create(&m_calibration, false);
    ASSERT_NE(transformation_handle, (k4a_transformation_t)NULL);

    int width = m_calibration.depth_camera_calibration.resolution_width;
    int height = m_calibration.depth_camera_calibration.resolution_height;
    k4a_image_t depth_image = NULL;
    ASSERT_EQ(image_create(K4A_IMAGE_FORMAT_DEPTH16, width, height, width * (int)sizeof(uint16_t), &depth_image),
              K4A_RESULT_SUCCEEDED);
    ASSERT_NE(depth_image, (k4a_image_t)NULL);
    k4a_transformation_image_descriptor_t depth_image_descriptor = image_get_descriptor(depth_image);

    uint16_t *depth_image_buffer = (uint16_t *)(void *)image_get_buffer(depth_image);
    for (int i = 0; i < width * height; i++)
    {
        depth_image_buffer[i] = (uint16_t)1000;
    }

    k4a_image_t xyz_image = NULL;
    ASSERT_EQ(image_create(K4A_IMAGE_FORMAT_CUSTOM, width, height, width * 3 * (int)sizeof(int16_t), &xyz_image),
              K4A_RESULT_SUCCEEDED);
    ASSERT_NE(xyz_image, (k4a_image_t)NULL);
    k4a_transformation_image_descriptor_t xyz_image_descriptor = image_get_descriptor(xyz_image);

    ASSERT_EQ(transformation_depth_image_to_point_cloud(transformation_handle,
                                                        image_get_buffer(depth_image),
                                                        &depth_image_descriptor,
                                                        K4A_CALIBRATION_TYPE_DEPTH,
                                                        image_get_buffer(xyz_image),
                                                        &xyz_image_descriptor),
              K4A_RESULT_SUCCEEDED);

    int16_t *xyz_image_buffer = (int16_t *)(void *)image_get_buffer(xyz_image);
    double check_sum = 0;
    for (int i = 0; i < 3 * width * height; i++)
    {
        check_sum += (double)abs(xyz_image_buffer[i]);
    }
    check_sum /= (double)(3 * width * height);

    // Comparison against reference hash value computed over the entire image. If result image is changed (e.g., due to
    // using a different calibration), the reference value needs to be updated.
    const double reference_val = 562.20976003011071;
    if (std::abs(check_sum - reference_val) > 0.001)
    {
        ASSERT_EQ(check_sum, reference_val);
    }

    image_dec_ref(depth_image);
    image_dec_ref(xyz_image);
    transformation_destroy(transformation_handle);
}

TEST_F(transformation_ut, transformation_all_image_functions_with_failure_cases)
{
    int depth_image_width_pixels = 640;
    int depth_image_height_pixels = 576;
    k4a_image_t depth_image = NULL;
    ASSERT_EQ(image_create(K4A_IMAGE_FORMAT_DEPTH16,
                           depth_image_width_pixels,
                           depth_image_height_pixels,
                           depth_image_width_pixels * 1 * (int)sizeof(uint16_t),
                           &depth_image),
              K4A_WAIT_RESULT_SUCCEEDED);

    int color_image_width_pixels = 1280;
    int color_image_height_pixels = 720;
    k4a_image_t color_image = NULL;
    ASSERT_EQ(image_create(K4A_IMAGE_FORMAT_DEPTH16,
                           color_image_width_pixels,
                           color_image_height_pixels,
                           color_image_width_pixels * 4 * (int)sizeof(uint8_t),
                           &color_image),
              K4A_WAIT_RESULT_SUCCEEDED);

    k4a_image_t transformed_color_image = NULL;
    ASSERT_EQ(image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
                           depth_image_width_pixels,
                           depth_image_height_pixels,
                           depth_image_width_pixels * 4 * (int)sizeof(uint8_t),
                           &transformed_color_image),
              K4A_WAIT_RESULT_SUCCEEDED);

    k4a_image_t transformed_depth_image = NULL;
    ASSERT_EQ(image_create(K4A_IMAGE_FORMAT_DEPTH16,
                           color_image_width_pixels,
                           color_image_height_pixels,
                           color_image_width_pixels * 1 * (int)sizeof(uint16_t),
                           &transformed_depth_image),
              K4A_WAIT_RESULT_SUCCEEDED);

    k4a_image_t xyz_depth_image = NULL;
    ASSERT_EQ(image_create(K4A_IMAGE_FORMAT_CUSTOM,
                           depth_image_width_pixels,
                           depth_image_height_pixels,
                           depth_image_width_pixels * 3 * (int)sizeof(int16_t),
                           &xyz_depth_image),
              K4A_RESULT_SUCCEEDED);

    k4a_image_t xyz_color_image = NULL;
    ASSERT_EQ(image_create(K4A_IMAGE_FORMAT_CUSTOM,
                           color_image_width_pixels,
                           color_image_height_pixels,
                           color_image_width_pixels * 3 * (int)sizeof(int16_t),
                           &xyz_color_image),
              K4A_RESULT_SUCCEEDED);

    k4a_transformation_image_descriptor_t depth_image_descriptor = image_get_descriptor(depth_image);
    k4a_transformation_image_descriptor_t color_image_descriptor = image_get_descriptor(color_image);
    k4a_transformation_image_descriptor_t transformed_color_image_descriptor = image_get_descriptor(
        transformed_color_image);
    k4a_transformation_image_descriptor_t transformed_depth_image_descriptor = image_get_descriptor(
        transformed_depth_image);
    k4a_transformation_image_descriptor_t xyz_depth_image_descriptor = image_get_descriptor(xyz_depth_image);
    k4a_transformation_image_descriptor_t xyz_color_image_descriptor = image_get_descriptor(xyz_color_image);

    uint8_t *depth_image_buffer = image_get_buffer(depth_image);
    uint8_t *color_image_buffer = image_get_buffer(color_image);
    uint8_t *transformed_depth_image_buffer = image_get_buffer(transformed_depth_image);
    uint8_t *transformed_color_image_buffer = image_get_buffer(transformed_color_image);
    uint8_t *xyz_depth_image_buffer = image_get_buffer(xyz_depth_image);
    uint8_t *xyz_color_image_buffer = image_get_buffer(xyz_color_image);

    for (int i = 0; i < 5; i++)
    {
        k4a_depth_mode_t depth_mode = K4A_DEPTH_MODE_OFF;
        k4a_color_resolution_t color_resolution = K4A_COLOR_RESOLUTION_OFF;

        switch (i)
        {
        case 0:
            depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
            color_resolution = K4A_COLOR_RESOLUTION_OFF;
            break;
        case 1:
            depth_mode = K4A_DEPTH_MODE_OFF;
            color_resolution = K4A_COLOR_RESOLUTION_720P;
            break;
        case 2:
            depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
            color_resolution = K4A_COLOR_RESOLUTION_720P;
            break;
        case 3:
            depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
            color_resolution = K4A_COLOR_RESOLUTION_2160P;
            break;
        default:
            depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
            color_resolution = K4A_COLOR_RESOLUTION_720P;
        }

        k4a_calibration_t calibration;
        k4a_result_t result =
            k4a_calibration_get_from_raw(g_test_json, sizeof(g_test_json), depth_mode, color_resolution, &calibration);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_transformation_t transformation_handle = transformation_create(&calibration, false);
        ASSERT_NE(transformation_handle, (k4a_transformation_t)NULL);

        k4a_result_t result_color_to_depth =
            transformation_color_image_to_depth_camera(transformation_handle,
                                                       depth_image_buffer,
                                                       &depth_image_descriptor,
                                                       color_image_buffer,
                                                       &color_image_descriptor,
                                                       transformed_color_image_buffer,
                                                       &transformed_color_image_descriptor);

        k4a_result_t result_depth_to_color =
            transformation_depth_image_to_color_camera(transformation_handle,
                                                       depth_image_buffer,
                                                       &depth_image_descriptor,
                                                       transformed_depth_image_buffer,
                                                       &transformed_depth_image_descriptor);

        k4a_result_t result_xyz_depth = transformation_depth_image_to_point_cloud(transformation_handle,
                                                                                  depth_image_buffer,
                                                                                  &depth_image_descriptor,
                                                                                  K4A_CALIBRATION_TYPE_DEPTH,
                                                                                  xyz_depth_image_buffer,
                                                                                  &xyz_depth_image_descriptor);

        k4a_result_t result_xyz_color = transformation_depth_image_to_point_cloud(transformation_handle,
                                                                                  transformed_depth_image_buffer,
                                                                                  &transformed_depth_image_descriptor,
                                                                                  K4A_CALIBRATION_TYPE_COLOR,
                                                                                  xyz_color_image_buffer,
                                                                                  &xyz_color_image_descriptor);

        if (i != 4)
        {
            ASSERT_NE(result_color_to_depth, K4A_RESULT_SUCCEEDED);
            ASSERT_NE(result_depth_to_color, K4A_RESULT_SUCCEEDED);
        }
        else
        {
            ASSERT_EQ(result_color_to_depth, K4A_RESULT_SUCCEEDED);
            ASSERT_EQ(result_depth_to_color, K4A_RESULT_SUCCEEDED);
        }

        if (i != 0 && i != 3 && i != 4)
        {
            ASSERT_NE(result_xyz_depth, K4A_RESULT_SUCCEEDED);
        }
        else
        {
            ASSERT_EQ(result_xyz_depth, K4A_RESULT_SUCCEEDED);
        }

        if (i != 1 && i != 2 && i != 4)
        {
            ASSERT_NE(result_xyz_color, K4A_RESULT_SUCCEEDED);
        }
        else
        {
            ASSERT_EQ(result_xyz_color, K4A_RESULT_SUCCEEDED);
        }

        transformation_destroy(transformation_handle);
    }

    image_dec_ref(depth_image);
    image_dec_ref(color_image);
    image_dec_ref(transformed_color_image);
    image_dec_ref(transformed_depth_image);
    image_dec_ref(xyz_color_image);
    image_dec_ref(xyz_depth_image);
}

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}
