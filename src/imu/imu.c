// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
// This library
#include <k4ainternal/imu.h>

// Dependent libraries
#include <k4ainternal/common.h>
#include <k4ainternal/logging.h>
#include <k4ainternal/math.h>
#include <k4ainternal/queue.h>
#include <k4ainternal/calibration.h>

// System dependencies
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//**************Symbolic Constant Macros (defines)  *************
#define IMU_TEMPERATURE_DIVISOR 256
#define IMU_TEMPERATURE_CONSTANT 15
#define IMU_SCALE_NORMALIZATION 1000000

// The raw readings from accelerometer are in g's and in the SDK g = 9.81 m/s^2 is used as constant factor to convert
// it. This gravitational constant is consistent with the parameter used in device calibration. Changing this constant
// to a different value would break the IMU accelerometer calibration.
#define IMU_GRAVITATIONAL_CONSTANT 9.81f

// The raw readings from gyroscope are in degrees per second and in the SDK, is converted to radians per second.
#define PI 3.141592f
#define IMU_RADIANS_PER_DEGREES (PI / 180.0f)

// Set the max timestamp expected from the IMU after starting it with the color camera. If this value is too large then
// starting and stopping the IMU & color camera in rapid succession can result in timestamps going backwards near each
// IMU start.
#define MAX_IMU_TIME_STAMP_MS 1500

//************************ Typedefs *****************************

// parameters used to compute the calibrated IMU
typedef struct _imu_calibration_rectifier_t
{
    float bias_gyro[3];
    float bias_accel[3];
    float mixing_matrix_gyro[3 * 3];
    float mixing_matrix_accel[3 * 3];
} imu_calibration_rectifier_t;

typedef struct _imu_context_t
{
    TICK_COUNTER_HANDLE tick;
    colormcu_t color_mcu;
    queue_t queue;
    uint32_t dropped_count;
    float temperature;

    k4a_calibration_imu_t gyro_calibration;
    k4a_calibration_imu_t accel_calibration;
    imu_calibration_rectifier_t calibration_rectifier;

    bool running;
    bool wait_for_ts_reset;
} imu_context_t;

//************ Declarations (Statics and globals) ***************
K4A_DECLARE_CONTEXT(imu_t, imu_context_t);

//******************* Function Prototypes ***********************
usb_cmd_stream_cb_t imu_capture_ready;

//*********************** Functions *****************************
/**
 *  Callback function used with the command module to handle received captures from the IMU device
 *
 *  @param result
 *   Status of the received capture.  If this isn't K4A_RESULT_SUCCEEDED, the capture content is not guaranteed and
 * should be discarded.
 *
 *  @param image
 *   image resource for IMU. This contains all of the information on the received capture.
 *
 *  @param p_context
 *   Callback context.  In this function, this is the handle to the initiating object that has the queue.
 *
 * \remarks
 * Capture is safe to use during this callback as the caller ensures a ref is held. If the callback function wants the
 * capture to exist beyond this callback, a ref must be taken with capture_inc_ref().
 */
