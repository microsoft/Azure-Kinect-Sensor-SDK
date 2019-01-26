/** \file IMU.h
 * Kinect For Azure SDK.
 */

#ifndef IMU_H
#define IMU_H

#include <k4a/k4atypes.h>
#include <k4ainternal/handle.h>
#include <k4ainternal/logging.h>
#include <k4ainternal/color_mcu.h>
#include <k4ainternal/calibration.h>
#include <azure_c_shared_utility/tickcounter.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handle to the imu device.
 *
 * Handles are created with \ref imu_create and closed
 * with \ref imu_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(imu_t);

/** Open a handle to the IMU device.
 *
 * \param tick_handle [IN]
 *    Handle to access the system tick
 *
 * \param color_mcu [IN]
 *    Handle to entity owning the device handle
 *
 * \param calibration_handle [IN]
 * Handle to the calibration module.
 *
 * \param p_imu_handle [OUT]
 *    A pointer to write the opened imu device handle to
 *
 * \return K4A_RESULT_SUCCEEDED if the device was opened, otherwise K4A_RESULT_FAILED
 *
 * If successful, \ref imu_create will return a IMU device handle in the IMU
 * parameter. This handle grants exclusive access to the device and may be used in
 * the other k4a API calls.
 *
 * When done with the device, close the handle with \ref imu_destroy
 */
k4a_result_t imu_create(TICK_COUNTER_HANDLE tick_handle,
                        colormcu_t color_mcu,
                        calibration_t calibration_handle,
                        imu_t *p_imu_handle);

/** Closes the imu module and free's it resources
 * */
void imu_destroy(imu_t imu_handle);

k4a_wait_result_t imu_get_sample(imu_t imu_handle, k4a_imu_sample_t *imu_sample, int32_t timeout_in_ms);

// TODO
k4a_result_t imu_start(imu_t imu_handle, tickcounter_ms_t color_camera_start_tick);

// TODO
void imu_stop(imu_t imu_handle);

k4a_calibration_extrinsics_t *imu_get_gyro_extrinsics(imu_t imu_handle);

k4a_calibration_extrinsics_t *imu_get_accel_extrinsics(imu_t imu_handle);

#ifdef __cplusplus
}
#endif

#endif /* IMU_H */
