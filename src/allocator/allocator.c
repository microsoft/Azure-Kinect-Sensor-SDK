// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/allocator.h>

// Dependent libraries
#include <k4ainternal/capture.h>
#include <k4ainternal/global.h>
#include <k4ainternal/rwlock.h>
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

// Global properties of the allocator
typedef struct
{
    k4a_rwlock_t lock;

    // Access to these function pointers may only occur
    // while holding lock
    k4a_memory_allocate_cb_t *alloc;
    k4a_memory_destroy_cb_t *free;
} allocator_global_t;

// This allocator implementation is used by default
static uint8_t *default_alloc(int size, void **context)
{
    *context = NULL;
    if (size < 0)
    {
        return NULL;
    }
    return (uint8_t *)malloc((size_t)size);
}

// This is the free function for the default allocator
static void default_free(void *buffer, void *context)
{
    (void)context;
    assert(context == NULL);
    free(buffer);
}

// This is a one time initialization of the global state for the allocator
static void allocator_global_init(allocator_global_t *g_allocator)
{
    rwlock_init(&g_allocator->lock);

    g_allocator->alloc = default_alloc;
    g_allocator->free = default_free;
}

// The allocation context is pre-pended to memory returned by the allocator
// This state is used to track the freeing of the allocation
typedef struct _allocation_context_t
{
    union _allocator_context
    {
        struct _context
        {
            allocation_source_t source;
            k4a_memory_destroy_cb_t *free;
            void *free_context;
        } context;

        // Keep 16 byte alignment so that allocations may be used with SSE
        char alignment[32];
    } u;
} allocation_context_t;

K4A_DECLARE_GLOBAL(allocator_global_t, allocator_global_init);

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
    k4a_rwlock_t lock;

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

k4a_result_t allocator_set_allocator(k4a_memory_allocate_cb_t allocate, k4a_memory_destroy_cb_t free)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, allocate == NULL && free != NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, allocate != NULL && free == NULL);

    allocator_global_t *g_allocator = allocator_global_t_get();
    rwlock_acquire_write(&g_allocator->lock);

    g_allocator->alloc = allocate ? allocate : default_alloc;
    g_allocator->free = free ? free : default_free;

    rwlock_release_write(&g_allocator->lock);

    return K4A_RESULT_SUCCEEDED;
}

uint8_t *allocator_alloc(allocation_source_t source, size_t alloc_size)
{
    allocator_global_t *g_allocator = allocator_global_t_get();

    RETURN_VALUE_IF_ARG(NULL, source < ALLOCATION_SOURCE_USER || source > ALLOCATION_SOURCE_USB_IMU);
    RETURN_VALUE_IF_ARG(NULL, alloc_size == 0);

    size_t required_bytes = alloc_size + sizeof(allocation_context_t);
    RETURN_VALUE_IF_ARG(NULL, required_bytes > INT32_MAX);

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

    rwlock_acquire_read(&g_allocator->lock);

    void *user_context;

    void *full_buffer = g_allocator->alloc((int)required_bytes, &user_context);

    // Store information about the allocation that we will need during free.
    allocation_context_t allocation_context;

    allocation_context.u.context.source = source;
    allocation_context.u.context.free = g_allocator->free;
    allocation_context.u.context.free_context = user_context;

    rwlock_release_read(&g_allocator->lock);

    if (full_buffer == NULL)
    {
        LOG_ERROR("User allocation function for %d bytes failed", required_bytes);
        return NULL;
    }

    // Memcpy the context information to the header of the full buffer.
    // Don't cast the buffer directly since there is no alignment constraint
    memcpy(full_buffer, &allocation_context, sizeof(allocation_context));

    // Provide the caller with the buffer after the allocation context header.
    uint8_t *buffer = (uint8_t *)full_buffer + sizeof(allocation_context_t);

    return buffer;
}

void allocator_free(void *buffer)
{
    void *full_buffer = (uint8_t *)buffer - sizeof(allocation_context_t);
    allocation_context_t allocation_context;
    memcpy(&allocation_context, full_buffer, sizeof(allocation_context));

    allocation_source_t source = allocation_context.u.context.source;

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

    allocation_context.u.context.free(full_buffer, allocation_context.u.context.free_context);
    full_buffer = NULL;
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
        rwlock_acquire_write(&capture->lock);
        for (int x = 0; x < IMAGE_TYPE_COUNT; x++)
        {
            if (capture->image[x])
            {
                image_dec_ref(capture->image[x]);
            }
        }
        rwlock_release_write(&capture->lock);
        rwlock_deinit(&capture->lock);
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

    capture_context_t *capture = k4a_capture_t_create(capture_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(capture != NULL);

    if (K4A_SUCCEEDED(result))
    {
        capture->ref_count = 1;
        capture->temperature_c = NAN;
        rwlock_init(&capture->lock);
    }

    return result;
}

k4a_image_t capture_get_color_image(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NULL, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    rwlock_acquire_read(&capture->lock);
    k4a_image_t *image = &capture->image[IMAGE_TYPE_COLOR];
    if (*image)
    {
        image_inc_ref(*image);
    }
    rwlock_release_read(&capture->lock);
    return *image;
}
k4a_image_t capture_get_depth_image(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NULL, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    rwlock_acquire_read(&capture->lock);
    k4a_image_t *image = &capture->image[IMAGE_TYPE_DEPTH];
    if (*image)
    {
        image_inc_ref(*image);
    }
    rwlock_release_read(&capture->lock);
    return *image;
}

k4a_image_t capture_get_ir_image(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NULL, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    rwlock_acquire_read(&capture->lock);
    k4a_image_t *image = &capture->image[IMAGE_TYPE_IR];
    if (*image)
    {
        image_inc_ref(*image);
    }
    rwlock_release_read(&capture->lock);
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

    rwlock_acquire_write(&capture->lock);
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
    rwlock_release_write(&capture->lock);
}
void capture_set_depth_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);

    rwlock_acquire_write(&capture->lock);
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
    rwlock_release_write(&capture->lock);
}
void capture_set_ir_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);
    rwlock_acquire_write(&capture->lock);
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
    rwlock_release_write(&capture->lock);
}
void capture_set_imu_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    // We just reuse the ir image location as this is never exposed to the user.
    capture_set_ir_image(capture_handle, image_handle);
}

void capture_set_temperature_c(k4a_capture_t capture_handle, float temperature_c)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_capture_t, capture_handle);
    RETURN_VALUE_IF_ARG(VOID_VALUE, isnan(temperature_c));

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);
    capture->temperature_c = temperature_c;
}

float capture_get_temperature_c(k4a_capture_t capture_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NAN, k4a_capture_t, capture_handle);

    capture_context_t *capture = k4a_capture_t_get_context(capture_handle);
    return capture->temperature_c;
}
