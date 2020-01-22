// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************

// This library
#include "usb_cmd_priv.h"

// System dependencies
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

// Ensure we have LIBUSB_API_VERSION defined if not defined by libusb.h
#ifndef LIBUSB_API_VERSION
#define LIBUSB_API_VERSION 0
#endif

FORCEINLINE k4a_result_t
TraceLibUsbError(int err, const char *szCall, const char *szFile, int line, const char *szFunction)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    if (err < 0)
    {
        // Example print:
        //  depth.cpp (86): allocator_create(&depth->allocator) returned ERROR_NOT_FOUND in depth_create

        LOG_ERROR("%s (%d): %s returned %s in %s ", szFile, line, szCall, libusb_error_name(err), szFunction);
        result = K4A_RESULT_FAILED;
    }

    return result;
}

#define K4A_RESULT_FROM_LIBUSB(_call_) TraceLibUsbError((_call_), #_call_, __FILE__, __LINE__, __func__)

//**************Symbolic Constant Macros (defines)  *************

//************************ Typedefs *****************************
typedef struct _usb_command_header_t
{
    uint32_t packet_type;
    uint32_t packet_transaction_id;
    uint32_t payload_size;
    uint32_t command;
    uint32_t reserved; // Must be zero
} usb_command_header_t;

typedef struct _usb_command_packet_t
{
    usb_command_header_t header;
    uint8_t data[USB_MAX_TX_DATA];
} usb_command_packet_t;

/////////////////////////////////////////////////////
// This is the response structure going to the host.
/////////////////////////////////////////////////////

typedef struct _usb_command_response_t
{
    uint32_t packet_type;
    uint32_t packet_transaction_id;
    uint32_t status;
    uint32_t reserved; // Will be zero
} usb_command_response_t;

typedef struct _descriptor_choice_t
{
    uint16_t pid;
    uint8_t interface;
    uint8_t cmd_tx_endpoint;
    uint8_t cmd_rx_endpoint;
    uint8_t stream_endpoint;
    usbcmd_context_t **handle_list;
} descriptor_choice_t;

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

#define UUID_STR_LENGTH sizeof("{00000000-0000-0000-0000-000000000000}")
static void uuid_to_string(const guid_t *guid, char *string, size_t string_size)
{
    snprintf(string,
             string_size,
             "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             guid->id[3],
             guid->id[2],
             guid->id[1],
             guid->id[0],
             guid->id[5],
             guid->id[4],
             guid->id[7],
             guid->id[6],
             guid->id[9],
             guid->id[8],
             guid->id[10],
             guid->id[11],
             guid->id[12],
             guid->id[13],
             guid->id[14],
             guid->id[15]);
}

// scale the libusb debug verbosity to match k4a
static k4a_result_t usb_cmd_set_libusb_debug_verbosity(usbcmd_context_t *usbcmd)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    libusb_context *libusb_ctx = usbcmd ? usbcmd->libusb_context : NULL;

    // #if (LIBUSB_API_VERSION >= 0x01000106)
    enum libusb_log_level level = LIBUSB_LOG_LEVEL_WARNING;
    if (usbcmd)
    {
        usbcmd->libusb_verbosity = level;
    }
    result = K4A_RESULT_FROM_LIBUSB(libusb_set_option(libusb_ctx, LIBUSB_OPTION_LOG_LEVEL, level));
    // #else
    //     libusb_set_debug(libusb_ctx, 3); // set verbosity level to 3, as suggested in the documentation
    // #endif
    return result;
}

// Stop LIBUSB from generating any debug messages
static void libusb_logging_disable(libusb_context *context)
{
    // #if (LIBUSB_API_VERSION >= 0x01000106)
    K4A_RESULT_FROM_LIBUSB(libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_NONE));
    // #else
    //     libusb_set_debug(context, 0);
    // #endif
}

// Restore LIBUSB's ability to generate debug messages
static void libusb_logging_restore(libusb_context *context, enum libusb_log_level verbosity)
{
    // #if (LIBUSB_API_VERSION >= 0x01000106)
    K4A_RESULT_FROM_LIBUSB(libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, verbosity));
    // #else
    //     libusb_set_debug(context, 3); // set verbosity level to 3, as suggested in the documentation
    // #endif
}

