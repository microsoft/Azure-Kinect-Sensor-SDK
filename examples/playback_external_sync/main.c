// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <malloc.h>
#include <k4a/k4a.h>
#include <k4arecord/playback.h>

typedef struct
{
    char *filename;
    k4a_playback_t handle;
    k4a_record_configuration_t record_config;
    k4a_capture_t capture;
} recording_t;

static uint64_t first_capture_timestamp(k4a_capture_t capture)
{
    uint64_t min_timestamp = (uint64_t)-1;
    k4a_image_t images[3];
    images[0] = k4a_capture_get_color_image(capture);
    images[1] = k4a_capture_get_depth_image(capture);
    images[2] = k4a_capture_get_ir_image(capture);

    for (int i = 0; i < 3; i++)
    {
        if (images[i] != NULL)
        {
            uint64_t timestamp = k4a_image_get_device_timestamp_usec(images[i]);
            if (timestamp < min_timestamp)
            {
                min_timestamp = timestamp;
            }
            k4a_image_release(images[i]);
            images[i] = NULL;
        }
    }

    return min_timestamp;
}

static void print_capture_info(recording_t *file)
{
    k4a_image_t images[3];
    images[0] = k4a_capture_get_color_image(file->capture);
    images[1] = k4a_capture_get_depth_image(file->capture);
    images[2] = k4a_capture_get_ir_image(file->capture);

    printf("%-32s", file->filename);
    for (int i = 0; i < 3; i++)
    {
        if (images[i] != NULL)
        {
            uint64_t timestamp = k4a_image_get_device_timestamp_usec(images[i]);
            printf("  %7ju usec", timestamp);
            k4a_image_release(images[i]);
            images[i] = NULL;
        }
        else
        {
            printf("  %12s", "");
        }
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: playback_external_sync.exe <master.mkv> <sub1.mkv>...\n");
        return 1;
    }

    size_t file_count = (size_t)(argc - 1);
    bool master_found = false;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    // Allocate memory to store the state of N recordings.
    recording_t *files = malloc(sizeof(recording_t) * file_count);
    if (files == NULL)
    {
        printf("Failed to allocate memory for playback (%zu bytes)\n", sizeof(recording_t) * file_count);
        return 1;
    }
    memset(files, 0, sizeof(recording_t) * file_count);

    // Open each recording file and validate they were recorded in master/subordinate mode.
    for (size_t i = 0; i < file_count; i++)
    {
        files[i].filename = argv[i + 1];

        result = k4a_playback_open(files[i].filename, &files[i].handle);
        if (result != K4A_RESULT_SUCCEEDED)
        {
            printf("Failed to open file: %s\n", files[i].filename);
            break;
        }

        result = k4a_playback_get_record_configuration(files[i].handle, &files[i].record_config);
        if (result != K4A_RESULT_SUCCEEDED)
        {
            printf("Failed to get record configuration for file: %s\n", files[i].filename);
            break;
        }

        if (files[i].record_config.wired_sync_mode == K4A_WIRED_SYNC_MODE_MASTER)
        {
            printf("Opened master recording file: %s\n", files[i].filename);
            if (master_found)
            {
                printf("ERROR: Multiple master recordings listed!\n");
                result = K4A_RESULT_FAILED;
                break;
            }
            else
            {
                master_found = true;
            }
        }
        else if (files[i].record_config.wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE)
        {
            printf("Opened subordinate recording file: %s\n", files[i].filename);
        }
        else
        {
            printf("ERROR: Recording file was not recorded in master/sub mode: %s\n", files[i].filename);
            result = K4A_RESULT_FAILED;
            break;
        }

        // Read the first capture of each recording into memory.
        k4a_stream_result_t stream_result = k4a_playback_get_next_capture(files[i].handle, &files[i].capture);
        if (stream_result == K4A_STREAM_RESULT_EOF)
        {
            printf("ERROR: Recording file is empty: %s\n", files[i].filename);
            result = K4A_RESULT_FAILED;
            break;
        }
        else if (stream_result == K4A_STREAM_RESULT_FAILED)
        {
            printf("ERROR: Failed to read first capture from file: %s\n", files[i].filename);
            result = K4A_RESULT_FAILED;
            break;
        }
    }

    if (result == K4A_RESULT_SUCCEEDED)
    {
        printf("%-32s  %12s  %12s  %12s\n", "Source file", "COLOR", "DEPTH", "IR");
        printf("==========================================================================\n");

        // Print the first 25 captures in order of timestamp across all the recordings.
        for (int frame = 0; frame < 25; frame++)
        {
            uint64_t min_timestamp = (uint64_t)-1;
            recording_t *min_file = NULL;

            // Find the lowest timestamp out of each of the current captures.
            for (size_t i = 0; i < file_count; i++)
            {
                if (files[i].capture != NULL)
                {
                    uint64_t timestamp = first_capture_timestamp(files[i].capture);
                    if (timestamp < min_timestamp)
                    {
                        min_timestamp = timestamp;
                        min_file = &files[i];
                    }
                }
            }

            print_capture_info(min_file);

            k4a_capture_release(min_file->capture);
            min_file->capture = NULL;

            // Advance the recording with the lowest current timestamp forward.
            k4a_stream_result_t stream_result = k4a_playback_get_next_capture(min_file->handle, &min_file->capture);
            if (stream_result == K4A_STREAM_RESULT_FAILED)
            {
                printf("ERROR: Failed to read next capture from file: %s\n", min_file->filename);
                result = K4A_RESULT_FAILED;
                break;
            }
        }
    }

    for (size_t i = 0; i < file_count; i++)
    {
        if (files[i].handle != NULL)
        {
            k4a_playback_close(files[i].handle);
            files[i].handle = NULL;
        }
    }
    free(files);
    return result == K4A_RESULT_SUCCEEDED ? 0 : 1;
}
