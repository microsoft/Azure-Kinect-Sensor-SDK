// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include "Capture.h"
#include "Cli.h"
#include "Example.h"
#include <k4a/k4a.h>
#include "UsbCmd.h"
#include "k4aCmd.h"
#include <k4ainternal/logging.h>

#include "Main.h"
//**************Symbolic Constant Macros (defines)  *************
#define MAX_SUPPORTED_DEVICES 4

//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************
static k4a_device_t k4a_handle[MAX_SUPPORTED_DEVICES];
static uint8_t opened_count = 0;

extern "C" {
// dynlib includes the logger
char K4A_ENV_VAR_LOG_TO_A_FILE[] = K4A_ENABLE_LOG_TO_A_FILE;
}

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

/**
 *  Initialize the K4A library and set up global data
 *
 */
void open_k4a(void)
{
    uint32_t device_count;

    // Determine how many devices are available
    device_count = k4a_device_get_installed_count();
    if (device_count > MAX_SUPPORTED_DEVICES)
    {
        device_count = MAX_SUPPORTED_DEVICES;
        printf("Warning, number of attached devices exceed %d.  Some devices will not be accessible\n",
               MAX_SUPPORTED_DEVICES);
    }
    for (uint8_t i = 0; i < device_count; i++)
    {
        if (k4a_device_open(i, &k4a_handle[opened_count]) == K4A_RESULT_SUCCEEDED)
        {
            opened_count++;
        }
        else
        {
            printf("Device %d could not be opened\n", i);
        }
    }
}

/**
 *  Shutdown K4A library
 *
 */
void close_k4a(void)
{
    for (uint8_t i = 0; i < opened_count; i++)
    {
        if (k4a_handle[i] != NULL)
        {
            k4a_device_close(k4a_handle[i]);
            k4a_handle[i] = NULL;
        }
    }
    opened_count = 0;
}

/**
 *  Get K4A handled based on index
 *
 *  @return
 *   k4a_device_t handle to K4A device
 *   NULL                handle could not be found
 *
 */
k4a_device_t get_k4a_handle(uint32_t index)
{
    if (index < opened_count)
    {
        return k4a_handle[index];
    }
    else
    {
        return NULL;
    }
}

/**
 *  Main entry point
 *
 *  @param Argc  Number of variables handed in
 *
 *  @param Argv  Pointer to variable array
 *
 *  @return
 *   CLI_SUCCESS     Everything executed correctly
 *   CLI_ERROR       Error occurred while handling the command
 *
 */
int main(int Argc, char **Argv)
{
    int status;

    // Initialize the K4A library
    open_k4a();

    // Initialize CLI Modules. For each functional test file, add an initialization routine
    ExampleInit();

    // K4A SDK commands
    k4a_cmd_init();

    // USB Command module
    usb_cmd_init();

    // Capture related commands
    capture_init();

    // process command
    status = CliExecute(Argc - 1, &Argv[1]);

    // close the K4A library
    close_k4a();

    return status;
}