static k4a_result_t populate_container_id(usbcmd_context_t *usbcmd)
{
    struct libusb_bos_descriptor *bos_desc = NULL;
    int i = 0;
    k4a_result_t result;
    struct libusb_container_id_descriptor *container_id = NULL;

    result = K4A_RESULT_FROM_LIBUSB(libusb_get_bos_descriptor(usbcmd->libusb, &bos_desc));

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FAILED;
        for (i = 0; i < bos_desc->bNumDeviceCaps; i++)
        {
            if (bos_desc->dev_capability[i]->bDevCapabilityType == LIBUSB_BT_CONTAINER_ID)
            {
                result = K4A_RESULT_SUCCEEDED;
                break;
            }
        }
        if (i >= bos_desc->bNumDeviceCaps)
        {
            LOG_ERROR("LIBUSB_BT_CONTAINER_ID not found", 0);
            result = K4A_RESULT_FAILED;
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_LIBUSB(
            libusb_get_container_id_descriptor(NULL, bos_desc->dev_capability[i], &container_id));
    }

    if (K4A_SUCCEEDED(result))
    {
        assert(container_id != NULL);
        assert(sizeof(usbcmd->container_id) == sizeof(container_id->ContainerID));
        memcpy((void *)&usbcmd->container_id, container_id->ContainerID, sizeof(usbcmd->container_id));
        libusb_free_container_id_descriptor(container_id);
    }

    if (bos_desc)
    {
        libusb_free_bos_descriptor(bos_desc);
    }
    return result;
}

static k4a_result_t populate_serialnumber(usbcmd_context_t *usbcmd, struct libusb_device_descriptor *desc)
{
    k4a_result_t result;

    result = K4A_RESULT_FROM_BOOL(desc->iSerialNumber);

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_LIBUSB(libusb_get_string_descriptor_ascii(usbcmd->libusb,
                                                                           desc->iSerialNumber,
                                                                           usbcmd->serial_number,
                                                                           sizeof(usbcmd->serial_number)));

        if (K4A_SUCCEEDED(result))
        {
            LOG_INFO("Serial Number found "
                     "%c%c%c%c%c%c%c%c%c%c%c%c",
                     usbcmd->serial_number[0],
                     usbcmd->serial_number[1],
                     usbcmd->serial_number[2],
                     usbcmd->serial_number[3],
                     usbcmd->serial_number[4],
                     usbcmd->serial_number[5],
                     usbcmd->serial_number[6],
                     usbcmd->serial_number[7],
                     usbcmd->serial_number[8],
                     usbcmd->serial_number[9],
                     usbcmd->serial_number[10],
                     usbcmd->serial_number[11]);
        }
    }
    return result;
}

