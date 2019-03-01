// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <stdio.h>
#include <k4a/k4a.h>
#include <k4ainternal/logging.h>
#include <k4ainternal/usbcommand.h>
#include <k4ainternal/imu.h>
#include "Cli.h"
#include "Main.h"
#include "../src/depth_mcu/depthcommands.h" // include private command definitions for testing
#include <azure_c_shared_utility/tickcounter.h>
#include "UsbCmd.h"
#include "assert.h"

//**************Symbolic Constant Macros (defines)  *************
#define CLI_MENU_CMD "command"
#define MAX_BUFFER_SIZE 2000000
#define SAMPLE_SIZE 128
#define DEFAULT_CAPTURE_COUNT 10
#define START_IMU_STREAM_CMD 0x80000003
#define STOP_IMU_STREAM_CMD 0x80000004

//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************
static uint8_t data_buffer[MAX_BUFFER_SIZE];
static FILE *p_file;
static TICK_COUNTER_HANDLE tick_handle;

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

static usb_cmd_stream_cb_t usb_cmd_stream_callback;

/**
 *  Command to read data from a device
 *
 *  @param Argc  Number of variables handed in
 *
 *  @param Argv  Pointer to variable array
 *
 *  @return
 *   CLI_SUCCESS    If executed properly
 *   CLI_ERROR      If error was detected
 *
 */
static CLI_STATUS usb_cmd_imu_read(int Argc, char **Argv)
{
    k4a_result_t result;
    usbcmd_t handle;
    CLI_STATUS status = CLI_SUCCESS;
    uint32_t device_num;
    uint32_t command;
    size_t data_size = 0;

    if (Argc < 3)
    {
        CliDisplayUsage(usb_cmd_imu_read);
        return CLI_ERROR;
    }

    // Get device number
    if (!CliGetStrVal(Argv[1], &device_num))
    {
        return CLI_ERROR;
    }

    // Get command
    if (!CliGetStrVal(Argv[2], &command))
    {
        return CLI_ERROR;
    }

    // Get data size
    if ((Argc > 3) && ((data_size = CliGetBin(Argv[2], data_buffer, 64)) == 0))
    {
        return CLI_ERROR;
    }

    // Close the K4A instance to allow direct access to device
    close_k4a();

    // use maximum buffer size for getting response
    // Get handle for imu (assumes K4A has been initialized)
    if (usb_cmd_create(USB_DEVICE_COLOR_IMU_PROCESSOR, device_num, NULL, &handle) == K4A_RESULT_SUCCEEDED)
    {
        result =
            usb_cmd_read(handle, command, data_buffer, (uint32_t)data_size, data_buffer, MAX_BUFFER_SIZE, &data_size);
        if (result == K4A_RESULT_SUCCEEDED)
        {
            // print binary data
            printf("%zu bytes read\n", data_size);
            for (size_t i = 0; i < data_size; i++)
            {
                printf("%d ", data_buffer[i]);
            }
            printf("\n");
        }
        else
        {
            printf("Failed with error code %d\n", result);
            status = CLI_ERROR;
        }
        usb_cmd_destroy(handle);
    }
    else
    {
        printf("Device not found\n");
        status = CLI_ERROR;
    }

    // re-open the k4a instance
    open_k4a();
    return status;
}

/**
 *  Command to read data from a device
 *
 *  @param Argc  Number of variables handed in
 *
 *  @param Argv  Pointer to variable array
 *
 *  @return
 *   CLI_SUCCESS    If executed properly
 *   CLI_ERROR      If error was detected
 *
 */
