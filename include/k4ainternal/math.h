/** \file math.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef K4AMATH_H
#define K4AMATH_H

#ifdef __cplusplus
extern "C" {
#endif

void math_transpose_3x3(const float in[3 * 3], float out[3 * 3]);

void math_negate_3(const float in[3], float out[3]);

void math_add_3(const float a[3], const float b[3], float out[3]);

void math_scale_3(const float in[3], float s, float out[3]);

void math_add_scaled_3(const float in[3], float s, float out[3]);

float math_dot_3(const float a[3], const float b[3]);

void math_mult_Ax_3x3(const float A[3 * 3], const float x[3], float out[3]);

void math_mult_Atx_3x3(const float A[3 * 3], const float x[3], float out[3]);

void math_mult_AB_3x3x3(const float A[3 * 3], const float B[3 * 3], float out[3 * 3]);

/** Evaluates a polynomial up to degree 3 at x
 *
 *  \param x
 *     value used in evaluation
 *
 * \param coef
 *    coefficients (4-dim vector)
 *    coef[3] * x^3 + coef[2] * x^2 + coef[1] * x + coef[0]
 *
 * \return  The value of polynomial at x
 */
float math_eval_poly_3(float x, const float coef[4]);

/** out = A*x + b*/
void math_affine_transform_3(const float A[3 * 3], const float x[3], const float b[3], float out[3]);

/** out = A*x + B*x^2 + b
 * x^2 is the element-wise square of the vector x, no cross-terms are involved in this operation
 */
void math_quadratic_transform_3(const float A[3 * 3],
                                const float B[3 * 3],
                                const float x[3],
                                const float b[3],
                                float out[3]);

#ifdef __cplusplus
}
#endif

#endif /* K4AMATH_H */