static k4a_result_t find_libusb_device(uint32_t device_index,
                                       const guid_t *container_id,
                                       struct libusb_device_descriptor *desc,
                                       usbcmd_context_t *usbcmd)
{
    k4a_result_t result = K4A_RESULT_FAILED;
    libusb_device **dev_list = NULL; // pointer to pointer of device, used to retrieve a list of devices
    ssize_t count = 0;               // holding number of devices in list
    bool found = false;
    int open_attempts = 0;
    int access_denied = 0;

    // Initialize library
    result = K4A_RESULT_FROM_LIBUSB(libusb_init(&usbcmd->libusb_context));

    if (K4A_SUCCEEDED(result))
    {
        result = usb_cmd_set_libusb_debug_verbosity(usbcmd);
    }

    if (K4A_SUCCEEDED(result))
    {
        // Get list of devices attached - LIBUSB (on Windows) will generate ERROR messages when this is called
        // immediately after a device has been detached from the USB port. So we disable debug output temperarilty.
        libusb_logging_disable(usbcmd->libusb_context);
        result = K4A_RESULT_FROM_BOOL((count = libusb_get_device_list(usbcmd->libusb_context, &dev_list)) < INT32_MAX);
        libusb_logging_restore(usbcmd->libusb_context, usbcmd->libusb_verbosity);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(count != 0);
    }

    if (K4A_SUCCEEDED(result))
    {
        // Traverse list looking for sensor matches
        uint8_t list_index = 0;
        for (int loop = 0; loop < count && found == false; loop++)
        {
            found = false;
            result = K4A_RESULT_FROM_LIBUSB(libusb_get_device_descriptor(dev_list[loop], desc));

            if (K4A_SUCCEEDED(result))
            {
                // Check if this is our device and correlates to the index number based on discovery order
                if ((desc->idVendor != K4A_MSFT_VID) || (desc->idProduct != usbcmd->pid) ||
                    ((device_index != list_index++) && container_id == NULL))
                {
                    continue;
                }
            }

            usbcmd->libusb = NULL;
            if (K4A_SUCCEEDED(result))
            {
                open_attempts++;
                int result_libusb;
                {
                    // LIBUSB (on Windows) will generate ERROR messages when open is called and the device has already
                    // been opened, which we need to do to get the serial number.
                    libusb_logging_disable(usbcmd->libusb_context);
                    result_libusb = libusb_open(dev_list[loop], &usbcmd->libusb);
                    libusb_logging_restore(usbcmd->libusb_context, usbcmd->libusb_verbosity);
                }
                if (LIBUSB_ERROR_ACCESS == result_libusb)
                {
                    access_denied++;
                }
                if (result_libusb < LIBUSB_SUCCESS)
                {
                    continue; // Device is already open
                }
            }

            if (K4A_SUCCEEDED(result))
            {
                result = populate_container_id(usbcmd);
            }

            if (K4A_SUCCEEDED(result))
            {
                if (container_id == NULL)
                {
                    // We opened the USB handle based on index
                    found = true;
                }
                else if (memcmp(container_id, &usbcmd->container_id, sizeof(*container_id)) == 0)
                {
                    // We have a container ID match
                    found = true;
                }
                else
                {
                    char container_id_string[UUID_STR_LENGTH];
                    uuid_to_string(&usbcmd->container_id, container_id_string, sizeof(container_id_string));
                    LOG_INFO("Found non matching Container ID: %s ", container_id_string);
                }
            }

            if (!found)
            {
                libusb_close(usbcmd->libusb);
                usbcmd->libusb = NULL;
            }
        }
    }

    result = K4A_RESULT_FAILED;
    if (found)
    {
        char container_id_string[UUID_STR_LENGTH];
        uuid_to_string(&usbcmd->container_id, container_id_string, sizeof(container_id_string));
        LOG_INFO("Container ID found: %s ", container_id_string);

        result = K4A_RESULT_SUCCEEDED;
    }
    else
    {
        if (usbcmd->libusb)
        {
            libusb_close(usbcmd->libusb);
            usbcmd->libusb = NULL;
        }

        if (container_id)
        {
            char container_id_string[UUID_STR_LENGTH];
            uuid_to_string(container_id, container_id_string, sizeof(container_id_string));
            LOG_ERROR("Unable to find Container ID: %s ", container_id_string);
        }
        else
        {
            if (open_attempts == access_denied)
            {
                LOG_CRITICAL("libusb device(s) are all unavalable. Is the device being used by another application?",
                             0);
            }
            else
            {
                LOG_ERROR("Unable to open LIBUSB at index %d ", device_index);
            }
        }
    }

    // free the list, unref the devices in it
    libusb_free_device_list(dev_list, (int)count);

    return result;
}

