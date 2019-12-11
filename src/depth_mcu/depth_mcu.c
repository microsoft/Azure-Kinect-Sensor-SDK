// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/depth_mcu.h>

// Dependent libraries
#include <k4ainternal/logging.h>
#include <azure_c_shared_utility/threadapi.h>

// System dependencies
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Private headers
#include "depthcommands.h"

typedef struct _depthmcu_context_t
{
    usbcmd_t usb_cmd;
    depthmcu_stream_cb_t *callback;
    void *callback_context;
    size_t payload_size;
    size_t mode_size;
} depthmcu_context_t;

K4A_DECLARE_CONTEXT(depthmcu_t, depthmcu_context_t);

typedef struct _depthmcu_package_info_t
{
    uint8_t last_package; // 0 = payload package, 1 = last package
    uint8_t package_size;
} depthmcu_package_info_t;

usb_cmd_stream_cb_t depthmcu_depth_capture_ready;

/** See usb_cmd_stream_cb_t documentation
 * \relates usb_cmd_stream_cb_t
 */
void depthmcu_depth_capture_ready(k4a_result_t status, k4a_image_t image_handle, void *context)
{
    depthmcu_context_t *depthmcu = (depthmcu_context_t *)context;

    if (image_get_size(image_handle) == depthmcu->mode_size)
    {
        depthmcu->callback(status, image_handle, depthmcu->callback_context);
    }
    else
    {
        LOG_INFO("Dropping raw image due to invalid size %lld expected, %lld received",
                 depthmcu->mode_size,
                 image_get_size(image_handle));
    }
}

k4a_result_t depthmcu_create(uint32_t device_index, depthmcu_t *depthmcu_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, device_index >= 100);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, depthmcu_handle == NULL);

    depthmcu_context_t *depthmcu = depthmcu_t_create(depthmcu_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(depthmcu != NULL);

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(usb_cmd_create(USB_DEVICE_DEPTH_PROCESSOR, device_index, NULL, &depthmcu->usb_cmd));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(usb_cmd_stream_register_cb(depthmcu->usb_cmd, depthmcu_depth_capture_ready, depthmcu));
    }

    if (K4A_FAILED(result))
    {
        depthmcu_destroy(*depthmcu_handle);
        *depthmcu_handle = NULL;
    }

    return result;
}

void depthmcu_destroy(depthmcu_t depthmcu_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, depthmcu_t, depthmcu_handle);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);

    if (depthmcu->usb_cmd)
    {
        usb_cmd_destroy(depthmcu->usb_cmd);
        depthmcu->usb_cmd = NULL;
    }

    depthmcu_t_destroy(depthmcu_handle);
}

k4a_buffer_result_t depthmcu_get_serialnum(depthmcu_t depthmcu_handle, char *serial_number, size_t *serial_number_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, serial_number_size == NULL);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);

    size_t caller_buffer_size = 0;
    caller_buffer_size = *serial_number_size;
    // initialize output parameters to safe values
    *serial_number_size = 0;
    if (serial_number != NULL && caller_buffer_size > 0)
    {
        serial_number[0] = '\0';
    }

    k4a_result_t result;

    char temp_serial_number[256];
    size_t bytes_read = 0;

    result = TRACE_CALL(
        usb_cmd_read(depthmcu->usb_cmd,
                     DEV_CMD_DEPTH_READ_PRODUCT_SN,
                     NULL,
                     0,
                     (uint8_t *)temp_serial_number,
                     sizeof(temp_serial_number) - 1 /* Leave enough space the NULL terminate if needed */,
                     &bytes_read));

    if (K4A_FAILED(result))
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    assert(bytes_read < sizeof(temp_serial_number));

    // Ensure string is null terminated by putting a NULL after the read data
    temp_serial_number[bytes_read] = '\0';

    size_t valid_bytes = bytes_read + 1; // Including the null

    size_t i = 0;
    for (; i < valid_bytes; i++)
    {
        char c = temp_serial_number[i];
        if (c == '\0')
        {
            // Shorten the valid bytes to the first NULL, even if the hardware returned more bytes
            valid_bytes = i + 1;
            break;
        }

        // Validate hardware reported serial number is valid printable ASCII before returning to user
        if ((c & 0x80) != 0                    // Locale specific extended ASCII
            || !isgraph(temp_serial_number[i]) // Non-printable character
        )
        {
            LOG_ERROR("depthmcu_get_serialnum found non-printable serial number (character %d is ASCII value %d)",
                      i,
                      temp_serial_number[i]);
            return K4A_BUFFER_RESULT_FAILED;
        }
    }

    // Successfully read the serial number
    *serial_number_size = valid_bytes;

    if (caller_buffer_size < valid_bytes || serial_number == NULL)
    {
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }

    if (serial_number != NULL)
    {
        memcpy(serial_number, temp_serial_number, valid_bytes);
    }
    return K4A_BUFFER_RESULT_SUCCEEDED;
}

