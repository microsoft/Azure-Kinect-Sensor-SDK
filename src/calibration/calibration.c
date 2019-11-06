// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/calibration.h>

// Dependent libraries
#include <k4ainternal/common.h>
#include <cJSON.h>
#include <locale.h> //cJSON.h need this set correctly.

// System dependencies
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define READ_RETRY_ALLOC_INCREASE (5 * 1024)
#define READ_RETRY_BASE_ALLOCATION (10 * 1024)
#define MAX_READ_RETRIES (10)

typedef struct _INTRINSIC_TYPE_TO_STRING_MAPPER
{
    k4a_calibration_model_type_t type_e;
    char *type_s;
} intrinsic_type_to_string_mapper_t;

static intrinsic_type_to_string_mapper_t intrinsic_type_mapper[] =
    { { K4A_CALIBRATION_LENS_DISTORTION_MODEL_THETA, "CALIBRATION_LensDistortionModelTheta" },
      { K4A_CALIBRATION_LENS_DISTORTION_MODEL_POLYNOMIAL_3K, "CALIBRATION_LensDistortionModelPolynomial3K" },
      { K4A_CALIBRATION_LENS_DISTORTION_MODEL_RATIONAL_6KT, "CALIBRATION_LensDistortionModelRational6KT" },
      { K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY, "CALIBRATION_LensDistortionModelBrownConrady" } };

typedef struct _calibration_context_t
{
    depthmcu_t depthmcu;

    size_t json_size;
    char *json; // string representation of JSON file

    k4a_calibration_camera_t depth_calibration;
    k4a_calibration_camera_t color_calibration;
    k4a_calibration_imu_t gyro_calibration;
    k4a_calibration_imu_t accel_calibration;
} calibration_context_t;

K4A_DECLARE_CONTEXT(calibration_t, calibration_context_t);

static k4a_result_t fill_array_of_floats(cJSON *json, float *data, unsigned int length)
{
    k4a_result_t result;
    unsigned int elements_to_read = 0;
    result = K4A_RESULT_FROM_BOOL(cJSON_IsArray(json) == true);

    if (K4A_SUCCEEDED(result))
    {
        elements_to_read = (unsigned int)cJSON_GetArraySize(json);
        result = K4A_RESULT_FROM_BOOL(elements_to_read <= length); // unexpected - too many element to read
        elements_to_read = elements_to_read <= length ? elements_to_read : length;
    }

    if (K4A_SUCCEEDED(result))
    {
        unsigned int x = 0;
        cJSON *element = NULL;
        cJSON_ArrayForEach(element, json)
        {
            result = K4A_RESULT_FROM_BOOL(element != NULL);

            if (K4A_SUCCEEDED(result))
            {
                result = K4A_RESULT_FROM_BOOL(cJSON_IsNumber(element) == true);
            }

            if (K4A_SUCCEEDED(result))
            {
                data[x++] = (float)element->valuedouble;
            }

            if (K4A_FAILED(result) || x >= elements_to_read)
            {
                break;
            }
        }
    }
    return result;
}