k4a_result_t usb_cmd_create(usb_command_device_type_t device_type,
                            uint32_t device_index,
                            const guid_t *container_id,
                            usbcmd_t *usbcmd_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, device_type >= USB_DEVICE_TYPE_COUNT);
    k4a_result_t result = K4A_RESULT_FAILED;
    libusb_device **dev_list = NULL; // pointer to pointer of device, used to retrieve a list of devices
    struct libusb_device_descriptor desc;
    ssize_t count = 0; // holding number of devices in list
    int32_t activeConfig = 0;
    usbcmd_context_t *usbcmd;

    result = K4A_RESULT_FROM_BOOL((usbcmd = usbcmd_t_create(usbcmd_handle)) != NULL);

    if (K4A_SUCCEEDED(result))
    {
        usbcmd->stream_going = false;
        result = K4A_RESULT_FROM_BOOL((usbcmd->lock = Lock_Init()) != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        if (device_type == USB_DEVICE_DEPTH_PROCESSOR)
        {
            usbcmd->pid = K4A_DEPTH_PID;
            usbcmd->interface = USB_CMD_DEPTH_INTERFACE;
            usbcmd->cmd_tx_endpoint = USB_CMD_DEPTH_IN_ENDPOINT;
            usbcmd->cmd_rx_endpoint = USB_CMD_DEPTH_OUT_ENDPOINT;
            usbcmd->stream_endpoint = USB_CMD_DEPTH_STREAM_ENDPOINT;
            usbcmd->source = ALLOCATION_SOURCE_USB_DEPTH;
        }
        else
        {
            usbcmd->pid = K4A_RGB_PID;
            usbcmd->interface = USB_CMD_IMU_INTERFACE;
            usbcmd->cmd_tx_endpoint = USB_CMD_IMU_IN_ENDPOINT;
            usbcmd->cmd_rx_endpoint = USB_CMD_IMU_OUT_ENDPOINT;
            usbcmd->stream_endpoint = USB_CMD_IMU_STREAM_ENDPOINT;
            usbcmd->source = ALLOCATION_SOURCE_USB_IMU;
        }
        result = find_libusb_device(device_index, container_id, &desc, usbcmd);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = populate_serialnumber(usbcmd, &desc);
    }

    if (K4A_SUCCEEDED(result))
    {
        // Set up the configuration and interfaces based on known descriptor definition
        result = K4A_RESULT_FROM_LIBUSB(libusb_get_configuration(usbcmd->libusb, &activeConfig));
    }

    if (K4A_SUCCEEDED(result) && (activeConfig != USB_CMD_DEFAULT_CONFIG))
    {
        result = K4A_RESULT_FROM_LIBUSB(libusb_set_configuration(usbcmd->libusb, USB_CMD_DEFAULT_CONFIG));
    }

    if (K4A_SUCCEEDED(result))
    {
        // Try to force detach kernel driver if on our interface
        if ((libusb_kernel_driver_active(usbcmd->libusb, usbcmd->interface) == 1))
        {
            result = K4A_RESULT_FROM_LIBUSB(libusb_detach_kernel_driver(usbcmd->libusb, usbcmd->interface));
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        // claim interface
        result = K4A_RESULT_FROM_LIBUSB(libusb_claim_interface(usbcmd->libusb, usbcmd->interface));
    }

    // close and free resources if error
    if (K4A_FAILED(result))
    {
        usb_cmd_destroy(*usbcmd_handle);
        *usbcmd_handle = NULL;
    }

    // free the list, unref the devices in it
    libusb_free_device_list(dev_list, (int)count);

    return result;
}

/**
 *  Function to destroy a previous device creation and
 *  releases the associated resources.
 *
 *  @param usbcmd_handle
 *   Handle to the device that is being destroyed.
 *
 */
void usb_cmd_destroy(usbcmd_t usbcmd_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, usbcmd_t, usbcmd_handle);
    usbcmd_context_t *usbcmd = usbcmd_t_get_context(usbcmd_handle);

    // Implicit stop (Must be called prior to releasing any entry resources)
    usb_cmd_stream_stop(usbcmd_handle);

    if (usbcmd->lock)
    {
        // Wait for any outstanding commands to process
        Lock(usbcmd->lock);
        Unlock(usbcmd->lock);
    }

    if (usbcmd->libusb)
    {
        // Release interface(s)
        (void)K4A_RESULT_FROM_LIBUSB(libusb_release_interface(usbcmd->libusb, usbcmd->interface));
    }

    if (usbcmd->libusb)
    {
        // close the device
        libusb_close(usbcmd->libusb); // This apparently unblocks the event handler
        usbcmd->libusb = NULL;
    }

    if (usbcmd->libusb_context)
    {
        // close the instance
        libusb_exit(usbcmd->libusb_context);
        usbcmd->libusb_context = 0;
    }

    if (usbcmd->lock)
    {
        // release lock resources
        Lock_Deinit(usbcmd->lock);
        usbcmd->lock = 0;
    }

    // Destroy the allocator
    usbcmd_t_destroy(usbcmd_handle);
}

k4a_buffer_result_t usb_cmd_get_serial_number(usbcmd_t usbcmd_handle, char *serial_number, size_t *serial_number_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, usbcmd_t, usbcmd_handle);
    RETURN_VALUE_IF_ARG(K4A_BUFFER_RESULT_FAILED, serial_number_size == NULL);

    k4a_buffer_result_t result_b = K4A_BUFFER_RESULT_SUCCEEDED;
    usbcmd_context_t *usbcmd;

    if (K4A_FAILED(K4A_RESULT_FROM_BOOL((usbcmd = usbcmd_t_get_context(usbcmd_handle)) != NULL)))
    {
        result_b = K4A_BUFFER_RESULT_FAILED;
    }

    size_t required_length = strlen((const char *)usbcmd->serial_number) + 1;

    if ((result_b == K4A_BUFFER_RESULT_SUCCEEDED) && (required_length > *serial_number_size))
    {
        *serial_number_size = required_length;
        result_b = K4A_BUFFER_RESULT_TOO_SMALL;
    }

    if ((result_b == K4A_BUFFER_RESULT_SUCCEEDED) && (serial_number == NULL))
    {
        LOG_ERROR("serial_number buffer cannot be NULL", 0);
        result_b = K4A_BUFFER_RESULT_FAILED;
    }

    if (result_b == K4A_BUFFER_RESULT_SUCCEEDED)
    {
        *serial_number_size = required_length;
        memset(serial_number, 0, *serial_number_size);
        memcpy(serial_number, usbcmd->serial_number, required_length - 1);
        result_b = K4A_BUFFER_RESULT_SUCCEEDED;
    }
    return result_b;
}

