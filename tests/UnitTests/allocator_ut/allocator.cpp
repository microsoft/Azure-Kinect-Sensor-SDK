// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include <math.h>

#include <gtest/gtest.h>

#include <k4ainternal/allocator.h>
#include <k4ainternal/capture.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}

#define GTEST_LOG_ERROR std::cout << "[    ERROR ] "

typedef struct _allocator_thread_adjust_ref_data_t
{
    k4a_capture_t capture;
    uint32_t test_case;

    uint32_t error;
    LOCK_HANDLE lock;
    volatile uint32_t done_event;
} allocator_thread_adjust_ref_data_t;

#define TEST_RETURN_VALUE (22)

static k4a_capture_t capture_manufacture(size_t size, bool depth)
{
    k4a_result_t result;
    k4a_capture_t capture = NULL;
    k4a_image_t image = NULL;

    result = TRACE_CALL(capture_create(&capture));
    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(image_create_empty_internal(ALLOCATION_SOURCE_IMU, size, &image));
    }
    if (K4A_SUCCEEDED(result))
    {
        if (depth)
        {
            capture_set_depth_image(capture, image);
        }
        else
        {
            capture_set_color_image(capture, image);
        }
    }

    if (K4A_FAILED(result))
    {
        capture_dec_ref(capture);
        capture = NULL;
    }

    if (image)
    {
        image_dec_ref(image);
        image = NULL;
    }
    return capture;
}

TEST(allocator_ut, allocator_api_validation)
{
    // allocator_t allocator;
    k4a_capture_t capture_d;
    k4a_capture_t capture_c;

    capture_d = capture_manufacture(sizeof(k4a_imu_sample_t), true);
    ASSERT_NE((k4a_capture_t)NULL, capture_d);
    capture_c = capture_manufacture(sizeof(k4a_imu_sample_t), false);
    ASSERT_NE((k4a_capture_t)NULL, capture_c);

    // Get/Set time stamps
    {
        k4a_image_t image_c;
        k4a_image_t image_d;
        ASSERT_NE((image_c = capture_get_color_image(capture_c)), (k4a_image_t)NULL);
        ASSERT_NE((image_d = capture_get_depth_image(capture_d)), (k4a_image_t)NULL);

        image_set_device_timestamp_usec(image_d, 0x1234);
        image_set_device_timestamp_usec(image_c, 0x5678);
        image_set_device_timestamp_usec(NULL, 0x2222);
        image_set_device_timestamp_usec(NULL, 0x1111);
        image_set_device_timestamp_usec(image_d, 0); // should be rejected for being 0
        image_set_device_timestamp_usec(image_c, 0); // should be rejected for being 0

        image_dec_ref(image_c);
        image_dec_ref(image_d);
    }

    {
        capture_set_temperature_c(capture_d, 100.0f);
        capture_set_temperature_c(capture_c, 50.0f);
        capture_set_temperature_c(NULL, 1234.0f);

        ASSERT_TRUE(std::isnan(capture_get_temperature_c(NULL)));
        ASSERT_EQ(100.0f, capture_get_temperature_c(capture_d));
        ASSERT_EQ(50.0f, capture_get_temperature_c(capture_c));
    }

    {
        k4a_image_t image;

        ASSERT_EQ((image = capture_get_color_image(NULL)), (k4a_image_t)NULL);
        ASSERT_EQ((image = capture_get_depth_image(NULL)), (k4a_image_t)NULL);
        ASSERT_EQ((image = capture_get_imu_image(NULL)), (k4a_image_t)NULL);
        ASSERT_EQ((image = capture_get_ir_image(NULL)), (k4a_image_t)NULL);

        ASSERT_NE((image = capture_get_color_image(capture_c)), (k4a_image_t)NULL);
        capture_set_color_image(capture_c, NULL);
        capture_set_color_image(NULL, image);
        capture_set_color_image(NULL, NULL);
        capture_set_depth_image(capture_c, NULL);
        capture_set_depth_image(NULL, image);
        capture_set_depth_image(NULL, NULL);
        capture_set_imu_image(capture_c, NULL);
        capture_set_imu_image(NULL, image);
        capture_set_imu_image(NULL, NULL);
        capture_set_ir_image(capture_c, NULL);
        capture_set_ir_image(NULL, image);
        capture_set_ir_image(NULL, NULL);

        // Free capture
        capture_dec_ref(capture_c);
        capture_c = NULL;

        // Assign color capture to this frame
        capture_set_color_image(capture_d, image);
        image_dec_ref(image);
        image = NULL;

        k4a_image_t image_c, image_d;
        ASSERT_NE((image_c = capture_get_color_image(capture_d)), (k4a_image_t)NULL);
        ASSERT_NE((image_d = capture_get_depth_image(capture_d)), (k4a_image_t)NULL);
        ASSERT_NE(image_c, image_d);
        image_dec_ref(image_c);
        image_dec_ref(image_d);
    }

    capture_dec_ref(capture_d);
    ASSERT_EQ(allocator_test_for_leaks(), 0);
}