bool depthmcu_wait_is_ready(depthmcu_t depthmcu_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);
    depthmcu_firmware_versions_t tmpVersion = { 0 };
    uint32_t cmd_status = 0;
    size_t bytes_read = 0;
    int retries = 0;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    do
    {
        result = TRACE_CALL(usb_cmd_read_with_status(depthmcu->usb_cmd,
                                                     DEV_CMD_COMPONENT_VERSION_GET,
                                                     NULL,
                                                     0,
                                                     (uint8_t *)&tmpVersion,
                                                     sizeof(tmpVersion),
                                                     &bytes_read,
                                                     &cmd_status));

        if (K4A_SUCCEEDED(result) && cmd_status != CMD_STATUS_PASS)
        {
            result = K4A_RESULT_FAILED;
        }

        if (K4A_SUCCEEDED(result))
        {
            result = K4A_RESULT_FROM_BOOL(bytes_read >= sizeof(tmpVersion));
        }

        if (K4A_FAILED(result))
        {
            ThreadAPI_Sleep(500);
            retries++;
        }
    } while (K4A_FAILED(result) && retries < 20);

    return (result == K4A_RESULT_SUCCEEDED);
}

k4a_result_t depthmcu_get_version(depthmcu_t depthmcu_handle, depthmcu_firmware_versions_t *version)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, version == NULL);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);
    depthmcu_firmware_versions_t tmpVersion = { 0 };
    size_t bytes_read = 0;

    k4a_result_t result = TRACE_CALL(usb_cmd_read(depthmcu->usb_cmd,
                                                  DEV_CMD_COMPONENT_VERSION_GET,
                                                  NULL,
                                                  0,
                                                  (uint8_t *)&tmpVersion,
                                                  sizeof(tmpVersion),
                                                  &bytes_read));

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(bytes_read >= sizeof(tmpVersion));
    }

    if (K4A_SUCCEEDED(result))
    {
        // Successful, copy the result
        *version = tmpVersion;
    }

    return result;
}

k4a_result_t depthmcu_depth_set_capture_mode(depthmcu_t depthmcu_handle, k4a_depth_mode_t capture_mode)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);
    uint32_t mode;

    switch (capture_mode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        mode = SENSOR_MODE_LONG_THROW_NATIVE;
        depthmcu->payload_size = SENSOR_MODE_LONG_THROW_NATIVE_PAYLOAD_SIZE;
        depthmcu->mode_size = SENSOR_MODE_LONG_THROW_NATIVE_SIZE;
        break;
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        mode = SENSOR_MODE_QUARTER_MEGA_PIXEL;
        depthmcu->payload_size = SENSOR_MODE_QUARTER_MEGA_PIXEL_PAYLOAD_SIZE;
        depthmcu->mode_size = SENSOR_MODE_QUARTER_MEGA_PIXEL_SIZE;
        break;
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        mode = SENSOR_MODE_MEGA_PIXEL;
        depthmcu->payload_size = SENSOR_MODE_MEGA_PIXEL_PAYLOAD_SIZE;
        depthmcu->mode_size = SENSOR_MODE_MEGA_PIXEL_SIZE;
        break;
    case K4A_DEPTH_MODE_PASSIVE_IR:
        mode = SENSOR_MODE_PSEUDO_COMMON;
        depthmcu->payload_size = SENSOR_MODE_PSEUDO_COMMON_PAYLOAD_SIZE;
        depthmcu->mode_size = SENSOR_MODE_PSEUDO_COMMON_SIZE;
        break;
    default:
        LOG_ERROR("Invalid mode %d", capture_mode);
        return K4A_RESULT_FAILED;
    }

    // send command (Note, sensor MUST be in the ON state)
    return TRACE_CALL(
        usb_cmd_write(depthmcu->usb_cmd, DEV_CMD_DEPTH_MODE_SET, (uint8_t *)&mode, sizeof(mode), NULL, 0));
}