/**
 *  Function to handle a command transaction with a sensor module
 *
 *  @param usbcmd_handle
 *   Handle to the device that is being read from.
 *
 *  @param cmd
 *   Which command to send
 *
 *  @param p_cmd_data
 *   Pointer to information needed for the command operation
 *
 *  @param cmd_data_size
 *   Size of the associated information
 *
 *  @param p_rx_data
 *   Pointer to where the data will be placed during a read operation
 *
 *  @param rx_data_size
 *   Size of the buffer provided to receive  data
 *
 *  @param p_tx_data
 *   Pointer to where the data will be placed during a write operation
 *
 *  @param tx_data_size
 *   Number of bytes to send
 *
 *  @param transfer_count
 *   Pointer to hold the number of bytes actually sent or read from the device.  Use NULL if not needed
 *
 *  @param cmd_status
 *   Pointer to hold the returned status of the command operation
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 *
 */
static k4a_result_t usb_cmd_io(usbcmd_t usbcmd_handle,
                               uint32_t cmd,
                               void *p_cmd_data,
                               size_t cmd_data_size,
                               void *p_rx_data,
                               size_t rx_data_size,
                               void *p_tx_data,
                               size_t tx_data_size,
                               size_t *transfer_count,
                               uint32_t *cmd_status)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, usbcmd_t, usbcmd_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, cmd_status == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, p_rx_data != NULL && p_tx_data != NULL);

    k4a_result_t result = K4A_RESULT_FAILED;
    usbcmd_context_t *usbcmd;
    int cmd_pkt_length;
    int err;
    int rx_size;
    usb_command_packet_t usb_cmd_pkt;
    usb_command_response_t response_packet;
    int usb_transfer_count = 0;
    uint32_t *p_data = (uint32_t *)p_cmd_data;

    size_t payload_size = (rx_data_size == 0 ? tx_data_size : rx_data_size);

    result = K4A_RESULT_FROM_BOOL((usbcmd = usbcmd_t_get_context(usbcmd_handle)) != NULL);

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(cmd_data_size <= sizeof(usb_cmd_pkt.data));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(payload_size <= INT32_MAX);
    }

    // Check for valid handle
    // Lock
    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FAILED;
        if ((cmd_data_size > 0) && (p_data != NULL))
        {
            LOG_TRACE("XFR: Cmd=%08x, CmdLength=%u, PayloadSize=%zu, CmdData=%08x %08x...",
                      cmd,
                      cmd_data_size,
                      payload_size,
                      p_data[0],
                      p_data[1]);
        }
        else
        {
            LOG_TRACE("XFR: Cmd=%08x, PayloadSize=%zu", cmd, payload_size);
        }

        Lock(usbcmd->lock);
        // format up request and send command
        usb_cmd_pkt.header.command = cmd;
        usb_cmd_pkt.header.packet_type = USB_CMD_PACKET_TYPE;
        usb_cmd_pkt.header.payload_size = (uint32_t)payload_size;
        usb_cmd_pkt.header.reserved = 0;
        usb_cmd_pkt.header.packet_transaction_id = usbcmd->transaction_id++;
        memcpy(usb_cmd_pkt.data, p_cmd_data, cmd_data_size);

        cmd_pkt_length = (int)(sizeof(usb_command_header_t) + cmd_data_size);

        // A read transaction has 3 states.
        //  1. Send Command
        //  2. Data transfer to or from
        //  3. Receive response status

        // Send command
        if ((err = libusb_bulk_transfer(usbcmd->libusb,
                                        usbcmd->cmd_tx_endpoint,
                                        (uint8_t *)&usb_cmd_pkt,
                                        cmd_pkt_length,
                                        NULL,
                                        USB_CMD_MAX_WAIT_TIME)) == LIBUSB_SUCCESS)
        {
            // Send Data if data to send
            if ((p_tx_data != NULL) && ((err = libusb_bulk_transfer(usbcmd->libusb,
                                                                    usbcmd->cmd_tx_endpoint,
                                                                    (uint8_t *)p_tx_data,
                                                                    (int)payload_size,
                                                                    &usb_transfer_count,
                                                                    USB_CMD_MAX_WAIT_TIME))) != LIBUSB_SUCCESS)
            {
                usb_transfer_count = 0;
                LOG_ERROR("Error calling libusb_bulk_transfer for tx, result:%s", libusb_error_name(err));
                goto exit;
            }

            // Get Data if resources provided to read into
            if ((p_rx_data != NULL) && ((err = libusb_bulk_transfer(usbcmd->libusb,
                                                                    usbcmd->cmd_rx_endpoint,
                                                                    (uint8_t *)p_rx_data,
                                                                    (int)payload_size,
                                                                    &usb_transfer_count,
                                                                    USB_CMD_MAX_WAIT_TIME)) != LIBUSB_SUCCESS))
            {
                usb_transfer_count = 0;
                LOG_ERROR("Error calling libusb_bulk_transfer for rx, result:%s", libusb_error_name(err));
                goto exit;
            }

            // Get Status
            if ((err = libusb_bulk_transfer(usbcmd->libusb,
                                            usbcmd->cmd_rx_endpoint,
                                            (uint8_t *)&response_packet,
                                            sizeof(response_packet),
                                            &rx_size,
                                            USB_CMD_MAX_WAIT_TIME)) == LIBUSB_SUCCESS)
            {
                // Check for errors in response packet. The packet status is checked by the caller in the
                // success cases, so it shouldn't be checked here.
                if ((rx_size != sizeof(response_packet)) ||
                    (response_packet.packet_transaction_id != usb_cmd_pkt.header.packet_transaction_id) ||
                    (response_packet.packet_type != USB_CMD_PACKET_TYPE_RESPONSE))
                {
                    LOG_ERROR("Command(%08X) sequence ended in failure, "
                              "TransactionId %08X == %08X "
                              "Response size 0x%08X == 0x%08X "
                              "Packet status 0x%08x == 0x%08x "
                              "Packet type 0x%08x == 0x%08x",
                              cmd,
                              response_packet.packet_transaction_id,
                              usb_cmd_pkt.header.packet_transaction_id,
                              rx_size,
                              sizeof(response_packet),
                              response_packet.status,
                              0,
                              response_packet.packet_type,
                              USB_CMD_PACKET_TYPE_RESPONSE);
                }
                else
                {
                    *cmd_status = response_packet.status;
                    result = K4A_RESULT_SUCCEEDED;
                }
            }
            else
            {
                LOG_ERROR("Error calling libusb_bulk_transfer for status, result:%s", libusb_error_name(err));
            }
        }
        else
        {
            LOG_ERROR("Error calling libusb_bulk_transfer for initial tx, result:%s", libusb_error_name(err));
        }

    exit:
        if (transfer_count != NULL)
        {
            // record the transfer size if requested
            *transfer_count = (size_t)usb_transfer_count;
        }
        // unlock
        Unlock(usbcmd->lock);
    }

    return result;
}

