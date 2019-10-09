// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

#include <k4ainternal/allocator.h>
#include <k4ainternal/image.h>
#include <k4ainternal/queue.h>
#include <k4ainternal/common.h>
#include <gtest/gtest.h>

#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/tickcounter.h>
#include <azure_c_shared_utility/threadapi.h>

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}

#define GTEST_LOG_ERROR std::cout << "[    ERROR ] "
#define GTEST_LOG_WARNING std::cout << "[  WARNING ] "
#define GTEST_LOG_INFO std::cout << "[     INFO ] "

#define THREAD_YEILD_TIME (1)
#define TEST_EXECUTION_TIME (5000)

#define TEST_RETURN_VALUE (22)

#define MAX_QUEUE_DEPTH_LENGTH (10000)

#define TEST_QUEUE_DEPTH 7

static int32_t g_timeout = 100;

typedef struct _threaded_queue_data_t
{
    queue_t queue;
    uint32_t pattern_start;
    uint32_t pattern_offset;
    uint32_t error;
    uint32_t dropped;
    LOCK_HANDLE lock;
    volatile uint32_t done_event;
} threaded_queue_data_t;

static k4a_capture_t capture_manufacture(size_t size)
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
        capture_set_color_image(capture, image);
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

static uint8_t *get_raw_byte_ptr(k4a_capture_t capture, size_t *size)
{
    k4a_image_t image;
    k4a_result_t result;
    uint8_t *buffer = NULL;

    result = K4A_RESULT_FROM_BOOL((image = capture_get_color_image(capture)) != NULL);
    if (K4A_SUCCEEDED(result))
    {
        buffer = image_get_buffer(image);
        if (size)
        {
            *size = image_get_size(image);
        }
        image_dec_ref(image);
    }
    return buffer;
}

static k4a_result_t fill_queue(queue_t queue, uint32_t starting_value, uint32_t number_of_entries)
{
    k4a_capture_t capture;
    size_t size;
    uint64_t time;

    for (uint32_t loop = 0; loop < number_of_entries; loop++)
    {
        k4a_image_t image;
        uint32_t payload = loop + starting_value;
        size = sizeof(uint32_t) * ((payload) % 4 + 1);
        time = (uint64_t)(sizeof(uint32_t) * ((payload) % 4 + 1));

        capture = capture_manufacture(size);
        EXPECT_NE(capture, (k4a_capture_t)NULL);
        if (capture == NULL)
        {
            return K4A_RESULT_FAILED;
        }

        image = capture_get_color_image(capture);
        if (image == NULL)
        {
            return K4A_RESULT_FAILED;
        }

        memcpy(image_get_buffer(image), &payload, sizeof(payload));
        // Set attributes
        image_set_size(image, size);
        image_set_device_timestamp_usec(image, time);

        queue_push(queue, capture);

        // queue owns the memory now
        image_dec_ref(image);
        capture_dec_ref(capture);
    }
    return K4A_RESULT_SUCCEEDED;
}

static k4a_wait_result_t drain_queue(queue_t queue, uint32_t starting_value, uint32_t number_to_drain)
{
    k4a_capture_t capture;
    size_t size;
    size_t expected_size;
    k4a_wait_result_t wresult;

    for (uint32_t loop = 0; loop < number_to_drain; loop++)
    {
        wresult = queue_pop(queue, 0, &capture);
        if (wresult != K4A_WAIT_RESULT_SUCCEEDED)
        {
            EXPECT_NE(capture, (k4a_capture_t)NULL);
            return wresult;
        }

        // validate capture contents.
        uint32_t integer = 0;
        uint8_t *memory = get_raw_byte_ptr(capture, &size);
        memcpy(&integer, memory, sizeof(integer));
        if (integer != loop + starting_value)
        {
            EXPECT_EQ(integer, loop + starting_value);
            return K4A_WAIT_RESULT_FAILED;
        }

        expected_size = (integer % 4 + 1) * sizeof(uint32_t);
        if (size != expected_size)
        {
            EXPECT_EQ(size, expected_size);
            return K4A_WAIT_RESULT_FAILED;
        }

        // remove the ref we now own
        capture_dec_ref(capture);
    }
    return K4A_WAIT_RESULT_SUCCEEDED;
}

