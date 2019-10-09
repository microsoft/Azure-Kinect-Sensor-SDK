// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include <ut_calibration_data.h>

// Module being tested
#include <k4ainternal/calibration.h>
#include <k4ainternal/depth_mcu.h>
#include <k4ainternal/common.h>

using namespace testing;

#define GTEST_LOG_INFO std::cout << "[     INFO ] "
#define FAKE_MCU ((depthmcu_t)0xface000)

// Define the symbols needed from the usb_cmd module.
// Only functions required to link the depth module are needed
k4a_result_t
depthmcu_get_extrinsic_calibration(depthmcu_t depthmcu_handle, char *json, size_t json_size, size_t *bytes_read)
{
    (void)depthmcu_handle;

    if (json_size < sizeof(g_test_json))
    {
        return K4A_RESULT_FAILED;
    }
    memcpy(json, g_test_json, sizeof(g_test_json));
    *bytes_read = sizeof(g_test_json);
    return K4A_RESULT_SUCCEEDED;
}

TEST(calibration_ut, api_validation)
{
    calibration_t calibration;
    k4a_calibration_camera_t depth;
    k4a_calibration_camera_t color;
    k4a_calibration_imu_t gyro;
    k4a_calibration_imu_t accel;

    // Sanity check failure
    ASSERT_EQ(calibration_create(NULL, &calibration), K4A_RESULT_FAILED);
    ASSERT_EQ(calibration_create(FAKE_MCU, NULL), K4A_RESULT_FAILED);
    ASSERT_EQ(calibration_create(NULL, NULL), K4A_RESULT_FAILED);

    // Sanity check success
    ASSERT_EQ(calibration_create(FAKE_MCU, &calibration), K4A_RESULT_SUCCEEDED);

    // Sanity check failure
    ASSERT_EQ(calibration_get_camera(calibration, K4A_CALIBRATION_TYPE_UNKNOWN, NULL), K4A_RESULT_FAILED);
    ASSERT_EQ(calibration_get_camera(NULL, K4A_CALIBRATION_TYPE_DEPTH, NULL), K4A_RESULT_FAILED);
    ASSERT_EQ(calibration_get_camera(NULL, K4A_CALIBRATION_TYPE_COLOR, NULL), K4A_RESULT_FAILED);
    ASSERT_EQ(calibration_get_imu(NULL, K4A_CALIBRATION_TYPE_GYRO, NULL), K4A_RESULT_FAILED);
    ASSERT_EQ(calibration_get_imu(NULL, K4A_CALIBRATION_TYPE_ACCEL, NULL), K4A_RESULT_FAILED);
    ASSERT_EQ(calibration_get_camera(NULL, K4A_CALIBRATION_TYPE_UNKNOWN, &color), K4A_RESULT_FAILED);
    calibration_destroy(NULL);

    // Sanity check success
    ASSERT_EQ(calibration_get_camera(calibration, K4A_CALIBRATION_TYPE_DEPTH, &depth), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(calibration_get_camera(calibration, K4A_CALIBRATION_TYPE_COLOR, &color), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(calibration_get_imu(calibration, K4A_CALIBRATION_TYPE_GYRO, &gyro), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(calibration_get_imu(calibration, K4A_CALIBRATION_TYPE_ACCEL, &accel), K4A_RESULT_SUCCEEDED);
    calibration_destroy(calibration);

    {
        k4a_calibration_camera_t depth_calibration;
        k4a_calibration_camera_t color_calibration;
        k4a_calibration_imu_t gyro_calibration;
        k4a_calibration_imu_t accel_calibration;

        // clang-format off
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), &depth_calibration, &color_calibration, &gyro_calibration,  &accel_calibration), K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), &depth_calibration, &color_calibration, &gyro_calibration,  NULL),               K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), &depth_calibration, &color_calibration, NULL,               &accel_calibration), K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), &depth_calibration, &color_calibration, NULL,               NULL),               K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), &depth_calibration, NULL,               &gyro_calibration,  &accel_calibration), K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), &depth_calibration, NULL,               &gyro_calibration,  NULL),               K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), &depth_calibration, NULL,               NULL,               &accel_calibration), K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), &depth_calibration, NULL,               NULL,               NULL),               K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), NULL,               &color_calibration, &gyro_calibration,  &accel_calibration), K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), NULL,               &color_calibration, &gyro_calibration,  NULL),               K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), NULL,               &color_calibration, NULL,               &accel_calibration), K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), NULL,               &color_calibration, NULL,               NULL),               K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), NULL,               NULL,               &gyro_calibration,  &accel_calibration), K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), NULL,               NULL,               &gyro_calibration,  NULL),               K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), NULL,               NULL,               NULL,               &accel_calibration), K4A_RESULT_SUCCEEDED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json), NULL,               NULL,               NULL,               NULL),               K4A_RESULT_FAILED);

        ASSERT_EQ(calibration_create_from_raw(NULL,        sizeof(g_test_json), &depth_calibration, &color_calibration, &gyro_calibration,  &accel_calibration), K4A_RESULT_FAILED);
        ASSERT_EQ(calibration_create_from_raw(g_test_json, sizeof(g_test_json)-2, &depth_calibration, &color_calibration, &gyro_calibration,  &accel_calibration), K4A_RESULT_FAILED);
        // clang-format on
    }
}

