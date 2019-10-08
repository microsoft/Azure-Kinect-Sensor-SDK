// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/image.h>
#include <k4ainternal/allocator.h>

// Dependent libraries
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/refcount.h>

// System dependencies
#include <stdlib.h>
#include <assert.h>

#ifndef _WIN32
#include <time.h>
#endif

typedef struct _image_context_t
{
    volatile long ref_count;
    LOCK_HANDLE lock;

    uint8_t *buffer;
    size_t buffer_size;

    k4a_image_format_t format;   /** capture type */
    int width_pixels;            /** width in pixels */
    int height_pixels;           /** height in pixels */
    int stride_bytes;            /** stride in bytes */
    uint64_t dev_timestamp_usec; /** device timestamp in microseconds */
    uint64_t sys_timestamp_nsec; /** system timestamp in nanoseconds */
    uint64_t exposure_time_usec; /** image exposure duration */
    size_t size_allocated;       /** size of the raw memory allocation */

    image_destroy_cb_t *memory_free_cb;
    void *memory_free_cb_context;

    union
    {
        struct
        {
            uint32_t white_balance;
            uint32_t iso_speed;
        } color;
    } metadata;

} image_context_t;

K4A_DECLARE_CONTEXT(k4a_image_t, image_context_t);

k4a_result_t image_create_from_buffer(k4a_image_format_t format,
                                      int width_pixels,
                                      int height_pixels,
                                      int stride_bytes,
                                      uint8_t *buffer,
                                      size_t buffer_size,
                                      image_destroy_cb_t *buffer_destroy_cb,
                                      void *buffer_destroy_cb_context,
                                      k4a_image_t *image_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, image_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, format < K4A_IMAGE_FORMAT_COLOR_MJPG || format > K4A_IMAGE_FORMAT_CUSTOM);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, width_pixels <= 0 || width_pixels > 20000);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, height_pixels <= 0 || height_pixels > 20000);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, buffer == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, buffer_size == 0);

    image_context_t *image;
    k4a_result_t result;

    result = K4A_RESULT_FROM_BOOL((image = k4a_image_t_create(image_handle)) != NULL);

    if (K4A_SUCCEEDED(result))
    {
        image->format = format;
        image->width_pixels = width_pixels;
        image->height_pixels = height_pixels;
        image->stride_bytes = stride_bytes;
        image->buffer_size = buffer_size;
        image->buffer = buffer;
        image->ref_count = 1;
        image->memory_free_cb = buffer_destroy_cb;
        image->memory_free_cb_context = buffer_destroy_cb_context;
        image->lock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(image->lock != NULL);
    }

    //
    // NOTE: Contract is that if we fail this function, buffer is still valid and that caller needs to free the memory.
    // No failure in this function should result in buffer being freed.
    //
    if (K4A_FAILED(result) && *image_handle)
    {
        if (image)
        {
            // Don't free memory caller needs to do that if we fail
            image->buffer = NULL;
        }
        k4a_image_t_destroy(*image_handle);
        *image_handle = NULL;
    }

    return result;
}

static void image_default_free_function(void *buffer, void *context)
{
    (void)context;
    allocator_free(buffer);
}

static k4a_result_t image_create_empty_image(allocation_source_t source, size_t size, k4a_image_t *image_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, image_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, size == 0);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, source < ALLOCATION_SOURCE_USER || source > ALLOCATION_SOURCE_USB_IMU);

    k4a_result_t result;
    image_context_t *image = NULL;

    result = K4A_RESULT_FROM_BOOL((image = k4a_image_t_create(image_handle)) != NULL);

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL((image->buffer = allocator_alloc(source, size)) != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        image->ref_count = 1;
        image->buffer_size = size;
        image->memory_free_cb = image_default_free_function;
        image->memory_free_cb_context = NULL;
        image->lock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(image->lock != NULL);
    }

    if (K4A_FAILED(result))
    {
        image_dec_ref(*image_handle);
        *image_handle = NULL;
    }
    return result;
}

k4a_result_t image_create_empty_internal(allocation_source_t source, size_t size, k4a_image_t *image_handle)
{
    // User is special and only allowed to be used by the user through a public API.
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, image_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, source == ALLOCATION_SOURCE_USER);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, size == 0);

    return image_create_empty_image(source, size, image_handle);
}

