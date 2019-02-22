// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>

#include <k4a/k4a.h>

k4a_wait_result_t get_most_recent_capture(k4a_device_t device_handle, k4a_capture_t *capture_out);

/** For a given device get_most_recent_capture() will read captures from the K4A SDK to get the freshest and most recent
 * capture. This is performed in a way to does not block the callers thread and ignores any data that may have queued up
 * while the caller was performing other synchronous operations and could not service the K4A api's to get more frames.
 *
 * \param device_handle [IN]
 * Handle obtained by k4a_device_open().
 *
 * \param capture_out [OUT]
 * A capture from the device
 *
 * \returns
 * ::K4A_WAIT_RESULT_SUCCEEDED if capture_out has a new capture. ::K4A_WAIT_RESULT_TIMEOUT is no data was present.
 * ::K4A_WAIT_RESULT_FAILED if an unexpected error was encountered.
 */
k4a_wait_result_t get_most_recent_capture(k4a_device_t device_handle, k4a_capture_t *capture_out)
{
    k4a_capture_t saved_capture = NULL;
    k4a_capture_t capture = NULL;
    k4a_wait_result_t result;

    // Loop calling k4a_device_get_capture until we don't get a capture, then return the previous capture.
    // Use a timeout of zero so this will never block.
    do
    {
        if (saved_capture)
        {
            k4a_capture_release(saved_capture);
        }
        saved_capture = capture;
        result = k4a_device_get_capture(device_handle, &capture, 0);
    } while (result == K4A_WAIT_RESULT_SUCCEEDED);

    if (result == K4A_WAIT_RESULT_SUCCEEDED || result == K4A_WAIT_RESULT_TIMEOUT)
    {
        if (saved_capture)
        {
            *capture_out = saved_capture;
            result = K4A_WAIT_RESULT_SUCCEEDED;
        }
    }
    else if (saved_capture)
    {
        k4a_capture_release(saved_capture);
    }
    return result;
}

int main()
{
    k4a_capture_t capture;
    get_most_recent_capture(NULL, &capture);
    return 0;
}