static uint32_t find_queue_depth(queue_t queue)
{
    k4a_result_t result;
    k4a_wait_result_t wresult;
    k4a_capture_t capture;
    uint32_t depth;
    size_t capture_size;

    queue_enable(queue);

    result = fill_queue(queue, 0, MAX_QUEUE_DEPTH_LENGTH);
    EXPECT_EQ(result, K4A_RESULT_SUCCEEDED);

    wresult = queue_pop(queue, 0, &capture);
    EXPECT_EQ(wresult, K4A_WAIT_RESULT_SUCCEEDED);
    EXPECT_NE((k4a_capture_t)NULL, capture);

    // we pushed 0 in the first element above, if we get it back it is
    // because our test didn't find the max depth to cause data to
    // be dropped.
    uint32_t integer = 0;
    uint64_t time = 0;
    size_t size;
    k4a_image_t image = capture_get_color_image(capture);
    EXPECT_NE((k4a_image_t)NULL, image);

    memcpy(&integer, image_get_buffer(image), sizeof(integer));
    capture_size = image_get_size(image);
    size = (size_t)(sizeof(uint32_t) * ((integer) % 4 + 1));
    time = (uint64_t)(sizeof(uint32_t) * ((integer) % 4 + 1));
    EXPECT_NE(integer, (uint32_t)0);
    EXPECT_LT(integer, (uint32_t)MAX_QUEUE_DEPTH_LENGTH);
    EXPECT_EQ(size, capture_size);
    EXPECT_EQ(time, image_get_device_timestamp_usec(image));

    depth = MAX_QUEUE_DEPTH_LENGTH - integer;

    image_dec_ref(image);
    capture_dec_ref(capture); // free the pop memory

    return depth;
}