void imu_capture_ready(k4a_result_t result, k4a_image_t image, void *p_context)
{
    imu_context_t *p_imu = (imu_context_t *)p_context;
    uint8_t *p_packet;
    imu_payload_metadata_t *p_metadata = NULL;
    xyz_vector_t *p_gyro_data = NULL;
    xyz_vector_t *p_accel_data = NULL;
    size_t capture_size;

    // place capture in queue
    if (result != K4A_RESULT_SUCCEEDED)
    {
        LOG_WARNING("A streaming IMU transfer failed", 0);
        // Stop the queue - this will notify users waiting for data.
        queue_stop(p_imu->queue);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(image != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        // Take apart the capture packet data and create captures for each sample
        p_packet = image_get_buffer(image);
        capture_size = image_get_size(image);

        if (capture_size < sizeof(imu_payload_metadata_t))
        {
            LOG_ERROR("IMU streaming payload size too small for imu_payload_metadata_t: %zu", capture_size);
            return;
        }
        else
        {
            p_metadata = (imu_payload_metadata_t *)p_packet;
        }

        if (capture_size < sizeof(imu_payload_metadata_t) + (p_metadata->gyro.sample_count * sizeof(xyz_vector_t)))
        {
            LOG_ERROR("IMU streaming payload size too small for gyro samples: %u size: %zu",
                      p_metadata->gyro.sample_count,
                      capture_size);
            return;
        }
        else
        {

            p_gyro_data = (xyz_vector_t *)(p_packet + sizeof(imu_payload_metadata_t));
        }

        if (capture_size < sizeof(imu_payload_metadata_t) + (p_metadata->gyro.sample_count * sizeof(xyz_vector_t)) +
                               (p_metadata->accel.sample_count * sizeof(xyz_vector_t)))
        {
            LOG_ERROR("IMU streaming payload size too small for accelerometer samples: %u size: %zu",
                      p_metadata->accel.sample_count,
                      capture_size);
            return;
        }
        else
        {
            p_accel_data = (xyz_vector_t *)(p_packet + sizeof(imu_payload_metadata_t) +
                                            (p_metadata->gyro.sample_count * sizeof(xyz_vector_t)));
        }

        if (p_metadata->gyro.sample_count != p_metadata->accel.sample_count)
        {
            LOG_WARNING("IMU payload sample accel(%d) != sample gyro(%d)",
                        p_metadata->accel.sample_count,
                        p_metadata->gyro.sample_count);
        }

        for (uint32_t i = 0; i < p_metadata->gyro.sample_count && i < p_metadata->accel.sample_count; i++)
        {
            result = K4A_RESULT_SUCCEEDED;

            // When starting the color camera the TS of the IMU gets reset back to 0. The process takes a couple seconds
            // at start up. So when the color camera start is recent this code waits for the IMU timestamp to drop to a
            // time near zero.
            if (p_imu->wait_for_ts_reset)
            {
                if (K4A_90K_HZ_TICK_TO_USEC(p_accel_data[i].pts) > (MAX_IMU_TIME_STAMP_MS * 1000))
                {
                    result = K4A_RESULT_FAILED; // dropping this IMU sample
                    p_imu->dropped_count++;
                }
                else
                {
                    if (p_imu->dropped_count != 0)
                    {
                        LOG_INFO("IMU startup dropped last %d samples, the timestamp is too large",
                                 p_imu->dropped_count);
                    }
                    p_imu->dropped_count = 0;
                    p_imu->wait_for_ts_reset = false;
                }
            }

            k4a_capture_t imu_capture = NULL;
            k4a_image_t imu_image = NULL;

            if (K4A_SUCCEEDED(result))
            {
                result = TRACE_CALL(
                    image_create_empty_internal(ALLOCATION_SOURCE_IMU, sizeof(k4a_imu_sample_t), &imu_image));
            }

            if (K4A_SUCCEEDED(result))
            {
                k4a_imu_sample_t sample = { 0 };
                sample.temperature = ((float)(p_metadata->temperature.value) / IMU_TEMPERATURE_DIVISOR) +
                                     IMU_TEMPERATURE_CONSTANT;
                sample.gyro_sample.xyz.x = (float)p_gyro_data[i].rx * p_metadata->gyro.sensitivity *
                                           IMU_RADIANS_PER_DEGREES / IMU_SCALE_NORMALIZATION;
                sample.gyro_sample.xyz.y = (float)p_gyro_data[i].ry * p_metadata->gyro.sensitivity *
                                           IMU_RADIANS_PER_DEGREES / IMU_SCALE_NORMALIZATION;
                sample.gyro_sample.xyz.z = (float)p_gyro_data[i].rz * p_metadata->gyro.sensitivity *
                                           IMU_RADIANS_PER_DEGREES / IMU_SCALE_NORMALIZATION;
                sample.gyro_timestamp_usec = K4A_90K_HZ_TICK_TO_USEC(p_gyro_data[i].pts);
                sample.acc_sample.xyz.x = (float)p_accel_data[i].rx * p_metadata->accel.sensitivity *
                                          IMU_GRAVITATIONAL_CONSTANT / IMU_SCALE_NORMALIZATION;
                sample.acc_sample.xyz.y = (float)p_accel_data[i].ry * p_metadata->accel.sensitivity *
                                          IMU_GRAVITATIONAL_CONSTANT / IMU_SCALE_NORMALIZATION;
                sample.acc_sample.xyz.z = (float)p_accel_data[i].rz * p_metadata->accel.sensitivity *
                                          IMU_GRAVITATIONAL_CONSTANT / IMU_SCALE_NORMALIZATION;
                sample.acc_timestamp_usec = K4A_90K_HZ_TICK_TO_USEC(p_accel_data[i].pts);

                memcpy(image_get_buffer(imu_image), &sample, sizeof(k4a_imu_sample_t));
            }

            if (K4A_SUCCEEDED(result))
            {
                result = TRACE_CALL(capture_create(&imu_capture));
            }

            if (K4A_SUCCEEDED(result))
            {
                capture_set_imu_image(imu_capture, imu_image);
                queue_push(p_imu->queue, imu_capture);
            }

            if (imu_image)
            {
                // remove local ref on image
                image_dec_ref(imu_image);
            }
            if (imu_capture)
            {
                // remove local ref on capture
                capture_dec_ref(imu_capture); // release our interest in the new capture
            }
        }
    }
}

/**
 *  Function to refresh the bias and mixing matrix rectifier for calibration based on sensor temperature
 *
 *  @param calibration
 *   Handle to this calibration parameters
 *
 *  @param temperature
 *   Value of sensor temperature
 *
 *  @param bias
 *   Pointer to bias rectifier
 *
 *  @param mixing_matrix
 *   Pointer to mixing matrix rectifier
 */
static void imu_refresh_bias_and_mixing_matrix(const k4a_calibration_imu_t calibration,
                                               float temperature,
                                               float *bias,
                                               float *mixing_matrix)
{
    assert(calibration.model_type_mask);

    const unsigned int MODEL_COEFFICIENTS = sizeof(calibration.bias_temperature_model) / (3 * sizeof(float));

    for (unsigned int row = 0; row < 3; ++row)
    {
        bias[row] = math_eval_poly_3(temperature, &calibration.bias_temperature_model[row * MODEL_COEFFICIENTS]);

        for (unsigned int col = 0; col < 3; ++col)
        {
            mixing_matrix[3 * row + col] =
                math_eval_poly_3(temperature,
                                 &calibration.mixing_matrix_temperature_model[(3 * row + col) * MODEL_COEFFICIENTS]);
        }
    }
}

/**
 *  Function to update the bias and mixing matrix based on sensor temperature
 *
 *  @param gyro_temperature
 *   Value of gyro sensor temperature
 *
 *  @param accel_temperature
 *   Value of accel sensor temperature
 *
 *  @param p_imu
 *   Pointer to the imu context, which includes the calibration information.
 */
static void imu_update_calibration_with_temperature(float gyro_temperature,
                                                    float accel_temperature,
                                                    imu_context_t *p_imu)
{
    imu_refresh_bias_and_mixing_matrix(p_imu->gyro_calibration,
                                       gyro_temperature,
                                       p_imu->calibration_rectifier.bias_gyro,
                                       p_imu->calibration_rectifier.mixing_matrix_gyro);

    imu_refresh_bias_and_mixing_matrix(p_imu->accel_calibration,
                                       accel_temperature,
                                       p_imu->calibration_rectifier.bias_accel,
                                       p_imu->calibration_rectifier.mixing_matrix_accel);
}

/**
 *  Function for creating and initializing the IMU object associated with
 *  a specific instance.
 *
 *  @param tick_handle
 *   handle to get the system tick
 *
 *  @param color_mcu
 *   Handle to entity that owns the handle to talk with the hardware
 *
 *  @param p_imu_handle
 *   Pointer to where to place the object handle
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED    Operation successful
 *   K4A_RESULT_FAILED       Operation failed
 */
k4a_result_t
imu_create(TICK_COUNTER_HANDLE tick_handle, colormcu_t color_mcu, calibration_t calibration_handle, imu_t *p_imu_handle)
{
    k4a_result_t result = K4A_RESULT_FAILED;
    imu_context_t *p_imu;

    // Validate parameters
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, color_mcu == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, calibration_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, tick_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, p_imu_handle == NULL);

    // Create the handle
    p_imu = imu_t_create(p_imu_handle);

    // Assign handle to device
    p_imu->color_mcu = color_mcu;
    p_imu->tick = tick_handle;
    p_imu->temperature = 0;

    // Create allocator handle

    result = TRACE_CALL(
        queue_create(QUEUE_CALC_DEPTH(K4A_IMU_SAMPLE_RATE, QUEUE_DEFAULT_DEPTH_USEC), "Queue_imu", &p_imu->queue));

    if (K4A_SUCCEEDED(result))
    {
        // Register stream callback with stream engine
        result = colormcu_imu_register_stream_cb(color_mcu, imu_capture_ready, p_imu);
    }

    if (K4A_SUCCEEDED(result))
    {

        result = TRACE_CALL(
            calibration_get_imu(calibration_handle, K4A_CALIBRATION_TYPE_GYRO, &p_imu->gyro_calibration));
    }

    if (K4A_SUCCEEDED(result))
    {

        result = TRACE_CALL(
            calibration_get_imu(calibration_handle, K4A_CALIBRATION_TYPE_ACCEL, &p_imu->accel_calibration));
    }

    if (K4A_SUCCEEDED(result))
    {
        imu_update_calibration_with_temperature(p_imu->gyro_calibration.temperature_in_c,
                                                p_imu->accel_calibration.temperature_in_c,
                                                p_imu);
    }

    if (K4A_SUCCEEDED(result))
    {
        // SDK may have crashed last session, so call stop()
        p_imu->running = true;
        imu_stop(*p_imu_handle);
    }

    if (K4A_FAILED(result))
    {
        imu_destroy(*p_imu_handle);
        *p_imu_handle = NULL;
    }
    return result;
}