static void image_free_function(void *buffer, void *context)
{
    (void)context;
    allocator_free(buffer);
}

TEST(allocator_ut, image_api_validation)
{
    uint8_t *buffer = NULL;
    k4a_image_t image = NULL;
    const int IMAGE_SIZE = 128;
    void *context = NULL;

    {
        allocation_source_t source_depth = ALLOCATION_SOURCE_DEPTH;
        allocation_source_t source_bad = (allocation_source_t)99;
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_empty_internal(source_bad, IMAGE_SIZE, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_empty_internal(source_depth, 0, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_empty_internal(source_depth, IMAGE_SIZE, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_empty_internal(source_bad, 0, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_empty_internal(source_depth, 0, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_empty_internal(source_bad, IMAGE_SIZE, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_empty_internal(source_bad, 0, NULL));

        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_create_empty_internal(source_depth, IMAGE_SIZE, &image));
        image_dec_ref(image);
        ASSERT_EQ(allocator_test_for_leaks(), 0);
    }

    {
        // Unsupported formats for this creation API.

        // Invalid FORMAT
        k4a_image_format_t bad_format = (k4a_image_format_t)99;
        ASSERT_EQ(K4A_RESULT_FAILED, image_create(bad_format, 10, 10, 1, ALLOCATION_SOURCE_USER, &image));

        // No output argument
        ASSERT_EQ(K4A_RESULT_FAILED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 1, ALLOCATION_SOURCE_USER, NULL));

        // Stride length
        // Validate a valid length and an invalid length

        // NV12
        //   Minimum stride
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 10, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(10 * 10 * 3 / 2, (int)image_get_size(image));
        image_dec_ref(image);
        //   Extra stride
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 11, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(10 * 11 * 3 / 2, (int)image_get_size(image));
        image_dec_ref(image);
        //   Insufficient stride
        ASSERT_EQ(K4A_RESULT_FAILED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 9, ALLOCATION_SOURCE_USER, &image));
        //   Odd number of rows
        ASSERT_EQ(K4A_RESULT_FAILED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 11, 20, ALLOCATION_SOURCE_USER, &image));
        //   Odd number of columns
        ASSERT_EQ(K4A_RESULT_FAILED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_NV12, 11, 10, 20, ALLOCATION_SOURCE_USER, &image));
        // Stride of zero (should succeed and infer the minimum stride)
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 0, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(10, image_get_stride_bytes(image));
        image_dec_ref(image);

        // YUY2
        //   Minimum stride
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_YUY2, 10, 10, 20, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(/* lines * stride*/ 10 * 20, (int)image_get_size(image));
        image_dec_ref(image);
        //   Extra stride
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_YUY2, 10, 10, 22, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(/* lines * stride*/ 10 * 22, (int)image_get_size(image));
        image_dec_ref(image);
        //   Insufficient stride
        ASSERT_EQ(K4A_RESULT_FAILED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_YUY2, 10, 10, 19, ALLOCATION_SOURCE_USER, &image));
        //   Odd number of rows
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_YUY2, 10, 11, 20, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(/* lines * stride*/ 11 * 20, (int)image_get_size(image));
        image_dec_ref(image);
        //   Odd number of columns
        ASSERT_EQ(K4A_RESULT_FAILED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_YUY2, 11, 10, 20, ALLOCATION_SOURCE_USER, &image));
        // Stride of zero (should succeed and infer the minimum stride)
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_YUY2, 10, 10, 0, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(10 * 2, image_get_stride_bytes(image));
        image_dec_ref(image);

        // BGRA32
        //   Minimum stride
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32, 10, 10, 40, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(/* lines * stride*/ 10 * 40, (int)image_get_size(image));
        image_dec_ref(image);
        //   Insufficient stride
        ASSERT_EQ(K4A_RESULT_FAILED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32, 10, 10, 39, ALLOCATION_SOURCE_USER, &image));
        // Stride of zero (should succeed and infer the minimum stride)
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32, 10, 10, 0, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(10 * 4, image_get_stride_bytes(image));
        image_dec_ref(image);

        // MJPEG (no length is valid)
        ASSERT_EQ(K4A_RESULT_FAILED,
                  image_create(K4A_IMAGE_FORMAT_COLOR_MJPG, 10, 10, 100, ALLOCATION_SOURCE_USER, &image));

        // DEPTH16
        //   Minimum stride
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_DEPTH16, 10, 10, 20, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(10 * 10 * 2, (int)image_get_size(image));
        image_dec_ref(image);
        //   Insufficient stride
        ASSERT_EQ(K4A_RESULT_FAILED,
                  image_create(K4A_IMAGE_FORMAT_DEPTH16, 10, 10, 19, ALLOCATION_SOURCE_USER, &image));
        // Stride of zero (should succeed and infer the minimum stride)
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_DEPTH16, 10, 10, 0, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(10 * 2, image_get_stride_bytes(image));
        image_dec_ref(image);

        // Custom8
        //   Minimum stride
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_CUSTOM8, 10, 10, 10, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(10 * 10, (int)image_get_size(image));
        image_dec_ref(image);
        //   Insufficient stride
        ASSERT_EQ(K4A_RESULT_FAILED,
                  (int)image_create(K4A_IMAGE_FORMAT_CUSTOM8, 10, 10, 9, ALLOCATION_SOURCE_USER, &image));
        // Stride of zero (should succeed and infer the minimum stride)
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create(K4A_IMAGE_FORMAT_CUSTOM8, 10, 10, 0, ALLOCATION_SOURCE_USER, &image));
        ASSERT_EQ(10 * 1, image_get_stride_bytes(image));
        image_dec_ref(image);

        // Height of zero
        ASSERT_EQ(K4A_RESULT_FAILED,
                  (int)image_create(K4A_IMAGE_FORMAT_CUSTOM8, 10, 0, 10, ALLOCATION_SOURCE_USER, &image));

        // Width of zero
        ASSERT_EQ(K4A_RESULT_FAILED,
                  (int)image_create(K4A_IMAGE_FORMAT_CUSTOM8, 0, 10, 10, ALLOCATION_SOURCE_USER, &image));

        ASSERT_EQ(allocator_test_for_leaks(), 0);
    }

    {
        k4a_image_format_t bad_format = (k4a_image_format_t)99;

        // clang-format off
        ASSERT_NE((uint8_t*)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));

        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,   10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, NULL));

        //ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 0, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 0, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 0,  1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 0,  1, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 0,  0, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 0,  0, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 0,  10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 0,  10, 1, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 0,  10, 0, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 0,  10, 0, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 0,  0,  1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 0,  0,  1, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 0,  0,  0, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 0,  0,  0, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  10, 10, 0, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  10, 10, 0, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  10, 0,  1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  10, 0,  1, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  10, 0,  0, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  10, 0,  0, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  0,  10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  0,  10, 1, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  0,  10, 0, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  0,  10, 0, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  0,  0,  1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  0,  0,  1, buffer, IMAGE_SIZE, image_free_function, context, NULL));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  0,  0,  0, buffer, IMAGE_SIZE, image_free_function, context, &image));
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(bad_format,                  0,  0,  0, buffer, IMAGE_SIZE, image_free_function, context, NULL));

        // No buffer
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 1, NULL,   IMAGE_SIZE, image_free_function, context, &image));
        // bad size
        ASSERT_EQ(K4A_RESULT_FAILED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 1, buffer, 0,          image_free_function, context, &image));

        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_MJPG, 10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        image_dec_ref(image);

        ASSERT_NE((uint8_t*)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        image_dec_ref(image);

        ASSERT_NE((uint8_t*)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_YUY2, 10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        image_dec_ref(image);

        ASSERT_NE((uint8_t*)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_BGRA32, 10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        image_dec_ref(image);

        ASSERT_NE((uint8_t*)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_create_from_buffer(K4A_IMAGE_FORMAT_DEPTH16, 10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        image_dec_ref(image);

        ASSERT_NE((uint8_t*)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_create_from_buffer(K4A_IMAGE_FORMAT_IR16, 10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        image_dec_ref(image);

        ASSERT_NE((uint8_t*)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_create_from_buffer(K4A_IMAGE_FORMAT_CUSTOM, 10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));
        image_dec_ref(image);

        ASSERT_NE((uint8_t*)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_MJPG, 10, 10, 1, buffer, IMAGE_SIZE, NULL, NULL, &image));
        image_dec_ref(image);
        allocator_free(buffer);
        ASSERT_EQ(allocator_test_for_leaks(), 0);

        // clang-format on
    }

    {
        ASSERT_NE((uint8_t *)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12,
                                           10,
                                           10,
                                           1,
                                           buffer,
                                           IMAGE_SIZE,
                                           image_free_function,
                                           context,
                                           &image));

        ASSERT_EQ((uint8_t *)NULL, image_get_buffer(NULL));
        ASSERT_EQ(buffer, image_get_buffer(image));

        ASSERT_EQ((size_t)0, image_get_size(NULL));
        ASSERT_EQ((size_t)IMAGE_SIZE, image_get_size(image));

        image_set_size(NULL, 5);
        ASSERT_EQ((size_t)IMAGE_SIZE, image_get_size(image));
        image_set_size(image, 5);
        ASSERT_EQ((size_t)5, image_get_size(image));

        ASSERT_EQ(K4A_IMAGE_FORMAT_CUSTOM, image_get_format(NULL));
        ASSERT_EQ(K4A_IMAGE_FORMAT_COLOR_NV12, image_get_format(image));

        ASSERT_EQ(0, image_get_width_pixels(NULL));
        ASSERT_EQ(10, image_get_width_pixels(image));

        ASSERT_EQ(0, image_get_height_pixels(NULL));
        ASSERT_EQ(10, image_get_height_pixels(image));

        ASSERT_EQ(0, image_get_stride_bytes(NULL));
        ASSERT_EQ(1, image_get_stride_bytes(image));
        ASSERT_EQ(0, image_get_device_timestamp_usec(NULL));
        ASSERT_EQ(0, image_get_device_timestamp_usec(image));
        image_set_device_timestamp_usec(NULL, 10);
        ASSERT_EQ(0, image_get_device_timestamp_usec(image));
        image_set_device_timestamp_usec(image, 10); // should succeed
        ASSERT_EQ(10, image_get_device_timestamp_usec(image));
        image_set_device_timestamp_usec(image, 0); // should succeed
        ASSERT_EQ(0, image_get_device_timestamp_usec(image));

        ASSERT_EQ(0, image_get_exposure_usec(NULL));
        ASSERT_EQ(0, image_get_exposure_usec(image));
        image_set_exposure_usec(NULL, 10);
        ASSERT_EQ(0, image_get_exposure_usec(image));
        image_set_exposure_usec(image, 10); // should succeed
        ASSERT_EQ(10, image_get_exposure_usec(image));

        ASSERT_EQ((uint32_t)0, image_get_white_balance(NULL));
        ASSERT_EQ((uint32_t)0, image_get_white_balance(image));
        image_set_white_balance(NULL, 10);
        ASSERT_EQ((uint32_t)0, image_get_white_balance(image));
        image_set_white_balance(image, 10); // should succeed
        ASSERT_EQ((uint32_t)10, image_get_white_balance(image));

        ASSERT_EQ((uint32_t)0, image_get_iso_speed(NULL));
        ASSERT_EQ((uint32_t)0, image_get_iso_speed(image));
        image_set_iso_speed(NULL, 10);
        ASSERT_EQ((uint32_t)0, image_get_iso_speed(image));
        image_set_iso_speed(image, 10); // should succeed
        ASSERT_EQ((uint32_t)10, image_get_iso_speed(image));

        image_dec_ref(image);
        ASSERT_EQ(allocator_test_for_leaks(), 0);
    }

    {
        ASSERT_NE((uint8_t *)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12,
                                           10,
                                           10,
                                           1,
                                           buffer,
                                           IMAGE_SIZE,
                                           image_free_function,
                                           context,
                                           &image));

        image_inc_ref(NULL);
        image_inc_ref(NULL);
        image_inc_ref(image);
        image_inc_ref(image);
        image_inc_ref(image);
        image_dec_ref(image);
        image_dec_ref(image);
        image_dec_ref(image);

        image_inc_ref(image);
        image_dec_ref(image);
        image_inc_ref(image);
        image_dec_ref(image);
        image_inc_ref(image);
        image_dec_ref(image);

        image_dec_ref(NULL);
        image_dec_ref(NULL);

        image_dec_ref(image);
        ASSERT_EQ(allocator_test_for_leaks(), 0);
    }

    {
        ASSERT_EQ(K4A_RESULT_FAILED, image_apply_system_timestamp(nullptr));
        ASSERT_EQ(0, image_get_system_timestamp_nsec(nullptr));
        image_set_system_timestamp_nsec(nullptr, 0);

        ASSERT_NE((uint8_t *)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED,
                  image_create_from_buffer(K4A_IMAGE_FORMAT_COLOR_NV12,
                                           10,
                                           10,
                                           1,
                                           buffer,
                                           IMAGE_SIZE,
                                           image_free_function,
                                           context,
                                           &image));

        ASSERT_EQ(0, image_get_system_timestamp_nsec(image));
        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_apply_system_timestamp(image));
        ASSERT_NE(0, image_get_system_timestamp_nsec(image));
        image_set_system_timestamp_nsec(image, 1);
        ASSERT_EQ(1, image_get_system_timestamp_nsec(image));

        image_dec_ref(image);
        ASSERT_EQ(allocator_test_for_leaks(), 0);
    }

    ASSERT_EQ(allocator_test_for_leaks(), 0);
}

//
// This test is sensitive to workload and should be a manual test.
//
TEST(allocator_ut, manual_image_system_time)
{
    uint8_t *buffer = NULL;
    k4a_image_t image;
    const int IMAGE_SIZE = 128;
    void *context = NULL;
    uint64_t ts_last;

    ASSERT_NE((uint8_t *)NULL, buffer = allocator_alloc(ALLOCATION_SOURCE_USER, IMAGE_SIZE));
    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              image_create_from_buffer(
                  K4A_IMAGE_FORMAT_COLOR_NV12, 10, 10, 1, buffer, IMAGE_SIZE, image_free_function, context, &image));

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_apply_system_timestamp(image));
    ASSERT_NE((ts_last = image_get_system_timestamp_nsec(image)), 0);

    for (int x = 0; x < 100; x++)
    {
        uint64_t ts;
        ThreadAPI_Sleep(50);

        ASSERT_EQ(K4A_RESULT_SUCCEEDED, image_apply_system_timestamp(image));
        ASSERT_NE((ts = image_get_system_timestamp_nsec(image)), 0);
        ASSERT_GT(ts, ts_last);
        ts_last = ts;
    }
    image_dec_ref(image);
    ASSERT_EQ(allocator_test_for_leaks(), 0);
}