static CLI_STATUS usb_cmd_depth_read(int Argc, char **Argv)
{
    k4a_result_t result;
    usbcmd_t handle;
    CLI_STATUS status = CLI_SUCCESS;
    uint32_t device_num;
    uint32_t command;
    size_t data_size = 0;

    if (Argc < 3)
    {
        CliDisplayUsage(usb_cmd_depth_read);
        return CLI_ERROR;
    }

    // Get device number
    if (!CliGetStrVal(Argv[1], &device_num))
    {
        return CLI_ERROR;
    }

    // Get command
    if (!CliGetStrVal(Argv[2], &command))
    {
        return CLI_ERROR;
    }

    // Get command data
    if ((Argc > 3) && ((data_size = CliGetBin(Argv[2], data_buffer, 64)) == 0))
    {
        return CLI_ERROR;
    }

    // Close the K4A instance to allow direct access to device
    close_k4a();

    // use maximum buffer size for getting response
    // Get handle for imu (assumes K4A has been initialized)
    if (usb_cmd_create(USB_DEVICE_DEPTH_PROCESSOR, device_num, NULL, &handle) == K4A_RESULT_SUCCEEDED)
    {
        result =
            usb_cmd_read(handle, command, data_buffer, (uint32_t)data_size, data_buffer, MAX_BUFFER_SIZE, &data_size);
        if (result == K4A_RESULT_SUCCEEDED)
        {
            printf("%zu bytes read\n", data_size);
            // print binary data
            for (size_t i = 0; i < data_size; i++)
            {
                printf("%d ", data_buffer[i]);
            }
            printf("\n");
        }
        else
        {
            printf("Failed with error code %d\n", result);
            status = CLI_ERROR;
        }
        usb_cmd_destroy(handle);
    }
    else
    {
        printf("Device not found\n");
        status = CLI_ERROR;
    }

    // re-open the k4a instance
    open_k4a();
    return status;
}

/**
 *  Command to write something to a device
 *
 *  @param Argc  Number of variables handed in
 *
 *  @param Argv  Pointer to variable array
 *
 *  @return
 *   CLI_SUCCESS    If executed properly
 *   CLI_ERROR      If error was detected
 *
 */
static CLI_STATUS usb_cmd_imu_write(int Argc, char **Argv)
{
    k4a_result_t result;
    usbcmd_t handle;
    CLI_STATUS status = CLI_SUCCESS;
    uint32_t device_num;
    uint32_t command;
    uint8_t tx_buffer[64];
    int data_size = 0;
    uint8_t tx_size = 0;

    if (Argc < 3)
    {
        CliDisplayUsage(usb_cmd_imu_write);
        return CLI_ERROR;
    }

    // Get device number
    if (!CliGetStrVal(Argv[1], &device_num))
    {
        return CLI_ERROR;
    }

    // Get command
    if (!CliGetStrVal(Argv[2], &command))
    {
        return CLI_ERROR;
    }

    // Get command data
    if ((Argc > 3) && ((data_size = CliGetBin(Argv[2], data_buffer, 64)) == 0))
    {
        return CLI_ERROR;
    }

    // Get data to write
    if ((Argc > 4) && ((tx_size = CliGetBin(Argv[2], tx_buffer, 64)) == 0))
    {
        return CLI_ERROR;
    }

    // Close the K4A instance to allow direct access to device
    close_k4a();

    // use maximum buffer size for getting response
    // Get handle for imu (assumes K4A has been initialized)
    if (usb_cmd_create(USB_DEVICE_COLOR_IMU_PROCESSOR, device_num, NULL, &handle) == K4A_RESULT_SUCCEEDED)
    {
        result = usb_cmd_write(handle, command, data_buffer, (uint32_t)data_size, tx_buffer, tx_size);
        if (result != K4A_RESULT_SUCCEEDED)
        {
            printf("Failed with error code %d\n", result);
            status = CLI_ERROR;
        }
        usb_cmd_destroy(handle);
    }
    else
    {
        printf("Device not found\n");
        status = CLI_ERROR;
    }

    open_k4a();
    return status;
}

/**
 *  Command to write something to a device
 *
 *  @param Argc  Number of variables handed in
 *
 *  @param Argv  Pointer to variable array
 *
 *  @return
 *   CLI_SUCCESS    If executed properly
 *   CLI_ERROR      If error was detected
 *
 */
