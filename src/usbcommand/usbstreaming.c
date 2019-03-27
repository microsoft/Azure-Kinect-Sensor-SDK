// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
// This library
#include <k4ainternal/usbcommand.h>
#include "usb_cmd_priv.h"

// System dependencies
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <azure_c_shared_utility/envvariable.h>

//**************Symbolic Constant Macros (defines)  *************
#define USB_CMD_LIBUSB_EVENT_TIMEOUT 1

//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************

//*********************** Functions *****************************
/**
 *  Utility function for releasing the transfer resources
 *
 *  @param p_bulk_transfer
 *   Pointer to the resources allocated for doing the usb transfer
 *
 */
static void usb_cmd_release_xfr(struct libusb_transfer *p_bulk_transfer)
{
    usbcmd_context_t *usbcmd = (usbcmd_context_t *)(p_bulk_transfer->user_data);

    for (uint32_t i = 0; i < USB_CMD_MAX_XFR_COUNT; i++)
    {
        if (usbcmd->p_bulk_transfer[i] == p_bulk_transfer)
        {
            usbcmd->p_bulk_transfer[i] = NULL;
            // dereference allocator handle if one was allocated
            if (usbcmd->image[i] != NULL)
            {
                image_dec_ref(usbcmd->image[i]);
                usbcmd->image[i] = NULL;
            }

            break;
        }
    }

    // free the allocated resources
    libusb_free_transfer(p_bulk_transfer);
}
/**
 *  Function for handling the callback from the libusb library as a result of a transfer request
 *
 *  @param p_bulk_transfer
 *   Pointer to the resources allocated for doing the usb transfer
 *
 */
void LIBUSB_CALL usb_cmd_libusb_cb(struct libusb_transfer *p_bulk_transfer)
{
    usbcmd_context_t *usbcmd = (usbcmd_context_t *)(p_bulk_transfer->user_data);
    k4a_result_t result = K4A_RESULT_FAILED;
    uint8_t image_index = 0;

    // Get index to allocated handle
    for (image_index = 0; image_index < USB_CMD_MAX_XFR_COUNT; image_index++)
    {
        if (usbcmd->p_bulk_transfer[image_index] == p_bulk_transfer)
        {
            break;
        }
    }

    assert(image_index < USB_CMD_MAX_XFR_COUNT);

    if (((p_bulk_transfer->status == LIBUSB_TRANSFER_COMPLETED) ||
         p_bulk_transfer->status == LIBUSB_TRANSFER_TIMED_OUT) &&
        (usbcmd->stream_going) && (image_index < USB_CMD_MAX_XFR_COUNT))
    {
        // if callback provided, callback with associated information
        if ((p_bulk_transfer->status == LIBUSB_TRANSFER_COMPLETED) && (usbcmd->callback != NULL))
        {
            image_set_size(usbcmd->image[image_index], (size_t)p_bulk_transfer->actual_length);
            usbcmd->callback(K4A_RESULT_SUCCEEDED, usbcmd->image[image_index], usbcmd->stream_context);
        }
        else
        {
            LOG_WARNING("USB timeout on streaming endpoint for %s",
                        usbcmd->interface == USB_CMD_DEPTH_INTERFACE ? "depth" : "imu");
        }

        // We guarantee the capture is valid during the callback if someone wants it to survive longer then they
        // need to add a ref
        image_dec_ref(usbcmd->image[image_index]);
        usbcmd->image[image_index] = NULL;

        // allocate next buffer and re-use transfer
        result = TRACE_CALL(
            image_create_empty_internal(usbcmd->source, usbcmd->stream_size, &usbcmd->image[image_index]));
        if (K4A_SUCCEEDED(result))
        {
            int err = LIBUSB_ERROR_OTHER;
            libusb_fill_bulk_transfer(p_bulk_transfer,
                                      usbcmd->libusb,
                                      usbcmd->stream_endpoint,
                                      image_get_buffer(usbcmd->image[image_index]),
                                      (int)usbcmd->stream_size,
                                      usb_cmd_libusb_cb,
                                      usbcmd,
                                      USB_CMD_MAX_WAIT_TIME);
            if ((err = libusb_submit_transfer(p_bulk_transfer)) != LIBUSB_SUCCESS)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR("Error calling libusb_submit_transfer for tx, result:%s", libusb_error_name(err));
                image_dec_ref(usbcmd->image[image_index]);
                usbcmd->image[image_index] = NULL;
            }
        }
    }
    if (K4A_FAILED(result))
    {
        if (usbcmd->stream_going && (p_bulk_transfer->status != LIBUSB_TRANSFER_CANCELLED) &&
            (p_bulk_transfer->status != LIBUSB_TRANSFER_OVERFLOW))
        {
            // Note: The overflow happens when the thread tries to submit the next transfer and the kernel doesn't
            // have the space for it. This is where the adaptive detection mechanism takes place. The adaptive
            // method submits until it gets an error from the submit call. The error produces the libusb_transfer_
            // overflow error which shows up in the callback. It's ignored since it is expected behavior during
            // the submission process and there are other trace messages that record the event.
            LOG_ERROR("Error LIBUSB transfer failed, result:%s", libusb_error_name((int)p_bulk_transfer->status));

            // check if the error state can be propagated
            if ((image_index < USB_CMD_MAX_XFR_COUNT) && (usbcmd->callback != NULL))
            {
                image_set_size(usbcmd->image[image_index], (size_t)0);
                usbcmd->callback(K4A_RESULT_FAILED, usbcmd->image[image_index], usbcmd->stream_context);
            }
        }
        // release resource for phy related changes or transfer stopped
        usb_cmd_release_xfr(p_bulk_transfer);
    }
}