/**
 *  Function to read data from the device.
 *
 *  @param usbcmd_handle
 *   Handle to the device that is being read from.
 *
 *  @param cmd
 *   Which read to perform
 *
 *  @param p_cmd_data
 *   Pointer to information needed for the read operation
 *
 *  @param cmd_data_size
 *   Size of the associated information
 *
 *  @param p_data
 *   Pointer to where the data will be placed during the read operation
 *
 *  @param data_size
 *   Size of the buffer provided to receive the data
 *
 *  @param bytes_read
 *   Pointer to hold the number of bytes actually read from the device.  Use NULL if not needed
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 *
 */
k4a_result_t usb_cmd_read(usbcmd_t usbcmd_handle,
                          uint32_t cmd,
                          uint8_t *p_cmd_data,
                          size_t cmd_data_size,
                          uint8_t *p_data,
                          size_t data_size,
                          size_t *bytes_read)
{
    uint32_t cmd_status;
    k4a_result_t result = K4A_RESULT_FAILED;

    result = TRACE_CALL(
        usb_cmd_io(usbcmd_handle, cmd, p_cmd_data, cmd_data_size, p_data, data_size, NULL, 0, bytes_read, &cmd_status));

    if (K4A_SUCCEEDED(result) && cmd_status != 0)
    {
        LOG_ERROR("Read command(%08X) ended in failure, Command status 0x%08x", cmd, cmd_status);
        result = K4A_RESULT_FAILED;
    }

    return result;
}