static CLI_STATUS usb_cmd_depth_write(int Argc, char **Argv)
{
    k4a_result_t result;
    usbcmd_t handle;
    CLI_STATUS status = CLI_SUCCESS;
    uint32_t device_num;
    uint32_t command;
    uint8_t tx_buffer[64];
    int data_size = 0;
    uint8_t tx_size = 0;

    if (Argc < 3)
    {
        CliDisplayUsage(usb_cmd_depth_write);
        return CLI_ERROR;
    }

    // Get device number
    if (!CliGetStrVal(Argv[1], &device_num))
    {
        return CLI_ERROR;
    }

    // Get command
    if (!CliGetStrVal(Argv[2], &command))
    {
        return CLI_ERROR;
    }

    // Get command data
    if ((Argc > 3) && ((data_size = CliGetBin(Argv[2], data_buffer, 64)) == 0))
    {
        return CLI_ERROR;
    }

    // Get data to write
    if ((Argc > 4) && ((tx_size = CliGetBin(Argv[2], tx_buffer, 64)) == 0))
    {
        return CLI_ERROR;
    }

    // Close the K4A instance to allow direct access to device
    close_k4a();

    // use maximum buffer size for getting response
    // Get handle for imu (assumes K4A has been initialized)
    if (usb_cmd_create(USB_DEVICE_DEPTH_PROCESSOR, device_num, NULL, &handle) == K4A_RESULT_SUCCEEDED)
    {
        result = usb_cmd_write(handle, command, data_buffer, (uint32_t)data_size, tx_buffer, tx_size);
        if (result != K4A_RESULT_SUCCEEDED)
        {
            printf("Failed with error code %d\n", result);
            status = CLI_ERROR;
        }
        usb_cmd_destroy(handle);
    }
    else
    {
        printf("Device not found\n");
        status = CLI_ERROR;
    }

    // re-open the k4a instance
    open_k4a();
    return status;
}

/**
 *  callback routine for displaying stream data
 *
 *  @param result            status of last transfer
 *
 *  @param image             handle to an image object
 *
 *  @param context           Parameter passed in
 *
 */
static void usb_cmd_stream_callback(k4a_result_t result, k4a_image_t image, void *context)
{
    int *count = (int *)context;
    size_t capture_size;
    tickcounter_ms_t now;
    uint8_t *p_buffer = NULL;

    p_buffer = image_get_buffer(image);
    capture_size = image_get_size(image);

    if (tickcounter_get_current_ms(tick_handle, &now) == 0)
    {
        printf("\nTick: %d\n", (int)now);
    }

    printf("countdown: %d\n", *count);
    printf("status: %d\n", result);
    printf("length: %zu\n", capture_size);
    printf("data:");
    for (uint32_t i = 0; i < SAMPLE_SIZE; i++)
    {
        printf("%02x ", ((uint8_t *)p_buffer)[i]);
    }
    printf("\n");
    *count = *count - 1;
    if (p_file != NULL)
    {
        fwrite(p_buffer, 1, capture_size, p_file);
    }
}

/**
 *  Command to read stream data from a device
 *
 *  @param Argc  Number of variables handed in
 *
 *  @param Argv  Pointer to variable array
 *
 *  @return
 *   CLI_SUCCESS    If executed properly
 *   CLI_ERROR      If error was detected
 *
 */
