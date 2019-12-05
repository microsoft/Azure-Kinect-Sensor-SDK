// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4ainternal/transformation.h>
#include <k4ainternal/logging.h>

#include <float.h>

// We don't like globals if we can help it. This one is for reducing critical logging noise when recorded files are used
// with Rational 6KT calibration. Production devices never had this calibration but recordings were made with this
// calibration. So we fire the warning 1 time instead of every time a transformation call is made
static int g_deprecated_6kt_message_fired = false;

static k4a_result_t transformation_project_internal(const k4a_calibration_camera_t *camera_calibration,
                                                    const float xy[2],
                                                    float uv[2],
                                                    int *valid,
                                                    float J_xy[2 * 2])
{
    if (K4A_FAILED(K4A_RESULT_FROM_BOOL(
            (camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT ||
             camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY) &&
            camera_calibration->intrinsics.parameter_count >= 14)))
    {
        LOG_ERROR("Unexpected camera calibration model type %d, should either be "
                  "K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT (%d) or "
                  "K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY (%d).",
                  camera_calibration->intrinsics.type,
                  K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT,
                  K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY);
        if (camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY)
        {
            LOG_ERROR("Unexpected parameter count %d, should be %d.",
                      camera_calibration->intrinsics.parameter_count,
                      14);
        }
        return K4A_RESULT_FAILED;
    }

    if (camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT &&
        g_deprecated_6kt_message_fired == false)
    {
        g_deprecated_6kt_message_fired = true;
        LOG_CRITICAL("Rational 6KT is deprecated (only supported early internal devices). Please replace your Azure "
                     "Kinect with a retail device.",
                     0);
    }

    const k4a_calibration_intrinsic_parameters_t *params = &camera_calibration->intrinsics.parameters;

    float cx = params->param.cx;
    float cy = params->param.cy;
    float fx = params->param.fx;
    float fy = params->param.fy;
    float k1 = params->param.k1;
    float k2 = params->param.k2;
    float k3 = params->param.k3;
    float k4 = params->param.k4;
    float k5 = params->param.k5;
    float k6 = params->param.k6;
    float codx = params->param.codx; // center of distortion is set to 0 for Brown Conrady model
    float cody = params->param.cody;
    float p1 = params->param.p1;
    float p2 = params->param.p2;
    float max_radius_for_projection = camera_calibration->metric_radius;

    if (K4A_FAILED(K4A_RESULT_FROM_BOOL(fx > 0.f && fy > 0.f)))
    {
        LOG_ERROR("Expect both fx and fy are larger than 0, actual values are fx: %lf, fy: %lf.",
                  (double)fx,
                  (double)fy);
        return K4A_RESULT_FAILED;
    }

    *valid = 1;

    float xp = xy[0] - codx;
    float yp = xy[1] - cody;

    float xp2 = xp * xp;
    float yp2 = yp * yp;
    float xyp = xp * yp;
    float rs = xp2 + yp2;
    if (rs > max_radius_for_projection * max_radius_for_projection)
    {
        *valid = 0;
        return K4A_RESULT_SUCCEEDED;
    }
    float rss = rs * rs;
    float rsc = rss * rs;
    float a = 1.f + k1 * rs + k2 * rss + k3 * rsc;
    float b = 1.f + k4 * rs + k5 * rss + k6 * rsc;
    float bi;
    if (b != 0.f)
    {
        bi = 1.f / b;
    }
    else
    {
        bi = 1.f;
    }
    float d = a * bi;

    float xp_d = xp * d;
    float yp_d = yp * d;

    float rs_2xp2 = rs + 2.f * xp2;
    float rs_2yp2 = rs + 2.f * yp2;

    if (camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT)
    {
        xp_d += rs_2xp2 * p2 + xyp * p1;
        yp_d += rs_2yp2 * p1 + xyp * p2;
    }
    else
    {
        // the only difference from Rational6ktCameraModel is 2 multiplier for the tangential coefficient term xyp*p1
        // and xyp*p2
        xp_d += rs_2xp2 * p2 + 2.f * xyp * p1;
        yp_d += rs_2yp2 * p1 + 2.f * xyp * p2;
    }

    float xp_d_cx = xp_d + codx;
    float yp_d_cy = yp_d + cody;

    uv[0] = xp_d_cx * fx + cx;
    uv[1] = yp_d_cy * fy + cy;

    if (J_xy == 0)
    {
        return K4A_RESULT_SUCCEEDED;
    }

    // compute Jacobian matrix
    float dudrs = k1 + 2.f * k2 * rs + 3.f * k3 * rss;
    // compute d(b)/d(r^2)
    float dvdrs = k4 + 2.f * k5 * rs + 3.f * k6 * rss;
    float bis = bi * bi;
    float dddrs = (dudrs * b - a * dvdrs) * bis;

    float dddrs_2 = dddrs * 2.f;
    float xp_dddrs_2 = xp * dddrs_2;
    float yp_xp_dddrs_2 = yp * xp_dddrs_2;
    // compute d(u)/d(xp)
    if (camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT)
    {
        J_xy[0] = fx * (d + xp * xp_dddrs_2 + 6.f * xp * p2 + yp * p1);
        J_xy[1] = fx * (yp_xp_dddrs_2 + 2.f * yp * p2 + xp * p1);
        J_xy[2] = fy * (yp_xp_dddrs_2 + 2.f * xp * p1 + yp * p2);
        J_xy[3] = fy * (d + yp * yp * dddrs_2 + 6.f * yp * p1 + xp * p2);
    }
    else
    {
        J_xy[0] = fx * (d + xp * xp_dddrs_2 + 6.f * xp * p2 + 2.f * yp * p1);
        J_xy[1] = fx * (yp_xp_dddrs_2 + 2.f * yp * p2 + 2.f * xp * p1);
        J_xy[2] = fy * (yp_xp_dddrs_2 + 2.f * xp * p1 + 2.f * yp * p2);
        J_xy[3] = fy * (d + yp * yp * dddrs_2 + 6.f * yp * p1 + 2.f * xp * p2);
    }

    return K4A_RESULT_SUCCEEDED;
}

