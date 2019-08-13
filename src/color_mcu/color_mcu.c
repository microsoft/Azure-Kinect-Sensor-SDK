// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library *
#include <k4ainternal/color_mcu.h>
#include <k4ainternal/common.h>

// System dependencies
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Private headers
#include "colorcommands.h"

// Set Packing for structures being sent over USB
#pragma pack(push, 1)

typedef enum
{
    EXTERNAL_SYNC_MODE_STANDALONE = 0,
    EXTERNAL_SYNC_MODE_MASTER = 1,
    EXTERNAL_SYNC_MODE_SUBORDINATE = 2,
} external_sync_mode_t;

typedef struct _synchronization_config_t
{
    uint32_t mode;                             // Standalone, master, subordinate
    uint32_t subordinate_delay_off_master_pts; // Capture delay between master and subordinate, units are 90kHz tick
    int32_t depth_delay_off_color_pts;         // Units are 90kHz tick
    uint8_t enable_privacy_led;                // 0 disabled; 1 enabled
} synchronization_config_t;

#pragma pack(pop)

typedef struct _colormcu_context_t
{
    usbcmd_t usb_cmd;
} colormcu_context_t;

K4A_DECLARE_CONTEXT(colormcu_t, colormcu_context_t);

k4a_result_t colormcu_create(const guid_t *container_id, colormcu_t *colormcu_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, colormcu_handle == NULL);

    colormcu_context_t *colormcu = colormcu_t_create(colormcu_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(colormcu != NULL);

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(
            usb_cmd_create(USB_DEVICE_COLOR_IMU_PROCESSOR, NULL_INDEX, container_id, &colormcu->usb_cmd));
    }

    if (K4A_FAILED(result))
    {
        colormcu_destroy(*colormcu_handle);
        *colormcu_handle = NULL;
    }

    return result;
}

k4a_result_t colormcu_create_by_index(uint32_t device_index, colormcu_t *colormcu_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, colormcu_handle == NULL);

    colormcu_context_t *colormcu = colormcu_t_create(colormcu_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(colormcu != NULL);

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(usb_cmd_create(USB_DEVICE_COLOR_IMU_PROCESSOR, device_index, NULL, &colormcu->usb_cmd));
    }

    if (K4A_FAILED(result))
    {
        colormcu_destroy(*colormcu_handle);
        *colormcu_handle = NULL;
    }

    return result;
}

void colormcu_destroy(colormcu_t colormcu_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, colormcu_t, colormcu_handle);
    colormcu_context_t *colormcu = colormcu_t_get_context(colormcu_handle);

    if (colormcu->usb_cmd)
    {
        usb_cmd_destroy(colormcu->usb_cmd);
        colormcu->usb_cmd = NULL;
    }

    colormcu_t_destroy(colormcu_handle);
}

k4a_buffer_result_t colormcu_get_usb_serialnum(colormcu_t colormcu_handle,
                                               char *serial_number,
                                               size_t *serial_number_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, colormcu_t, colormcu_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, serial_number_size == NULL);
    colormcu_context_t *colormcu = colormcu_t_get_context(colormcu_handle);

    return usb_cmd_get_serial_number(colormcu->usb_cmd, serial_number, serial_number_size);
}

/**
 *  Function to start the IMU stream.
 *
 *  @param colormcu_handle
 *   Handle to this specific object
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED    Operation was successful
 *   K4A_RESULT_FAILED       Operation was not successful
 */
k4a_result_t colormcu_imu_start_streaming(colormcu_t colormcu_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, colormcu_t, colormcu_handle)
    colormcu_context_t *colormcu = colormcu_t_get_context(colormcu_handle);

    // Send command to start the IMU on the device
    k4a_result_t result = TRACE_CALL(usb_cmd_write(colormcu->usb_cmd, DEV_CMD_IMU_STREAM_START, NULL, 0, NULL, 0));

    if (K4A_SUCCEEDED(result))
    {
        // Start imu stream thread
        result = TRACE_CALL(usb_cmd_stream_start(colormcu->usb_cmd, IMU_MAX_PAYLOAD_SIZE));
    }

    return result;
}

/**
 *  Function to stop the IMU stream.
 *
 *  @param colormcu_handle
 *   Handle to this specific object
 *
 */
void colormcu_imu_stop_streaming(colormcu_t colormcu_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, colormcu_t, colormcu_handle)
    colormcu_context_t *colormcu = colormcu_t_get_context(colormcu_handle);

    // stop the imu stream thread
    TRACE_CALL(usb_cmd_stream_stop(colormcu->usb_cmd));

    // Send the stop command
    TRACE_CALL(usb_cmd_write(colormcu->usb_cmd, DEV_CMD_IMU_STREAM_STOP, NULL, 0, NULL, 0));
}