static CLI_STATUS usb_cmd_read_imu_stream(int Argc, char **Argv)
{
    k4a_result_t result;
    usbcmd_t handle;
    uint32_t device_num;
    CLI_STATUS status = CLI_SUCCESS;
    uint32_t stream_count = 0;

    if (Argc < 2)
    {
        CliDisplayUsage(usb_cmd_read_imu_stream);
        return CLI_ERROR;
    }

    // Get device number
    if (!CliGetStrVal(Argv[1], &device_num))
    {
        return CLI_ERROR;
    }

    // Get number of captures to read
    stream_count = DEFAULT_CAPTURE_COUNT;
    if ((Argc > 2) && (!CliGetStrVal(Argv[2], &stream_count)))
    {
        return CLI_ERROR;
    }

    // Close the K4A instance to allow direct access to device
    close_k4a();

    // Get handle instance
    if (usb_cmd_create(USB_DEVICE_COLOR_IMU_PROCESSOR, device_num, NULL, &handle) == K4A_RESULT_SUCCEEDED)
    {
        // attach callback and wait for it to complete
        if ((result = usb_cmd_stream_register_cb(handle, usb_cmd_stream_callback, &stream_count)) !=
            K4A_RESULT_SUCCEEDED)
        {
            printf("Failed with error code %d\n", result);
            status = CLI_ERROR;
            goto handleDestroy;
        }
        // Send command to start the IMU on the device
        if ((result = usb_cmd_write(handle, START_IMU_STREAM_CMD, NULL, 0, NULL, 0)) != K4A_RESULT_SUCCEEDED)
        {
            printf("Start IMU stream failed with error code %d\n", result);
            status = CLI_ERROR;
            goto handleDestroy;
        }

        printf("Starting IMU streaming\n");
#ifdef _WIN32
        if (fopen_s(&p_file, "imu.rec", "wb") == 0)
        {
#else
        if ((p_file = fopen("imu.rec", "wb")) == NULL)
        {
#endif
            usb_cmd_stream_start(handle, IMU_MAX_PAYLOAD_SIZE);
            while (stream_count > 0)
            {
            }
            usb_cmd_stream_stop(handle);
            fclose(p_file);
        }
        printf("IMU Stream stopped\n");

        // Send command to stop the IMU on the device
        if ((result = usb_cmd_write(handle, STOP_IMU_STREAM_CMD, NULL, 0, NULL, 0)) != K4A_RESULT_SUCCEEDED)
        {
            printf("Stop IMU stream failed with error code %d\n", result);
            status = CLI_ERROR;
        }

    handleDestroy:
        usb_cmd_destroy(handle);
    }
    else
    {
        printf("Device not found\n");
        status = CLI_ERROR;
    }

    // re-open the k4a instance
    open_k4a();
    return status;
}

/**
 *  Command to read stream data from a device
 *
 *  @param Argc  Number of variables handed in
 *
 *  @param Argv  Pointer to variable array
 *
 *  @return
 *   CLI_SUCCESS    If executed properly
 *   CLI_ERROR      If error was detected
 *
 */
static CLI_STATUS usb_cmd_read_depth_stream(int Argc, char **Argv)
{
    k4a_result_t result;
    usbcmd_t handle;
    uint32_t device_num;
    CLI_STATUS status = CLI_SUCCESS;
    uint32_t stream_count = 0;
    uint32_t stream_mode;
    uint32_t fps = 30;
    uint32_t nv_tag = DEVICE_NV_IR_SENSOR_CALIBRATION;
    size_t bytes_read;
    FILE *p_cal_file;
    size_t payload_size = 0;

    if (Argc < 4)
    {
        CliDisplayUsage(usb_cmd_read_depth_stream);
        return CLI_ERROR;
    }

    // Get device number
    if (!CliGetStrVal(Argv[1], &device_num))
    {
        return CLI_ERROR;
    }

    // Get number of captures to read
    stream_count = DEFAULT_CAPTURE_COUNT;
    if ((!CliGetStrVal(Argv[2], &stream_count)))
    {
        return CLI_ERROR;
    }

    // Get mode
    if (!CliGetStrVal(Argv[3], &stream_mode))
    {
        return CLI_ERROR;
    }

    // Get FPS
    if ((Argc > 4) && (!CliGetStrVal(Argv[4], &fps)))
    {
        return CLI_ERROR;
    }

    // Check for valid FPS
    if (!((fps == 30) || (fps == 15) || (fps == 5)))
    {
        return CLI_ERROR;
    }

    // Close the K4A instance to allow direct access to device
    close_k4a();

    switch (stream_mode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        payload_size = SENSOR_MODE_LONG_THROW_NATIVE_PAYLOAD_SIZE;
        break;
    case K4A_DEPTH_MODE_PASSIVE_IR:
        payload_size = SENSOR_MODE_PSEUDO_COMMON_PAYLOAD_SIZE;
        break;
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        payload_size = SENSOR_MODE_QUARTER_MEGA_PIXEL_PAYLOAD_SIZE;
        break;
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        payload_size = SENSOR_MODE_MEGA_PIXEL_PAYLOAD_SIZE;
        break;
    default:
        assert(0);
        LOG_ERROR("%s unknown stream_mode", stream_mode);
        break;
    }

    // Get handle instance
    if (usb_cmd_create(USB_DEVICE_DEPTH_PROCESSOR, device_num, NULL, &handle) == K4A_RESULT_SUCCEEDED)
    {
        // Register callback routine with handle
        if ((result = usb_cmd_stream_register_cb(handle, usb_cmd_stream_callback, &stream_count)) !=
            K4A_RESULT_SUCCEEDED)
        {
            printf("Failed with error code %d\n", result);
            status = CLI_ERROR;
            goto handleDestroy;
        }

        // Send power up command
        usb_cmd_write(handle,
                      DEV_CMD_DEPTH_POWER_ON, // Power Up Command
                      NULL,
                      0,
                      NULL,
                      0);

        // Send set mode command
        if ((result = usb_cmd_write(handle,
                                    DEV_CMD_DEPTH_MODE_SET, // Set Mode
                                    (uint8_t *)&stream_mode,
                                    sizeof(stream_mode),
                                    NULL,
                                    0)) != K4A_RESULT_SUCCEEDED)
        {

            printf("Set depth mode command failed with error code %d\n", result);
            status = CLI_ERROR;
            goto handleDestroy;
        }

        // Send set FPS command
        if ((result = usb_cmd_write(handle,
                                    DEV_CMD_DEPTH_FPS_SET, // Set FPS
                                    (uint8_t *)&fps,
                                    sizeof(fps),
                                    NULL,
                                    0)) != K4A_RESULT_SUCCEEDED)
        {

            printf("Set depth FPS command failed with error code %d\n", result);
            status = CLI_ERROR;
            goto handleDestroy;
        }

        // Get the calibration data and store to file
        if ((result = usb_cmd_read(handle,
                                   DEV_CMD_NV_DATA_GET,
                                   (uint8_t *)&nv_tag,
                                   sizeof(nv_tag),
                                   data_buffer,
                                   MAX_BUFFER_SIZE,
                                   &bytes_read)) != K4A_RESULT_SUCCEEDED)
        {
#ifdef _WIN32
            if (fopen_s(&p_cal_file, "depth.ccb", "wb") == 0)
            {
#else
            if ((p_cal_file = fopen("depth.ccb", "wb")) != NULL)
            {
#endif
                fwrite(data_buffer, 1, bytes_read, p_cal_file);
                fclose(p_cal_file);
            }
        }

        // Send command to start the depth sensor on the device
        if ((result = usb_cmd_write(handle, DEV_CMD_DEPTH_START, NULL, 0, NULL, 0)) != K4A_RESULT_SUCCEEDED)
        {
            printf("Start depth sensor command failed with error code %d\n", result);
            status = CLI_ERROR;
            goto handleDestroy;
        }

        // Send start stream command
        if ((result = usb_cmd_write(handle, DEV_CMD_DEPTH_STREAM_START, NULL, 0, NULL, 0)) != K4A_RESULT_SUCCEEDED)
        {
            printf("Start depth stream command failed with error code %d\n", result);
            status = CLI_ERROR;
            goto handleDestroy;
        }

        printf("Starting Depth streaming\n");
#ifdef _WIN32
        if (fopen_s(&p_file, "depth.rec", "wb") == 0)
        {
#else
        if ((p_file = fopen("depth.rec", "wb")) == NULL)
        {
#endif
            usb_cmd_stream_start(handle, payload_size);
            while (stream_count > 0)
            {
            }
            usb_cmd_stream_stop(handle);
            fclose(p_file);
        }

        // Send command to stop the depth stream on the device
        if ((result = usb_cmd_write(handle, DEV_CMD_DEPTH_STREAM_STOP, NULL, 0, NULL, 0)) != K4A_RESULT_SUCCEEDED)
        {
            printf("Stop depth stream command failed with error code %d\n", result);
            status = CLI_ERROR;
        }

        if ((result = usb_cmd_write(handle, DEV_CMD_DEPTH_STOP, NULL, 0, NULL, 0)) != K4A_RESULT_SUCCEEDED)
        {
            printf("Stop depth command failed with error code %d\n", result);
            status = CLI_ERROR;
        }
    handleDestroy:
        usb_cmd_destroy(handle);
    }
    else
    {
        printf("Device not found\n");
        status = CLI_ERROR;
    }

    // re-open the k4a instance
    open_k4a();
    return status;
}

/**
 *  Example unit test file
 *
 */
void usb_cmd_init(void)
{
    CliRegister(CLI_MENU_CMD,
                "imurd",
                usb_cmd_imu_read,
                "Request a read from an IMU device",
                "Read from an IMU device\n"
                "Syntax: imurd <device index> <command> [space separated command data]\n"
                "Example: imurd 1 2 \"6 5 4\"");
    CliRegister(CLI_MENU_CMD,
                "imuwr",
                usb_cmd_imu_write,
                "Request a write to an IMU device",
                "Write to an IMU device\n"
                "Syntax: imuwr <device index> <command> [\"space separated data\"] [\"space separated data\"]\n"
                "Example: imuwr 1 2 \"6 5 4\" \"23 32 45\"");
    CliRegister(CLI_MENU_CMD,
                "depthrd",
                usb_cmd_depth_read,
                "Request a read from a depth device",
                "Read from a depth device\n"
                "Syntax: depthrd <device index> <command> [\"space separated command data\"]\n"
                "Example: depthrd 1 2 \"6 5 4\"");
    CliRegister(CLI_MENU_CMD,
                "depthwr",
                usb_cmd_depth_write,
                "Request a write to a depth device",
                "Write to a depth device\n"
                "Syntax: depthwr <device index> <command> [\"space separated data\"] [\"space separated data\"]\n"
                "Example: depthwr 1 2 \"6 5 4\" \"23 32 45\"");
    CliRegister(CLI_MENU_CMD,
                "imustream",
                usb_cmd_read_imu_stream,
                "Read stream from a device",
                "Read imu stream from an IMU device\n"
                "Syntax: stream <device index> <number of captures>\n"
                "Example: stream 1 20");
    CliRegister(CLI_MENU_CMD,
                "depthstream",
                usb_cmd_read_depth_stream,
                "Read stream from a device",
                "Read depth stream from a depth device\n"
                "Syntax: stream <device index> <number of captures> <mode> [fps]\n"
                "Example: stream 1 20 4\n"
                "Acceptable modes:\n"
                "PSEUDO_COMMON        = 3\n"
                "LONG_THROW_NATIVE    = 4\n"
                "MEGA_PIXEL           = 5\n"
                "QUARTER_MEGA_PIXEL   = 7\n"
                "Acceptable FPS (Default = 30):\n"
                "30 fps               = 30\n"
                "15 fps               = 15\n"
                "5 fps                = 5\n");
    tick_handle = tickcounter_create();
}