#define ASSERT_EQ_FLT(A, B)                                                                                            \
    {                                                                                                                  \
        std::cout.precision(std::numeric_limits<float>::max_digits10);                                                 \
        float delta = (A) - (B);                                                                                       \
        if (delta < 0)                                                                                                 \
        {                                                                                                              \
            delta *= -1;                                                                                               \
        }                                                                                                              \
        if (delta > std::numeric_limits<float>::epsilon())                                                             \
        {                                                                                                              \
            std::cout << #A << " (" << A << ") is != " << #B << " (" << B << ") \n";                                   \
            ASSERT_EQ((A), (B));                                                                                       \
        }                                                                                                              \
    }

TEST(calibration_ut, calibration_validation)
{
    calibration_t calibration;
    k4a_calibration_camera_t color_expected = { { { .99999475479125973f,
                                                    0.000089527980890125036f,
                                                    -0.0032330064568668604f,
                                                    0.0001289974752580747f,
                                                    0.99771732091903687f,
                                                    0.0675286054611206f,
                                                    0.00323167210444808f,
                                                    -0.067528672516345978f,
                                                    0.99771207571029663f },
                                                  { -0.031942266970872879f,
                                                    -0.0024762286338955164f,
                                                    0.0036488529294729233f } },
                                                { K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY,
                                                  14,
                                                  { { 0.49626153707504272f,
                                                      0.50453948974609375f,
                                                      0.48109495639801025f,
                                                      0.85502892732620239f,
                                                      -1.05756676197052f,
                                                      -1.3993426561355591f,
                                                      1.7508856058120728f,
                                                      -1.1485179662704468f,
                                                      -1.1924415826797485f,
                                                      1.6220585107803345f,
                                                      0.f,
                                                      0.f,
                                                      -0.000012259531104064081f,
                                                      0.00069101608823984861f } } },
                                                3840,
                                                2160,
                                                0.65614030566253234f };
    k4a_calibration_camera_t depth_expected = { { { 1, 0, 0, 0, 1, 0, 0, 0, 1 }, { 0, 0, 0 } },
                                                { K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY,
                                                  14,
                                                  { { 0.50218582153320313f,
                                                      0.50179535150527954f,
                                                      0.49281969666481018f,
                                                      0.49292713403701782f,
                                                      0.58251643180847168f,
                                                      0.0061361165717244148f,
                                                      -0.0013186802389100194f,
                                                      0.92561346292495728f,
                                                      0.12568977475166321f,
                                                      -0.0065302476286888123f,
                                                      0.f,
                                                      0.f,
                                                      -0.000020833556845900603f,
                                                      -0.00012877916742581874f } } },
                                                1024,
                                                1024,
                                                1.737323045730591f };
    k4a_calibration_imu_t gyro_expected =
        { { { 0.006215147208422422f,
              0.112122543156147f,
              -0.9936749339103699f,
              -0.9999266862869263f,
              -0.009632264263927937f,
              -0.007341118063777685f,
              -0.010394444689154625f,
              0.9936476945877075f,
              0.11205445230007172f },
            { 0, 0, 0 } },
          16,
          { 0.0009500000160187483f, 0.0009500000160187483f, 0.0009500000160187483f, 0, 0, 0 },
          0,
          { -0.035886555910110474f, 0, 0, 0, 0.018185537308454514f, 0, 0, 0, -0.0170263834297657f, 0, 0, 0 },
          { 1.0006208419799805f,    0, 0, 0, 0.000861089036334306f,  0, 0, 0, 0.006612793542444706f, 0, 0, 0,
            0.0008673131815157831f, 0, 0, 0, 0.9934653043746948f,    0, 0, 0, -0.00831980723887682f, 0, 0, 0,
            0.006603339221328497f,  0, 0, 0, -0.008248291909694672f, 0, 0, 0, 1.0021218061447144f,   0, 0, 0 },
          { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
          { 9.999999747378752e-05f, 9.999999747378752e-05f, 9.999999747378752e-05f },
          { 5.0f, 60.0f } };

    k4a_calibration_imu_t accel_expected =
        { { { 0.0003031194501090795f,
              0.1085868701338768f,
              -0.9940869212150574f,
              -0.9999820590019226f,
              -0.005916988011449575f,
              -0.000951246009208262f,
              -0.005985293071717024f,
              0.9940693378448486f,
              0.10858312249183655f },
            { -0.05079442635178566f, 0.0034077665768563747f, 0.0014520891709253192f } },
          56,
          { 0.010700000450015068f, 0.010700000450015068f, 0.010700000450015068f, 0, 0, 0 },
          0,
          { -0.04158460721373558f, 0, 0, 0, -0.0007273331866599619f, 0, 0, 0, -0.08497647196054459f, 0, 0, 0 },
          { 1.0162577629089355f,     0, 0, 0, -0.0003413598460610956f, 0, 0, 0, -0.0005690594553016126f, 0, 0, 0,
            -0.0003392015059944242f, 0, 0, 0, 1.022727608680725f,      0, 0, 0, 0.0019331017974764109f,  0, 0, 0,
            -0.0005674033891409636f, 0, 0, 0, 0.0019397408468648791f,  0, 0, 0, 1.019227385520935f,      0, 0, 0 },
          { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
          { 0.009999999776482582f, 0.009999999776482582f, 0.009999999776482582f },
          { 5.0f, 60.0f } };

    k4a_calibration_camera_t color;
    k4a_calibration_camera_t depth;
    k4a_calibration_imu_t gyro;
    k4a_calibration_imu_t accel;

    ASSERT_EQ(calibration_create(FAKE_MCU, &calibration), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(calibration_get_camera(calibration, K4A_CALIBRATION_TYPE_DEPTH, &depth), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(calibration_get_camera(calibration, K4A_CALIBRATION_TYPE_COLOR, &color), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(calibration_get_imu(calibration, K4A_CALIBRATION_TYPE_GYRO, &gyro), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(calibration_get_imu(calibration, K4A_CALIBRATION_TYPE_ACCEL, &accel), K4A_RESULT_SUCCEEDED);
    calibration_destroy(calibration);

    for (int type = 0; type < 2; type++)
    {
        k4a_calibration_camera_t *cal_read;
        k4a_calibration_camera_t *cal_expected;

        if (type == 0)
        {
            cal_read = &color;
            cal_expected = &color_expected;
        }
        else
        {
            cal_read = &depth;
            cal_expected = &depth_expected;
        }

        ASSERT_EQ(cal_read->intrinsics.parameter_count, cal_expected->intrinsics.parameter_count);
        ASSERT_EQ(cal_read->intrinsics.type, cal_expected->intrinsics.type);
        ASSERT_EQ(cal_read->resolution_width, cal_expected->resolution_width);
        ASSERT_EQ(cal_read->resolution_height, cal_expected->resolution_height);
        ASSERT_EQ_FLT(cal_read->metric_radius, cal_expected->metric_radius);

        for (uint32_t x = 0; x < COUNTOF(cal_read->extrinsics.rotation); x++)
        {
            ASSERT_EQ_FLT(cal_read->extrinsics.rotation[x], cal_expected->extrinsics.rotation[x]);
        }
        for (uint32_t x = 0; x < COUNTOF(cal_read->extrinsics.translation); x++)
        {
            ASSERT_EQ_FLT(cal_read->extrinsics.translation[x], 1000.f * cal_expected->extrinsics.translation[x]);
        }
        for (uint32_t x = 0; x < cal_read->intrinsics.parameter_count; x++)
        {
            ASSERT_EQ_FLT(cal_read->intrinsics.parameters.v[x], cal_expected->intrinsics.parameters.v[x]);
        }
    }
    for (int type = 0; type < 2; type++)
    {
        k4a_calibration_imu_t *cal_read;
        k4a_calibration_imu_t *cal_expected;

        if (type == 0)
        {
            cal_read = &gyro;
            cal_expected = &gyro_expected;
        }
        else
        {
            cal_read = &accel;
            cal_expected = &accel_expected;
        }

        ASSERT_EQ_FLT(cal_read->temperature_in_c, cal_expected->temperature_in_c);
        ASSERT_EQ(cal_read->model_type_mask, cal_expected->model_type_mask);

        for (uint32_t x = 0; x < COUNTOF(cal_read->depth_to_imu.rotation); x++)
        {
            ASSERT_EQ_FLT(cal_read->depth_to_imu.rotation[x], cal_expected->depth_to_imu.rotation[x]);
        }
        for (uint32_t x = 0; x < COUNTOF(cal_read->depth_to_imu.translation); x++)
        {
            ASSERT_EQ_FLT(cal_read->depth_to_imu.translation[x], 1000.f * cal_expected->depth_to_imu.translation[x]);
        }
        for (uint32_t x = 0; x < COUNTOF(cal_read->noise); x++)
        {
            ASSERT_EQ_FLT(cal_read->noise[x], cal_expected->noise[x]);
        }
        for (uint32_t x = 0; x < COUNTOF(cal_read->bias_temperature_model); x++)
        {
            ASSERT_EQ_FLT(cal_read->bias_temperature_model[x], cal_expected->bias_temperature_model[x]);
        }
        for (uint32_t x = 0; x < COUNTOF(cal_read->mixing_matrix_temperature_model); x++)
        {
            ASSERT_EQ_FLT(cal_read->mixing_matrix_temperature_model[x],
                          cal_expected->mixing_matrix_temperature_model[x]);
        }
        for (uint32_t x = 0; x < COUNTOF(cal_read->second_order_scaling); x++)
        {
            ASSERT_EQ_FLT(cal_read->second_order_scaling[x], cal_expected->second_order_scaling[x]);
        }
        for (uint32_t x = 0; x < COUNTOF(cal_read->bias_uncertainty); x++)
        {
            ASSERT_EQ_FLT(cal_read->bias_uncertainty[x], cal_expected->bias_uncertainty[x]);
        }
        for (uint32_t x = 0; x < COUNTOF(cal_read->temperature_bounds); x++)
        {
            ASSERT_EQ_FLT(cal_read->temperature_bounds[x], cal_expected->temperature_bounds[x]);
        }
    }
}

TEST(calibration_ut, validate_raw_data_api)
{
    calibration_t calibration;
    size_t allocate_size;
    size_t json_size;

    // Sanity check success
    ASSERT_EQ(calibration_create(FAKE_MCU, &calibration), K4A_RESULT_SUCCEEDED);

    // Sanity check failure
    ASSERT_EQ(calibration_get_raw_data(NULL, NULL, NULL), K4A_BUFFER_RESULT_FAILED);
    ASSERT_EQ(calibration_get_raw_data(calibration, NULL, NULL), K4A_BUFFER_RESULT_FAILED);
    allocate_size = 0;
    ASSERT_EQ(calibration_get_raw_data(calibration, NULL, &allocate_size), K4A_BUFFER_RESULT_TOO_SMALL);
    ASSERT_GT(allocate_size, (size_t)0);
    ASSERT_LT(allocate_size, (size_t)(1024 * 1024)); // file is about 5k in size, so sanity check it is below 1MByte

    json_size = allocate_size;
    uint8_t *json = (uint8_t *)malloc(json_size);
    ASSERT_NE(json, (uint8_t *)NULL);

    allocate_size = 2;
    ASSERT_EQ(calibration_get_raw_data(calibration, json, &allocate_size), K4A_BUFFER_RESULT_TOO_SMALL);
    ASSERT_EQ(allocate_size, json_size);

    ASSERT_EQ(calibration_get_raw_data(NULL, json, &allocate_size), K4A_BUFFER_RESULT_FAILED);

    allocate_size = json_size;
    ASSERT_EQ(calibration_get_raw_data(calibration, json, &allocate_size), K4A_BUFFER_RESULT_SUCCEEDED);
    ASSERT_NE(isprint(json[0]), 0); // lazily validate the json is well formed
    ASSERT_NE(isprint(json[1]), 0);
    ASSERT_NE(isprint(json[2]), 0);

    GTEST_LOG_INFO << "JSON file is being dumped\n" << (char *)json << "\n";

    calibration_destroy(NULL);
    calibration_destroy(calibration);
    free(json);
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