k4a_result_t image_create(k4a_image_format_t format,
                          int width_pixels,
                          int height_pixels,
                          int stride_bytes,
                          allocation_source_t source,
                          k4a_image_t *image_handle)
{
    // User is special and only allowed to be used by the user through a public API.
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, image_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED,
                        !(format >= K4A_IMAGE_FORMAT_COLOR_MJPG && format <= K4A_IMAGE_FORMAT_CUSTOM));
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, !(width_pixels > 0 && width_pixels < 20000));
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, !(height_pixels > 0 && height_pixels < 20000));

    image_context_t *image = NULL;
    k4a_result_t result;
    size_t size = 0;

    *image_handle = NULL;

    switch (format)
    {
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
    {
        LOG_ERROR("K4A_IMAGE_FORMAT_COLOR_MJPG does not have a constant stride. Buffer size cannot be calculated.", 0);
        result = K4A_RESULT_FAILED;
        break;
    }

    case K4A_IMAGE_FORMAT_COLOR_NV12:
    {
        if (stride_bytes == 0)
        {
            // If stride isn't specified, assume the minimum stride
            stride_bytes = width_pixels;
        }

        if (height_pixels % 2 != 0)
        {
            LOG_ERROR("NV12 requires an even number of lines. Height %d is invalid.", height_pixels);
            result = K4A_RESULT_FAILED;
        }
        else if (width_pixels % 2 != 0)
        {
            LOG_ERROR("NV12 requires an even number of pixels per line. Width of %d is invalid.", width_pixels);
            result = K4A_RESULT_FAILED;
        }
        else if (stride_bytes < 1 * width_pixels)
        {
            LOG_ERROR("Insufficient stride (%d bytes) to represent image width (%d pixels).",
                      stride_bytes,
                      width_pixels);
            result = K4A_RESULT_FAILED;
        }
        else
        {
            // Calculate correct size for NV12 (extra color lines follow Y samples)
            size = 3 * (size_t)height_pixels * (size_t)stride_bytes / 2;
            result = K4A_RESULT_SUCCEEDED;
        }
        break;
    }

    // 1 Byte per pixel
    case K4A_IMAGE_FORMAT_CUSTOM8:
    {
        if (stride_bytes == 0)
        {
            // If stride isn't specified, assume the minimum stride
            stride_bytes = width_pixels;
        }

        if (stride_bytes < 1 * width_pixels)
        {
            LOG_ERROR("Insufficient stride (%d bytes) to represent image width (%d pixels).",
                      stride_bytes,
                      width_pixels);
            result = K4A_RESULT_FAILED;
        }
        else
        {
            size = (size_t)height_pixels * (size_t)stride_bytes;
            result = K4A_RESULT_SUCCEEDED;
        }
        break;
    }

    // 2 Bytes per pixel
    case K4A_IMAGE_FORMAT_DEPTH16:
    case K4A_IMAGE_FORMAT_IR16:
    case K4A_IMAGE_FORMAT_CUSTOM16:
    {
        if (stride_bytes == 0)
        {
            // If stride isn't specified, assume the minimum stride
            stride_bytes = width_pixels * 2;
        }

        if (stride_bytes < 2 * width_pixels)
        {
            LOG_ERROR("Insufficient stride (%d bytes) to represent image width (%d pixels).",
                      stride_bytes,
                      width_pixels);
            result = K4A_RESULT_FAILED;
        }
        else
        {
            size = (size_t)height_pixels * (size_t)stride_bytes;
            result = K4A_RESULT_SUCCEEDED;
        }
        break;
    }

    // 2 Bytes per pixel
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
    {
        if (stride_bytes == 0)
        {
            // If stride isn't specified, assume the minimum stride
            stride_bytes = width_pixels * 2;
        }

        if (width_pixels % 2 != 0)
        {
            LOG_ERROR("YUY2 requires an even number of pixels per line. Width of %d is invalid.", width_pixels);
            result = K4A_RESULT_FAILED;
        }
        else if (stride_bytes < 2 * width_pixels)
        {
            LOG_ERROR("Insufficient stride (%d bytes) to represent image width (%d pixels).",
                      stride_bytes,
                      width_pixels);
            result = K4A_RESULT_FAILED;
        }
        else
        {
            size = (size_t)height_pixels * (size_t)stride_bytes;
            result = K4A_RESULT_SUCCEEDED;
        }
        break;
    }

    // 4 Bytes per pixel
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
    {
        if (stride_bytes == 0)
        {
            // If stride isn't specified, assume the minimum stride
            stride_bytes = width_pixels * 4;
        }

        if (stride_bytes < 4 * width_pixels)
        {
            LOG_ERROR("Insufficient stride (%d bytes) to represent image width (%d pixels).",
                      stride_bytes,
                      width_pixels);
            result = K4A_RESULT_FAILED;
        }
        else
        {
            size = (size_t)height_pixels * (size_t)stride_bytes;
            result = K4A_RESULT_SUCCEEDED;
        }
        break;
    }

    // Unknown
    case K4A_IMAGE_FORMAT_CUSTOM:
    default:
    {
        size = (size_t)height_pixels * (size_t)stride_bytes;
        result = K4A_RESULT_SUCCEEDED;
        break;
    }
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(image_create_empty_image(source, size, image_handle));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL((image = k4a_image_t_get_context(*image_handle)) != NULL);
    }
    if (K4A_SUCCEEDED(result))
    {
        image->format = format;
        image->width_pixels = width_pixels;
        image->height_pixels = height_pixels;
        image->stride_bytes = stride_bytes;
    }

    if (K4A_FAILED(result) && *image_handle)
    {
        image_dec_ref(*image_handle);
        *image_handle = NULL;
        image = NULL;
    }

    return result;
}

