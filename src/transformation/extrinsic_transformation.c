// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4ainternal/transformation.h>
#include <k4ainternal/math.h>

static void transformation_extrinsics_mult(const k4a_calibration_extrinsics_t *a,
                                           const k4a_calibration_extrinsics_t *b,
                                           k4a_calibration_extrinsics_t *ab)
{
    float Rt[3];
    math_mult_Ax_3x3(a->rotation, b->translation, Rt);
    math_add_3(Rt, a->translation, ab->translation);

    math_mult_AB_3x3x3(a->rotation, b->rotation, ab->rotation);
}

static void transformation_extrinsics_transform_point_3(const k4a_calibration_extrinsics_t *source_to_target,
                                                        const float x[3],
                                                        float y[3])
{
    const float a = x[0], b = x[1], c = x[2];
    const float *R = source_to_target->rotation;
    const float *t = source_to_target->translation;

    y[0] = R[0] * a + R[1] * b + R[2] * c + t[0];
    y[1] = R[3] * a + R[4] * b + R[5] * c + t[1];
    y[2] = R[6] * a + R[7] * b + R[8] * c + t[2];
}

static void transformation_extrinsics_invert(const k4a_calibration_extrinsics_t *x, k4a_calibration_extrinsics_t *xinv)
{
    math_transpose_3x3(x->rotation, xinv->rotation);
    math_mult_Ax_3x3(xinv->rotation, x->translation, xinv->translation);
    math_negate_3(xinv->translation, xinv->translation);
}

k4a_result_t transformation_get_extrinsic_transformation(const k4a_calibration_extrinsics_t *world_to_source,
                                                         const k4a_calibration_extrinsics_t *world_to_target,
                                                         k4a_calibration_extrinsics_t *source_to_target)
{
    if (world_to_source == 0 || world_to_target == 0)
    {
        memset(source_to_target, 0, sizeof(k4a_calibration_extrinsics_t));
        return K4A_RESULT_SUCCEEDED;
    }

    k4a_calibration_extrinsics_t source_to_world;
    transformation_extrinsics_invert(world_to_source, &source_to_world);

    transformation_extrinsics_mult(world_to_target, &source_to_world, source_to_target);

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t transformation_apply_extrinsic_transformation(const k4a_calibration_extrinsics_t *source_to_target,
                                                           const float source_point3d[3],
                                                           float target_point3d[3])
{
    transformation_extrinsics_transform_point_3(source_to_target, source_point3d, target_point3d);

    return K4A_RESULT_SUCCEEDED;
}
