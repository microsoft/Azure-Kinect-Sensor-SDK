/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

//************************ Includes *****************************

// This library
#include "usb_cmd_priv.h"

// System dependencies
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Ensure we have LIBUSB_API_VERSION defined if not defined by libusb.h
#ifndef LIBUSB_API_VERSION
#define LIBUSB_API_VERSION 0
#endif

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
    usb_cmd_handle_t **handle_list;
} descriptor_choice_t;

//************ Declarations (Statics and globals) ***************
// List management (used for handle validation and reporting)
static usb_cmd_handle_t *g_p_usb_cmd_imu_handle_list = NULL;
static usb_cmd_handle_t *g_p_usb_cmd_depth_handle_list = NULL;

// device setup information table
static descriptor_choice_t device_info_table[USB_DEVICE_TYPE_COUNT] = { { K4A_DEPTH_PID,
                                                                          USB_CMD_DEPTH_INTERFACE,
                                                                          USB_CMD_DEPTH_IN_ENDPOINT,
                                                                          USB_CMD_DEPTH_OUT_ENDPOINT,
                                                                          USB_CMD_DEPTH_STREAM_ENDPOINT,
                                                                          &g_p_usb_cmd_depth_handle_list },
                                                                        { K4A_RGB_PID,
                                                                          USB_CMD_IMU_INTERFACE,
                                                                          USB_CMD_IMU_IN_ENDPOINT,
                                                                          USB_CMD_IMU_OUT_ENDPOINT,
                                                                          USB_CMD_IMU_STREAM_ENDPOINT,
                                                                          &g_p_usb_cmd_imu_handle_list } };

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

/**
 *  Utility to add an entry to the available connection list
 *  Note: consumers of this function must have resources protected
 *  prior to call.
 *
 *  @param device_type
 *   Which device list to use
 *
 *  @param p_handle
 *   Entry to add
 *
 */
static void usb_cmd_add_handle(usb_command_device_type_t device_type, usb_cmd_handle_t *p_handle)
{
    usb_cmd_handle_t *p_handle_list_base = NULL;

    // place entry in list
    if (*(device_info_table[device_type].handle_list) == NULL)
    {
        // First entry
        *(device_info_table[device_type].handle_list) = p_handle;
    }
    else
    {
        p_handle_list_base = *(device_info_table[device_type].handle_list);
        while (p_handle_list_base->next != NULL)
        {
            p_handle_list_base = p_handle_list_base->next;
        }
        p_handle_list_base->next = p_handle;
    }
}

/**
 *  Utility to remove an entry from the connection lists
 *  Note: consumers of this function must have resources protected
 *  prior to call.
 *
 *  @param p_handle
 *   Entry to remove
 *
 */
static void usb_cmd_remove_handle(usb_cmd_handle_t *p_handle)
{
    usb_cmd_handle_t *p_handle_list_base;

    if (p_handle != NULL)
    {
        // traverse both list to find the entry pointer to remove
        if (g_p_usb_cmd_depth_handle_list == p_handle)
        {
            // first depth entry
            g_p_usb_cmd_depth_handle_list = p_handle->next;
        }

        if (g_p_usb_cmd_imu_handle_list == p_handle)
        {
            // first imu entry
            g_p_usb_cmd_imu_handle_list = p_handle->next;
        }

        p_handle_list_base = g_p_usb_cmd_depth_handle_list;
        while ((p_handle_list_base != NULL) && (p_handle_list_base->next != NULL))
        {
            if (p_handle_list_base->next == p_handle)
            {
                p_handle_list_base->next = p_handle->next;
            }
            p_handle_list_base = p_handle_list_base->next;
        }

        p_handle_list_base = g_p_usb_cmd_imu_handle_list;
        while ((p_handle_list_base != NULL) && (p_handle_list_base->next != NULL))
        {
            if (p_handle_list_base->next == p_handle)
            {
                p_handle_list_base->next = p_handle->next;
            }
            p_handle_list_base = p_handle_list_base->next;
        }
    }
}

/**
 *  This function is used to validate the handle.
 *
 *  @param p_handle
 *   handle to check
 *
 *  @return
 *   true                    Handle is valid
 *   false                   Handle is not valid
 *
 */