void image_dec_ref(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);

    long count = DEC_REF_VAR(image->ref_count);

    if (count == 0)
    {
        if (image->memory_free_cb)
        {
            image->memory_free_cb(image->buffer, image->memory_free_cb_context);
        }
        Lock_Deinit(image->lock);
        k4a_image_t_destroy(image_handle);
    }
}

void image_inc_ref(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);

    INC_REF_VAR(image->ref_count);
}

uint8_t *image_get_buffer(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(NULL, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->buffer;
}

size_t image_get_size(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->buffer_size;
}

void image_set_size(k4a_image_t image_handle, size_t size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);

    // We should only be reducing the size
    assert(image->buffer_size >= size);
    image->buffer_size = size;
}

k4a_image_format_t image_get_format(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_IMAGE_FORMAT_CUSTOM, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->format;
}

int image_get_width_pixels(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->width_pixels;
}

int image_get_height_pixels(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->height_pixels;
}

int image_get_stride_bytes(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->stride_bytes;
}

uint64_t image_get_device_timestamp_usec(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->dev_timestamp_usec;
}

uint64_t image_get_system_timestamp_nsec(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->sys_timestamp_nsec;
}

uint64_t image_get_exposure_usec(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->exposure_time_usec;
}

uint32_t image_get_white_balance(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->metadata.color.white_balance;
}

uint32_t image_get_iso_speed(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(0, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    return image->metadata.color.iso_speed;
}

void image_set_device_timestamp_usec(k4a_image_t image_handle, uint64_t timestamp_usec)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    image->dev_timestamp_usec = timestamp_usec;
}

void image_set_system_timestamp_nsec(k4a_image_t image_handle, uint64_t timestamp_nsec)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    image->sys_timestamp_nsec = timestamp_nsec;
}

k4a_result_t image_apply_system_timestamp(k4a_image_t image_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    k4a_result_t result;

#ifdef _WIN32
    LARGE_INTEGER qpc = { 0 }, freq = { 0 };
    result = K4A_RESULT_FROM_BOOL(QueryPerformanceCounter(&qpc) != 0);
    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(QueryPerformanceFrequency(&freq) != 0);
    }

    if (K4A_SUCCEEDED(result))
    {
        // Calculate seconds in such a way we minimize overflow.
        // Rollover happens, for a 1MHz Freq, when qpc.QuadPart > 0x003F FFFF FFFF FFFF; ~571 Years after boot.
        image->sys_timestamp_nsec = qpc.QuadPart / freq.QuadPart * 1000000000;
        image->sys_timestamp_nsec += qpc.QuadPart % freq.QuadPart * 1000000000 / freq.QuadPart;
    }
#else
    struct timespec ts_time;
    result = K4A_RESULT_FROM_BOOL(clock_gettime(CLOCK_MONOTONIC, &ts_time) == 0);
    if (K4A_SUCCEEDED(result))
    {
        // Rollover happens about ~136 years after boot.
        image->sys_timestamp_nsec = (uint64_t)ts_time.tv_sec * 1000000000 + (uint64_t)ts_time.tv_nsec;
    }
#endif

    return result;
}

void image_set_exposure_usec(k4a_image_t image_handle, uint64_t exposure_usec)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    image->exposure_time_usec = exposure_usec;
}

void image_set_white_balance(k4a_image_t image_handle, uint32_t white_balance)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    image->metadata.color.white_balance = white_balance;
}

void image_set_iso_speed(k4a_image_t image_handle, uint32_t iso_speed)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_image_t, image_handle);
    image_context_t *image = k4a_image_t_get_context(image_handle);
    image->metadata.color.iso_speed = iso_speed;
}
