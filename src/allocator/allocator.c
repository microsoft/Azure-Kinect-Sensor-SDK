// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/allocator.h>

// Dependent libraries
#include <k4ainternal/capture.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/refcount.h>

// System dependencies
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

typedef enum
{
    IMAGE_TYPE_COLOR = 0,
    IMAGE_TYPE_DEPTH,
    IMAGE_TYPE_IR,
    IMAGE_TYPE_COUNT,
} image_type_index_t;

//
// Simple counts of memory allocations for the purpose of detecting leaks of the K4A SDK's larger memory objects.
//
// NOTE about using globals vs. an allocated context. In most cases we prefer to avoid globals and instead allocate a
// context that gets passed around. The drawback to an allocated context in this case is how to use one and abstract it
// away from the user such that they can hold onto memory after k4a_device_close has been called, which would ultimately
// need to destroy the context.
//
// Globals have the drawback that they are shared for instances in the same process. So they count the allocations for
// the entire process, not just the current session (k4a_device_open). As a result we must also count the active
// sessions so we can properly report leaks when all active sessions end (k4a_device_close).
static volatile long g_allocated_image_count_user = 0;
static volatile long g_allocated_image_count_color = 0;
static volatile long g_allocated_image_count_depth = 0;
static volatile long g_allocated_image_count_imu = 0;
static volatile long g_allocated_image_count_usb_depth = 0;
static volatile long g_allocated_image_count_usb_imu = 0;

// Count the number of active sessions for this process. A session maps to k4a_device_open
static volatile long g_allocator_sessions = 0;

typedef struct _capture_context_t
{
    volatile long ref_count;
    LOCK_HANDLE lock;

    k4a_image_t image[IMAGE_TYPE_COUNT];

    float temperature_c; /** Temperature in Celsius */
} capture_context_t;

K4A_DECLARE_CONTEXT(k4a_capture_t, capture_context_t);

void allocator_initialize(void)
{
    INC_REF_VAR(g_allocator_sessions);
}

void allocator_deinitialize(void)
{
    DEC_REF_VAR(g_allocator_sessions);
}

uint8_t *allocator_alloc(allocation_source_t source, size_t alloc_size, void **context)
{
    RETURN_VALUE_IF_ARG(NULL, source < ALLOCATION_SOURCE_USER || source > ALLOCATION_SOURCE_USB_IMU);
    RETURN_VALUE_IF_ARG(NULL, alloc_size == 0);
    RETURN_VALUE_IF_ARG(NULL, context == NULL);

    volatile long *ref = NULL;
    switch (source)
    {
    case ALLOCATION_SOURCE_USER:
        ref = &g_allocated_image_count_user;
        break;
    case ALLOCATION_SOURCE_DEPTH:
        ref = &g_allocated_image_count_depth;
        break;
    case ALLOCATION_SOURCE_COLOR:
        ref = &g_allocated_image_count_color;
        break;
    case ALLOCATION_SOURCE_IMU:
        ref = &g_allocated_image_count_imu;
        break;
    case ALLOCATION_SOURCE_USB_DEPTH:
        ref = &g_allocated_image_count_usb_depth;
        break;
    case ALLOCATION_SOURCE_USB_IMU:
        ref = &g_allocated_image_count_usb_imu;
        break;
    default:
        assert(0);
        break;
    }

    INC_REF_VAR(*ref);

    memcpy((uint8_t *)context, &source, sizeof(allocation_source_t));

    return malloc(alloc_size);
}

void allocator_free(void *buffer, void *context)
{
    allocation_source_t source;
    memcpy(&source, (uint8_t *)&context, sizeof(allocation_source_t));

    RETURN_VALUE_IF_ARG(VOID_VALUE, source < ALLOCATION_SOURCE_USER || source > ALLOCATION_SOURCE_USB_IMU);
    RETURN_VALUE_IF_ARG(VOID_VALUE, buffer == NULL);

    volatile long *ref = NULL;

    switch (source)
    {
    case ALLOCATION_SOURCE_USER:
        ref = &g_allocated_image_count_user;
        break;
    case ALLOCATION_SOURCE_DEPTH:
        ref = &g_allocated_image_count_depth;
        break;
    case ALLOCATION_SOURCE_COLOR:
        ref = &g_allocated_image_count_color;
        break;
    case ALLOCATION_SOURCE_IMU:
        ref = &g_allocated_image_count_imu;
        break;
    case ALLOCATION_SOURCE_USB_DEPTH:
        ref = &g_allocated_image_count_usb_depth;
        break;
    case ALLOCATION_SOURCE_USB_IMU:
        ref = &g_allocated_image_count_usb_imu;
        break;
    default:
        assert(0);
        break;
    }

    DEC_REF_VAR(*ref);
    free(buffer);
}

