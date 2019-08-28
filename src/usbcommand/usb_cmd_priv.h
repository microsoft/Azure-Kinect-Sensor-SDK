// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/**
 * Private header file for the command and streaming interface
 */

#pragma once

//************************ Includes *****************************
#include <inttypes.h>
#include <stdbool.h>
// This library
#include <k4ainternal/usbcommand.h>
#include <k4ainternal/common.h>

// Dependent libraries
#include <k4ainternal/allocator.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/threadapi.h>

// Exteranl dependencis
#include <libusb.h>

// Ensure we have LIBUSB_API_VERSION defined if not defined by libusb.h
#ifndef LIBUSB_API_VERSION
#define LIBUSB_API_VERSION 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

//**************Symbolic Constant Macros (defines)  *************
#define USB_CMD_MAX_WAIT_TIME 2000
#define USB_CMD_MAX_XFR_COUNT 8 // Upper limit to the number of outstanding transfer
#ifdef _WIN32
#define USB_CMD_MAX_XFR_POOL 80000000 // Memory pool size for outstanding transfers (based on empirical testing)
#else
#define USB_CMD_MAX_XFR_POOL 10000000 // Memory pool size for outstanding transfers (based on empirical testing)
#endif
#define USB_CMD_PORT_DEPTH 8

#define USB_CMD_EVENT_WAIT_TIME 1
#define USB_MAX_TX_DATA 128
#define USB_CMD_PACKET_TYPE 0x06022009
#define USB_CMD_PACKET_TYPE_RESPONSE 0x0A6FE000
#define K4A_MSFT_VID 0x045E
#define K4A_RGB_PID 0x097D
#define K4A_DEPTH_PID 0x097C
#define USB_CMD_DEFAULT_CONFIG 1

#define USB_CMD_DEPTH_INTERFACE 0
#define USB_CMD_DEPTH_IN_ENDPOINT 0x02
#define USB_CMD_DEPTH_OUT_ENDPOINT 0x81
#define USB_CMD_DEPTH_STREAM_ENDPOINT 0x83

#define USB_CMD_IMU_INTERFACE 2
#define USB_CMD_IMU_IN_ENDPOINT 0x04
#define USB_CMD_IMU_OUT_ENDPOINT 0x83
#define USB_CMD_IMU_STREAM_ENDPOINT 0x82

//************************ Typedefs *****************************
typedef struct _usb_async_transfer_data_t
{
    struct _usbcmd_context_t *usbcmd;
    struct libusb_transfer *bulk_transfer;
    k4a_image_t image;
    uint32_t list_index;
} usb_async_transfer_data_t;

typedef struct _usbcmd_context_t
{
    allocation_source_t source;

    // LIBUSB properties
    libusb_device_handle *libusb;
    libusb_context *libusb_context;
    enum libusb_log_level libusb_verbosity;

    uint8_t index;
    uint16_t pid;
    uint8_t interface;
    uint8_t cmd_tx_endpoint;
    uint8_t cmd_rx_endpoint;
    uint8_t stream_endpoint;
    uint32_t transaction_id;

    unsigned char serial_number[MAX_SERIAL_NUMBER_LENGTH];
    guid_t container_id;

    usb_cmd_stream_cb_t *callback;
    void *stream_context;
    bool stream_going;
    usb_async_transfer_data_t *transfer_list[USB_CMD_MAX_XFR_COUNT];
    size_t stream_size;
    LOCK_HANDLE lock;
    THREAD_HANDLE stream_handle;
} usbcmd_context_t;

K4A_DECLARE_CONTEXT(usbcmd_t, usbcmd_context_t);

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************
void LIBUSB_CALL usb_cmd_libusb_cb(struct libusb_transfer *bulk_transfer);

#ifdef __cplusplus
}
#endif
