// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4a/k4a.h>

#include "cmdparser.h"
#include "recorder.h"
#include "assert.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#include <iostream>
#include <atomic>
#include <ctime>
#include <csignal>
#include <math.h>
#include <k4ainternal\math.h>

static time_t exiting_timestamp;

static void signal_handler(int s)
{
    (void)s; // Unused

    if (!exiting)
    {
        std::cout << "Stopping recording..." << std::endl;
        exiting_timestamp = clock();
        exiting = true;
    }
    // If Ctrl-C is received again after 1 second, force-stop the application since it's not responding.
    else if (exiting_timestamp != 0 && clock() - exiting_timestamp > CLOCKS_PER_SEC)
    {
        std::cout << "Forcing stop." << std::endl;
        exit(1);
    }
}

static int string_compare(const char *s1, const char *s2)
{
    assert(s1 != NULL);
    assert(s2 != NULL);

    while (tolower((unsigned char)*s1) == tolower((unsigned char)*s2))
    {
        if (*s1 == '\0')
        {
            return 0;
        }
        s1++;
        s2++;
    }
    // The return value shows the relations between s1 and s2.
    // Return value   Description
    //     < 0        s1 less than s2
    //       0        s1 identical to s2
    //     > 0        s1 greater than s2
    return (int)tolower((unsigned char)*s1) - (int)tolower((unsigned char)*s2);
}

