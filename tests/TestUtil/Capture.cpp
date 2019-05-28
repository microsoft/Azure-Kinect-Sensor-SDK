// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <stdio.h>
#include <k4a/k4a.h>
#include <k4ainternal/capture.h>
#include <k4ainternal/logging.h>
#include <k4ainternal/usbcommand.h>
#include <assert.h>

#include "Cli.h"
#include "Main.h"
#include "../src/depth_mcu/depthcommands.h" // include private command definitions for testing
#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/lock.h>
#include "UsbCmd.h"
#include "Capture.h"

#ifdef _WIN32
#include "mfcamerareader.h"

//**************Symbolic Constant Macros (defines)  *************
#define CLI_MENU_CAPTURE "capture"
#define MAX_BUFFER_SIZE 2000000
#define SAMPLE_SIZE 128
#define DEFAULT_CAPTURE_COUNT 10
#define START_IMU_STREAM_CMD 0x80000003
#define STOP_IMU_STREAM_CMD 0x80000004
#define PSEUDO_COMMON 3
#define LONG_THROW_NATIVE 4
#define MEGA_PIXEL 5
#define QUARTER_MEGA_PIXEL 7
#define COLOR_SENSOR "color"
#define DEPTH_SENSOR "depth"

//************************ Typedefs *****************************
#pragma pack(push, 1)
typedef struct _InputFrameFooter_t
{
    unsigned int Signature;
    unsigned short BlockSize;
    unsigned short BlockVer;
    unsigned long long TimeStamp;
    float SensorTemp;
    float ModuleTemp;
    unsigned long long USBSoFSeqNum;
    unsigned long long USBSoFPTS;
} InputFrameFooter_t;
#pragma pack(pop)

//************ Declarations (Statics and globals) ***************
static uint8_t data_buffer[MAX_BUFFER_SIZE];
static FILE *p_file = NULL;
static TICK_COUNTER_HANDLE tick_handle;
static uint32_t stream_count;
static LOCK_HANDLE stream_mutex;

//******************* Function Prototypes ***********************

//*********************** Functions *****************************
static usb_cmd_stream_cb_t image_stream_callback;

static void capture_stream_callback(k4a_result_t result, k4a_capture_t capture_handle, void *context)
{
    if (result == K4A_RESULT_SUCCEEDED)
    {
        k4a_image_t image = capture_get_color_image(capture_handle);
        image_stream_callback(result, image, context);
        image_dec_ref(image);
    }
    else
    {
        image_stream_callback(result, NULL, context);
    }
}

/**
 *  callback routine for displaying stream data
 *
 *  @param result            status of last transfer
 *
 *  @param capture_handle    Memory reference
 *
 *  @param context           Parameter passed in
 *
 */
static void image_stream_callback(k4a_result_t result, k4a_image_t image_handle, void *context)
{
    char *p_type = (char *)context;
    size_t capture_size;
    uint8_t *p_frame = NULL;
    InputFrameFooter_t *p_depth_info = NULL;
    tickcounter_ms_t now;

    p_frame = image_get_buffer(image_handle);
    assert(p_frame != 0);

    capture_size = image_get_size(image_handle);

    (void)result;
    Lock(stream_mutex);
    printf("%s", p_type);
    printf("%11d", stream_count);
    if (tickcounter_get_current_ms(tick_handle, &now) == 0)
    {
        printf("%12lld", now);
    }

    if (p_type[0] == 'd')
    {
        p_frame += capture_size;
        p_frame -= sizeof(InputFrameFooter_t);
        p_depth_info = (InputFrameFooter_t *)p_frame;
        printf("%32lld    ", K4A_90K_HZ_TICK_TO_USEC(p_depth_info->TimeStamp));
    }
    else
    {
        printf("%16lld                    ", image_get_device_timestamp_usec(image_handle));
    }
    printf("%10zu\n", capture_size);
    if (p_file != NULL)
    {
        fprintf(p_file, "\n%s, ", p_type);
        fprintf(p_file, "%d, ", stream_count);
        if (tickcounter_get_current_ms(tick_handle, &now) == 0)
        {
            fprintf(p_file, "%lld, ", now);
        }

        if (p_type[0] == 'd')
        {
            fprintf(p_file, "%lld, ", K4A_90K_HZ_TICK_TO_USEC(p_depth_info->TimeStamp));
        }
        else
        {
            fprintf(p_file, "%lld, ", image_get_device_timestamp_usec(image_handle));
        }
        fprintf(p_file, "%zu", capture_size);
    }
    stream_count--;
    Unlock(stream_mutex);
}