TEST(queue_ut, queue_find_depth)
{
    queue_t queue;

    uint32_t queue_depth_to_test = 8;
    ASSERT_EQ(queue_create(queue_depth_to_test, "queue_test", &queue), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(find_queue_depth(queue), queue_depth_to_test);
    queue_destroy(queue);

    queue_depth_to_test = 13;
    ASSERT_EQ(queue_create(queue_depth_to_test, "queue_test", &queue), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(find_queue_depth(queue), queue_depth_to_test);
    queue_destroy(queue);

    queue_depth_to_test = 97;
    ASSERT_EQ(queue_create(queue_depth_to_test, "queue_test", &queue), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(find_queue_depth(queue), queue_depth_to_test);
    queue_destroy(queue);

    queue_depth_to_test = 100;
    ASSERT_EQ(queue_create(queue_depth_to_test, "queue_test", &queue), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(find_queue_depth(queue), queue_depth_to_test);
    queue_destroy(queue);

    queue_depth_to_test = 1999;
    ASSERT_EQ(queue_create(queue_depth_to_test, "queue_test", &queue), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(find_queue_depth(queue), queue_depth_to_test);
    queue_destroy(queue);

    ASSERT_EQ(allocator_test_for_leaks(), 0);
}

typedef struct
{
    int32_t pop_api_timeout;
    int32_t push_api_delay;
} pop_empty_queue_thread_tests_t;

static pop_empty_queue_thread_tests_t g_empty_queue_thread_test_data[] = {
    { 0, K4A_WAIT_INFINITE },   // zero based timeout (to)
    { 500, K4A_WAIT_INFINITE }, // thread blocked based to
    { K4A_WAIT_INFINITE, 0 },   // successful read
    { K4A_WAIT_INFINITE, 500 }, // successful read
};

typedef struct
{
    int32_t pop_api_timeout;
    int32_t push_api_delay;
    k4a_capture_t capture;
    LOCK_HANDLE lock;
    queue_t queue;
} empty_queue_read_write_data_t;

static int thread_pop_empty_queue_reader(void *param)
{
    empty_queue_read_write_data_t *data = (empty_queue_read_write_data_t *)param;
    k4a_capture_t capture = NULL;
    k4a_wait_result_t wresult;

    // sync test start
    Lock(data->lock);
    Unlock(data->lock);

    wresult = queue_pop(data->queue, data->pop_api_timeout, &capture);
    if (capture)
    {
        capture_dec_ref(capture);
    }
    return wresult;
}
static int thread_pop_empty_queue_writer(void *param)
{
    empty_queue_read_write_data_t *data = (empty_queue_read_write_data_t *)param;

    // sync test start
    Lock(data->lock);
    Unlock(data->lock);

    if (K4A_WAIT_INFINITE != data->push_api_delay)
    {
        ThreadAPI_Sleep((unsigned int)data->push_api_delay);
        queue_push(data->queue, data->capture);
        return K4A_RESULT_SUCCEEDED;
    }
    return K4A_RESULT_SUCCEEDED;
}

TEST(queue_ut, queue_pop_on_empty_queue)
{
    queue_t queue;
    k4a_capture_t capture;
    uint32_t queue_depth = TEST_QUEUE_DEPTH;

    ASSERT_EQ(queue_create(queue_depth, "queue_test", &queue), K4A_RESULT_SUCCEEDED);
    queue_enable(queue);

    // Queue pop should timeout on multiple timeouts
    ASSERT_EQ(queue_pop(queue, 0, &capture), K4A_WAIT_RESULT_TIMEOUT);
    ASSERT_EQ(queue_pop(queue, 100, &capture), K4A_WAIT_RESULT_TIMEOUT);
    ASSERT_EQ(queue_pop(queue, 200, &capture), K4A_WAIT_RESULT_TIMEOUT);
    ASSERT_EQ(queue_pop(queue, 300, &capture), K4A_WAIT_RESULT_TIMEOUT);
    ASSERT_EQ(queue_pop(queue, 400, &capture), K4A_WAIT_RESULT_TIMEOUT);

    empty_queue_read_write_data_t data;
    data.queue = queue;
    data.capture = capture_manufacture(10);
    data.lock = Lock_Init();
    ASSERT_NE(data.capture, (k4a_capture_t)NULL);
    ASSERT_NE(data.lock, (LOCK_HANDLE)NULL);

    for (uint32_t x = 0; x < COUNTOF(g_empty_queue_thread_test_data); x++)
    {
        THREAD_HANDLE t1, t2;

        GTEST_LOG_INFO << "test iteration " << x << "\n";

        data.pop_api_timeout = g_empty_queue_thread_test_data[x].pop_api_timeout;
        data.push_api_delay = g_empty_queue_thread_test_data[x].push_api_delay;

        // prevent the threads from running
        Lock(data.lock);

        ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&t1, thread_pop_empty_queue_writer, &data));
        ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&t2, thread_pop_empty_queue_reader, &data));

        Unlock(data.lock);

        int write_result, read_result;
        ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(t1, &write_result));
        ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(t2, &read_result));

        if ((((int64_t)data.push_api_delay >= (int64_t)data.pop_api_timeout) &&
             (data.pop_api_timeout != K4A_WAIT_INFINITE)) ||
            (data.push_api_delay == K4A_WAIT_INFINITE))
        {
            ASSERT_EQ((k4a_wait_result_t)read_result, K4A_WAIT_RESULT_TIMEOUT);
        }
        else
        {
            ASSERT_EQ((k4a_wait_result_t)read_result, K4A_WAIT_RESULT_SUCCEEDED);
        }
        ASSERT_EQ((k4a_wait_result_t)write_result, K4A_WAIT_RESULT_SUCCEEDED);
    }

    capture_dec_ref(data.capture);

    // Verify all our allocations were released
    ASSERT_EQ(allocator_test_for_leaks(), 0);

    Lock_Deinit(data.lock);
    queue_destroy(queue);
}