[[noreturn]] static void list_devices()
{
    uint32_t device_count = k4a_device_get_installed_count();
    if (device_count > 0)
    {
        for (uint8_t i = 0; i < device_count; i++)
        {
            std::cout << std::endl;
            std::cout << "Index: " << (int)i << std::endl << std::endl;
            k4a_device_t device;
            if (K4A_SUCCEEDED(k4a_device_open(i, &device)))
            {
                char serial_number_buffer[256];
                size_t serial_number_buffer_size = sizeof(serial_number_buffer);
                if (k4a_device_get_serialnum(device, serial_number_buffer, &serial_number_buffer_size) ==
                    K4A_BUFFER_RESULT_SUCCEEDED)
                {
                    std::cout << "\tSerial: " << serial_number_buffer << std::endl << std::endl;
                }
                else
                {
                    std::cout << "\tSerial: ERROR" << std::endl << std::endl;
                }

                // device info
                k4a_device_info_t device_info = { sizeof(k4a_device_info_t), K4A_ABI_VERSION, { 0 } };
                if (k4a_device_get_info(device, &device_info) == K4A_RESULT_SUCCEEDED)
                {
                    bool hasColorDevice = false;
                    bool hasDepthDevice = false;
                    bool hasIMUDevice = false;
                    uint32_t capabilities = (uint32_t)device_info.capabilities;

                    hasDepthDevice = (capabilities & 0x0001) == 1;
                    hasColorDevice = ((capabilities >> 1) & 0x01) == 1;
                    hasIMUDevice = ((capabilities >> 2) & 0x01) == 1;

                    k4a_hardware_version_t version_info;
                    if (K4A_SUCCEEDED(k4a_device_get_version(device, &version_info)))
                    {
                        if (hasColorDevice)
                        {
                            std::cout << "\tColor: Supported (" << version_info.rgb.major << "."
                                      << version_info.rgb.minor << "." << version_info.rgb.iteration << ")"
                                      << std::endl;
                        }
                        else
                        {
                            std::cout << "\tColor: Unsupported";
                        }
                        if (hasDepthDevice)
                        {
                            std::cout << "\tDepth: Supported (" << version_info.depth.major << "."
                                      << version_info.depth.minor << "." << version_info.depth.iteration << ")"
                                      << std::endl;
                        }
                        else
                        {
                            std::cout << "\tDepth: Unsupported";
                        }
                        if (hasIMUDevice)
                        {
                            std::cout << "\tIMU: Supported";
                        }
                        else
                        {
                            std::cout << "\tIMU: Unsupported";
                        }

                        std::cout << std::endl;

                        if (hasColorDevice)
                        {
                            int color_mode_count = 0;
                            k4a_device_get_color_mode_count(device, &color_mode_count);
                            if (color_mode_count > 0)
                            {
                                std::cout << std::endl;
                                std::cout << "\tColor modes: \tid = description" << std::endl;
                                std::cout << "\t\t\t----------------" << std::endl;
                                for (int j = 0; j < color_mode_count; j++)
                                {
                                    k4a_color_mode_info_t color_mode_info = { sizeof(k4a_color_mode_info_t),
                                                                              K4A_ABI_VERSION,
                                                                              { 0 } };
                                    if (k4a_device_get_color_mode(device, j, &color_mode_info) == K4A_RESULT_SUCCEEDED)
                                    {
                                        std::cout << "\t\t\t" << j << " = ";
                                        if (j == 0)
                                        {
                                            std::cout << "OFF" << std::endl;
                                        }
                                        else
                                        {
                                            int width = color_mode_info.width;
                                            int height = color_mode_info.height;
                                            int common_factor = math_get_common_factor(width, height);
                                            if (height < 1000)
                                            {
                                                std::cout << " ";
                                            }
                                            std::cout << height << "p ";
                                            std::cout << width / common_factor << ":";
                                            std::cout << height / common_factor << std::endl;
                                        }
                                    }
                                }
                            }
                        }

                        if (hasDepthDevice)
                        {
                            int depth_mode_count = 0;
                            k4a_device_get_depth_mode_count(device, &depth_mode_count);
                            if (depth_mode_count > 0)
                            {
                                std::cout << std::endl;
                                std::cout << "\tDepth modes: \tid = description" << std::endl;
                                std::cout << "\t\t\t----------------" << std::endl;
                                for (int j = 0; j < depth_mode_count; j++)
                                {
                                    k4a_depth_mode_info_t depth_mode_info = { sizeof(k4a_depth_mode_info_t),
                                                                              K4A_ABI_VERSION,
                                                                              { 0 } };
                                    if (k4a_device_get_depth_mode(device, j, &depth_mode_info) == K4A_RESULT_SUCCEEDED)
                                    {
                                        std::cout << "\t\t\t" << j << " = ";
                                        if (j == 0)
                                        {
                                            std::cout << "OFF" << std::endl;
                                        }
                                        else
                                        {
                                            int width = depth_mode_info.width;
                                            int height = depth_mode_info.height;
                                            float fov = depth_mode_info.horizontal_fov;
                                            if (depth_mode_info.passive_ir_only)
                                            {
                                                std::cout << "Passive IR" << std::endl;
                                            }
                                            else
                                            {
                                                if (height < 1000)
                                                {
                                                    std::cout << " ";
                                                }
                                                std::cout << width << "x";
                                                std::cout << height << ", ";
                                                std::cout << fov << "Deg" << std::endl;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (hasColorDevice || hasDepthDevice)
                        {
                            int fps_mode_count = 0;
                            k4a_device_get_fps_mode_count(device, &fps_mode_count);
                            if (fps_mode_count > 0)
                            {
                                std::cout << std::endl;
                                std::cout << "\tFPS modes: \tid = description" << std::endl;
                                std::cout << "\t\t\t----------------" << std::endl;
                                for (int j = 0; j < fps_mode_count; j++)
                                {
                                    k4a_fps_mode_info_t fps_mode_info = { sizeof(k4a_fps_mode_info_t),
                                                                          K4A_ABI_VERSION,
                                                                          { 0 } };
                                    if (k4a_device_get_fps_mode(device, j, &fps_mode_info) == K4A_RESULT_SUCCEEDED)
                                    {
                                        std::cout << "\t\t\t" << j << " = " << fps_mode_info.fps << std::endl;
                                    }
                                }
                            }
                        }
                    }
                }

                k4a_device_close(device);
            }
            else
            {
                std::cout << i << "\tDevice Open Failed";
            }
            std::cout << std::endl;
        }
    }
    else
    {
        std::cout << "No devices connected." << std::endl;
    }
    exit(0);
}

static k4a_result_t get_device_info(uint32_t device_index, bool *hasDepthDevice, bool *hasColorDevice, bool *hasIMUDevice)
{
    uint32_t device_count = k4a_device_get_installed_count();
    if (device_count > 0)
    {
        k4a_device_t device;
        if (K4A_SUCCEEDED(k4a_device_open(device_index, &device)))
        {
            k4a_device_info_t device_info = { sizeof(k4a_device_info_t), K4A_ABI_VERSION, { 0 } };
            if (k4a_device_get_info(device, &device_info) == K4A_RESULT_SUCCEEDED)
            {
                uint32_t capabilities = (uint32_t)device_info.capabilities;

                *hasDepthDevice = (capabilities & 0x0001) == 1;
                *hasColorDevice = ((capabilities >> 1) & 0x01) == 1;
                *hasIMUDevice = ((capabilities >> 2) & 0x01) == 1;
                return K4A_RESULT_SUCCEEDED;
            }
            else
            {
                std::cout << device_index << "Device Get Info Failed" << std::endl;
            }
            k4a_device_close(device);
        }
        else
        {
            std::cout << device_index << "Device Open Failed" << std::endl;
        }
    }
    else
    {
        std::cout << "No devices connected or unkown device specified." << std::endl;
    }
    return K4A_RESULT_FAILED;
}

static k4a_result_t get_color_mode_info(uint32_t device_index,
                                        int32_t *mode_id,
                                        k4a_image_format_t image_format,
                                        k4a_color_mode_info_t *color_mode_info)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    uint32_t device_count = k4a_device_get_installed_count();
    if (device_count > 0)
    {
        k4a_device_t device;
        if (K4A_SUCCEEDED(k4a_device_open(device_index, &device)))
        {
            int mode_count = 0;
            k4a_device_get_color_mode_count(device, &mode_count);
            if (mode_count > 0)
            {
                if (image_format == K4A_IMAGE_FORMAT_COLOR_NV12 || image_format == K4A_IMAGE_FORMAT_COLOR_YUY2)
                {
                    *mode_id = -1;
                }
                if (*mode_id == -1)
                {
                    for (int i = 1; i < mode_count; i++)
                    {
                        k4a_color_mode_info_t mode_info;
                        if (K4A_SUCCEEDED(k4a_device_get_color_mode(device, i, &mode_info)))
                        {
                            if (mode_info.height <= 720)
                            {
                                *mode_id = i;
                                break;
                            }
                        }
                    }
                }

                result = k4a_device_get_color_mode(device, *mode_id, color_mode_info);
                if (result == K4A_RESULT_FAILED)
                {
                    color_mode_info->mode_id = 0;
                }
            }
            else
            {
                result = K4A_RESULT_FAILED;
            }

            k4a_device_close(device);
        }
        else
        {
            std::cout << device_index << "Device Open Failed" << std::endl;
            result = K4A_RESULT_FAILED;
        }
    }
    else
    {
        std::cout << "No devices connected or unkown device specified." << std::endl;
        result = K4A_RESULT_FAILED;
    }

    return result;
}

static k4a_result_t get_depth_mode_info(uint32_t device_index, int32_t * mode_id, k4a_depth_mode_info_t *depth_mode_info)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    uint32_t device_count = k4a_device_get_installed_count();
    if (device_count > 0)
    {
        k4a_device_t device;
        if (K4A_SUCCEEDED(k4a_device_open(device_index, &device)))
        {
            int mode_count = 0;
            k4a_device_get_depth_mode_count(device, &mode_count);
            if (mode_count > 0)
            {
                if (*mode_id == -1)
                {
                    for (int i = 1; i < mode_count; i++)
                    {
                        k4a_depth_mode_info_t mode_info;
                        if (K4A_SUCCEEDED(k4a_device_get_depth_mode(device, i, &mode_info)))
                        {
                            if (mode_info.width > 320 && mode_info.height > 288 && mode_info.horizontal_fov < 120.0f &&
                                mode_info.vertical_fov < 120.0f)
                            {
                                *mode_id = i;
                                break;
                            }
                        }
                    }
                }

                result = k4a_device_get_depth_mode(device, *mode_id, depth_mode_info);
                if (result == K4A_RESULT_FAILED)
                {
                    depth_mode_info->mode_id = 0;
                }
            }
            else
            {
                result = K4A_RESULT_FAILED;
            }

            k4a_device_close(device);
        }
        else
        {
            std::cout << device_index << "Device Open Failed" << std::endl;
            result = K4A_RESULT_FAILED;
        }
    }
    else
    {
        std::cout << "No devices connected or unkown device specified." << std::endl;
        result = K4A_RESULT_FAILED;
    }

    return result;
}

static k4a_result_t get_fps_mode_info(uint32_t device_index, int32_t * fps_mode_id, k4a_color_mode_info_t *color_mode_info, k4a_depth_mode_info_t *depth_mode_info, k4a_fps_mode_info_t *fps_mode_info)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    uint32_t device_count = k4a_device_get_installed_count();
    if (device_count > 0)
    {
        k4a_device_t device;
        if (K4A_SUCCEEDED(k4a_device_open(device_index, &device)))
        {
            int mode_count = 0;
            k4a_device_get_fps_mode_count(device, &mode_count);
            if (mode_count > 0)
            {
                if (*fps_mode_id == -1)
                {
                    int max_fps = 0;
                    for (int i = 1; i < mode_count; i++)
                    {
                        k4a_fps_mode_info_t mode_info;
                        if (K4A_SUCCEEDED(k4a_device_get_fps_mode(device, i, &mode_info)))
                        {
                            if (mode_info.fps > max_fps)
                            {
                                *fps_mode_id = i;
                                max_fps = mode_info.fps;
                            }
                        }
                    }
                }


                result = k4a_device_get_fps_mode(device, *fps_mode_id, fps_mode_info);
                if (K4A_SUCCEEDED(result))
                {
                    if (color_mode_info->height >= 3072 ||
                        (depth_mode_info->height >= 1024 && depth_mode_info->horizontal_fov >= 120.0f &&
                         depth_mode_info->vertical_fov >= 120.0f && depth_mode_info->min_range >= 250 &&
                         depth_mode_info->max_range >= 2500))
                    {
                        int fps = 0;
                        int mode_id = 0;
                        for (int i = 1; i < mode_count; i++)
                        {
                            k4a_fps_mode_info_t mode_info;
                            if (K4A_SUCCEEDED(k4a_device_get_fps_mode(device, i, &mode_info)))
                            {
                                if (mode_info.fps > fps && mode_info.fps <= 15)
                                {
                                    mode_id = i;
                                    fps = mode_info.fps;
                                }
                            }
                        }

                        std::cout << "Could not recording using 30 frames per second with camera settings." << std::endl;

                        if (mode_id != 0 && mode_id != *fps_mode_id)
                        {
                            result = k4a_device_get_fps_mode(device, *fps_mode_id, fps_mode_info);
                        }
                        else
                        {
                            result = K4A_RESULT_FAILED;
                        }
                    }
                }
            }
            else
            {
                result = K4A_RESULT_FAILED;
            }

            k4a_device_close(device);
        }
        else
        {
            std::cout << device_index << "Device Open Failed" << std::endl;
            result = K4A_RESULT_FAILED;
        }
    }
    else
    {
        std::cout << "No devices connected or unkown device specified." << std::endl;
        result = K4A_RESULT_FAILED;
    }

    return result;
}

int main(int argc, char **argv)
{
    int device_index = 0;
    int recording_length = -1;
    k4a_image_format_t recording_color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    int32_t recording_color_mode = -1;
    int32_t recording_depth_mode = -1;
    int32_t recording_fps_mode = -1;
    bool recording_imu_enabled = true;
    k4a_wired_sync_mode_t wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
    int32_t depth_delay_off_color_usec = 0;
    uint32_t subordinate_delay_off_master_usec = 0;
    int absoluteExposureValue = defaultExposureAuto;
    int gain = defaultGainAuto;
    char *recording_filename;

    CmdParser::OptionParser cmd_parser;
    cmd_parser.RegisterOption("-h|--help", "Prints this help", [&]() {
        std::cout << "k4arecorder [options] output.mkv" << std::endl << std::endl;
        cmd_parser.PrintOptions();
        exit(0);
    });
    cmd_parser.RegisterOption("--list",
                              "List the currently connected devices (includes color, depth and fps modes)",
                              list_devices);
    cmd_parser.RegisterOption("--device",
                              "Specify the device index to use (default: 0)",
                              1,
                              [&](const std::vector<char *> &args) {
                                  try
                                  {
                                      device_index = std::stoi(args[0]);
                                      if (device_index < 0 || device_index > 255)
                                          throw std::runtime_error("Device index must 0-255");
                                  }
                                  catch (const std::exception &)
                                  {
                                      std::ostringstream str;
                                      str << "Unknown device index specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("-l|--record-length",
                              "Limit the recording to N seconds (default: infinite)",
                              1,
                              [&](const std::vector<char *> &args) {
                                  try
                                  {
                                      recording_length = std::stoi(args[0]);
                                      if (recording_length < 0)
                                          throw std::runtime_error("Recording length must be positive");
                                  }
                                  catch (const std::exception &)
                                  {
                                      std::ostringstream str;
                                      str << "Unknown record length specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("-c|--color-mode",
                              "Set the color sensor mode (default: 0 for OFF), Use --list to see the available modes.",
                              1,
                              [&](const std::vector<char *> &args) {
                                  try
                                  {
                                      recording_color_mode = std::stoi(args[0]);
                                      if (recording_color_mode < 0)
                                          throw std::runtime_error("Color mode must be positive");
                                  }
                                  catch (const std::exception &)
                                  {
                                      std::ostringstream str;
                                      str << "Unknown color mode specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("-i|--image-format",
                              "Set the image format (default: MJPG), Available options:\n"
                              "MJPG, NV12, YUY2\n"
                              "Note that for NV12 and YUY2, the color resolution must not be greater than 720p.",
                              1,
                              [&](const std::vector<char *> &args) {
                                  if (string_compare(args[0], "MJPG") == 0)
                                  {
                                      recording_color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
                                  }
                                  else if (string_compare(args[0], "NV12") == 0)
                                  {
                                      recording_color_format = K4A_IMAGE_FORMAT_COLOR_NV12;
                                  }
                                  else if (string_compare(args[0], "YUY2") == 0)
                                  {
                                      recording_color_format = K4A_IMAGE_FORMAT_COLOR_YUY2;
                                  }
                                  else
                                  {
                                      std::ostringstream str;
                                      str << "Unknown image format specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("-d|--depth-mode",
                              "Set the depth sensor mode (default: 0 for OFF), Use --list to see the available modes.",
                              1,
                              [&](const std::vector<char *> &args) {
                                  try
                                  {
                                      recording_depth_mode = std::stoi(args[0]);
                                      if (recording_depth_mode < 0)
                                          throw std::runtime_error("Depth mode must be positive");
                                  }
                                  catch (const std::exception &)
                                  {
                                      std::ostringstream str;
                                      str << "Unknown depth mode specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("--depth-delay",
                              "Set the time offset between color and depth frames in microseconds (default: 0)\n"
                              "A negative value means depth frames will arrive before color frames.\n"
                              "The delay must be less than 1 frame period.",
                              1,
                              [&](const std::vector<char *> &args) {
                                  try
                                  {
                                      depth_delay_off_color_usec = std::stoi(args[0]);
                                      if (depth_delay_off_color_usec < 1)
                                          throw std::runtime_error("Depth delay must be less than 1 frame period");
                                  }
                                  catch (const std::exception &)
                                  {
                                      std::ostringstream str;
                                      str << "Unknown depth delay specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("-r|--rate",
                              "Set the camera frame rate in Frames per Second\n"
                              "Default is the maximum rate supported by the camera modes.\n"
                              "Use --list to see the available modes.",
                              1,
                              [&](const std::vector<char *> &args) {
                                  try
                                  {
                                      recording_fps_mode = std::stoi(args[0]);
                                      if (recording_fps_mode < 0)
                                          throw std::runtime_error("Frame rate must be positive");
                                  }
                                  catch (const std::exception &)
                                  {
                                      std::ostringstream str;
                                      str << "Unknown frame rate specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("--imu",
                              "Set the IMU recording mode (ON, OFF, default: ON)",
                              1,
                              [&](const std::vector<char *> &args) {
                                  if (string_compare(args[0], "on") == 0)
                                  {
                                      recording_imu_enabled = true;
                                  }
                                  else if (string_compare(args[0], "off") == 0)
                                  {
                                      recording_imu_enabled = false;
                                  }
                                  else
                                  {
                                      std::ostringstream str;
                                      str << "Unknown imu mode specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("--external-sync",
                              "Set the external sync mode (Master, Subordinate, Standalone default: Standalone)",
                              1,
                              [&](const std::vector<char *> &args) {
                                  if (string_compare(args[0], "master") == 0)
                                  {
                                      wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
                                  }
                                  else if (string_compare(args[0], "subordinate") == 0 ||
                                           string_compare(args[0], "sub") == 0)
                                  {
                                      wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
                                  }
                                  else if (string_compare(args[0], "standalone") == 0)
                                  {
                                      wired_sync_mode = K4A_WIRED_SYNC_MODE_STANDALONE;
                                  }
                                  else
                                  {
                                      std::ostringstream str;
                                      str << "Unknown external sync mode specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("--sync-delay",
                              "Set the external sync delay off the master camera in microseconds (default: 0)\n"
                              "This setting is only valid if the camera is in Subordinate mode.",
                              1,
                              [&](const std::vector<char *> &args) {
                                  try
                                  {
                                      subordinate_delay_off_master_usec = std::stoi(args[0]);
                                      if (subordinate_delay_off_master_usec < 0)
                                          throw std::runtime_error("External sync delay must be positive.");
                                  }
                                  catch (const std::exception &)
                                  {
                                      std::ostringstream str;
                                      str << "Unknown sync delay specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("-e|--exposure-control",
                              "Set manual exposure value from 2 us to 200,000us for the RGB camera (default: \n"
                              "auto exposure). This control also supports MFC settings of -11 to 1).",
                              1,
                              [&](const std::vector<char *> &args) {
                                  try
                                  {
                                      int exposureValue = std::stoi(args[0]);
                                      if (exposureValue >= -11 && exposureValue <= 1)
                                      {
                                          absoluteExposureValue = static_cast<int32_t>(exp2f((float)exposureValue) *
                                                                                       1000000.0f);
                                      }
                                      else if (exposureValue >= 2 && exposureValue <= 200000)
                                      {
                                          absoluteExposureValue = exposureValue;
                                      }
                                      else
                                      {
                                          throw std::runtime_error("Exposure value range is 2 to 5s, or -11 to 1.");
                                      }
                                  }
                                  catch (const std::exception &)
                                  {
                                      std::ostringstream str;
                                      str << "Unknown specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });
    cmd_parser.RegisterOption("-g|--gain",
                              "Set cameras manual gain. The valid range is 0 to 255. (default: auto)",
                              1,
                              [&](const std::vector<char *> &args) {
                                  try
                                  {
                                      int gainSetting = std::stoi(args[0]);
                                      if (gainSetting < 0 || gainSetting > 255)
                                      {
                                          throw std::runtime_error("Gain value must be between 0 and 255.");
                                      }
                                      gain = gainSetting;
                                  }
                                  catch (const std::exception &)
                                  {
                                      std::ostringstream str;
                                      str << "Unknown specified: " << args[0];
                                      throw std::runtime_error(str.str());
                                  }
                              });

    int args_left = 0;
    try
    {
        args_left = cmd_parser.ParseCmd(argc, argv);
    }
    catch (CmdParser::ArgumentError &e)
    {
        std::cerr << e.option() << ": " << e.what() << std::endl;
        return 1;
    }
    if (args_left == 1)
    {
        recording_filename = argv[argc - 1];
    }
    else
    {
        std::cout << "k4arecorder [options] output.mkv" << std::endl << std::endl;
        cmd_parser.PrintOptions();
        return 0;
    }

    if (recording_color_mode == -1 && recording_depth_mode == -1)
    {
        std::cout << "A recording requires either a color or a depth device." << std::endl;
        return 1;
    }

    bool hasColorDevice = false;
    bool hasDepthDevice = false;
    bool hasIMUDevice = false;

    k4a_color_mode_info_t color_mode_info = { sizeof(k4a_color_mode_info_t), K4A_ABI_VERSION, { 0 } };
    k4a_depth_mode_info_t depth_mode_info = { sizeof(k4a_depth_mode_info_t), K4A_ABI_VERSION, { 0 } };
    k4a_fps_mode_info_t fps_mode_info = { sizeof(k4a_fps_mode_info_t), K4A_ABI_VERSION, { 0 } };

    k4a_result_t device_info_result = get_device_info(device_index, &hasDepthDevice, &hasColorDevice, &hasIMUDevice);

    if (K4A_SUCCEEDED(device_info_result))
    {
        if (hasColorDevice)
        {
            if (!K4A_SUCCEEDED(get_color_mode_info(device_index, &recording_color_mode, recording_color_format, &color_mode_info))) {
                recording_color_mode = 0;
            }
        }
        else
        {
            recording_color_mode = 0;
        }

        if (hasDepthDevice)
        {
            if (!K4A_SUCCEEDED(get_depth_mode_info(device_index, &recording_depth_mode, &depth_mode_info)))
            {
                recording_depth_mode = 0;
            }
        }
        else 
        {
            recording_depth_mode = 0;
        }

        if (recording_color_mode == 0 && recording_depth_mode == 0)
        {
            std::cout << "A recording requires either a color or a depth device." << std::endl;
            return 1;
        }

        if (!K4A_SUCCEEDED(get_fps_mode_info(
                device_index, &recording_fps_mode, &color_mode_info, &depth_mode_info, &fps_mode_info)))
        {
            std::cout << "Error finding valid framerate for recording camera settings." << std::endl;
            return 1;
        }

        // validate imu
        if (recording_imu_enabled && !hasIMUDevice)
        {
            recording_imu_enabled = false;
            std::cout << "Warning: device " << device_index << " does not support IMU, so, IMU has been disabled."
                      << std::endl;
        }
    }
    else
    {
        return 1;
    }

    if (subordinate_delay_off_master_usec > 0 && wired_sync_mode != K4A_WIRED_SYNC_MODE_SUBORDINATE)
    {
        std::cerr << "--sync-delay is only valid if --external-sync is set to Subordinate." << std::endl;
        return 1;
    }

#if defined(_WIN32)
    SetConsoleCtrlHandler(
        [](DWORD event) {
            if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT)
            {
                signal_handler(0);
                return TRUE;
            }
            return FALSE;
        },
        true);
#else
    struct sigaction act;
    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);
#endif

    k4a_device_configuration_t device_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    device_config.color_format = recording_color_format;
    device_config.color_mode_id = recording_color_mode;
    device_config.depth_mode_id = recording_depth_mode;
    device_config.fps_mode_id = recording_fps_mode;
    device_config.wired_sync_mode = wired_sync_mode;
    device_config.depth_delay_off_color_usec = depth_delay_off_color_usec;
    device_config.subordinate_delay_off_master_usec = subordinate_delay_off_master_usec;

    return do_recording((uint8_t)device_index,
                        recording_filename,
                        recording_length,
                        &device_config,
                        recording_imu_enabled,
                        absoluteExposureValue,
                        gain);
}