/**
 *  LibUsb context thread for monitoring events in the usb lib
 *
 *  @param var
 *   context variable.  In this case, this points to a command handle
 *   associated with the streaming.
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 *
 */
static int usb_cmd_lib_usb_thread(void *var)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    usbcmd_context_t *usbcmd = (usbcmd_context_t *)var;
    libusb_context *p_ctx = usbcmd->libusb_context;
    int err = LIBUSB_SUCCESS;
    struct timeval tv = { 0 };
    size_t xfer_pool = usbcmd->stream_size;
    size_t max_xfr_pool = USB_CMD_MAX_XFR_POOL;

    // override the xfr pool if the environment variable is defined
    const char *env_max_pool = environment_get_variable("K4A_MAX_LIBUSB_POOL");
    if (env_max_pool != NULL && env_max_pool[0] != '\0')
    {
        max_xfr_pool = (size_t)strtol(env_max_pool, NULL, 10);
    }

    tv.tv_sec = USB_CMD_LIBUSB_EVENT_TIMEOUT;

    if (usbcmd->stream_size > INT32_MAX)
    {
        result = K4A_RESULT_FAILED;
    }
    else
    {
        // set up the transfers.  Limit the overall amount of resources to a predefined amount
        for (uint32_t i = 0; (i < USB_CMD_MAX_XFR_COUNT) && (xfer_pool < max_xfr_pool); i++)
        {
            xfer_pool += usbcmd->stream_size;
            usbcmd->p_bulk_transfer[i] = libusb_alloc_transfer(0);
            if (usbcmd->p_bulk_transfer[i] == NULL)
            {
                LOG_ERROR("libusb transfer could not be allocated", 0);
                result = K4A_RESULT_FAILED;
                break;
            }

            result = TRACE_CALL(image_create_empty_internal(usbcmd->source, usbcmd->stream_size, &usbcmd->image[i]));

            if (K4A_FAILED(result))
            {
                LOG_ERROR("stream buffer could not be allocated", 0);
                result = K4A_RESULT_FAILED;
                break;
            }

            libusb_fill_bulk_transfer(usbcmd->p_bulk_transfer[i],
                                      usbcmd->libusb,
                                      usbcmd->stream_endpoint,
                                      image_get_buffer(usbcmd->image[i]),
                                      (int)usbcmd->stream_size,
                                      usb_cmd_libusb_cb,
                                      usbcmd,
                                      USB_CMD_MAX_WAIT_TIME);

            if ((err = libusb_submit_transfer(usbcmd->p_bulk_transfer[i])) != LIBUSB_SUCCESS)
            {
                if (i == 0)
                {
                    // Could not even submit one.  This is an error
                    LOG_ERROR("No libusb transfers could not be submitted, error:%s", libusb_error_name(err));
                    result = K4A_RESULT_FAILED;
                }
                else
                {
                    // Could not allocate a transfer within the predefined amount.
                    // This could indicate other resource are competing and the allocation
                    // pool needs to be adjusted
                    LOG_WARNING("Less than optimal %d libusb transfers submitted. Please evaluate available resources",
                                i + 1);
                }

                image_dec_ref(usbcmd->image[i]);
                usbcmd->image[i] = NULL;

                // dealloc transfer
                libusb_free_transfer(usbcmd->p_bulk_transfer[i]);
                usbcmd->p_bulk_transfer[i] = NULL;
                break;
            }
        }
    }

    // loop servicing libusb
    if (result == K4A_RESULT_SUCCEEDED)
    {
        while (usbcmd->stream_going)
        {
            if ((err = libusb_handle_events_timeout_completed(p_ctx, &tv, NULL)) < 0)
            {
                usbcmd->stream_going = false; // Close stream if error is detected
                LOG_ERROR("Error calling libusb_handle_events_timeout failed, result:%s", libusb_error_name(err));
                result = K4A_RESULT_FAILED;
            }
        }
    }

    // cancel everything just in case of errors
    for (uint32_t i = 0; i < USB_CMD_MAX_XFR_COUNT; i++)
    {
        if (usbcmd->p_bulk_transfer[i] != NULL)
        {
            // Cancel any outstanding transfer
            libusb_cancel_transfer(usbcmd->p_bulk_transfer[i]);
            // Service the library after  cancellation
            if ((err = libusb_handle_events_timeout_completed(p_ctx, &tv, NULL)) < 0)
            {
                LOG_ERROR("Error calling libusb_handle_events_timeout failed, result:%s", libusb_error_name(err));
                result = K4A_RESULT_FAILED;
            }
        }
    }

    ThreadAPI_Exit((int)result);
    return 0;
}