TEST(queue_ut, test_queue_push_w_dropped)
{
    queue_t queue;
    k4a_capture_t capture1;
    k4a_capture_t capture2;
    k4a_capture_t capture_dropped;

    ASSERT_EQ(queue_create(1, "queue_test", &queue), K4A_RESULT_SUCCEEDED);
    capture1 = capture_manufacture(10);
    ASSERT_NE(capture1, (k4a_capture_t)NULL);
    capture2 = capture_manufacture(10);
    ASSERT_NE(capture2, (k4a_capture_t)NULL);

    // all expected to fail
    queue_push_w_dropped(NULL, NULL, NULL);
    queue_push_w_dropped(queue, NULL, NULL);
    queue_push_w_dropped(NULL, capture1, NULL);

    // queue is empty, so capture_dropped should be NULL
    capture_dropped = NULL;
    queue_push_w_dropped(queue, capture1, &capture_dropped);
    ASSERT_EQ(capture_dropped, (k4a_capture_t)NULL);

    // queue is full, so capture_dropped should be capture_1
    capture_dropped = NULL;
    queue_push_w_dropped(queue, capture2, &capture_dropped);
    ASSERT_EQ(capture_dropped, (k4a_capture_t)NULL);
    capture_dec_ref(capture_dropped);

    // queue is empty, dropped lock should not be touched
    queue_push_w_dropped(queue, capture1, NULL);
    ASSERT_EQ(capture_dropped, (k4a_capture_t)NULL);

    // queue is full, dropped logic is NULL and should deal with it internally
    capture_dropped = NULL;
    queue_push_w_dropped(queue, capture2, NULL);
    ASSERT_EQ(capture_dropped, (k4a_capture_t)NULL);

    capture_dec_ref(capture1);
    capture_dec_ref(capture2);

    queue_enable(queue);
    queue_destroy(queue);
    ASSERT_EQ(allocator_test_for_leaks(), 0);
}

