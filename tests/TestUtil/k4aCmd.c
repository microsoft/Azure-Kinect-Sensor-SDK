// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <k4a/k4a.h>
#include <k4ainternal/usbcommand.h>
#include "Cli.h"
#include "Main.h"
#include "k4aCmd.h"

//**************Symbolic Constant Macros (defines)  *************
#define CLI_MENU_K4A "k4a"
#define MAX_BUFFER_SIZE 256
#define TIMESTAMP_CONVERSION 1000000
#define ERROR_COUNT_MAX 10
#define FILE_FORMAT_BINARY "wb"
#define FILE_FORMAT_ASCII "w"

//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************
static uint8_t data_buffer[MAX_BUFFER_SIZE];

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

/**
 *  Command to get the serial numbers
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
static CLI_STATUS k4a_serial_num(int Argc, char **Argv)
{
    (void)Argc; // unused
    (void)Argv; // unused
    k4a_device_t handle;
    CLI_STATUS status = CLI_SUCCESS;

    handle = get_k4a_handle(0);

    if (handle != NULL)
    {
        size_t serial_number_size = MAX_BUFFER_SIZE;
        if (k4a_device_get_serialnum(handle, (char *)data_buffer, &serial_number_size) == K4A_BUFFER_RESULT_SUCCEEDED)
        {
            // display the serial number data
            printf("SerialNumber: ");
            for (uint8_t loop = 0; loop < 64; loop += 2)
            {
                if (loop % 32 == 0)
                    printf("\n    ");
                uint16_t data = 0;
                memcpy(&data, &data_buffer[loop], sizeof(data));
                printf("%04X ", data);
            }
            printf("\n");
        }
        else
        {
            printf("Couldn't read from device\n");
            status = CLI_ERROR;
        }
    }
    else
    {
        printf("Device not found\n");
        status = CLI_ERROR;
    }

    return status;
}

/**
 *  Command to get the version information
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
static CLI_STATUS k4a_version(int Argc, char **Argv)
{
    (void)Argc; // unused
    (void)Argv; // unused
    k4a_device_t handle;
    CLI_STATUS status = CLI_SUCCESS;

    k4a_hardware_version_t version;

    handle = get_k4a_handle(0);

    if (handle != NULL)
    {
        if (k4a_device_get_version(handle, &version) == K4A_RESULT_SUCCEEDED)
        {
            // display the version
            printf("RGB version: %d.%d.%d\n", version.rgb.major, version.rgb.minor, version.rgb.iteration);
            printf("Depth version: %d.%d.%d\n", version.depth.major, version.depth.minor, version.depth.iteration);
            printf("Audio version: %d.%d.%d\n", version.audio.major, version.audio.minor, version.audio.iteration);
            printf("Depth Sequence version: %d.%d\n", version.depth_sensor.major, version.depth_sensor.minor);
            printf("%s\n", version.firmware_build == K4A_FIRMWARE_BUILD_RELEASE ? "Release" : "Debug");
            if (version.firmware_signature == K4A_FIRMWARE_SIGNATURE_MSFT)
            {
                printf("MSFT\n");
            }
            if (version.firmware_signature == K4A_FIRMWARE_SIGNATURE_TEST)
            {
                printf("TEST\n");
            }
            if (version.firmware_signature == K4A_FIRMWARE_SIGNATURE_UNSIGNED)
            {
                printf("No Signature\n");
            }
        }
        else
        {
            printf("Couldn't read from device\n");
            status = CLI_ERROR;
        }
    }
    else
    {
        printf("Device not found\n");
        status = CLI_ERROR;
    }

    return status;
}

/**
 *  Command to record a number of depth frames to disk
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
static CLI_STATUS k4a_record_depth(int Argc, char **Argv)
{
    CLI_STATUS status = CLI_SUCCESS;
    uint32_t device_num = 0;
    uint32_t stream_count = 0;
    uint32_t mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    uint32_t fps = K4A_FRAMES_PER_SECOND_30;
    char *file_name = "depth.rec";
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    int32_t timeout_ms = 70;
    k4a_capture_t capture;
    k4a_device_t device = NULL;
    uint32_t wait_error_count = 0;
    FILE *p_file;

    if (Argc < 3)
    {
        CliDisplayUsage(k4a_record_depth);
        return CLI_ERROR;
    }

    // Get device number
    if (!CliGetStrVal(Argv[1], &device_num))
    {
        return CLI_ERROR;
    }

    // Get number of frames to read
    if (!CliGetStrVal(Argv[2], &stream_count))
    {
        return CLI_ERROR;
    }

    // Get optional operating mode
    if ((Argc > 3) && ((!CliGetStrVal(Argv[3], &mode)) || (mode > K4A_DEPTH_MODE_PASSIVE_IR)))
    {
        return CLI_ERROR;
    }

    // Get optional FPS
    if ((Argc > 4) &&
        ((!CliGetStrVal(Argv[4], &fps)) ||
         (fps != K4A_FRAMES_PER_SECOND_5 && fps != K4A_FRAMES_PER_SECOND_15 && fps != K4A_FRAMES_PER_SECOND_30)))
    {
        return CLI_ERROR;
    }

    device = get_k4a_handle(device_num);

    // open a file to write to
#ifdef _WIN32
    if (fopen_s(&p_file, file_name, FILE_FORMAT_BINARY) == 0)
    {
#else
    if ((p_file = fopen(file_name, FILE_FORMAT_BINARY)) != NULL)
    {
#endif
        // start the stream
        config.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        config.color_resolution = K4A_COLOR_RESOLUTION_OFF;
        config.depth_mode = mode;
        config.camera_fps = fps;

        // start streaming.
        if (K4A_FAILED(k4a_device_start_cameras(device, &config)))
        {
            return CLI_ERROR;
        }

        // loop through stream data
        while (stream_count-- > 0)
        {
            // Synchronize
            if (K4A_WAIT_RESULT_SUCCEEDED == k4a_device_get_capture(device, &capture, timeout_ms))
            {
                wait_error_count = 0; // reset the error count

                uint8_t *buffer;
                k4a_image_t image;

                image = k4a_capture_get_depth_image(capture);
                if (image)
                {
                    buffer = k4a_image_get_buffer(image);
                    if (buffer)
                    {
                        // store to disk
                        fwrite(buffer, 1, k4a_image_get_size(image), p_file);
                    }
                    k4a_image_release(image);
                }

                image = k4a_capture_get_ir_image(capture);
                if (image)
                {
                    buffer = k4a_image_get_buffer(image);
                    if (buffer)
                    {
                        // store to disk
                        fwrite(buffer, 1, k4a_image_get_size(image), p_file);
                    }
                }

                k4a_capture_release(capture);
            }
            else
            {
                wait_error_count++;
                if (wait_error_count > ERROR_COUNT_MAX)
                {
                    printf("Failed to receive a frame within %d mSec\n", timeout_ms * ERROR_COUNT_MAX);
                    status = CLI_ERROR;
                    break;
                }
            }
        };

        k4a_device_stop_cameras(device);
        fclose(p_file);
    }
    else
    {
        printf("Could not open %s to write\n", file_name);
        status = CLI_ERROR;
    }

    return status;
}

/**
 *  Command to record IMU samples
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
static CLI_STATUS k4a_record_imu(int Argc, char **Argv)
{
    CLI_STATUS status = CLI_SUCCESS;
    uint32_t stream_count;
    uint32_t device_num = 0;
    bool csv = false;
    FILE *p_file = 0;
    k4a_device_t device = NULL;
    k4a_wait_result_t result;
    char *file_name = "imu.rec";
    uint32_t wait_error_count = 0;
    int32_t timeout_ms = 150;
    k4a_imu_sample_t imu_sample;
    char *file_format = FILE_FORMAT_BINARY;

    if (Argc < 3)
    {
        CliDisplayUsage(k4a_record_imu);
        return CLI_ERROR;
    }

    // Get device number
    if (!CliGetStrVal(Argv[1], &device_num))
    {
        return CLI_ERROR;
    }

    // Get number of samples to record
    if (!CliGetStrVal(Argv[2], &stream_count))
    {
        return CLI_ERROR;
    }

    // Check if CSV option is selected
    if ((Argc > 3) && (memcmp(Argv[3], "csv", 3) == 0))
    {
        csv = true;
        file_format = FILE_FORMAT_ASCII;
        file_name = "imu.csv";
    }

    device = get_k4a_handle(device_num);

    // open a file to write to
#ifdef _WIN32
    if (fopen_s(&p_file, file_name, file_format) == 0)
    {
#else
    if ((p_file = fopen(file_name, file_format)) != NULL)
    {
#endif
        // configure and start stream
        if (k4a_device_start_imu(device) == K4A_RESULT_SUCCEEDED)
        {
            // loop through stream data
            if (csv)
            {
                // Print csv header
                fprintf(p_file, "Temp, Accel TS, Accel X, Accel Y, Accel Z, Gyro TS, Gyro X, Gyro Y, Gyro Z\n");
            }
            while (stream_count-- > 0)
            {
                result = k4a_device_get_imu_sample(device, &imu_sample, timeout_ms);
                if (result == K4A_WAIT_RESULT_TIMEOUT)
                {
                    wait_error_count++;
                    if (wait_error_count > ERROR_COUNT_MAX)
                    {
                        printf("Failed to receive a frame within %d mSec\n", timeout_ms * ERROR_COUNT_MAX);
                        status = CLI_ERROR;
                        break;
                    }
                }
                else if (result == K4A_WAIT_RESULT_SUCCEEDED)
                {
                    wait_error_count = 0; // reset the error count

                    // store to disk
                    if (csv)
                    {
                        fprintf(p_file,
                                "%f, %f, %f, %f, %f, %f, %f, %f, %f\n",
                                (double)imu_sample.temperature,
                                (double)imu_sample.acc_timestamp_usec / TIMESTAMP_CONVERSION,
                                (double)imu_sample.acc_sample.xyz.x,
                                (double)imu_sample.acc_sample.xyz.y,
                                (double)imu_sample.acc_sample.xyz.z,
                                (double)imu_sample.gyro_timestamp_usec / TIMESTAMP_CONVERSION,
                                (double)imu_sample.gyro_sample.xyz.x,
                                (double)imu_sample.gyro_sample.xyz.y,
                                (double)imu_sample.gyro_sample.xyz.z);
                    }
                    else
                    {
                        fwrite(&imu_sample, 1, sizeof(imu_sample), p_file);
                    }
                }
            };
        }
        else
        {
            printf("Couldn't start the stream\n");
            status = CLI_ERROR;
        }

        k4a_device_stop_imu(device);
        fclose(p_file);
    }
    else
    {
        printf("Could not open %s to write\n", file_name);
        status = CLI_ERROR;
    }

    return status;
}

/**
 *  initialization for the K4A command interface
 *
 */