/**
 *  Function to read data from the device.
 *
 *  @param usbcmd_handle
 *   Handle to the device that is being read from.
 *
 *  @param cmd
 *   Which read to perform
 *
 *  @param p_cmd_data
 *   Pointer to information needed for the read operation
 *
 *  @param cmd_data_size
 *   Size of the associated information
 *
 *  @param p_data
 *   Pointer to where the data will be placed during the read operation
 *
 *  @param data_size
 *   Size of the buffer provided to receive the data
 *
 *  @param bytes_read
 *   Pointer to hold the number of bytes actually read from the device.  Use NULL if not needed
 *
 *  @param cmd_status
 *   Pointer to hold the returned status of the command operation
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 *
 */
k4a_result_t usb_cmd_read_with_status(usbcmd_t usbcmd_handle,
                                      uint32_t cmd,
                                      uint8_t *p_cmd_data,
                                      size_t cmd_data_size,
                                      uint8_t *p_data,
                                      size_t data_size,
                                      size_t *bytes_read,
                                      uint32_t *cmd_status)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, cmd_status == NULL);

    return usb_cmd_io(
        usbcmd_handle, cmd, p_cmd_data, cmd_data_size, p_data, data_size, NULL, 0, bytes_read, cmd_status);
}

/**
 *  Function to write data to the device.
 *
 *  @param usbcmd_handle
 *   Handle to the device that is being written to.
 *
 *  @param cmd
 *   Which write to perform
 *
 *  @param p_cmd_data
 *   Pointer to information needed for the write operation
 *
 *  @param cmd_data_size
 *   Size of the associated information
 *
 *  @param p_data
 *   Pointer to data being sent to the device
 *
 *  @param data_size
 *   Size of the data buffer being sent
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 *
 */
k4a_result_t usb_cmd_write(usbcmd_t usbcmd_handle,
                           uint32_t cmd,
                           uint8_t *p_cmd_data,
                           size_t cmd_data_size,
                           uint8_t *p_data,
                           size_t data_size)
{
    uint32_t cmd_status;
    k4a_result_t result = K4A_RESULT_FAILED;

    result = TRACE_CALL(
        usb_cmd_io(usbcmd_handle, cmd, p_cmd_data, cmd_data_size, NULL, 0, p_data, data_size, NULL, &cmd_status));

    if (K4A_SUCCEEDED(result) && cmd_status != 0)
    {
        LOG_ERROR("Write command(%08X) ended in failure, Command status 0x%08x", cmd, cmd_status);
        result = K4A_RESULT_FAILED;
    }

    return result;
}

/**
 *  Function to write data to the device.
 *
 *  @param usbcmd_handle
 *   Handle to the device that is being written to.
 *
 *  @param cmd
 *   Which write to perform
 *
 *  @param p_cmd_data
 *   Pointer to information needed for the write operation
 *
 *  @param cmd_data_size
 *   Size of the associated information
 *
 *  @param p_data
 *   Pointer to data being sent to the device
 *
 *  @param data_size
 *   Size of the data buffer being sent
 *
 *  @param cmd_status
 *   Pointer to hold the returned status of the command operation
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 *
 */
k4a_result_t usb_cmd_write_with_status(usbcmd_t usbcmd_handle,
                                       uint32_t cmd,
                                       uint8_t *p_cmd_data,
                                       size_t cmd_data_size,
                                       uint8_t *p_data,
                                       size_t data_size,
                                       uint32_t *cmd_status)
{
    k4a_result_t result = K4A_RESULT_FAILED;

    result = TRACE_CALL(
        usb_cmd_io(usbcmd_handle, cmd, p_cmd_data, cmd_data_size, NULL, 0, p_data, data_size, NULL, cmd_status));

    return result;
}

/**
 *  Function registering the callback function associated with
 *  streaming data.
 *
 *  @param usbcmd_handle
 *   Handle to the context where the callback will be stored
 *
 *  @param context
 *   Data that will be handed back with the callback
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 *
 */