TEST(queue_ut, queue_multiple_queues)
{
    queue_t queue1, queue2, queue3;
    uint32_t starting_sequence1, starting_sequence2, starting_sequence3;
    uint32_t size;
    k4a_capture_t capture;
    uint32_t queue_depth = TEST_QUEUE_DEPTH;

    size = queue_depth + 1;
    starting_sequence1 = 10;
    starting_sequence2 = 10;
    starting_sequence3 = 10;

    ASSERT_EQ(queue_create(queue_depth, "queue_test", &queue1), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(queue_create(queue_depth, "queue_test", &queue2), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(queue_create(queue_depth, "queue_test", &queue3), K4A_RESULT_SUCCEEDED);

    queue_enable(queue1);
    queue_enable(queue2);
    queue_enable(queue3);

    ASSERT_EQ(fill_queue(queue1, starting_sequence1, size), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(fill_queue(queue2, starting_sequence2, size), K4A_RESULT_SUCCEEDED);
    ASSERT_EQ(fill_queue(queue3, starting_sequence3, size), K4A_RESULT_SUCCEEDED);

    // expected starting seq is +1 from base due to writeing queue_depth + 1
    starting_sequence1++;
    starting_sequence2++;
    starting_sequence3++;

    ASSERT_EQ(drain_queue(queue1, starting_sequence1, size / 2), K4A_WAIT_RESULT_SUCCEEDED);
    ASSERT_EQ(drain_queue(queue3, starting_sequence3, size / 2), K4A_WAIT_RESULT_SUCCEEDED);
    ASSERT_EQ(drain_queue(queue2, starting_sequence2, size / 2), K4A_WAIT_RESULT_SUCCEEDED);

    starting_sequence1 += size / 2;
    starting_sequence2 += size / 2;
    starting_sequence3 += size / 2;

    ASSERT_EQ(drain_queue(queue2, starting_sequence2, size / 2 - 1), K4A_WAIT_RESULT_SUCCEEDED);
    ASSERT_EQ(drain_queue(queue1, starting_sequence1, size / 2 - 1), K4A_WAIT_RESULT_SUCCEEDED);
    ASSERT_EQ(drain_queue(queue3, starting_sequence3, size / 2 - 1), K4A_WAIT_RESULT_SUCCEEDED);

    queue_t queues[] = { queue1, queue2, queue3 };

    for (uint32_t x = 0; x < COUNTOF(queues); x++)
    {
        int32_t timeout = g_timeout;
        ASSERT_EQ(queue_pop(queues[x], timeout, &capture), K4A_WAIT_RESULT_TIMEOUT);
    }

    queue_destroy(queue1);
    queue_destroy(queue2);
    queue_destroy(queue3);

    // Verify all our allocations were released
    ASSERT_EQ(allocator_test_for_leaks(), 0);
}

static int thread_write_queue(void *param)
{
    threaded_queue_data_t *data = (threaded_queue_data_t *)param;
    uint32_t loop = data->pattern_start;
    tickcounter_ms_t start_time_ms, now;
    TICK_COUNTER_HANDLE tick = NULL;

    // Sync start - its go time when we get this lock
    Lock(data->lock);
    Unlock(data->lock);

    tick = tickcounter_create();
    if (tick == NULL)
    {
        GTEST_LOG_ERROR << "tickcounter_create failed in thread_write_queue\n";
        data->error = 1;
        goto exit;
    }
    if (0 != tickcounter_get_current_ms(tick, &start_time_ms))
    {
        GTEST_LOG_ERROR << "tickcounter_get_current_ms[1] failed in thread_write_queue\n";
        data->error = 1;
        goto exit;
    }
    now = start_time_ms;

    do
    {
        k4a_capture_t capture = capture_manufacture(sizeof(uint32_t));
        memcpy(get_raw_byte_ptr(capture, NULL), &loop, sizeof(loop));

        queue_push(data->queue, capture);

        capture_dec_ref(capture);

        ThreadAPI_Sleep(1);
        if (0 != tickcounter_get_current_ms(tick, &now))
        {
            GTEST_LOG_ERROR << "tickcounter_get_current_ms[2] failed in thread_write_queue\n";
            data->error = 1;
            goto exit;
        }
        loop += data->pattern_offset;
    } while ((now - start_time_ms) < TEST_EXECUTION_TIME);

exit:
    data->done_event = 1;

    if (tick)
    {
        tickcounter_destroy(tick);
    }
    return TEST_RETURN_VALUE;
}

static int thread_read_queue(void *param)
{
    threaded_queue_data_t *reader = (threaded_queue_data_t *)param;
    TICK_COUNTER_HANDLE tick = NULL;

    // Sync start - its go time when we get this lock
    Lock(reader->lock);
    Unlock(reader->lock);

    uint32_t t1_expected_data = 1;
    uint32_t t2_expected_data = 2;
    uint32_t t3_expected_data = 3;
    uint32_t *expected_data;
    uint32_t max_sample = 0;

    tickcounter_ms_t start_time_ms, now;

    tick = tickcounter_create();
    if (tick == NULL)
    {
        GTEST_LOG_ERROR << "tickcounter_create failed in thread_read_queue\n";
        reader->error = 1;
        goto exit;
    }
    if (0 != tickcounter_get_current_ms(tick, &start_time_ms))
    {
        GTEST_LOG_ERROR << "tickcounter_get_current_ms[1] failed in thread_read_queue\n";
        reader->error = 1;
        goto exit;
    }
    now = start_time_ms;

    do
    {
        k4a_capture_t capture;
        size_t size;
        k4a_wait_result_t wresult;

        wresult = queue_pop(reader->queue, 0, &capture);
        if (wresult != K4A_WAIT_RESULT_SUCCEEDED)
        {
            if (wresult != K4A_WAIT_RESULT_TIMEOUT)
            {
                GTEST_LOG_ERROR << "queue_pop returned unexpected error\n";
                reader->error = 1;
                break;
            }
            else
            {
                ThreadAPI_Sleep(THREAD_YEILD_TIME); // quick 1ms yield
            }
        }
        else
        {
            uint32_t queue_int = 0;
            uint8_t *buffer = get_raw_byte_ptr(capture, &size);

            if (size != sizeof(uint32_t))
            {
                EXPECT_EQ(size, sizeof(uint32_t));
                reader->error = 1;
            }

            memcpy(&queue_int, buffer, sizeof(queue_int));
            switch (queue_int % 3)
            {
            case 0:
                expected_data = &t3_expected_data;
                break;
            case 1:
                expected_data = &t1_expected_data;
                break;
            default:
                expected_data = &t2_expected_data;
                break;
            }

            if (*expected_data == queue_int)
            {
                *expected_data = queue_int + reader->pattern_offset;
            }
            else
            {
                EXPECT_GT(queue_int, *expected_data);

                // If this failed then we have the error we are testing for. We might drop a couple samples in the queue
                // due to thread starvation, but the pattern should be maintained.
                if ((queue_int - *expected_data) % 3 != 0)
                {
                    EXPECT_EQ(*expected_data, queue_int);
                    reader->error = 1;
                }
                else
                {
                    // attempt to recover from the data that was dropped
                    reader->dropped += ((queue_int - *expected_data) / reader->pattern_offset);
                    *expected_data = queue_int + reader->pattern_offset;
                }
            }

            if (max_sample < queue_int)
            {
                max_sample = queue_int;
            }

            capture_dec_ref(capture);
        }

        if (0 != tickcounter_get_current_ms(tick, &now))
        {
            GTEST_LOG_ERROR << "tickcounter_get_current_ms[2] failed in thread_read_queue\n";
            reader->error = 1;
            goto exit;
        }
    } while ((now - start_time_ms) <= TEST_EXECUTION_TIME);

exit:
    if (tick)
    {
        tickcounter_destroy(tick);
    }
    reader->done_event = 1;
    GTEST_LOG_INFO << "Test Complete after getting " << max_sample << " samples\n";
    return TEST_RETURN_VALUE;
}

TEST(queue_ut, queue_threaded)
{
    queue_t queue;
    LOCK_HANDLE lock;
    threaded_queue_data_t data1, data2, data3, reader;
    THREAD_HANDLE t1, t2, t3, r1;

    ASSERT_EQ(queue_create(TEST_QUEUE_DEPTH, "queue_test", &queue), K4A_RESULT_SUCCEEDED);
    queue_enable(queue);
    ASSERT_NE((lock = Lock_Init()), (LOCK_HANDLE)NULL);

    data1.queue = queue;
    data1.pattern_start = 1;
    data1.pattern_offset = 3;
    data1.done_event = 0;
    data1.error = 0;
    data1.lock = lock;

    data2.queue = queue;
    data2.pattern_start = 2;
    data2.pattern_offset = 3;
    data2.done_event = 0;
    data2.error = 0;
    data2.lock = lock;

    data3.queue = queue;
    data3.pattern_start = 3;
    data3.pattern_offset = 3;
    data3.done_event = 0;
    data3.error = 0;
    data3.lock = lock;

    reader.queue = queue;
    reader.pattern_offset = 3;
    reader.done_event = 0;
    reader.error = 0;
    reader.dropped = 0;
    reader.lock = lock;

    // prevent the threads from running
    Lock(lock);

    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&t1, thread_write_queue, &data1));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&t2, thread_write_queue, &data2));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&t3, thread_write_queue, &data3));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&r1, thread_read_queue, &reader));

    Unlock(lock);

    int32_t total_sleep_time = 0;
    while (data1.done_event == 0 || data2.done_event == 0 || data3.done_event == 0 || reader.done_event == 0)
    {
        ThreadAPI_Sleep(500);
        total_sleep_time += 500;
    };

    // Wait for the thread to terminate
    int result1, result2, result3, result4;
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(t1, &result1));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(t2, &result2));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(t3, &result3));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(r1, &result4));

    ASSERT_EQ(result1, TEST_RETURN_VALUE);
    ASSERT_EQ(result2, TEST_RETURN_VALUE);
    ASSERT_EQ(result3, TEST_RETURN_VALUE);
    ASSERT_EQ(result4, TEST_RETURN_VALUE);

    ASSERT_EQ(data1.error, (uint32_t)0);
    ASSERT_EQ(data2.error, (uint32_t)0);
    ASSERT_EQ(data3.error, (uint32_t)0);
    ASSERT_EQ(reader.error, (uint32_t)0);

    if (reader.dropped != 0)
    {
        GTEST_LOG_WARNING << "WARNING: queue dropped " << reader.dropped << " samples \n";
    }

    queue_destroy(queue);

    // Verify all our allocations were released
    ASSERT_EQ(allocator_test_for_leaks(), 0);

    Lock_Deinit(lock);
}