bool usb_cmd_is_handle_valid(usb_cmd_handle_t *p_handle)
{
    usb_cmd_handle_t *p_handle_list;

    if (p_handle != NULL)
    {
        p_handle_list = g_p_usb_cmd_imu_handle_list;
        while (p_handle_list != NULL)
        {
            if (p_handle_list == p_handle)
            {
                return true;
            }
            p_handle_list = p_handle_list->next;
        }

        p_handle_list = g_p_usb_cmd_depth_handle_list;
        while (p_handle_list != NULL)
        {
            if (p_handle_list == p_handle)
            {
                return true;
            }
            p_handle_list = p_handle_list->next;
        }
    }

    logger_error(LOGGER_USB_CMD, "Error, Command Handle invalid");
    return false;
}

/**
 *  This function creates a connection to a sensor module based on the type
 *  requested and the order the device is attached to the host.
 *
 *  @param device_type
 *   Type of connection to create
 *
 *  @param device_index
 *   Index to the attached sensor module
 *
 *  @param p_command_handle
 *   pointer to where the handle pointer will be placed
 *
 *  @param logger
 *   handle to the logger to use
 *
 *  @return
 *   K4A_RESULT_SUCCEEDED   Operation successful
 *   K4A_RESULT_FAILED      Operation failed
 */
k4a_result_t usb_cmd_create(usb_command_device_type_t device_type,
                            uint8_t device_index,
                            usb_command_handle_t *p_command_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, device_type >= USB_DEVICE_TYPE_COUNT);
    k4a_result_t result = K4A_RESULT_FAILED;
    uint8_t bus;
    uint8_t port_path[USB_CMD_PORT_DEPTH];
    uint8_t list_index = 0;
    usb_cmd_handle_t *p_handle = NULL;
    int port_count;
    libusb_context *p_context = NULL;
    libusb_device **dev_list; // pointer to pointer of device, used to retrieve a list of devices
    struct libusb_device_descriptor desc;
    ssize_t count; // holding number of devices in list
    int err;

    // Check if this index has already been opened (based on previously discovered device order).  This is not an error
    p_handle = *(device_info_table[device_type].handle_list);
    while (p_handle != NULL)
    {
        if ((p_handle->index == device_index) && (p_handle->libusb != NULL))
        {
            // returning the already opened handle
            *p_command_handle = p_handle;
            return K4A_RESULT_SUCCEEDED;
        }
        p_handle = p_handle->next;
    }

    // Initialize library
    if ((err = libusb_init(&p_context)) < 0)
    {
        logger_error(LOGGER_USB_CMD, "Error calling libusb_init, result:%s", err);
        return K4A_RESULT_FAILED;
    }