k4a_result_t depthmcu_depth_set_fps(depthmcu_t depthmcu_handle, k4a_fps_t capture_fps)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);
    uint32_t fps;

    // Translate FPS to values understood by sensor module
    switch (capture_fps)
    {
    case K4A_FRAMES_PER_SECOND_30:
        fps = 30;
        break;
    case K4A_FRAMES_PER_SECOND_15:
        fps = 15;
        break;
    case K4A_FRAMES_PER_SECOND_5:
        fps = 5;
        break;
    default:
        LOG_ERROR("Invalid FPS %d", capture_fps);
        return K4A_RESULT_FAILED;
    }

    // Send command (Note, sensor MUST be in the ON state)
    return TRACE_CALL(usb_cmd_write(depthmcu->usb_cmd, DEV_CMD_DEPTH_FPS_SET, (uint8_t *)&fps, sizeof(fps), NULL, 0));
}

k4a_result_t depthmcu_depth_start_streaming(depthmcu_t depthmcu_handle,
                                            depthmcu_stream_cb_t *callback,
                                            void *callback_context)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    depthmcu->callback = callback;
    depthmcu->callback_context = callback_context;

    // Send start sensor command (Note, sensor MUST be in the ON state)
    result = TRACE_CALL(usb_cmd_write(depthmcu->usb_cmd, DEV_CMD_DEPTH_START, NULL, 0, NULL, 0));

    if (K4A_SUCCEEDED(result))
    {
        // Send start streaming command (Note, sensor MUST be in the ON state)
        result = TRACE_CALL(usb_cmd_write(depthmcu->usb_cmd, DEV_CMD_DEPTH_STREAM_START, NULL, 0, NULL, 0));
    }

    if (K4A_SUCCEEDED(result))
    {
        // Start the streaming thread
        result = TRACE_CALL(usb_cmd_stream_start(depthmcu->usb_cmd, depthmcu->payload_size));
    }

    return result;
}

void depthmcu_depth_stop_streaming(depthmcu_t depthmcu_handle, bool quiet)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, depthmcu_t, depthmcu_handle);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);
    k4a_result_t result;
    uint32_t cmd_status = 0;

    // Start the streaming thread
    TRACE_CALL(usb_cmd_stream_stop(depthmcu->usb_cmd));

    // Send stop stream command (Note, sensor MUST be in the ON state)
    result = TRACE_CALL(
        usb_cmd_write_with_status(depthmcu->usb_cmd, DEV_CMD_DEPTH_STREAM_STOP, NULL, 0, NULL, 0, &cmd_status));
    if (K4A_SUCCEEDED(result) && (!quiet))
    {
        result = K4A_RESULT_FROM_BOOL(cmd_status == CMD_STATUS_PASS);
        if (K4A_FAILED(result))
        {
            LOG_ERROR("ERROR: cmd_status=0x%08X", cmd_status);
        }
    }

    // Send stop sensor command (Note, sensor MUST be in the ON state)
    result = TRACE_CALL(
        usb_cmd_write_with_status(depthmcu->usb_cmd, DEV_CMD_DEPTH_STOP, NULL, 0, NULL, 0, &cmd_status));
    if (K4A_SUCCEEDED(result) && (!quiet))
    {
        result = K4A_RESULT_FROM_BOOL(cmd_status == CMD_STATUS_PASS);
        if (K4A_FAILED(result))
        {
            LOG_ERROR("ERROR: cmd_status=0x%08X", cmd_status);
        }
    }
}