static int allocator_thread_adjust_ref(void *param)
{
    allocator_thread_adjust_ref_data_t *data = (allocator_thread_adjust_ref_data_t *)param;
    tickcounter_ms_t start_time_ms, now;

    Lock(data->lock);
    Unlock(data->lock);

    TICK_COUNTER_HANDLE tick = tickcounter_create();
    if (tick == NULL)
    {
        GTEST_LOG_ERROR << "tickcounter_create failed in allocator_thread_adjust_ref\n";
        data->error = 1;
        goto exit;
    }
    if (0 != tickcounter_get_current_ms(tick, &start_time_ms))
    {
        GTEST_LOG_ERROR << "tickcounter_get_current_ms failed in allocator_thread_adjust_ref\n";
        data->error = 1;
        goto exit;
    }

    do
    {
        switch (data->test_case)
        {
        case 0:
        {
            capture_inc_ref(data->capture);
            capture_dec_ref(data->capture);
            break;
        }
        case 1:
        {
            capture_inc_ref(data->capture);
            capture_inc_ref(data->capture);
            capture_dec_ref(data->capture);
            capture_dec_ref(data->capture);
            break;
        }
        default:
        {
            capture_inc_ref(data->capture);
            capture_inc_ref(data->capture);
            capture_inc_ref(data->capture);
            capture_dec_ref(data->capture);
            capture_dec_ref(data->capture);
            capture_dec_ref(data->capture);
            break;
        }
        };

        if (0 != tickcounter_get_current_ms(tick, &now))
        {
            GTEST_LOG_ERROR << "tickcounter_get_current_ms2 failed in allocator_thread_adjust_ref\n";
            data->error = 1;
            goto exit;
        }
    } while (now - start_time_ms <= 10000);

exit:
    tickcounter_destroy(tick);
    data->done_event = 1;
    return TEST_RETURN_VALUE;
}

