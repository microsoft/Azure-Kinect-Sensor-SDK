// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <k4a/k4a.h>
#include <k4arecord/record.h>

#define VERIFY(result)                                                                                                 \
    if (result != K4A_RESULT_SUCCEEDED)                                                                                \
    {                                                                                                                  \
        printf("%s \n - (File: %s, Function: %s, Line: %d)\n", #result " failed", __FILE__, __FUNCTION__, __LINE__);   \
        exit(1);                                                                                                       \
    }

#define FOURCC(cc) ((cc)[0] | (cc)[1] << 8 | (cc)[2] << 16 | (cc)[3] << 24)

// Codec context struct for Codec ID: "V_MS/VFW/FOURCC"
// See https://docs.microsoft.com/en-us/windows/desktop/wmdm/-bitmapinfoheader
typedef struct
{
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;

void fill_bitmap_header(uint32_t width, uint32_t height, BITMAPINFOHEADER *out);
void fill_bitmap_header(uint32_t width, uint32_t height, BITMAPINFOHEADER *out)
{
    out->biSize = sizeof(BITMAPINFOHEADER);
    out->biWidth = width;
    out->biHeight = height;
    out->biPlanes = 1;
    out->biBitCount = 16;
    out->biCompression = FOURCC("YUY2");
    out->biSizeImage = sizeof(uint16_t) * width * height;
    out->biXPelsPerMeter = 0;
    out->biYPelsPerMeter = 0;
    out->biClrUsed = 0;
    out->biClrImportant = 0;
}

static k4a_result_t
get_device_mode_ids(k4a_device_t device, uint32_t *color_mode_id, uint32_t *depth_mode_id, uint32_t *fps_mode_id)
{
    // 1. get available modes from device info
    k4a_device_info_t device_info = { sizeof(k4a_device_info_t), K4A_ABI_VERSION, 0 };
    if (!k4a_device_get_info(device, &device_info) == K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to get device info");
        return K4A_RESULT_FAILED;
    }

    bool hasDepthDevice = (device_info.capabilities.bitmap.bHasDepth == 1);
    bool hasColorDevice = (device_info.capabilities.bitmap.bHasColor == 1);

    // 2. declare mode infos
    k4a_color_mode_info_t color_mode_info = { sizeof(k4a_color_mode_info_t), K4A_ABI_VERSION, 0 };
    k4a_depth_mode_info_t depth_mode_info = { sizeof(k4a_depth_mode_info_t), K4A_ABI_VERSION, 0 };
    k4a_fps_mode_info_t fps_mode_info = { sizeof(k4a_fps_mode_info_t), K4A_ABI_VERSION, 0 };

    // 3. get the count of modes
    uint32_t color_mode_count = 0;
    uint32_t depth_mode_count = 0;
    uint32_t fps_mode_count = 0;

    if (hasColorDevice && !K4A_SUCCEEDED(k4a_device_get_color_mode_count(device, &color_mode_count)))
    {
        printf("Failed to get color mode count\n");
        return K4A_RESULT_FAILED;
    }

    if (hasDepthDevice && !K4A_SUCCEEDED(k4a_device_get_depth_mode_count(device, &depth_mode_count)))
    {
        printf("Failed to get depth mode count\n");
        return K4A_RESULT_FAILED;
    }

    if (!k4a_device_get_fps_mode_count(device, &fps_mode_count) == K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to get fps mode count\n");
        return K4A_RESULT_FAILED;
    }

    // 4. find the mode ids you want
    if (hasColorDevice && color_mode_count > 1)
    {
        for (uint32_t c = 1; c < color_mode_count; c++)
        {
            if (k4a_device_get_color_mode(device, c, &color_mode_info) == K4A_RESULT_SUCCEEDED)
            {
                if (color_mode_info.mode_id == 0)
                {
                    *color_mode_id = c;
                    break;
                }
            }
        }
    }

    if (hasDepthDevice && depth_mode_count > 1)
    {
        for (uint32_t d = 1; d < depth_mode_count; d++)
        {
            if (k4a_device_get_depth_mode(device, d, &depth_mode_info) == K4A_RESULT_SUCCEEDED)
            {
                if (depth_mode_info.height <= 288 && depth_mode_info.vertical_fov <= 65)
                {
                    *depth_mode_id = d;
                    break;
                }
            }
        }
    }

    if (fps_mode_count > 1)
    {
        uint32_t max_fps = 0;
        for (uint32_t f = 1; f < fps_mode_count; f++)
        {
            if (k4a_device_get_fps_mode(device, f, &fps_mode_info) == K4A_RESULT_SUCCEEDED)
            {
                if (fps_mode_info.fps >= (int)max_fps)
                {
                    max_fps = (uint32_t)fps_mode_info.fps;
                    *fps_mode_id = f;
                }
            }
        }
    }

    // 5. fps mode id must not be set to 0, which is Off, and either color mode id or depth mode id must not be set to 0
    if (*fps_mode_id == 0)
    {
        printf("Fps mode id must not be set to 0 (Off)\n");
        return K4A_RESULT_FAILED;
    }

    if (*color_mode_id == 0 && *depth_mode_id == 0)
    {
        printf("Either color mode id or depth mode id must not be set to 0 (Off)\n");
        return K4A_RESULT_FAILED;
    }

    return K4A_RESULT_SUCCEEDED;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: k4arecord_custom_track output.mkv\n");
        exit(0);
    }

    char *recording_filename = argv[1];

    k4a_device_t device;
    VERIFY(k4a_device_open(0, &device));

    uint32_t color_mode_id = 0;
    uint32_t depth_mode_id = 0;
    uint32_t fps_mode_id = 0;

    if (!K4A_SUCCEEDED(get_device_mode_ids(device, &color_mode_id, &depth_mode_id, &fps_mode_id)))
    {
        printf("Failed to get device mode ids\n");
        exit(-1);
    }

    k4a_device_configuration_t device_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    device_config.depth_mode_id = depth_mode_id;
    device_config.fps_mode_id = fps_mode_id;

    VERIFY(k4a_device_start_cameras(device, &device_config));

    printf("Device started\n");

    k4a_record_t recording;
    if (K4A_FAILED(k4a_record_create(recording_filename, device, device_config, &recording)))
    {
        printf("Unable to create recording file: %s\n", recording_filename);
        return 1;
    }

    // Add a hello_world.txt attachment to the recording
    const char *attachment_data = "Hello, World!\n";
    VERIFY(k4a_record_add_attachment(recording,
                                     "hello_world.txt",
                                     (const uint8_t *)attachment_data,
                                     strlen(attachment_data)));

    // Add a custom recording tag
    VERIFY(k4a_record_add_tag(recording, "CUSTOM_TAG", "Hello, World!"));

    // Add a custom video track to store processed depth images.
    // Read the depth resolution from the camera configuration so we can create our custom track with the same size.
    k4a_calibration_t sensor_calibration;
    VERIFY(k4a_device_get_calibration(device, depth_mode_id, color_mode_id, &sensor_calibration));
    uint32_t depth_width = (uint32_t)sensor_calibration.depth_camera_calibration.resolution_width;
    uint32_t depth_height = (uint32_t)sensor_calibration.depth_camera_calibration.resolution_height;

    BITMAPINFOHEADER codec_header;
    fill_bitmap_header(depth_width, depth_height, &codec_header);

    k4a_record_video_settings_t video_settings;
    video_settings.width = depth_width;
    video_settings.height = depth_height;
    video_settings.frame_rate = 30; // Should be the same rate as device_config.camera_fps

    // Add the video track to the recording.
    VERIFY(k4a_record_add_custom_video_track(recording,
                                             "PROCESSED_DEPTH",
                                             "V_MS/VFW/FOURCC",
                                             (uint8_t *)(&codec_header),
                                             sizeof(codec_header),
                                             &video_settings));

    // Write the recording header now that all the track metadata is set up.
    VERIFY(k4a_record_write_header(recording));

    // Start reading 100 depth frames (~3 seconds at 30 fps) from the camera.
    for (int frame = 0; frame < 100; frame++)
    {
        k4a_capture_t capture;
        k4a_wait_result_t get_capture_result = k4a_device_get_capture(device, &capture, K4A_WAIT_INFINITE);
        if (get_capture_result == K4A_WAIT_RESULT_SUCCEEDED)
        {
            // Write the capture to the built-in tracks
            VERIFY(k4a_record_write_capture(recording, capture));

            // Get the depth image from the capture so we can write a processed copy to our custom track.
            k4a_image_t depth_image = k4a_capture_get_depth_image(capture);
            if (depth_image)
            {
                // The YUY2 image format is the same stride as the 16-bit depth image, so we can modify it in-place.
                uint8_t *depth_buffer = k4a_image_get_buffer(depth_image);
                size_t depth_buffer_size = k4a_image_get_size(depth_image);
                for (size_t i = 0; i < depth_buffer_size; i += 2)
                {
                    // Convert the depth value (16-bit, in millimeters) to the YUY2 color format.
                    // The YUY2 format should be playable in video players such as VLC.
                    uint16_t depth = (uint16_t)(depth_buffer[i + 1] << 8 | depth_buffer[i]);
                    // Clamp the depth range to ~1 meter and scale it to fit in the Y channel of the image (8-bits).
                    if (depth > 0x3FF)
                    {
                        depth_buffer[i] = 0xFF;
                    }
                    else
                    {
                        depth_buffer[i] = (uint8_t)(depth >> 2);
                    }
                    // Set the U/V channel to 128 (i.e. grayscale).
                    depth_buffer[i + 1] = 128;
                }

                VERIFY(k4a_record_write_custom_track_data(recording,
                                                          "PROCESSED_DEPTH",
                                                          k4a_image_get_device_timestamp_usec(depth_image),
                                                          depth_buffer,
                                                          (uint32_t)depth_buffer_size));

                k4a_image_release(depth_image);
            }

            k4a_capture_release(capture);
        }
        else if (get_capture_result == K4A_WAIT_RESULT_TIMEOUT)
        {
            // TIMEOUT should never be returned when K4A_WAIT_INFINITE is set.
            printf("k4a_device_get_capture() timed out!\n");
            break;
        }
        else
        {
            printf("k4a_device_get_capture() returned error: %d\n", get_capture_result);
            break;
        }
    }

    k4a_device_stop_cameras(device);

    printf("Saving recording...\n");
    VERIFY(k4a_record_flush(recording));
    k4a_record_close(recording);

    printf("Done\n");
    k4a_device_close(device);

    return 0;
}