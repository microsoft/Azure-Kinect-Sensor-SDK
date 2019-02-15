#include <utcommon.h>
#include <ut_calibration_data.h>

// Module being tested
#include <k4a/k4a.h>
#include <k4ainternal/transformation.h>
#include <k4ainternal/common.h>

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

TEST_F(transformation_ut, transformation_create)
{
    k4a_depth_mode_t depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    k4a_color_resolution_t color_resolution = K4A_COLOR_RESOLUTION_OFF;

    k4a_calibration_t calibration;
    k4a_result_t result =
        k4a_calibration_get_from_raw(g_test_json, sizeof(g_test_json), depth_mode, color_resolution, &calibration);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4a_transformation_t transformation_depth_only = k4a_transformation_create(&calibration);
    ASSERT_NE(transformation_depth_only, (k4a_transformation_t)NULL);

    depth_mode = K4A_DEPTH_MODE_OFF;
    color_resolution = K4A_COLOR_RESOLUTION_720P;

    result = k4a_calibration_get_from_raw(g_test_json, sizeof(g_test_json), depth_mode, color_resolution, &calibration);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4a_transformation_t transformation_rgb_only = k4a_transformation_create(&calibration);
    ASSERT_NE(transformation_rgb_only, (k4a_transformation_t)NULL);
}

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}