k4a_result_t usb_cmd_stream_register_cb(usbcmd_t usbcmd_handle, usb_cmd_stream_cb_t *capture_ready_cb, void *context)
{
    k4a_result_t result;
    usbcmd_context_t *usbcmd;

    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, usbcmd_t, usbcmd_handle)

    result = K4A_RESULT_FROM_BOOL((usbcmd = usbcmd_t_get_context(usbcmd_handle)) != NULL);

    // check if handle is valid
    if (K4A_SUCCEEDED(result))
    {
        usbcmd->callback = capture_ready_cb;
        usbcmd->stream_context = context;
    }
    return result;
}

/**
 *  Function to get the number of sensor modules attached
 *
 *  @param p_device_count
 *   Pointer to where the device count will be placed
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 *
 */
k4a_result_t usb_cmd_get_device_count(uint32_t *p_device_count)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    struct libusb_device_descriptor desc;
    libusb_context *libusb_ctx;
    libusb_device **dev_list; // pointer to pointer of device, used to retrieve a list of devices
    ssize_t count;            // holding number of devices in list
    int err;
    uint32_t color_device_count = 0;
    uint32_t depth_device_count = 0;

    if (p_device_count == NULL)
    {
        LOG_ERROR("Error p_device_count is NULL", 0);
        return K4A_RESULT_FAILED;
    }

    *p_device_count = 0;
    // initialize library
    if ((err = libusb_init(&libusb_ctx)) < 0)
    {
        LOG_ERROR("Error calling libusb_init, result:%s", libusb_error_name(err));
        return K4A_RESULT_FAILED;
    }

    // We disable all LIBUSB logging for this function, which only used this local context. LIBUSB (on Windows)
    // generates errors when a device is detached moments before this is called.
    libusb_logging_disable(libusb_ctx);

    count = libusb_get_device_list(libusb_ctx, &dev_list); // get the list of devices
    if (count > INT32_MAX)
    {
        LOG_ERROR("List too large", 0);
        return K4A_RESULT_FAILED;
    }
    if (count <= 0)
    {
        LOG_ERROR("No devices found", 0);
        return K4A_RESULT_FAILED;
    }

    // Loop through and count matching VID / PID
    for (int loop = 0; loop < count; loop++)
    {
        result = K4A_RESULT_FROM_LIBUSB(libusb_get_device_descriptor(dev_list[loop], &desc));
        if (K4A_FAILED(result))
        {
            break;
        }

        //  Count how many color or depth end points we find.
        if (desc.idVendor == K4A_MSFT_VID)
        {
            if (desc.idProduct == K4A_RGB_PID)
            {
                color_device_count += 1;
            }
            else if (desc.idProduct == K4A_DEPTH_PID)
            {
                depth_device_count += 1;
            }
        }
    }
    // free the list, unref the devices in it
    libusb_free_device_list(dev_list, (int)count);

    // close the instance
    libusb_exit(libusb_ctx);

    // Color or Depth end point my be in a bad state so we cound both and return the larger count
    *p_device_count = color_device_count > depth_device_count ? color_device_count : depth_device_count;

    return result;
}

// Waiting on hot-plugging support
#if 0
/**
 *  Function to get the attachment bus number and port path for a particular handle
 *
 *  @param usbcmd
 *   Handle that will be used to get the bus and port path
 *
 *  @param p_bus
 *   Pointer to where the bus number will be placed
 *
 *  @param p_path
 *   Pointer to where the port numbers will be placed
 *
 *  @param path_size
 *   number of port numbers supported in the port path array
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED           Operation successful
 *   K4A_RESULT_FAILED              Operation failed
 *
 */
k4a_result_t usb_cmd_path_get(usbcmd_t usbcmd_handle, uint8_t *p_bus, uint8_t *p_path, uint8_t path_size)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    usbcmd_t *usbcmd = (usbcmd_t *)usbcmd_handle;
    if (usbcmd != NULL)
    {
        *p_bus = usbcmd->bus;
        // clear path
        memset(p_path, 0, path_size);
        // copy over the path
        if (path_size > USB_CMD_PORT_DEPTH)
        {
            path_size = USB_CMD_PORT_DEPTH;
        }
        memcpy(p_path, usbcmd->port_path, path_size);
    }
    else
    {
        LOG_ERROR( "Error usbcmd is NULL",0);
        result = K4A_RESULT_FAILED;
    }
    return result;
}
#endif

const guid_t *usb_cmd_get_container_id(usbcmd_t usbcmd_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NULL, usbcmd_t, usbcmd_handle);

    usbcmd_context_t *usbcmd = usbcmd_t_get_context(usbcmd_handle);

    return &usbcmd->container_id;
}