TEST(queue_ut, queue_enable_disable)
{

    queue_t queue;
    k4a_capture_t capture, capture_read;

    ASSERT_EQ(queue_create(TEST_QUEUE_DEPTH, "queue_test", &queue), K4A_RESULT_SUCCEEDED);

    // multiple calls should not crash
    queue_enable(queue);
    queue_enable(queue);
    queue_enable(queue);

    // multiple calls should not crash
    queue_disable(queue);
    queue_disable(queue);
    queue_disable(queue);

    ASSERT_NE((capture = capture_manufacture(10)), (k4a_capture_t)NULL);

    // disabled
    {
        queue_disable(queue);
        queue_push(queue, capture);
        queue_push(queue, capture);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_FAILED);
    }

    // enabled
    {
        queue_enable(queue);
        queue_push(queue, capture);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_SUCCEEDED);
        ASSERT_EQ(capture, capture_read);
        capture_dec_ref(capture_read);
        // there should only be 1 capture in the queue.
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_TIMEOUT);
    }

    // enabled, put captures in, disable, veriry no captures, enable, verify still no captures
    {
        queue_enable(queue);
        queue_push(queue, capture);
        queue_push(queue, capture);
        queue_push(queue, capture);
        queue_disable(queue);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_FAILED);

        // the queue should have been purged when disabled;
        queue_enable(queue);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_TIMEOUT);
    }

    // disable, put captures in, enable, veriry no captures,
    {
        queue_disable(queue);
        queue_push(queue, capture);
        queue_push(queue, capture);
        queue_push(queue, capture);
        queue_enable(queue);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_TIMEOUT);

        // the queue should have never received captures
        queue_disable(queue);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_FAILED);
    }

    capture_dec_ref(capture);
    queue_destroy(queue);
    ASSERT_EQ(allocator_test_for_leaks(), 0);
}