TEST(allocator_ut, allocator_threaded)
{
    LOCK_HANDLE lock;
    allocator_thread_adjust_ref_data_t data1, data2, data3;
    THREAD_HANDLE th1, th2, th3;

    ASSERT_NE((lock = Lock_Init()), (LOCK_HANDLE)NULL);

    k4a_capture_t capture = capture_manufacture(sizeof(uint32_t), false);
    ASSERT_NE(capture, (k4a_capture_t)NULL);

    data1.capture = capture;
    data1.test_case = 0;
    data1.done_event = 0;
    data1.lock = lock;

    data2.capture = capture;
    data2.test_case = 1;
    data2.done_event = 0;
    data2.lock = lock;

    data3.capture = capture;
    data3.test_case = 2;
    data3.done_event = 0;
    data3.lock = lock;

    // prevent the threads from running
    Lock(lock);

    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th1, allocator_thread_adjust_ref, &data1));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th2, allocator_thread_adjust_ref, &data2));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th3, allocator_thread_adjust_ref, &data3));

    // start the test
    Unlock(lock);

    int32_t total_sleep_time = 0;
    while (data1.done_event == 0 || data2.done_event == 0 || data3.done_event == 0)
    {
        ThreadAPI_Sleep(500);
        total_sleep_time += 500;
        ASSERT_LT(total_sleep_time, 15000);
    };

    // Wait for the thread to terminate
    int result1, result2, result3;
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th1, &result1));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th2, &result2));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th3, &result3));

    ASSERT_EQ(result1, TEST_RETURN_VALUE);
    ASSERT_EQ(result2, TEST_RETURN_VALUE);
    ASSERT_EQ(result3, TEST_RETURN_VALUE);

    capture_dec_ref(capture);

    // Verify all our allocations were released
    ASSERT_EQ(allocator_test_for_leaks(), 0);
    Lock_Deinit(lock);
}