void k4a_cmd_init(void)
{
    CliRegister(CLI_MENU_K4A,
                "serialnum",
                k4a_serial_num,
                "Display the serial numbers",
                "Display the serial numbers\n"
                "Syntax: serialnum\n"
                "Example: serialnum\n");
    CliRegister(CLI_MAIN_MENU,
                "version",
                k4a_version,
                "Display version information",
                "Display version information\n"
                "Syntax: version\n"
                "Example: version\n");
    CliRegister(CLI_MENU_K4A,
                "recdepth",
                k4a_record_depth,
                "Record depth frames",
                "Store to disk a set number of frames\n"
                "Syntax: recdepth <device> <# of frames> [mode] [FPS]\n"
                "Example: recdepth 0 60 1 1\n"
                "Acceptable modes (Default NFOV_UNBINNED:\n"
                " NFOV_2x2BINNED = 0\n"
                " NFOV_UNBINNED  = 1\n"
                " WFOV_2x2BINNED = 2\n"
                " WFOV_UNBINNED  = 3\n"
                " PASSIVE_IR     = 4\n"
                "Acceptable FPS (Default 30 FPS, depends on mode):\n"
                " 30 FPS         = 0\n"
                " 15 FPS         = 1\n"
                " 5 FPS          = 2\n");
    CliRegister(CLI_MENU_K4A,
                "recimu",
                k4a_record_imu,
                "Record IMU stream",
                "Store to disk IMU set number of samples\n"
                "Syntax: recimu <device> <# of samples>\n"
                "Example: recimu 0 100\n");
}