/**
 *  Command to read color and depth stream and display timing
 *  metrics between frames
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
static CLI_STATUS capture_cmd_sync(int Argc, char **Argv)
{
    CLI_STATUS status = CLI_SUCCESS;
    k4a_result_t result;
    usbcmd_t handle;
    uint32_t depth_mode;
    uint32_t fps;
    uint32_t depth_fps;
    uint32_t color_format;
    uint32_t color_resolution;
    uint32_t width = 0;
    uint32_t height = 0;
    float color_fps = 1.0f;
    Microsoft::WRL::ComPtr<CMFCameraReader> spCameraReader;
    size_t payload_size = 0;

    uint32_t nv_tag = DEVICE_NV_IR_SENSOR_CALIBRATION;
    size_t bytes_read;
    FILE *p_cal_file;

    uint8_t ContainerID[16];
    Microsoft::WRL::MakeAndInitialize<CMFCameraReader>(&spCameraReader, (GUID *)ContainerID);

    // Get user input
    if (Argc < 2)
    {
        // Needs at least the stream count
        CliDisplayUsage(capture_cmd_sync);
        return CLI_ERROR;
    }
    // Get number of captures to read
    if ((!CliGetStrVal(Argv[1], &stream_count)))
    {
        return CLI_ERROR;
    }

    if (Argc < 7)
    {
        printf("Invalid number of parameters, using defaults:\n");
        printf("   Depth Mode = LONG_THROW_NATIVE\n");
        printf("   Depth FPS = 30 FPS\n");
        printf("   Color Format = K4A_IMAGE_FORMAT_COLOR_MJPG\n");
        printf("   Color resolution = K4A_COLOR_RESOLUTION_2160P\n");
        printf("   Color FPS = 30 FPS\n");
        depth_mode = LONG_THROW_NATIVE;
        depth_fps = 30;
        color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        width = 3840;
        height = 2160;
        color_fps = 30.0f;
    }
    else
    {
        // Get depth mode
        if (!CliGetStrVal(Argv[2], &depth_mode))
        {
            return CLI_ERROR;
        }

        // Get FPS
        if (!CliGetStrVal(Argv[3], &fps))
        {
            return CLI_ERROR;
        }

        switch (depth_mode)
        {
        case PSEUDO_COMMON:
            payload_size = SENSOR_MODE_PSEUDO_COMMON_PAYLOAD_SIZE;
            break;
        case LONG_THROW_NATIVE:
            payload_size = SENSOR_MODE_LONG_THROW_NATIVE_PAYLOAD_SIZE;
            break;
        case MEGA_PIXEL:
            payload_size = SENSOR_MODE_MEGA_PIXEL_PAYLOAD_SIZE;
            break;
        case QUARTER_MEGA_PIXEL:
            payload_size = SENSOR_MODE_QUARTER_MEGA_PIXEL_PAYLOAD_SIZE;
            break;
        default:
            printf("depth mode %d is invalid", depth_mode);
            CliDisplayUsage(capture_cmd_sync);
            return CLI_ERROR;
        }

        switch (fps)
        {
        case K4A_FRAMES_PER_SECOND_30:
            depth_fps = 30;
            break;
        case K4A_FRAMES_PER_SECOND_15:
            depth_fps = 15;
            break;
        case K4A_FRAMES_PER_SECOND_5:
            depth_fps = 5;
            break;
        default:
            printf("depth fps %d is invalid", fps);
            CliDisplayUsage(capture_cmd_sync);
            return CLI_ERROR;
        }

        // Get color format
        if (!CliGetStrVal(Argv[4], &color_format))
        {
            return CLI_ERROR;
        }

        // Get color resolution
        if (!CliGetStrVal(Argv[5], &color_resolution))
        {
            return CLI_ERROR;
        }

        // Get color fps
        if (!CliGetStrVal(Argv[6], &fps))
        {
            return CLI_ERROR;
        }

        switch (color_format)
        {
        case K4A_IMAGE_FORMAT_COLOR_NV12:
        case K4A_IMAGE_FORMAT_COLOR_YUY2:
        case K4A_IMAGE_FORMAT_COLOR_MJPG:
            break;
        default:
            printf("color fomat %d is invalid", color_format);
            CliDisplayUsage(capture_cmd_sync);
            return CLI_ERROR;
        }

        // set up color operating parameters based on input
        switch (color_resolution)
        {
        case K4A_COLOR_RESOLUTION_720P:
            width = 1280;
            height = 720;
            break;
        case K4A_COLOR_RESOLUTION_2160P:
            width = 3840;
            height = 2160;
            break;
        case K4A_COLOR_RESOLUTION_1440P:
            width = 2560;
            height = 1440;
            break;
        case K4A_COLOR_RESOLUTION_1080P:
            width = 1920;
            height = 1080;
            break;
        case K4A_COLOR_RESOLUTION_3072P:
            width = 4096;
            height = 3072;
            break;
        case K4A_COLOR_RESOLUTION_1536P:
            width = 2048;
            height = 1536;
            break;
        default:
            printf("color resolution %d is invalid", color_resolution);
            CliDisplayUsage(capture_cmd_sync);
            return CLI_ERROR;
        }

        switch (fps)
        {
        case K4A_FRAMES_PER_SECOND_30:
            color_fps = 30.0f;
            break;
        case K4A_FRAMES_PER_SECOND_15:
            color_fps = 15.0f;
            break;
        case K4A_FRAMES_PER_SECOND_5:
            color_fps = 5.0f;
            break;
        default:
            printf("color fps %d is invalid", fps);
            CliDisplayUsage(capture_cmd_sync);
            return CLI_ERROR;
        }
    }

    // Close the K4A instance to allow direct access to lower level libraries
    close_k4a();

    // Get handle instance
    if (usb_cmd_create(USB_DEVICE_DEPTH_PROCESSOR, 0, NULL, &handle) == K4A_RESULT_SUCCEEDED)
    {
        tick_handle = tickcounter_create();
        stream_mutex = Lock_Init();

        printf("                              Color           Depth\n");
        printf("Sensor  Countdown  Tick(mSec) Timestamp(uSec) Timestamp(uSec)   Size (bytes)\n");

        // Start up color
        spCameraReader->Start(width,
                              height,                           // Resolution
                              color_fps,                        // Frame rate
                              (k4a_image_format_t)color_format, // Color format enum
                              &capture_stream_callback,         // Callback
                              (void *)COLOR_SENSOR);            // Callback context

        // Start up depth
        // Register callback routine with handle
        if ((result = usb_cmd_stream_register_cb(handle, image_stream_callback, (void *)DEPTH_SENSOR)) !=
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
                                    (uint8_t *)&depth_mode,
                                    sizeof(depth_mode),
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
                                    (uint8_t *)&depth_fps,
                                    sizeof(depth_fps),
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
            if (fopen_s(&p_cal_file, "depth.ccb", "wb") == 0)
            {
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

        if (fopen_s(&p_file, "capture.csv", "w") == 0)
        {
            // Write column headers
            fprintf(p_file, "Sensor, Countdown, HostTick, Timestamp, Size");

            usb_cmd_stream_start(handle, payload_size);
            while (((int32_t)stream_count) > 0)
            {
                ThreadAPI_Sleep(5);
            }
            usb_cmd_stream_stop(handle);
            Lock(stream_mutex);
            fclose(p_file);
            p_file = NULL;
            Unlock(stream_mutex);
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
        spCameraReader->Shutdown();
        spCameraReader.Reset();

        usb_cmd_destroy(handle);

        Lock_Deinit(stream_mutex);
        tickcounter_destroy(tick_handle);
    }
    else
    {
        printf("Device not found\n");
        status = CLI_ERROR;
    }

    // close out displayed data
    printf("\n");

    // re-open the k4a instance
    open_k4a();

    return status;
}
#endif
// _WIN32

/**
 *  Initialization for this module
 *
 */