/**
 *  Function for destroying this instance of the IMU object
 *
 *  @param imu_handle
 *   Handle to this specific object
 *
 *  @return
 *   None
 */
void imu_destroy(imu_t imu_handle)
{
    imu_context_t *imu = imu_t_get_context(imu_handle);

    // implicit stop
    imu_stop(imu_handle);

    // Destroy queue
    if (imu->queue != NULL)
    {
        queue_destroy(imu->queue);
        imu->queue = NULL;
    }

    imu_t_destroy(imu_handle);
}

/**
 *  Function to adjust the sensor measurement according to calibration data
 *
 *  @param p_imu_sample
 *   Pointer to this specific imu sample
 *
 *  @param p_imu
 *   Pointer to the imu context, which includes the calibration information.
 *
 */
static void imu_apply_intrinsic_calibration(k4a_imu_sample_t *p_imu_sample, const imu_context_t *p_imu)
{
    math_affine_transform_3(p_imu->calibration_rectifier.mixing_matrix_gyro,
                            p_imu_sample->gyro_sample.v,
                            p_imu->calibration_rectifier.bias_gyro,
                            p_imu_sample->gyro_sample.v);

    math_quadratic_transform_3(p_imu->calibration_rectifier.mixing_matrix_accel,
                               p_imu->accel_calibration.second_order_scaling,
                               p_imu_sample->acc_sample.v,
                               p_imu->calibration_rectifier.bias_accel,
                               p_imu_sample->acc_sample.v);
}

