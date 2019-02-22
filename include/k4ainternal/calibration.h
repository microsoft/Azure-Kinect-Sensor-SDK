/** \file calibration.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <k4a/k4atypes.h>
#include <k4ainternal/depth_mcu.h>
#include <k4ainternal/color_mcu.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handle to the calibration module
 *
 * Handles are created with \ref calibration_create and closed
 * with \ref color_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(calibration_t);

/** The number of coefficients in temperature model for imu sensor calibration
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
#define CALIBRATION_INERTIALSENSOR_TEMPERATURE_MODEL_COEFFICIENTS 4 /**< polynomial of degree 3 or less */

/** IMU calibration contains Inertial intrinsic and extrinsic calibration information
 *
 * \relates k4a_calibration_imu_t
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef struct _k4a_calibration_imu_t
{
    k4a_calibration_extrinsics_t depth_to_imu; /**< extrinsic calibration data */

    double model_type_mask; /**<  bitmask with values defined in CALIBRATION_InertialSensorType */

    /**< for each vector in IMU sample, there are 3 elements (x,y and z). */
    float noise[3 * 2];     /**<  Optional: 3x2 Standard deviation of the noise, indices {3,4,5} form linear model with
                               temperature */
    float temperature_in_c; /**<  Optional: Calibration temperature in Celsius (when the bias/mixingmatrix model is
                               constant over temperature). */

    /**< Bias a polynomial function of temperature.
         Defined as ==> CALIBRATED_SAMPLE = MIXING_MATRIX * RAW_SAMPLE + BIAS
         All coefficients of x then y then z, low-order coefficients first.
         3x4: Offset polynomial in temperature (4 coefficients each) */
    float bias_temperature_model[3 * CALIBRATION_INERTIALSENSOR_TEMPERATURE_MODEL_COEFFICIENTS];

    /**< 3x3 mixing matrix where each element is a cubic polynomial of temperature
         Defined as ==> CALIBRATED_SAMPLE = MIXING_MATRIX * RAW_SAMPLE + BIAS
         row order, all coefficients of x, then y, then z, low-order coefficients first.
         3x3x4 :Mixing matrix(3x3) polynomials in (4 coefficients each) */
    float mixing_matrix_temperature_model[3 * 3 * CALIBRATION_INERTIALSENSOR_TEMPERATURE_MODEL_COEFFICIENTS];

    float second_order_scaling[3 * 3]; /**< Optional: 2nd-order scaling term on raw measurement 3x3 matrix in row major
                                          order */
    float bias_uncertainty[3];         /**< initial variance for each channel */
    float temperature_bounds[2]; /**< temperature bounds (interval over which calibration was performed like, trivial
                                    example: [0,20]), in Celsius */
} k4a_calibration_imu_t;

/** Creates an calibration instance
 *
 * \param depthmcu
 *  Handle to the depthmcu that
 *
 * \param calibration_handle
 * pointer to a handle location to store the handle. This is only written on K4A_RESULT_SUCCEEDED;
 *
 * To cleanup this resource call \ref calibration_destroy.
 *
 * \return K4A_RESULT_SUCCEEDED is returned on success, otherwise K4A_RESULT_FAILED is returned
 */
k4a_result_t calibration_create(depthmcu_t depthmcu, calibration_t *calibration_handle);

/** Creates an calibration instance
 *
 * \param raw_calibration
 *  Raw JSON calibration string, which is null terminated.
 *
 * \param raw_calibration_size
 *  Raw JSON calibration string size
 *
 * \param depth_calibration
 * pointer to a calibration structure to store the depth camera calibration.
 * This is only written on K4A_RESULT_SUCCEEDED;
 *
 * \param color_calibration
 * pointer to a calibration structure to store the color camera calibration.
 * This is only written on K4A_RESULT_SUCCEEDED;
 *
 * \param gyro_calibration
 * pointer to a calibration structure to store the gyro sensor calibration.
 * This is only written on K4A_RESULT_SUCCEEDED;
 *
 * \param accel_calibration
 * pointer to a calibration structure to store the accel sensor calibration.
 *
 * This is only written on K4A_RESULT_SUCCEEDED;
 *
 * To cleanup this resource call \ref calibration_destroy.
 *
 * \return K4A_RESULT_SUCCEEDED is returned on success, otherwise K4A_RESULT_FAILED is returned
 */
k4a_result_t calibration_create_from_raw(char *raw_calibration,
                                         size_t raw_calibration_size,
                                         k4a_calibration_camera_t *depth_calibration,
                                         k4a_calibration_camera_t *color_calibration,
                                         k4a_calibration_imu_t *gyro_calibration,
                                         k4a_calibration_imu_t *accel_calibration);

/** Destroys an calibration instance
 *
 * \param calibration_handle
 * The calibration handle to destroy
 *
 * This call cleans up the calibration handle and its internal resources.
 */
void calibration_destroy(calibration_t calibration_handle);

/** Get the intrinsic and extrinsic camera calibration for depth and / or color sensors
 *
 * \param calibration_handle
 * Handle to the calibration module.
 *
 * \param type
 * The type of calibration the user wants.
 *
 * \param calibration
 * Location to write the intrinsic and extrinsic calibration to.
 *
 * \returns result_success if calibration was successfully written, otherwise K4A_RESULT_FAILED
 */
k4a_result_t calibration_get_camera(calibration_t calibration_handle,
                                    k4a_calibration_type_t type,
                                    k4a_calibration_camera_t *calibration);

/** Get the imu calibration for gyro and / or accel sensors
 *
 * \param calibration_handle
 * Handle to the calibration module.
 *
 * \param type
 * The type of calibration the user wants.
 *
 * \param calibration
 * Location to write the calibration to.
 *
 * \returns result_success if calibration was successfully written, otherwise K4A_RESULT_FAILED
 */
k4a_result_t calibration_get_imu(calibration_t calibration_handle,
                                 k4a_calibration_type_t type,
                                 k4a_calibration_imu_t *calibration);

/** Get the intrinsic and extrinsic camera calibration for depth and / or color sensors
 *
 * \param calibration_handle
 * Handle to the calibration module.
 *
 * \param data
 * Location to write the calibration raw data to.
 *
 * \param data_size
 * On passing data_size into the function this variable represents the available size to write the raw data to. On
 * return this variable is updated with the amount of data actually written to the buffer.
 *
 * \returns buffer_result_success if data was successfully written, otherwise K4A_BUFFER_RESULT_FAILED. if data is
 * specified as NULL then K4A_BUFFER_RESULT_TOO_SMALL is returned and data_size is updated to contain the minimum buffer
 * size needed to capture the calibration data.
 */
k4a_buffer_result_t calibration_get_raw_data(calibration_t calibration_handle, uint8_t *data, size_t *data_size);

#ifdef __cplusplus
}
#endif

#endif /* CALIBRATION_H */