static k4a_result_t fill_rotation_matrix(cJSON *rt, k4a_calibration_extrinsics_t *extrinsics)
{
    k4a_result_t result;
    cJSON *rotation = NULL;
    cJSON *translation = NULL;

    rotation = cJSON_GetObjectItem(rt, "Rotation");
    result = K4A_RESULT_FROM_BOOL(rotation != NULL);

    if (K4A_SUCCEEDED(result))
    {
        translation = cJSON_GetObjectItem(rt, "Translation");
        result = K4A_RESULT_FROM_BOOL(translation != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_array_of_floats(rotation, extrinsics->rotation, COUNTOF(extrinsics->rotation));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_array_of_floats(translation, extrinsics->translation, COUNTOF(extrinsics->translation));
        // raw calibration stores extrinsics in meters. convert to mm to align with k4a depth resolution.
        for (unsigned int i = 0; i < COUNTOF(extrinsics->translation); i++)
        {
            extrinsics->translation[i] *= 1000.f;
        }
    }

    return result;
}

static k4a_result_t fill_intrinsics(cJSON *intrinsics, k4a_calibration_intrinsics_t *intrinsic_data)
{
    k4a_result_t result;
    cJSON *count;
    cJSON *type = NULL;
    cJSON *parameters = NULL;
    unsigned int parameter_count = 0;

    count = cJSON_GetObjectItem(intrinsics, "ModelParameterCount");
    result = K4A_RESULT_FROM_BOOL(count != NULL);

    if (K4A_SUCCEEDED(result))
    {
        type = cJSON_GetObjectItem(intrinsics, "ModelType");
        result = K4A_RESULT_FROM_BOOL(type != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        parameters = cJSON_GetObjectItem(intrinsics, "ModelParameters");
        result = K4A_RESULT_FROM_BOOL(parameters != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cJSON_IsNumber(count) == true);
    }

    if (K4A_SUCCEEDED(result))
    {
        const unsigned int max_size = COUNTOF(intrinsic_data->parameters.v);
        parameter_count = (unsigned int)count->valueint;
        result = K4A_RESULT_FROM_BOOL(parameter_count <= max_size);
        parameter_count = parameter_count <= max_size ? parameter_count : max_size;
        intrinsic_data->parameter_count = parameter_count;
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_array_of_floats(parameters, intrinsic_data->parameters.v, parameter_count);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cJSON_IsString(type) == true);
    }
    if (K4A_SUCCEEDED(result))
    {
        intrinsic_data->type = K4A_CALIBRATION_LENS_DISTORTION_MODEL_UNKNOWN;
        for (unsigned int x = 0; x < COUNTOF(intrinsic_type_mapper); x++)
        {
            if (strcmp(type->valuestring, intrinsic_type_mapper[x].type_s) == 0)
            {
                intrinsic_data->type = intrinsic_type_mapper[x].type_e;
                break;
            }
        }
        result = K4A_RESULT_FROM_BOOL(intrinsic_data->type != K4A_CALIBRATION_LENS_DISTORTION_MODEL_UNKNOWN);
    }
    return result;
}

static k4a_result_t fill_in_camera_cal_data(k4a_calibration_camera_t *cal, cJSON *camera)
{
    k4a_result_t result;
    cJSON *rotation = NULL;
    cJSON *intrinsics = NULL;
    cJSON *height = NULL;
    cJSON *width = NULL;
    cJSON *radius = NULL;

    intrinsics = cJSON_GetObjectItem(camera, "Intrinsics");
    result = K4A_RESULT_FROM_BOOL(intrinsics != NULL);
    if (K4A_SUCCEEDED(result))
    {
        rotation = cJSON_GetObjectItem(camera, "Rt");
        result = K4A_RESULT_FROM_BOOL(rotation != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_rotation_matrix(rotation, &cal->extrinsics);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_intrinsics(intrinsics, &cal->intrinsics);
    }

    if (K4A_SUCCEEDED(result))
    {
        height = cJSON_GetObjectItem(camera, "SensorHeight");
        result = K4A_RESULT_FROM_BOOL(height != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        width = cJSON_GetObjectItem(camera, "SensorWidth");
        result = K4A_RESULT_FROM_BOOL(width != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        radius = cJSON_GetObjectItem(camera, "MetricRadius");
        result = K4A_RESULT_FROM_BOOL(radius != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cJSON_IsNumber(height) == true);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cJSON_IsNumber(width) == true);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cJSON_IsNumber(radius) == true);
    }

    if (K4A_SUCCEEDED(result))
    {
        cal->resolution_width = width->valueint;
        cal->resolution_height = height->valueint;
        cal->metric_radius = (float)radius->valuedouble;
        // metric_radius equals 0 means that calibration failed to estimate this parameter
        if (cal->metric_radius <= 0.0001f)
        {
            cal->metric_radius = 1.7f; // default value corresponds to ~120 degree FoV
        }
    }

    return result;
}

static k4a_result_t fill_in_imu_cal_data(k4a_calibration_imu_t *cal, cJSON *inertial_sensor)
{
    k4a_result_t result;
    cJSON *bias = NULL;
    cJSON *bias_uncertainty = NULL;
    cJSON *mixing_matrix = NULL;
    cJSON *model_type_mask = NULL;
    cJSON *noise = NULL;
    cJSON *rotation = NULL;
    cJSON *second_order_scaling = NULL;
    cJSON *temperature_bounds = NULL;
    cJSON *temperature = NULL;

    bias = cJSON_GetObjectItem(inertial_sensor, "BiasTemperatureModel");
    result = K4A_RESULT_FROM_BOOL(bias != NULL);

    if (K4A_SUCCEEDED(result))
    {
        bias_uncertainty = cJSON_GetObjectItem(inertial_sensor, "BiasUncertainty");
        result = K4A_RESULT_FROM_BOOL(bias_uncertainty != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        mixing_matrix = cJSON_GetObjectItem(inertial_sensor, "MixingMatrixTemperatureModel");
        result = K4A_RESULT_FROM_BOOL(mixing_matrix != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        model_type_mask = cJSON_GetObjectItem(inertial_sensor, "ModelTypeMask");
        result = K4A_RESULT_FROM_BOOL(model_type_mask != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        noise = cJSON_GetObjectItem(inertial_sensor, "Noise");
        result = K4A_RESULT_FROM_BOOL(noise != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        rotation = cJSON_GetObjectItem(inertial_sensor, "Rt");
        result = K4A_RESULT_FROM_BOOL(rotation != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        second_order_scaling = cJSON_GetObjectItem(inertial_sensor, "SecondOrderScaling");
        result = K4A_RESULT_FROM_BOOL(second_order_scaling != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        temperature_bounds = cJSON_GetObjectItem(inertial_sensor, "TemperatureBounds");
        result = K4A_RESULT_FROM_BOOL(temperature_bounds != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        temperature = cJSON_GetObjectItem(inertial_sensor, "TemperatureC");
        result = K4A_RESULT_FROM_BOOL(temperature != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_array_of_floats(bias, cal->bias_temperature_model, COUNTOF(cal->bias_temperature_model));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_array_of_floats(bias_uncertainty, cal->bias_uncertainty, COUNTOF(cal->bias_uncertainty));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_array_of_floats(mixing_matrix,
                                      cal->mixing_matrix_temperature_model,
                                      COUNTOF(cal->mixing_matrix_temperature_model));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cJSON_IsNumber(model_type_mask) == true);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_array_of_floats(noise, cal->noise, COUNTOF(cal->noise));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_rotation_matrix(rotation, &cal->depth_to_imu);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_array_of_floats(second_order_scaling,
                                      cal->second_order_scaling,
                                      COUNTOF(cal->second_order_scaling));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = fill_array_of_floats(temperature_bounds, cal->temperature_bounds, COUNTOF(cal->temperature_bounds));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cJSON_IsNumber(temperature) == true);
    }

    if (K4A_SUCCEEDED(result))
    {
        cal->temperature_in_c = (float)temperature->valuedouble;
        cal->model_type_mask = model_type_mask->valuedouble;
    }

    return result;
}

static k4a_result_t get_camera_calibration(char *json, k4a_calibration_camera_t *cal, char *location)
{
    k4a_result_t result;
    bool found = false;

    cJSON *main_object = cJSON_Parse(json);
    if (main_object == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            LOG_ERROR("cJSON_Parse error %s", error_ptr);
        }
        return K4A_RESULT_FAILED;
    }

    cJSON *cal_info = NULL;
    cal_info = cJSON_GetObjectItem(main_object, "CalibrationInformation");
    result = K4A_RESULT_FROM_BOOL(cal_info != NULL);

    cJSON *cameras = NULL;
    if (K4A_SUCCEEDED(result))
    {
        cameras = cJSON_GetObjectItem(cal_info, "Cameras");
        result = K4A_RESULT_FROM_BOOL(cameras != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cJSON_IsArray(cameras));
    }

    if (K4A_SUCCEEDED(result))
    {
        cJSON *camera = NULL;
        cJSON_ArrayForEach(camera, cameras)
        {
            result = K4A_RESULT_FROM_BOOL(camera != NULL);

            cJSON *obj_location;
            obj_location = cJSON_GetObjectItem(camera, "Location");
            result = K4A_RESULT_FROM_BOOL(obj_location != NULL);

            if (K4A_SUCCEEDED(result))
            {
                result = K4A_RESULT_FROM_BOOL(cJSON_IsString(obj_location));
            }

            if (K4A_SUCCEEDED(result))
            {
                result = K4A_RESULT_FROM_BOOL(obj_location->valuestring != NULL);
            }

            if (K4A_SUCCEEDED(result))
            {
                if (strcmp(obj_location->valuestring, location) == 0)
                {
                    result = fill_in_camera_cal_data(cal, camera);
                    found = true;
                    break;
                }
                else
                {
                    continue;
                }
            }

            if (K4A_FAILED(result))
            {
                break;
            }
        }

        result = K4A_RESULT_FROM_BOOL(found == true);
    }
    cJSON_Delete(main_object);
    return result;
}

static k4a_result_t get_imu_calibration(char *json, k4a_calibration_imu_t *cal, char *sensor_type)
{
    k4a_result_t result;
    bool found = false;

    cJSON *main_object = cJSON_Parse(json);
    if (main_object == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            LOG_ERROR("cJSON_Parse error %s", error_ptr);
        }
        return K4A_RESULT_FAILED;
    }

    cJSON *cal_info = NULL;
    cal_info = cJSON_GetObjectItem(main_object, "CalibrationInformation");
    result = K4A_RESULT_FROM_BOOL(cal_info != NULL);

    cJSON *inertial_sensors = NULL;
    if (K4A_SUCCEEDED(result))
    {
        inertial_sensors = cJSON_GetObjectItem(cal_info, "InertialSensors");
        result = K4A_RESULT_FROM_BOOL(inertial_sensors != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cJSON_IsArray(inertial_sensors));
    }

    if (K4A_SUCCEEDED(result))
    {
        cJSON *inertial_sensor = NULL;
        cJSON_ArrayForEach(inertial_sensor, inertial_sensors)
        {
            result = K4A_RESULT_FROM_BOOL(inertial_sensor != NULL);
            cJSON *obj_sensor_type;
            obj_sensor_type = cJSON_GetObjectItem(inertial_sensor, "SensorType");
            result = K4A_RESULT_FROM_BOOL(obj_sensor_type != NULL);

            if (K4A_SUCCEEDED(result))
            {
                result = K4A_RESULT_FROM_BOOL(cJSON_IsString(obj_sensor_type));
            }

            if (K4A_SUCCEEDED(result))
            {
                result = K4A_RESULT_FROM_BOOL(obj_sensor_type->valuestring != NULL);
            }

            if (K4A_SUCCEEDED(result))
            {
                if (strcmp(obj_sensor_type->valuestring, sensor_type) == 0)
                {
                    result = fill_in_imu_cal_data(cal, inertial_sensor);
                    found = true;
                    break;
                }
                else
                {
                    continue;
                }
            }

            if (K4A_FAILED(result))
            {
                break;
            }
        }

        result = K4A_RESULT_FROM_BOOL(found == true);
    }
    // recusively frees children
    cJSON_Delete(main_object);
    return result;
}

static k4a_result_t read_extrinsic_calibration(calibration_context_t *calibration)
{
    size_t json_size;
    char *json;
    k4a_result_t result;
    int tries = 0;

    json_size = READ_RETRY_BASE_ALLOCATION;
    if (calibration->json_size != 0)
    {
        json_size = calibration->json_size;
    }

    do
    {
        size_t bytes_read = 0;

        json = malloc(json_size);
        result = K4A_RESULT_FROM_BOOL(json != NULL);

        if (K4A_SUCCEEDED(result))
        {
            result = depthmcu_get_extrinsic_calibration(calibration->depthmcu, json, json_size, &bytes_read);
        }

        if (K4A_SUCCEEDED(result))
        {
            result = K4A_RESULT_FROM_BOOL(bytes_read < json_size);
        }

        if (K4A_SUCCEEDED(result))
        {
            json[bytes_read] = '\0'; // NULL terminate the json calibration, which is ASCII text
            calibration->json = json;
            calibration->json_size = bytes_read + 1; // ++ for NULL
        }
        else
        {
            // failed
            free(json);
            json_size += READ_RETRY_ALLOC_INCREASE;
            tries++;
        }
    } while (K4A_FAILED(result) && tries < MAX_READ_RETRIES);

    return result;
}

k4a_result_t calibration_create(depthmcu_t depthmcu, calibration_t *calibration_handle)
{
    calibration_context_t *calibration;
    k4a_result_t result;

    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, depthmcu == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, calibration_handle == NULL);

    calibration = calibration_t_create(calibration_handle);
    result = K4A_RESULT_FROM_BOOL(calibration != NULL);

    if (K4A_SUCCEEDED(result))
    {
        calibration->depthmcu = depthmcu;

        result = read_extrinsic_calibration(calibration);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = calibration_create_from_raw(calibration->json,
                                             calibration->json_size,
                                             &calibration->depth_calibration,
                                             &calibration->color_calibration,
                                             &calibration->gyro_calibration,
                                             &calibration->accel_calibration);
    }

    if (K4A_FAILED(result) && *calibration_handle != NULL)
    {
        calibration_destroy(*calibration_handle);
        *calibration_handle = NULL;
        result = K4A_RESULT_FAILED;
    }

    return result;
}

k4a_result_t calibration_create_from_raw(char *raw_calibration,
                                         size_t raw_calibration_size,
                                         k4a_calibration_camera_t *depth_calibration,
                                         k4a_calibration_camera_t *color_calibration,
                                         k4a_calibration_imu_t *gyro_calibration,
                                         k4a_calibration_imu_t *accel_calibration)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, raw_calibration == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, !(strnlen(raw_calibration, raw_calibration_size) < raw_calibration_size));
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED,
                        depth_calibration == NULL && color_calibration == NULL && gyro_calibration == NULL &&
                            accel_calibration == NULL);

    k4a_result_t result = K4A_RESULT_SUCCEEDED;

#ifdef _WIN32
    int previous_thread_locale = -1;
    if (K4A_SUCCEEDED(result))
    {
        previous_thread_locale = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
        result = K4A_RESULT_FROM_BOOL(previous_thread_locale == _ENABLE_PER_THREAD_LOCALE ||
                                      previous_thread_locale == _DISABLE_PER_THREAD_LOCALE);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(setlocale(LC_ALL, "C") != NULL);
    }

#else // NOT _WIN32

    locale_t thread_locale = newlocale(LC_ALL_MASK, "C", (locale_t)0);
    locale_t previous_locale = uselocale(thread_locale);

#endif

    if (K4A_SUCCEEDED(result) && depth_calibration != NULL)
    {
        result = get_camera_calibration(raw_calibration, depth_calibration, "CALIBRATION_CameraLocationD0");
    }

    if (K4A_SUCCEEDED(result) && color_calibration != NULL)
    {
        result = get_camera_calibration(raw_calibration, color_calibration, "CALIBRATION_CameraLocationPV0");
    }

    if (K4A_SUCCEEDED(result) && gyro_calibration != NULL)
    {
        result = get_imu_calibration(raw_calibration, gyro_calibration, "CALIBRATION_InertialSensorType_Gyro");
    }

    if (K4A_SUCCEEDED(result) && accel_calibration != NULL)
    {
        result = get_imu_calibration(raw_calibration,
                                     accel_calibration,
                                     "CALIBRATION_InertialSensorType_Accelerometer");
    }

#ifdef _WIN32
    if (previous_thread_locale == _ENABLE_PER_THREAD_LOCALE || previous_thread_locale == _DISABLE_PER_THREAD_LOCALE)
    {
        if (K4A_FAILED(K4A_RESULT_FROM_BOOL(_configthreadlocale(previous_thread_locale) != -1)))
        {
            // Only set result to failed, don't let this call succeed and clear a failure that might have happened
            // already.
            result = K4A_RESULT_FAILED;
        }
    }
#else // NOT _WIN32
    if ((previous_locale != NULL) && (K4A_FAILED(K4A_RESULT_FROM_BOOL(uselocale(previous_locale) != NULL))))
    {
        // Only set result to failed, don't let this call succeed and clear a failure that might have happened
        // already.
        result = K4A_RESULT_FAILED;
    }
    if (thread_locale)
    {
        freelocale(thread_locale);
    }
#endif

    return result;
}

void calibration_destroy(calibration_t calibration_handle)
{
    calibration_context_t *calibration;

    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, calibration_t, calibration_handle);

    calibration = calibration_t_get_context(calibration_handle);

    if (calibration->json)
    {
        free(calibration->json);
    }

    calibration_t_destroy(calibration_handle);
}

k4a_result_t calibration_get_camera(calibration_t calibration_handle,
                                    k4a_calibration_type_t type,
                                    k4a_calibration_camera_t *cal_data)
{
    calibration_context_t *calibration;

    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, calibration_t, calibration_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, type != K4A_CALIBRATION_TYPE_DEPTH && type != K4A_CALIBRATION_TYPE_COLOR);

    calibration = calibration_t_get_context(calibration_handle);

    if (type == K4A_CALIBRATION_TYPE_DEPTH)
    {
        memcpy(cal_data, &calibration->depth_calibration, sizeof(k4a_calibration_camera_t));
    }
    else //  if (type == K4A_CALIBRATION_TYPE_COLOR)
    {
        assert(type == K4A_CALIBRATION_TYPE_COLOR);
        memcpy(cal_data, &calibration->color_calibration, sizeof(k4a_calibration_camera_t));
    }
    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t calibration_get_imu(calibration_t calibration_handle,
                                 k4a_calibration_type_t type,
                                 k4a_calibration_imu_t *cal_data)
{
    calibration_context_t *calibration;

    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, calibration_t, calibration_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, type != K4A_CALIBRATION_TYPE_GYRO && type != K4A_CALIBRATION_TYPE_ACCEL);

    calibration = calibration_t_get_context(calibration_handle);

    if (type == K4A_CALIBRATION_TYPE_GYRO)
    {
        memcpy(cal_data, &calibration->gyro_calibration, sizeof(k4a_calibration_imu_t));
    }
    else //  if (type == K4A_CALIBRATION_TYPE_ACCEL)
    {
        assert(type == K4A_CALIBRATION_TYPE_ACCEL);
        memcpy(cal_data, &calibration->accel_calibration, sizeof(k4a_calibration_imu_t));
    }
    return K4A_RESULT_SUCCEEDED;
}

k4a_buffer_result_t calibration_get_raw_data(calibration_t calibration_handle, uint8_t *data, size_t *data_size)
{
    calibration_context_t *calibration;
    k4a_buffer_result_t bresult = K4A_BUFFER_RESULT_FAILED;

    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, calibration_t, calibration_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, data_size == NULL);

    calibration = calibration_t_get_context(calibration_handle);

    if (data == NULL)
    {
        (*data_size) = calibration->json_size;
        bresult = K4A_BUFFER_RESULT_TOO_SMALL;
    }
    else
    {
        k4a_result_t result;
        result = K4A_RESULT_FROM_BOOL(*data_size >= calibration->json_size);

        if (K4A_SUCCEEDED(result))
        {
            memcpy(data, calibration->json, calibration->json_size);
            *data_size = calibration->json_size;
            bresult = K4A_BUFFER_RESULT_SUCCEEDED;
        }
        else
        {
            *data_size = calibration->json_size;
            bresult = K4A_BUFFER_RESULT_TOO_SMALL;
        }
    }
    return bresult;
}