/**
 *  Function to get the next capture in the stream.  Note, if excessive time has passed since the last call, some
 * captures may have been discarded.
 *
 *  @param imu_handle
 *   Handle to this specific object
 *
 *  @param imu_sample
 *   Pointer to where the sample will be written to
 *
 *  @param timeout_in_ms
 *   Number of mSecs to wait until timing out for getting a capture
 *
 *  @return
 *   K4A_WAIT_RESULT_TIMEOUT     Operation timed out
 *   K4A_WAIT_RESULT_SUCCEEDED   Operation was successful and a capture was retrieved
 *   K4A_WAIT_RESULT_FAILED      Operation failed due to invalid input or unknown reason
 */
k4a_wait_result_t imu_get_sample(imu_t imu_handle, k4a_imu_sample_t *imu_sample, int32_t timeout_in_ms)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_WAIT_RESULT_FAILED, imu_t, imu_handle);
    RETURN_VALUE_IF_ARG(K4A_WAIT_RESULT_FAILED, (imu_sample == NULL));

    k4a_wait_result_t wresult = K4A_WAIT_RESULT_SUCCEEDED;
    imu_context_t *p_imu = imu_t_get_context(imu_handle);
    k4a_capture_t capture = NULL;
    k4a_image_t image = NULL;
    uint8_t *buffer = NULL;

    wresult = queue_pop(p_imu->queue, timeout_in_ms, &capture);

    if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
    {
        k4a_result_t result = K4A_RESULT_FROM_BOOL((image = capture_get_imu_image(capture)) != NULL);
        if (K4A_FAILED(result))
        {
            wresult = K4A_WAIT_RESULT_FAILED;
        }
    }

    if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
    {
        buffer = image_get_buffer(image);
        k4a_result_t result = K4A_RESULT_FROM_BOOL(buffer != NULL);
        if (K4A_FAILED(result))
        {
            wresult = K4A_WAIT_RESULT_FAILED;
        }
    }

    if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
    {
        assert(sizeof(k4a_imu_sample_t) <= image_get_size(image));
        memcpy(imu_sample, buffer, sizeof(k4a_imu_sample_t));

        // update the calibration when the temperature changes more than 0.25C
        if ((imu_sample->temperature > (p_imu->temperature + 0.25f)) ||
            (imu_sample->temperature < (p_imu->temperature - 0.25f)))
        {
            imu_update_calibration_with_temperature(imu_sample->temperature, imu_sample->temperature, p_imu);
            p_imu->temperature = imu_sample->temperature;
        }
        // The application of intrinsic calibration is delayed until the IMU sample is queried.
        imu_apply_intrinsic_calibration(imu_sample, p_imu);
    }

    if (image)
    {
        image_dec_ref(image);
    }
    if (capture)
    {
        capture_dec_ref(capture);
    }

    return wresult;
}