TEST(queue_ut, queue_stop)
{

    queue_t queue;
    k4a_capture_t capture, capture_read;

    ASSERT_EQ(queue_create(TEST_QUEUE_DEPTH, "queue_test", &queue), K4A_RESULT_SUCCEEDED);

    ASSERT_NE((capture = capture_manufacture(10)), (k4a_capture_t)NULL);

    // stop
    {
        queue_stop(queue);
        queue_push(queue, capture);
        queue_push(queue, capture);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_FAILED);
    }

    // enabled
    {
        queue_enable(queue);
        queue_push(queue, capture);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_SUCCEEDED);
        ASSERT_EQ(capture, capture_read);
        capture_dec_ref(capture_read);
        // there should only be 1 capture in the queue.
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_TIMEOUT);
    }

    // enabled, put captures in, stop, veriry no captures, enable, verify still no captures
    {
        queue_enable(queue);
        queue_push(queue, capture);
        queue_push(queue, capture);
        queue_push(queue, capture);
        queue_stop(queue);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_FAILED);

        // the queue should have been purged when disabled;
        queue_enable(queue);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_TIMEOUT);
    }

    // disable, put captures in, enable, veriry no captures,
    {
        queue_stop(queue);
        queue_push(queue, capture);
        queue_push(queue, capture);
        queue_push(queue, capture);
        queue_enable(queue);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_TIMEOUT);

        // the queue should have never received captures
        queue_stop(queue);
        ASSERT_EQ(queue_pop(queue, 0, &capture_read), K4A_WAIT_RESULT_FAILED);
    }

    capture_dec_ref(capture);
    queue_destroy(queue);
    ASSERT_EQ(allocator_test_for_leaks(), 0);
}