static void invert_2x2(const float J[2 * 2], float Jinv[2 * 2])
{
    float detJ = J[0] * J[3] - J[1] * J[2];
    float inv_detJ = 1.f / detJ;

    Jinv[0] = inv_detJ * J[3];
    Jinv[3] = inv_detJ * J[0];
    Jinv[1] = -inv_detJ * J[1];
    Jinv[2] = -inv_detJ * J[2];
}

static k4a_result_t transformation_iterative_unproject(const k4a_calibration_camera_t *camera_calibration,
                                                       const float uv[2],
                                                       float xy[2],
                                                       int *valid,
                                                       unsigned int max_passes)
{
    *valid = 1;
    float Jinv[2 * 2];
    float best_xy[2] = { 0.f, 0.f };
    float best_err = FLT_MAX;

    for (unsigned int pass = 0; pass < max_passes; pass++)
    {
        float p[2];
        float J[2 * 2];

        if (K4A_FAILED(TRACE_CALL(transformation_project_internal(camera_calibration, xy, p, valid, J))))
        {
            return K4A_RESULT_FAILED;
        }
        if (*valid == 0)
        {
            return K4A_RESULT_SUCCEEDED;
        }

        float err_x = uv[0] - p[0];
        float err_y = uv[1] - p[1];
        float err = err_x * err_x + err_y * err_y;
        if (err >= best_err)
        {
            xy[0] = best_xy[0];
            xy[1] = best_xy[1];
            break;
        }

        best_err = err;
        best_xy[0] = xy[0];
        best_xy[1] = xy[1];
        invert_2x2(J, Jinv);
        if (pass + 1 == max_passes || best_err < 1e-22f)
        {
            break;
        }

        float dx = Jinv[0] * err_x + Jinv[1] * err_y;
        float dy = Jinv[2] * err_x + Jinv[3] * err_y;

        xy[0] += dx;
        xy[1] += dy;
    }

    if (best_err > 1e-6f)
    {
        *valid = 0;
    }

    return K4A_RESULT_SUCCEEDED;
}

