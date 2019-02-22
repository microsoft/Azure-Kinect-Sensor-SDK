// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <k4a/k4a.h>

int main()
{
    k4a_device_t device = NULL;

    uint32_t device_count = k4a_device_get_installed_count();
    printf("Found %d connected devices:\n", device_count);
    for (uint8_t deviceIndex = 0; deviceIndex < device_count; deviceIndex++)
    {
        if (K4A_RESULT_SUCCEEDED != k4a_device_open(deviceIndex, &device))
        {
            printf("%d: Failed to open device\n", deviceIndex);
        }

        char *serial_number = NULL;
        size_t serial_number_length = 0;

        if (K4A_BUFFER_RESULT_TOO_SMALL != k4a_device_get_serialnum(device, NULL, &serial_number_length))
        {
            printf("%d: Failed to get serial number length\n", deviceIndex);
            k4a_device_close(device);
            device = NULL;
            continue;
        }

        serial_number = malloc(serial_number_length);
        if (serial_number == NULL)
        {
            printf("%d: Failed to allocate memory for serial number (%zu bytes)\n", deviceIndex, serial_number_length);
            k4a_device_close(device);
            device = NULL;
            continue;
        }

        if (K4A_BUFFER_RESULT_SUCCEEDED != k4a_device_get_serialnum(device, serial_number, &serial_number_length))
        {
            printf("%d: Failed to get serial number\n", deviceIndex);
            free(serial_number);
            serial_number = NULL;
            k4a_device_close(device);
            device = NULL;
            continue;
        }

        printf("%d: Device \"%s\"\n", deviceIndex, serial_number);

        free(serial_number);
        serial_number = NULL;
        k4a_device_close(device);
        device = NULL;
    }

    return (int)0;
}