long allocator_test_for_leaks(void)
{
    if (g_allocator_sessions != 0)
    {
        // See comment at the top of the file about counting active sessions
        return 0;
    }

    if (g_allocated_image_count_user || g_allocated_image_count_depth || g_allocated_image_count_color ||
        g_allocated_image_count_imu || g_allocated_image_count_usb_depth || g_allocated_image_count_usb_imu)
    {
        logger_log(K4A_LOG_LEVEL_CRITICAL,
                   __FILE__,
                   __LINE__,
                   "Leaked usr:%d, color:%d, depth:%d, imu:%d, usb depth:%d, usb imu%d",
                   g_allocated_image_count_user,
                   g_allocated_image_count_color,
                   g_allocated_image_count_depth,
                   g_allocated_image_count_imu,
                   g_allocated_image_count_usb_depth,
                   g_allocated_image_count_usb_imu);
    }

    assert(g_allocated_image_count_user == 0);
    assert(g_allocated_image_count_color == 0);
    assert(g_allocated_image_count_depth == 0);
    assert(g_allocated_image_count_imu == 0);
    assert(g_allocated_image_count_usb_depth == 0);
    assert(g_allocated_image_count_usb_imu == 0);

    return g_allocated_image_count_user + g_allocated_image_count_depth + g_allocated_image_count_color +
           g_allocated_image_count_imu + g_allocated_image_count_usb_depth + g_allocated_image_count_usb_imu;
}

void capture_dec_ref(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_capture_t, capture_handle);
    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);
    long new_count;

    new_count = DEC_REF_VAR(capture->ref_count);

    if (new_count == 0)
    {
        Lock(capture->lock);
        for (int x = 0; x < IMAGE_TYPE_COUNT; x++)
        {
            if (capture->image[x])
            {
                image_dec_ref(capture->image[x]);
            }
        }
        Unlock(capture->lock);
        Lock_Deinit(capture->lock);
        k4a_capture_t_destroy(capture_handle);
    }
}

void capture_inc_ref(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_capture_t, capture_handle);
    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    INC_REF_VAR(capture->ref_count);
}

k4a_result_t capture_create(k4a_capture_t *capture_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, capture_handle == NULL);

    k4a_result_t result;
    capture_context_t *capture = k4a_capture_t_create(capture_handle);
    result = K4A_RESULT_FROM_BOOL(capture != NULL);

    if (K4A_SUCCEEDED(result))
    {
        capture->ref_count = 1;
        capture->temperature_c = NAN;
        capture->lock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(capture->lock != NULL);
    }

    if (K4A_FAILED(result) && capture)
    {
        capture_dec_ref(*capture_handle);
        capture_handle = NULL;
    }

    return result;
}

k4a_image_t capture_get_color_image(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NULL, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    Lock(capture->lock);
    k4a_image_t *image = &capture->image[IMAGE_TYPE_COLOR];
    if (*image)
    {
        image_inc_ref(*image);
    }
    Unlock(capture->lock);
    return *image;
}
k4a_image_t capture_get_depth_image(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NULL, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    Lock(capture->lock);
    k4a_image_t *image = &capture->image[IMAGE_TYPE_DEPTH];
    if (*image)
    {
        image_inc_ref(*image);
    }
    Unlock(capture->lock);
    return *image;
}

k4a_image_t capture_get_ir_image(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NULL, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    Lock(capture->lock);
    k4a_image_t *image = &capture->image[IMAGE_TYPE_IR];
    if (*image)
    {
        image_inc_ref(*image);
    }
    Unlock(capture->lock);
    return *image;
}

k4a_image_t capture_get_imu_image(k4a_capture_t capture_handle)
{
    // We just reuse the ir image location as this is never exposed to the user or combined with ir/color/depth.
    return capture_get_ir_image(capture_handle);
}

void capture_set_color_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    Lock(capture->lock);
    k4a_image_t *image = &capture->image[IMAGE_TYPE_COLOR];
    if (*image)
    {
        image_dec_ref(*image); // drop the image that was here
    }
    *image = image_handle;
    if (image_handle != NULL)
    {
        image_inc_ref(*image);
    }
    Unlock(capture->lock);
}
void capture_set_depth_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    Lock(capture->lock);
    k4a_image_t *image = &capture->image[IMAGE_TYPE_DEPTH];
    if (*image)
    {
        image_dec_ref(*image); // drop the image that was here
    }
    *image = image_handle;
    if (image_handle != NULL)
    {
        image_inc_ref(*image);
    }
    Unlock(capture->lock);
}
void capture_set_ir_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);
    Lock(capture->lock);
    k4a_image_t *image = &capture->image[IMAGE_TYPE_IR];
    if (*image)
    {
        image_dec_ref(*image); // drop the image that was here
    }
    *image = image_handle;
    if (image_handle != NULL)
    {
        image_inc_ref(*image);
    }
    Unlock(capture->lock);
}
void capture_set_imu_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    // We just reuse the ir image location as this is never exposed to the user.
    capture_set_ir_image(capture_handle, image_handle);
}

// On Ubuntu 16.04 this works without warnings, but on Ubuntu 18.04 isnan actually
// takes a long double, we get a double-promotion warning here.  Unfortunately,
// isnan has an implementation-defined argument type, so there's not a specific
// type we can cast it to in order to avoid clang's precision warnings, so we
// just need to suppress the warning.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif
void capture_set_temperature_c(k4a_capture_t capture_handle, float temperature_c)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_capture_t, capture_handle);
    RETURN_VALUE_IF_ARG(VOID_VALUE, isnan(temperature_c));

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);
    capture->temperature_c = temperature_c;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

float capture_get_temperature_c(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NAN, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);
    return capture->temperature_c;
}