/**
 *  Function to start the IMU stream.
 *
 *  @param imu_handle
 *   Handle to this specific object
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED    Operation was successful
 *   K4A_RESULT_FAILED       Operation was not successful
 */
k4a_result_t imu_start(imu_t imu_handle, tickcounter_ms_t color_camera_start_tick)
{
    imu_context_t *p_imu = imu_t_get_context(imu_handle);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    tickcounter_ms_t current_tick;

    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, p_imu == NULL);

    p_imu->running = true;
    queue_enable(p_imu->queue);

    p_imu->wait_for_ts_reset = false;
    if (color_camera_start_tick != 0)
    {
        // Color camera start reset's time stamp. So it can cause IMU time stamps to look as if they are going
        // backwards. If start happened recently, then we will wait for time stamps reset before sending them on to the
        // user. However we only do this when we know the color camera recently started.
        result = K4A_RESULT_FROM_BOOL(tickcounter_get_current_ms(p_imu->tick, &current_tick) == 0);
        if (K4A_SUCCEEDED(result))
        {
            if ((current_tick - color_camera_start_tick) <= MAX_IMU_TIME_STAMP_MS)
            {
                p_imu->wait_for_ts_reset = true;
            }
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        result = colormcu_imu_start_streaming(p_imu->color_mcu);
    }
    return result;
}

/**
 *  Function to stop the IMU stream.
 *
 *  @param imu_handle
 *   Handle to this specific object
 *
 */
void imu_stop(imu_t imu_handle)
{
    imu_context_t *p_imu = imu_t_get_context(imu_handle);

    RETURN_VALUE_IF_ARG(VOID_VALUE, p_imu == NULL);

    // It is ok to call this multiple times, so no lock. Only doing it once is an optimization to not stop if the sensor
    // was never started.
    if (p_imu->running)
    {
        colormcu_imu_stop_streaming(p_imu->color_mcu);
        queue_disable(p_imu->queue);
    }
    p_imu->running = false;
}

/**
 *  Function returning a pointer to extrinsic calibration of gyro.
 *
 *  @param imu_handle
 *   Handle to this specific object
 *
 */
k4a_calibration_extrinsics_t *imu_get_gyro_extrinsics(imu_t imu_handle)
{
    imu_context_t *p_imu = imu_t_get_context(imu_handle);

    RETURN_VALUE_IF_ARG(0, p_imu == NULL);

    return &p_imu->gyro_calibration.depth_to_imu;
}

/**
 *  Function returning a pointer to extrinsic calibration of accel.
 *
 *  @param imu_handle
 *   Handle to this specific object
 *
 */
k4a_calibration_extrinsics_t *imu_get_accel_extrinsics(imu_t imu_handle)
{
    imu_context_t *p_imu = imu_t_get_context(imu_handle);

    RETURN_VALUE_IF_ARG(0, p_imu == NULL);

    return &p_imu->accel_calibration.depth_to_imu;
}

#ifdef __cplusplus
}
#endif