k4a_result_t depthmcu_get_cal(depthmcu_t depthmcu_handle, uint8_t *calibration, size_t cal_size, size_t *bytes_read)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, bytes_read == NULL);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);
    uint32_t nv_tag = DEVICE_NV_IR_SENSOR_CALIBRATION;
    k4a_result_t result;

    // Send get calibration data command (Note, sensor MUST be in the ON state)
    result = TRACE_CALL(usb_cmd_read(
        depthmcu->usb_cmd, DEV_CMD_NV_DATA_GET, (uint8_t *)&nv_tag, sizeof(nv_tag), calibration, cal_size, bytes_read));

    if (K4A_SUCCEEDED(result))
    {
        if (*bytes_read < sizeof(uint32_t))
        {
            LOG_ERROR("Depth calibration data not available or sensor not turned on (bytes_read = %zu)", *bytes_read);
            result = K4A_RESULT_FAILED;
        }
    }

    return result;
}

k4a_result_t
depthmcu_get_extrinsic_calibration(depthmcu_t depthmcu_handle, char *json, size_t json_size, size_t *bytes_read)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, json == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, bytes_read == NULL);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);

    return TRACE_CALL(usb_cmd_read(
        depthmcu->usb_cmd, DEV_CMD_DEPTH_READ_CALIBRATION_DATA, NULL, 0, (uint8_t *)json, json_size, bytes_read));
}

k4a_result_t depthmcu_download_firmware(depthmcu_t depthmcu_handle, uint8_t *firmwarePayload, size_t firmwareSize)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmwarePayload == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, firmwareSize == 0);

    depthmcu_package_info_t info = { 0 };
    k4a_result_t result;
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);

    // Sending payload
    LOG_INFO("Sending firmware to Depth MCU...", 0);
    LOG_INFO("Firmware is %d bytes", firmwareSize);
    info.last_package = 1;

    // This is legacy code from when a packet was limited to 255 bytes.
    // The firmware is not checking this.
    info.package_size = (uint8_t)firmwareSize;

    result = TRACE_CALL(usb_cmd_write(
        depthmcu->usb_cmd, DEV_CMD_DOWNLOAD_FIRMWARE, (uint8_t *)&info, sizeof(info), firmwarePayload, firmwareSize));

    LOG_INFO("Writing firmware to Depth MCU complete.", 0);
    return result;
}

k4a_result_t depthmcu_get_firmware_update_status(depthmcu_t depthmcu_handle,
                                                 depthmcu_firmware_update_status_t *update_status)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, update_status == NULL);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);

    return TRACE_CALL(usb_cmd_read(depthmcu->usb_cmd,
                                   DEV_CMD_GET_FIRMWARE_UPDATE_STATUS,
                                   NULL,
                                   0,
                                   (uint8_t *)update_status,
                                   sizeof(depthmcu_firmware_update_status_t),
                                   NULL));
}

k4a_result_t depthmcu_reset_device(depthmcu_t depthmcu_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, depthmcu_t, depthmcu_handle);
    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);

    return TRACE_CALL(usb_cmd_write(depthmcu->usb_cmd, DEV_CMD_RESET, NULL, 0, NULL, 0));
}

const guid_t *depthmcu_get_container_id(depthmcu_t depthmcu_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NULL, depthmcu_t, depthmcu_handle);

    depthmcu_context_t *depthmcu = depthmcu_t_get_context(depthmcu_handle);

    return usb_cmd_get_container_id(depthmcu->usb_cmd);
}