void capture_init(void)
{
#ifdef _WIN32
    CliRegister(
        CLI_MENU_CAPTURE,
        "capsync",
        capture_cmd_sync,
        "Capture frames and verify sync data",
        "Captures both depth and color and display time discrepancies\n"
        "Syntax: capsync <number of captures> <depth mode> <depth fps> <color format> <color resolution> <color fps>\n"
        "Example: capsync 20 4 3 3 5 3\n"
        "Acceptable depth modes:\n"
        "3 = PSEUDO_COMMON {K4A_DEPTH_MODE_PASSIVE_IR}\n"
        "4 = LONG_THROW_NATIVE {K4A_DEPTH_MODE_NFOV_2X2BINNED & K4A_DEPTH_MODE_NFOV_UNBINNED}\n"
        "5 = MEGA_PIXEL {K4A_DEPTH_MODE_WFOV_UNBINNED}\n"
        "7 = QUARTER_MEGA_PIXEL {K4A_DEPTH_MODE_WFOV_2X2BINNED}\n"
        "Acceptable Depth FPS:\n"
        "1 = 5  fps\n"
        "2 = 15 fps\n"
        "3 = 30 fps \n"
        "Acceptable color format:\n"
        "1 = K4A_IMAGE_FORMAT_COLOR_NV12\n"
        "2 = K4A_IMAGE_FORMAT_COLOR_YUY2\n"
        "3 = K4A_IMAGE_FORMAT_COLOR_MJPG\n"
        "Acceptable color resolution:\n"
        "1 = K4A_COLOR_RESOLUTION_720P\n"
        "2 = K4A_COLOR_RESOLUTION_1080P\n"
        "3 = K4A_COLOR_RESOLUTION_1440P\n"
        "4 = K4A_COLOR_RESOLUTION_1536P\n"
        "5 = K4A_COLOR_RESOLUTION_2160P\n"
        "6 = K4A_COLOR_RESOLUTION_3072P\n"
        "Acceptable color FPS:\n"
        "1 = 5  fps\n"
        "2 = 15 fps\n"
        "3 = 30 fps\n");
#endif
}