#if (LIBUSB_API_VERSION >= 0x01000106)
    libusb_set_option(p_context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
#else
    libusb_set_debug(p_context, 3); // set verbosity level to 3, as suggested in the documentation
#endif

    // Get list of devices attached
    count = libusb_get_device_list(p_context, &dev_list); // get the list of devices
    if (count > INT32_MAX)
    {
        logger_error(LOGGER_USB_CMD, "List too large");
        return K4A_RESULT_FAILED;
    }

    if (count == 0)
    {
        logger_error(LOGGER_USB_CMD, "No devices found");
        return K4A_RESULT_FAILED;
    }

    // Traverse list looking for sensor matches
    list_index = 0;
    for (int loop = 0; loop < count; loop++)
    {
        bus = libusb_get_bus_number(dev_list[loop]);
        port_count = libusb_get_port_numbers(dev_list[loop], port_path, sizeof(port_path));

        if (port_count < 0)
        {
            logger_error(LOGGER_USB_CMD, "Error calling libusb_get_port_numbers, port_count:%d", port_count);
            break;
        }
        err = libusb_get_device_descriptor(dev_list[loop], &desc);
        if (err != LIBUSB_SUCCESS)
        {
            logger_error(LOGGER_USB_CMD,
                         "Error calling libusb_get_device_descriptor, result:%s",
                         libusb_error_name(err));
            break;
        }

        // Check if this is our device and correlates to the index number based on discovery order
        if ((desc.idVendor == K4A_MSFT_VID) && (desc.idProduct == device_info_table[device_type].pid) &&
            (device_index == list_index++))
        {
            // Get new cleared entry
            p_handle = (usb_cmd_handle_t *)calloc(1, sizeof(usb_cmd_handle_t));
            if (p_handle == NULL)
            {
                logger_error(LOGGER_USB_CMD, "Failed to allocate new handle");
                return K4A_RESULT_FAILED;
            }

            // open
            p_handle->libusb = NULL;
            if ((err = libusb_open(dev_list[loop], &p_handle->libusb)) != LIBUSB_SUCCESS)
            {
                logger_error(LOGGER_USB_CMD, "Error calling libusb_open, result:%s", libusb_error_name(err));
                break;
            }

            // Set up the configuration and interfaces based on known descriptor definition
            int32_t activeConfig;
            err = libusb_get_configuration(p_handle->libusb, &activeConfig);
            if (err != LIBUSB_SUCCESS)
            {
                logger_error(LOGGER_USB_CMD,
                             "Error calling libusb_get_configuration, result:%s",
                             libusb_error_name(err));
                break;
            }
            if (activeConfig != USB_CMD_DEFAULT_CONFIG)
            {
                err = libusb_set_configuration(p_handle->libusb, USB_CMD_DEFAULT_CONFIG);
                if (err != LIBUSB_SUCCESS)
                {
                    logger_error(LOGGER_USB_CMD,
                                 "Error calling libusb_set_configuration, result:%s",
                                 libusb_error_name(err));
                    break;
                }
            }

            // Try to force detach kernel driver if on our interface
            if ((libusb_kernel_driver_active(p_handle->libusb, device_info_table[device_type].interface) == 1))
            {
                err = libusb_detach_kernel_driver(p_handle->libusb, device_info_table[device_type].interface);
                if (err != LIBUSB_SUCCESS)
                {
                    logger_error(LOGGER_USB_CMD,
                                 "Error calling libusb_detach_kernel_driver, result:%s",
                                 libusb_error_name(err));
                    break;
                }
            }

            // claim interface
            err = libusb_claim_interface(p_handle->libusb, device_info_table[device_type].interface);
            if (err != LIBUSB_SUCCESS)
            {
                logger_error(LOGGER_USB_CMD, "Error calling libusb_claim_interface, result:%s", libusb_error_name(err));
                break;
            }

            // fill in data
            p_handle->cmd_mutex = Lock_Init();
            if (p_handle->cmd_mutex == NULL)
            {
                logger_error(LOGGER_USB_CMD, "Could not initialize mutex");
                break;
            }
            p_handle->index = device_index;
            p_handle->bus = bus;
            if (port_count > USB_CMD_PORT_DEPTH)
            {
                port_count = USB_CMD_PORT_DEPTH;
            }
            memcpy(p_handle->port_path, port_path, (unsigned long)port_count);
            p_handle->interface = device_info_table[device_type].interface;
            p_handle->depth_context = p_handle->interface == USB_CMD_DEPTH_INTERFACE;
            p_handle->cmd_tx_endpoint = device_info_table[device_type].cmd_tx_endpoint;
            p_handle->cmd_rx_endpoint = device_info_table[device_type].cmd_rx_endpoint;
            p_handle->stream_endpoint = device_info_table[device_type].stream_endpoint;
            p_handle->source = p_handle->depth_context ? ALLOCATION_SOURCE_USB_DEPTH : ALLOCATION_SOURCE_USB_IMU;
            p_handle->usblib_context = p_context;
            p_handle->stream_going = false;

            p_handle->next = NULL;

            // place entry in list
            usb_cmd_add_handle(device_type, p_handle);

            // Provide handle to entry in list
            *p_command_handle = p_handle;

            // only path for success
            result = K4A_RESULT_SUCCEEDED;

            break;
        }
    }

    // close and free resources if error
    if (result != K4A_RESULT_SUCCEEDED)
    {
        logger_error(LOGGER_USB_CMD, "Error calling usb_cmd_create, result:%d", result);
        if (p_handle != NULL)
        {
            if (p_handle->libusb != NULL)
            {
                libusb_close(p_handle->libusb);
                p_handle->libusb = NULL;
                // Remove from list
                usb_cmd_remove_handle(p_handle);
            }
            if (p_handle->cmd_mutex != NULL)
            {
                Lock_Deinit(p_handle->cmd_mutex);
            }
            free(p_handle);
        }
    }

    // free the list, unref the devices in it
    libusb_free_device_list(dev_list, (int)count);

    return result;
}

/**
 *  Function to destroy a previous device creation and
 *  releases the associated resources.
 *
 *  @param p_command_handle
 *   Handle to the device that is being destroyed.
 *
 */
