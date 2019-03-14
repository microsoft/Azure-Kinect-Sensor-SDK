// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>

#include <k4a/k4a.h>

#include <k4arecord/record.h>
#include <k4aexperiment/record_experiment.h>

#define VERIFY(result, error)                                                                                          \
    if (result != K4A_RESULT_SUCCEEDED)                                                                                \
    {                                                                                                                  \
        printf("%s \n - (File: %s, Function: %s, Line: %d)\n", error, __FILE__, __FUNCTION__, __LINE__);               \
        exit(1);                                                                                                       \
    }

struct BITMAPINFOHEADER
{
    uint32_t biSize = sizeof(BITMAPINFOHEADER);
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes = 1;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter = 0;
    uint32_t biYPelsPerMeter = 0;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "k4arecorder_custom_track output.mkv" << std::endl << std::endl;
        exit(0);
    }

    char *recording_filename = argv[1];

    k4a_device_configuration_t device_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    device_config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;

    k4a_device_t device;
    VERIFY(k4a_device_open(0, &device), "Open K4A Device failed!");
    VERIFY(k4a_device_start_cameras(device, &device_config), "Start K4A cameras failed!");

    std::cout << "Device started" << std::endl;

    k4a_record_t recording;
    // In order to test the custom track recording, we disable the default capture recording
    if (K4A_FAILED(k4a_record_create(recording_filename, device, K4A_DEVICE_CONFIG_INIT_DISABLE_ALL, &recording)))
    {
        std::cerr << "Unable to create recording file: " << recording_filename << std::endl;
        return 1;
    }

    BITMAPINFOHEADER depth_codec_header;
    depth_codec_header.biWidth = 640;
    depth_codec_header.biWidth = 576;
    depth_codec_header.biBitCount = 16;
    depth_codec_header.biCompression = 0x32595559; // YUY2 little endian
    depth_codec_header.biSizeImage = sizeof(uint16_t) * 640 * 576;

    k4a_record_video_info_t depth_video_info;
    depth_video_info.width = 640;
    depth_video_info.height = 576;
    depth_video_info.frame_rate = 30;

    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    result = k4a_record_add_custom_track(recording,
                                         "DEPTH",
                                         K4A_RECORD_TRACK_TYPE_VIDEO,
                                         "V_MS/VFW/FOURCC",
                                         reinterpret_cast<uint8_t *>(&depth_codec_header),
                                         sizeof(depth_codec_header),
                                         &depth_video_info);
    VERIFY(result, "Add Depth custom track failed!");

    result = k4a_record_add_custom_track(recording,
                                         "IR",
                                         K4A_RECORD_TRACK_TYPE_VIDEO,
                                         "V_MS/VFW/FOURCC",
                                         reinterpret_cast<uint8_t *>(&depth_codec_header),
                                         sizeof(depth_codec_header),
                                         &depth_video_info);
    VERIFY(result, "Add IR custom track failed!");

    result = k4a_record_add_custom_track_tag(recording, "DEPTH", "K4A_DEPTH_MODE", "NFOV_UNBINNED");
    VERIFY(result, "Add custom track tag failed!");

    VERIFY(k4a_record_write_header(recording), "K4A Write Header Failed");

    int frame_count = 0;
    do
    {
        k4a_capture_t capture;
        k4a_wait_result_t get_capture_result = k4a_device_get_capture(device, &capture, K4A_WAIT_INFINITE);
        if (get_capture_result == K4A_WAIT_RESULT_SUCCEEDED)
        {
            frame_count++;

            printf("Start processing frame %d\n", frame_count);

            k4a_image_t depth_image = k4a_capture_get_depth_image(capture);
            k4a_image_t ir_image = k4a_capture_get_ir_image(capture);

            result = k4a_record_write_custom_track_data(recording,
                                                        "DEPTH",
                                                        k4a_image_get_timestamp_usec(depth_image) * 1000,
                                                        k4a_image_get_buffer(depth_image),
                                                        static_cast<uint32_t>(k4a_image_get_size(depth_image)));
            VERIFY(result, "Write DEPTH custom track data failed!");

            result = k4a_record_write_custom_track_data(recording,
                                                        "IR",
                                                        k4a_image_get_timestamp_usec(ir_image) * 1000,
                                                        k4a_image_get_buffer(ir_image),
                                                        static_cast<uint32_t>(k4a_image_get_size(ir_image)));
            VERIFY(result, "Write IR custom track data failed!");

            k4a_image_release(depth_image);
            k4a_image_release(ir_image);
            k4a_capture_release(capture);
        }
        else if (get_capture_result == K4A_WAIT_RESULT_TIMEOUT)
        {
            // It should never hit time out when K4A_WAIT_INFINITE is set.
            printf("Error! Get depth frame time out!\n");
            break;
        }
        else
        {
            printf("Get depth capture returned error: %d\n", get_capture_result);
            break;
        }

    } while (frame_count < 100);

    printf("Finished body tracking processing!\n");

    k4a_device_stop_cameras(device);


    std::cout << "Saving recording..." << std::endl;
    VERIFY(k4a_record_flush(recording), "Flush recording failed!");
    k4a_record_close(recording);

    std::cout << "Done" << std::endl;
    k4a_device_close(device);

    return 0;
}