/**
 *  Function registering the callback function associated with
 *  streaming data.
 *
 *  @param colormcu_handle
 *   Handle to this object
 *
 *  @param frame_ready_cb
 *   Callback invoked when a frame has been received.  Called within the context of the usb_command tread
 *
 *  @param context
 *   Data that will be handed back with the callback
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED    Operation successful
 *   K4A_RESULT_FAILED       Operation failed
 *
 */
k4a_result_t colormcu_imu_register_stream_cb(colormcu_t colormcu_handle,
                                             usb_cmd_stream_cb_t *frame_ready_cb,
                                             void *context)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, colormcu_t, colormcu_handle)
    colormcu_context_t *colormcu = colormcu_t_get_context(colormcu_handle);

    return TRACE_CALL(usb_cmd_stream_register_cb(colormcu->usb_cmd, frame_ready_cb, context));
}

/**
 *  Function to read the state of the synchronization jacks on the back of the device
 *
 *  @param colormcu_handle
 *   Handle to this object
 *
 *  @param sync_in_jack_connected [OPTIONAL]
 *   set to true if a cable is connected
 *
 *  @param sync_out_jack_connected [OPTIONAL]
 *   set to true if a cable is connected
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED    Operation successful
 *   K4A_RESULT_FAILED       Operation failed
 *
 */
k4a_result_t colormcu_get_external_sync_jack_state(colormcu_t colormcu_handle,
                                                   bool *sync_in_jack_connected,
                                                   bool *sync_out_jack_connected)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, colormcu_t, colormcu_handle)
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, sync_in_jack_connected == NULL && sync_out_jack_connected == NULL);

    colormcu_context_t *colormcu = colormcu_t_get_context(colormcu_handle);
    uint8_t state;
    size_t bytes_read;

    k4a_result_t result = TRACE_CALL(usb_cmd_read(
        colormcu->usb_cmd, DEV_CMD_GET_JACK_STATE, NULL, 0, (uint8_t *)&state, sizeof(state), &bytes_read));
    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(bytes_read == sizeof(state));
    }

    if (K4A_SUCCEEDED(result))
    {
        if (sync_in_jack_connected)
        {
            *sync_in_jack_connected = (state & 0x1) ? true : false;
        }

        if (sync_out_jack_connected)
        {
            *sync_out_jack_connected = (state & 0x2) ? true : false;
        }
    }
    return result;
}

/**
 *  Function to read the state of the synchronization jacks on the back of the device
 *
 *  @param colormcu_handle
 *   Handle to this object
 *
 *  @param sync_in_jack_connected
 *   set to true if a cable is connected
 *
 *  @param sync_out_jack_connected
 *   set to true if a cable is connected
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED    Operation successful
 *   K4A_RESULT_FAILED       Operation failed
 *
 */
k4a_result_t colormcu_set_multi_device_mode(colormcu_t colormcu_handle, const k4a_device_configuration_t *config)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, colormcu_t, colormcu_handle)
    colormcu_context_t *colormcu = colormcu_t_get_context(colormcu_handle);

    synchronization_config_t sync_config;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    switch (config->wired_sync_mode)
    {
    case K4A_WIRED_SYNC_MODE_STANDALONE:
        sync_config.mode = EXTERNAL_SYNC_MODE_STANDALONE;
        break;
    case K4A_WIRED_SYNC_MODE_MASTER:
        sync_config.mode = EXTERNAL_SYNC_MODE_MASTER;
        break;
    case K4A_WIRED_SYNC_MODE_SUBORDINATE:
        sync_config.mode = EXTERNAL_SYNC_MODE_SUBORDINATE;
        break;
    default:
        result = K4A_RESULT_FAILED;
        LOG_ERROR("Unexpected value in  config->wired_sync_mode:%d", config->wired_sync_mode);
        break;
    }

    if (K4A_SUCCEEDED(result))
    {
        sync_config.subordinate_delay_off_master_pts = K4A_USEC_TO_90K_HZ_TICK(
            config->subordinate_delay_off_master_usec);
        sync_config.depth_delay_off_color_pts = K4A_USEC_TO_90K_HZ_TICK(config->depth_delay_off_color_usec);
        sync_config.enable_privacy_led = !config->disable_streaming_indicator;

        result = TRACE_CALL(usb_cmd_write(
            colormcu->usb_cmd, DEV_CMD_SET_SYS_CFG, (uint8_t *)&sync_config, sizeof(sync_config), NULL, 0));
    }
    return result;
}

k4a_result_t colormcu_reset_device(colormcu_t colormcu_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, colormcu_t, colormcu_handle);
    colormcu_context_t *colormcu = colormcu_t_get_context(colormcu_handle);

    return TRACE_CALL(usb_cmd_write(colormcu->usb_cmd, DEV_CMD_RESET, NULL, 0, NULL, 0));
}