void usb_cmd_destroy(usb_command_handle_t p_command_handle)
{
    usb_cmd_handle_t *p_handle = (usb_cmd_handle_t *)p_command_handle;

    // Implicit stop (Must be called prior to releasing any entry resources)
    usb_cmd_stream_stop(p_command_handle);

    // check if handle is valid
    if (usb_cmd_is_handle_valid(p_handle))
    {
        // Wait for any outstanding commands to process (under the current 'contract', this shouldn't be needed)
        Lock(p_handle->cmd_mutex);
        Unlock(p_handle->cmd_mutex);

        // Release interface(s)
        libusb_release_interface(p_handle->libusb, p_handle->interface);

        // close the device
        libusb_close(p_handle->libusb); // This apparently unblocks the event handler
        p_handle->libusb = NULL;

        // close the instance
        libusb_exit(p_handle->usblib_context);

        // release lock resources
        Lock_Deinit(p_handle->cmd_mutex);

        // Remove entry from list
        usb_cmd_remove_handle(p_handle);

        // Destroy the allocator
        free(p_handle);
    }
}

/**
 *  Function to handle a command transaction with a sensor module
 *
 *  @param p_command_handle
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
static k4a_result_t usb_cmd_io(usb_command_handle_t p_command_handle,
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
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, cmd_status == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, p_rx_data != NULL && p_tx_data != NULL);

    k4a_result_t result = K4A_RESULT_FAILED;
    usb_cmd_handle_t *p_handle = (usb_cmd_handle_t *)p_command_handle;
    int cmd_pkt_length;
    int err;
    int rx_size;
    usb_command_packet_t usb_cmd_pkt;
    usb_command_response_t response_packet;
    int usb_transfer_count = 0;
    uint32_t *p_data = (uint32_t *)p_cmd_data;

    size_t payload_size = (rx_data_size == 0 ? tx_data_size : rx_data_size);

    if (cmd_data_size > sizeof(usb_cmd_pkt.data))
    {
        logger_error(LOGGER_USB_CMD, "Command size too large");
        return K4A_RESULT_FAILED;
    }

    if (payload_size > INT32_MAX)
    {
        logger_error(LOGGER_USB_CMD, "IO transfer size too large");
        return K4A_RESULT_FAILED;
    }

    // Check for valid handle
    // Lock
    if (usb_cmd_is_handle_valid(p_handle))
    {
        if ((cmd_data_size > 0) && (p_data != NULL))
        {
            logger_trace(LOGGER_USB_CMD,
                         "XFR: Cmd=%08x, CmdLength=%u, PayloadSize=%zu, CmdData=%08x %08x...",
                         cmd,
                         cmd_data_size,
                         payload_size,
                         p_data[0],
                         p_data[1]);
        }
        else
        {
            logger_trace(LOGGER_USB_CMD, "XFR: Cmd=%08x, PayloadSize=%zu", cmd, payload_size);
        }

        Lock(p_handle->cmd_mutex);
        // format up request and send command
        usb_cmd_pkt.header.command = cmd;
        usb_cmd_pkt.header.packet_type = USB_CMD_PACKET_TYPE;
        usb_cmd_pkt.header.payload_size = (uint32_t)payload_size;
        usb_cmd_pkt.header.reserved = 0;
        usb_cmd_pkt.header.packet_transaction_id = p_handle->transaction_id++;
        memcpy(usb_cmd_pkt.data, p_cmd_data, cmd_data_size);

        cmd_pkt_length = (int)(sizeof(usb_command_header_t) + cmd_data_size);

        // A read transaction has 3 states.
        //  1. Send Command
        //  2. Data transfer to or from
        //  3. Receive response status

        // Send command
        if ((err = libusb_bulk_transfer(p_handle->libusb,
                                        p_handle->cmd_tx_endpoint,
                                        (uint8_t *)&usb_cmd_pkt,
                                        cmd_pkt_length,
                                        NULL,
                                        USB_CMD_MAX_WAIT_TIME)) == LIBUSB_SUCCESS)
        {
            // Send Data if data to send
            if ((p_tx_data != NULL) && ((err = libusb_bulk_transfer(p_handle->libusb,
                                                                    p_handle->cmd_tx_endpoint,
                                                                    (uint8_t *)p_tx_data,
                                                                    (int)payload_size,
                                                                    &usb_transfer_count,
                                                                    USB_CMD_MAX_WAIT_TIME))) != LIBUSB_SUCCESS)
            {
                usb_transfer_count = 0;
                logger_error(LOGGER_USB_CMD,
                             "Error calling libusb_bulk_transfer for tx, result:%s",
                             libusb_error_name(err));
                goto exit;
            }

            // Get Data if resources provided to read into
            if ((p_rx_data != NULL) && ((err = libusb_bulk_transfer(p_handle->libusb,
                                                                    p_handle->cmd_rx_endpoint,
                                                                    (uint8_t *)p_rx_data,
                                                                    (int)payload_size,
                                                                    &usb_transfer_count,
                                                                    USB_CMD_MAX_WAIT_TIME)) != LIBUSB_SUCCESS))
            {
                usb_transfer_count = 0;
                logger_error(LOGGER_USB_CMD,
                             "Error calling libusb_bulk_transfer for rx, result:%s",
                             libusb_error_name(err));
                goto exit;
            }

            // Get Status
            if ((err = libusb_bulk_transfer(p_handle->libusb,
                                            p_handle->cmd_rx_endpoint,
                                            (uint8_t *)&response_packet,
                                            sizeof(response_packet),
                                            &rx_size,
                                            USB_CMD_MAX_WAIT_TIME)) == LIBUSB_SUCCESS)
            {
                // Check for errors in response packet. The packet status is checked by the caller in the success cases,
                // so it shouldn't be checked here.
                if ((rx_size != sizeof(response_packet)) ||
                    (response_packet.packet_transaction_id != usb_cmd_pkt.header.packet_transaction_id) ||
                    (response_packet.packet_type != USB_CMD_PACKET_TYPE_RESPONSE))
                {
                    logger_error(LOGGER_USB_CMD,
                                 "Command(%08X) sequence ended in failure, "
                                 "transationId %08X == %08X "
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
                logger_error(LOGGER_USB_CMD,
                             "Error calling libusb_bulk_transfer for status, result:%s",
                             libusb_error_name(err));
            }
        }
        else
        {
            logger_error(LOGGER_USB_CMD,
                         "Error calling libusb_bulk_transfer for initial tx, result:%s",
                         libusb_error_name(err));
        }

    exit:
        if (transfer_count != NULL)
        {
            // record the transfer size if requested
            *transfer_count = (size_t)usb_transfer_count;
        }
        // unlock
        Unlock(p_handle->cmd_mutex);
    }

    return result;
}

/**
 *  Function to read data from the device.
 *
 *  @param p_command_handle
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
k4a_result_t usb_cmd_read(usb_command_handle_t p_command_handle,
                          uint32_t cmd,
                          uint8_t *p_cmd_data,
                          size_t cmd_data_size,
                          uint8_t *p_data,
                          size_t data_size,
                          size_t *bytes_read)
{
    uint32_t cmd_status;
    k4a_result_t result = K4A_RESULT_FAILED;

    result = TRACE_CALL(usb_cmd_io(
        p_command_handle, cmd, p_cmd_data, cmd_data_size, p_data, data_size, NULL, 0, bytes_read, &cmd_status));

    if (K4A_SUCCEEDED(result) && cmd_status != 0)
    {
        logger_error(LOGGER_USB_CMD, "Read command(%08X) ended in failure, Command status 0x%08x", cmd, cmd_status);
        result = K4A_RESULT_FAILED;
    }

    return result;
}

/**
 *  Function to read data from the device.
 *
 *  @param p_command_handle
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
k4a_result_t usb_cmd_read_with_status(usb_command_handle_t p_command_handle,
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
        p_command_handle, cmd, p_cmd_data, cmd_data_size, p_data, data_size, NULL, 0, bytes_read, cmd_status);
}

/**
 *  Function to write data to the device.
 *
 *  @param p_command_handle
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
k4a_result_t usb_cmd_write(usb_command_handle_t p_command_handle,
                           uint32_t cmd,
                           uint8_t *p_cmd_data,
                           size_t cmd_data_size,
                           uint8_t *p_data,
                           size_t data_size)
{
    uint32_t cmd_status;
    k4a_result_t result = K4A_RESULT_FAILED;

    result = TRACE_CALL(
        usb_cmd_io(p_command_handle, cmd, p_cmd_data, cmd_data_size, NULL, 0, p_data, data_size, NULL, &cmd_status));

    if (K4A_SUCCEEDED(result) && cmd_status != 0)
    {
        logger_error(LOGGER_USB_CMD, "Write command(%08X) ended in failure, Command status 0x%08x", cmd, cmd_status);
        result = K4A_RESULT_FAILED;
    }

    return result;
}

/**
 *  Function to write data to the device.
 *
 *  @param p_command_handle
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
k4a_result_t usb_cmd_write_with_status(usb_command_handle_t p_command_handle,
                                       uint32_t cmd,
                                       uint8_t *p_cmd_data,
                                       size_t cmd_data_size,
                                       uint8_t *p_data,
                                       size_t data_size,
                                       uint32_t *cmd_status)
{
    k4a_result_t result = K4A_RESULT_FAILED;

    result = TRACE_CALL(
        usb_cmd_io(p_command_handle, cmd, p_cmd_data, cmd_data_size, NULL, 0, p_data, data_size, NULL, cmd_status));

    return result;
}

/**
 *  Function registering the callback function associated with
 *  streaming data.
 *
 *  @param p_command_handle
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
k4a_result_t usb_cmd_stream_register_cb(usb_command_handle_t p_command_handle,
                                        usb_cmd_stream_cb_t *capture_ready_cb,
                                        void *context)
{
    k4a_result_t result = K4A_RESULT_FAILED;
    usb_cmd_handle_t *p_handle = (usb_cmd_handle_t *)p_command_handle;

    // check if handle is valid
    if (usb_cmd_is_handle_valid(p_handle))
    {
        p_handle->callback = capture_ready_cb;
        p_handle->stream_context = context;
        result = K4A_RESULT_SUCCEEDED;
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
    libusb_device **dev_list; // pointer to pointer of device, used to retrieve a list of devices
    ssize_t count;            // holding number of devices in list
    int err;

    if (p_device_count == NULL)
    {
        logger_error(LOGGER_USB_CMD, "Error p_device_count is NULL");
        return K4A_RESULT_FAILED;
    }

    *p_device_count = 0;
    // initialize library
    if ((err = libusb_init(NULL)) < 0)
    {
        logger_error(LOGGER_USB_CMD, "Error calling libusb_init, result:%s", libusb_error_name(err));
        return K4A_RESULT_FAILED;
    }

#if (LIBUSB_API_VERSION >= 0x01000106)
    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
#else
    libusb_set_debug(NULL, 3);      // set verbosity level to 3, as suggested in the documentation
#endif

    count = libusb_get_device_list(NULL, &dev_list); // get the list of devices
    if (count > INT32_MAX)
    {
        logger_error(LOGGER_USB_CMD, "List too large");
        return K4A_RESULT_FAILED;
    }
    if (count == 0)
    {
        logger_error(LOGGER_USB_CMD, "No devices found");
        return K4A_RESULT_FAILED;
    }

    // Loop through and count matching VID / PID
    for (int loop = 0; loop < count; loop++)
    {
        if ((err = libusb_get_device_descriptor(dev_list[loop], &desc)) != LIBUSB_SUCCESS)
        {
            logger_error(LOGGER_USB_CMD,
                         "Error calling libusb_get_device_descriptor, result:%s",
                         libusb_error_name(err));
            result = K4A_RESULT_FAILED;
            break;
        }

        //  Just check for one PID assuming the other is in the package
        if ((desc.idVendor == K4A_MSFT_VID) && (desc.idProduct == K4A_RGB_PID))
        {
            *p_device_count += 1;
        }
    }
    // free the list, unref the devices in it
    libusb_free_device_list(dev_list, (int)count);

    // close the instance
    libusb_exit(NULL);

    return result;
}

// Waiting on hot-plugging support
#if 0
/**
 *  Function to get the attachment bus number and port path for a particular handle
 *
 *  @param p_handle
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
k4a_result_t usb_cmd_path_get(usb_command_handle_t p_command_handle, uint8_t *p_bus, uint8_t *p_path, uint8_t path_size)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    usb_cmd_handle_t *p_handle = (usb_cmd_handle_t *)p_command_handle;
    if (p_handle != NULL)
    {
        *p_bus = p_handle->bus;
        // clear path
        memset(p_path, 0, path_size);
        // copy over the path
        if (path_size > USB_CMD_PORT_DEPTH)
        {
            path_size = USB_CMD_PORT_DEPTH;
        }
        memcpy(p_path, p_handle->port_path, path_size);
    }
    else
    {
        logger_error(LOGGER_USB_CMD, "Error p_handle is NULL");
        result = K4A_RESULT_FAILED;
    }
    return result;
}
#endif