/**
 *  Function to queue up the stream transfer.  This function will allocation
 *  USB_CMD_MAX_XFR_COUNT number of transfers on the stream pipe and
 *  start the transfers.
 *
 *  @param usbcmd_handle
 *   Handle to the entry that will be passed into the usb library as a context
 *
 *  @param payload_size
 *   Max size of the payload that will be streamed
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed or stream already started
 *
 */
k4a_result_t usb_cmd_stream_start(usbcmd_t usbcmd_handle, size_t payload_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, usbcmd_t, usbcmd_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, payload_size == 0);

    usbcmd_context_t *usbcmd;
    k4a_result_t result;

    result = K4A_RESULT_FROM_BOOL((usbcmd = usbcmd_t_get_context(usbcmd_handle)) != NULL);

    // start stream handler
    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FAILED;

        // Sync operation with commands going to device
        Lock(usbcmd->lock);
        if (usbcmd->stream_going)
        {
            // Steam already going (Error?)
            LOG_INFO("Stream already in progress", 0);
        }
        else
        {
            usbcmd->stream_size = payload_size;
            usbcmd->stream_going = true;
            if (ThreadAPI_Create(&(usbcmd->stream_handle), usb_cmd_lib_usb_thread, usbcmd) != THREADAPI_OK)
            {
                usbcmd->stream_going = false;
                LOG_ERROR("Could not start stream thread", 0);
            }
            else
            {
                result = K4A_RESULT_SUCCEEDED;
            }
        }
        Unlock(usbcmd->lock);
    }

    return result;
}

/**
 *  Function for stopping the streaming on a handle. This function
 *  will block until the stream is stopped.  It is called implicitly
 *  by the usb_cmd_destroy() function
 *
 *  @param usbcmd_handle
 *   Handle that contains the transfer resources used.
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 *
 */
k4a_result_t usb_cmd_stream_stop(usbcmd_t usbcmd_handle)
{
    k4a_result_t result;
    usbcmd_context_t *usbcmd;

    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, usbcmd_t, usbcmd_handle)

    result = K4A_RESULT_FROM_BOOL((usbcmd = usbcmd_t_get_context(usbcmd_handle)) != NULL);

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FAILED;

        // Sync operation with commands going to device
        Lock(usbcmd->lock);
        usbcmd->stream_going = false;

        // This function is the only place that kills the thread so this should be safe
        if (usbcmd->stream_handle != NULL) // check if the thread has already stopped
        {
            ThreadAPI_Join(usbcmd->stream_handle, NULL);
            usbcmd->stream_handle = NULL;
        }
        Unlock(usbcmd->lock);

        result = K4A_RESULT_SUCCEEDED;
    }

    return result;
}
