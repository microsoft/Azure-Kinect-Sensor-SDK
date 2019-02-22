// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
// This library
#include <k4ainternal/math.h>

void math_transpose_3x3(const float in[3 * 3], float out[3 * 3])
{
    for (int i = 0; i < 3; ++i)
    {
        out[i * (3 + 1)] = in[i * (3 + 1)];
        for (int j = i + 1; j < 3; ++j)
        {
            const int upper = i * 3 + j;
            const int lower = j * 3 + i;
            const float tmp = in[upper];
            out[upper] = in[lower];
            out[lower] = tmp;
        }
    }
}

void math_negate_3(const float in[3], float out[3])
{
    out[0] = -in[0];
    out[1] = -in[1];
    out[2] = -in[2];
}

void math_add_3(const float a[3], const float b[3], float out[3])
{
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

void math_scale_3(const float in[3], float s, float out[3])
{
    out[0] = in[0] * s;
    out[1] = in[1] * s;
    out[2] = in[2] * s;
}

void math_add_scaled_3(const float in[3], float s, float out[3])
{
    out[0] += in[0] * s;
    out[1] += in[1] * s;
    out[2] += in[2] * s;
}

float math_dot_3(const float a[3], const float b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void math_mult_Ax_3x3(const float A[3 * 3], const float x[3], float out[3])
{
    const float y0 = math_dot_3(A, x);
    const float y1 = math_dot_3(A + 3, x);
    const float y2 = math_dot_3(A + 6, x);
    out[0] = y0;
    out[1] = y1;
    out[2] = y2;
}

void math_mult_Atx_3x3(const float A[3 * 3], const float x[3], float out[3])
{
    const float x0 = x[0], x1 = x[1], x2 = x[2];
    math_scale_3(A, x0, out);
    math_add_scaled_3(A + 3, x1, out);
    math_add_scaled_3(A + 6, x2, out);
}

void math_mult_AB_3x3x3(const float A[3 * 3], const float B[3 * 3], float out[3 * 3])
{
    math_mult_Atx_3x3(B, A, out);
    math_mult_Atx_3x3(B, A + 3, out + 3);
    math_mult_Atx_3x3(B, A + 6, out + 6);
}

float math_eval_poly_3(float x, const float coef[4])
{
    return coef[0] + x * (coef[1] + x * (coef[2] + x * coef[3]));
}

void math_affine_transform_3(const float A[3 * 3], const float x[3], const float b[3], float out[3])
{
    math_mult_Ax_3x3(A, x, out);
    math_add_3(out, b, out);
}

void math_quadratic_transform_3(const float A[3 * 3],
                                const float B[3 * 3],
                                const float x[3],
                                const float b[3],
                                float out[3])
{
    float x2[3]; // x2 is x^2
    float temp[3];

    // temp = A*x + b
    math_affine_transform_3(A, x, b, temp);

    for (unsigned int i = 0; i < 3; i++)
        x2[i] = x[i] * x[i];

    // y = B*x2 + temp
    math_affine_transform_3(B, x2, temp, out);
}