static k4a_result_t transformation_unproject_internal(const k4a_calibration_camera_t *camera_calibration,
                                                      const float uv[2],
                                                      float xy[2],
                                                      int *valid)
{
    if (K4A_FAILED(K4A_RESULT_FROM_BOOL(
            (camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT ||
             camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY) &&
            camera_calibration->intrinsics.parameter_count >= 14)))
    {
        LOG_ERROR("Unexpected camera calibration model type %d, should either be "
                  "K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT (%d) or "
                  "K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY (%d).",
                  camera_calibration->intrinsics.type,
                  K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT,
                  K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY);
        if (camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY)
        {
            LOG_ERROR("Unexpected parameter count %d, should be %d.",
                      camera_calibration->intrinsics.parameter_count,
                      14);
        }
        return K4A_RESULT_FAILED;
    }

    if (camera_calibration->intrinsics.type == K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT &&
        g_deprecated_6kt_message_fired == false)
    {
        g_deprecated_6kt_message_fired = true;
        LOG_CRITICAL("Rational 6KT is deprecated (only supported early internal devices). Please replace your Azure "
                     "Kinect with a retail device.",
                     0);
    }

    const k4a_calibration_intrinsic_parameters_t *params = &camera_calibration->intrinsics.parameters;

    float cx = params->param.cx;
    float cy = params->param.cy;
    float fx = params->param.fx;
    float fy = params->param.fy;
    float k1 = params->param.k1;
    float k2 = params->param.k2;
    float k3 = params->param.k3;
    float k4 = params->param.k4;
    float k5 = params->param.k5;
    float k6 = params->param.k6;
    float codx = params->param.codx; // center of distortion is set to 0 for Brown Conrady model
    float cody = params->param.cody;
    float p1 = params->param.p1;
    float p2 = params->param.p2;

    if (K4A_FAILED(K4A_RESULT_FROM_BOOL(fx > 0.f && fy > 0.f)))
    {
        LOG_ERROR("Expect both fx and fy are larger than 0, actual values are fx: %lf, fy: %lf.",
                  (double)fx,
                  (double)fy);
        return K4A_RESULT_FAILED;
    }

    // correction for radial distortion
    float xp_d = (uv[0] - cx) / fx - codx;
    float yp_d = (uv[1] - cy) / fy - cody;

    float rs = xp_d * xp_d + yp_d * yp_d;
    float rss = rs * rs;
    float rsc = rss * rs;
    float a = 1.f + k1 * rs + k2 * rss + k3 * rsc;
    float b = 1.f + k4 * rs + k5 * rss + k6 * rsc;
    float ai;
    if (a != 0.f)
    {
        ai = 1.f / a;
    }
    else
    {
        ai = 1.f;
    }
    float di = ai * b;

    xy[0] = xp_d * di;
    xy[1] = yp_d * di;

    // approximate correction for tangential params
    float two_xy = 2.f * xy[0] * xy[1];
    float xx = xy[0] * xy[0];
    float yy = xy[1] * xy[1];

    xy[0] -= (yy + 3.f * xx) * p2 + two_xy * p1;
    xy[1] -= (xx + 3.f * yy) * p1 + two_xy * p2;

    // add on center of distortion
    xy[0] += codx;
    xy[1] += cody;

    return transformation_iterative_unproject(camera_calibration, uv, xy, valid, 20);
}

k4a_result_t transformation_unproject(const k4a_calibration_camera_t *camera_calibration,
                                      const float point2d[2],
                                      const float depth,
                                      float point3d[3],
                                      int *valid)
{
    if (depth == 0.f)
    {
        point3d[0] = 0.f;
        point3d[1] = 0.f;
        point3d[2] = 0.f;
        *valid = 0;
        return K4A_RESULT_SUCCEEDED;
    }

    if (K4A_FAILED(TRACE_CALL(transformation_unproject_internal(camera_calibration, point2d, point3d, valid))))
    {
        return K4A_RESULT_FAILED;
    }

    point3d[0] *= depth;
    point3d[1] *= depth;
    point3d[2] = depth;

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t transformation_project(const k4a_calibration_camera_t *camera_calibration,
                                    const float point3d[3],
                                    float point2d[2],
                                    int *valid)
{
    if (point3d[2] <= 0.f)
    {
        point2d[0] = 0.f;
        point2d[1] = 0.f;
        *valid = 0;
        return K4A_RESULT_SUCCEEDED;
    }

    float xy[2];
    xy[0] = point3d[0] / point3d[2];
    xy[1] = point3d[1] / point3d[2];

    if (K4A_FAILED(TRACE_CALL(transformation_project_internal(camera_calibration, xy, point2d, valid, 0))))
    {
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}
