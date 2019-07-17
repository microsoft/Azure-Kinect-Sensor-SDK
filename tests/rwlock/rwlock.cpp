// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

#include <k4ainternal/rwlock.h>
#include <k4ainternal/common.h>
#include <gtest/gtest.h>

#include <azure_c_shared_utility/threadapi.h>

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}

#define GTEST_LOG_INFO std::cout << "[     INFO ] "

TEST(rwlock_ft, rwlock_init)
{
    k4a_rwlock_t lock;

    rwlock_init(&lock);

    rwlock_deinit(&lock);
}

typedef struct
{
    k4a_rwlock_t test_lock;

    // Number of times reader 1 gets the lock
    volatile int reader1_count;

    // Number of times reader 2 gets the lock
    volatile int reader2_count;

    // Number of times reader 2 fails to get the lock
    volatile int reader2_fail_count;

    // Number of times writer 1 gets the lock
    volatile int writer1_count;

    // Number of times writer 2 gets the lock
    volatile int writer2_count;

    // Number of times writer 2 fails to get the lock
    volatile int writer2_fail_count;

    volatile bool run_test;
} threaded_rwlock_test_context_t;

static void thread_reader1(threaded_rwlock_test_context_t *context)
{
    while (context->run_test)
    {
        rwlock_acquire_read(&context->test_lock);

        int writer1 = context->writer1_count;
        int writer2 = context->writer2_count;

        context->reader1_count++;

        // No writers should be able to access the lock while
        // a reader has the lock
        EXPECT_EQ(writer1, context->writer1_count);
        EXPECT_EQ(writer2, context->writer2_count);

        ThreadAPI_Sleep(1);

        rwlock_release_read(&context->test_lock);

        ThreadAPI_Sleep(1);
    }
}
static int thread_reader1_threadproc(void *ctx)
{
    thread_reader1((threaded_rwlock_test_context_t *)ctx);
    return 0;
}

static void thread_reader2(threaded_rwlock_test_context_t *context)
{

    while (context->run_test)
    {
        if (rwlock_try_acquire_read(&context->test_lock))
        {
            int writer1 = context->writer1_count;
            int writer2 = context->writer2_count;

            context->reader2_count++;

            // No writers should be able to access the lock while
            // a reader has the lock
            EXPECT_EQ(writer1, context->writer1_count);
            EXPECT_EQ(writer2, context->writer2_count);

            ThreadAPI_Sleep(2);

            rwlock_release_read(&context->test_lock);
        }
        else
        {
            context->reader2_fail_count++;
        }

        ThreadAPI_Sleep(1);
    }
}

static int thread_reader2_threadproc(void *ctx)
{
    thread_reader2((threaded_rwlock_test_context_t *)ctx);
    return 0;
}

static void thread_writer1(threaded_rwlock_test_context_t *context)
{
    while (context->run_test)
    {
        rwlock_acquire_write(&context->test_lock);

        // Capture the current counts
        int reader1 = context->reader1_count;
        int reader2 = context->reader2_count;
        int reader2_fail = context->reader2_fail_count;
        int writer2 = context->writer2_count;
        int writer2_fail = context->writer2_fail_count;

        context->writer1_count++;

        // Wait a moment to allow other threads to run
        ThreadAPI_Sleep(100);

        // Wait for the other threads to hit contention
        while (reader2_fail == context->reader2_fail_count || writer2_fail == context->writer2_fail_count)
        {
            ThreadAPI_Sleep(10);

            // Don't wait if we reach the end of the test
            if (!context->run_test)
            {
                break;
            }
        }

        // No other thread should have the lock, the values should not change
        EXPECT_EQ(reader1, context->reader1_count);
        EXPECT_EQ(reader2, context->reader2_count);
        EXPECT_EQ(writer2, context->writer2_count);

        // If we reached the end of the test, we may not have waited long enough
        // for these to be true
        if (context->run_test)
        {
            // Since we have held the lock for some time, we expect reader 2 and writer 2
            // to have failed to acquire the lock while we have held it
            EXPECT_NE(reader2_fail, context->reader2_fail_count);
            EXPECT_NE(writer2_fail, context->writer2_fail_count);
        }

        rwlock_release_write(&context->test_lock);

        ThreadAPI_Sleep(100);
    }
}

static int thread_writer1_threadproc(void *ctx)
{
    thread_writer1((threaded_rwlock_test_context_t *)ctx);
    return 0;
}

static void thread_writer2(threaded_rwlock_test_context_t *context)
{
    while (context->run_test)
    {
        if (rwlock_try_acquire_write(&context->test_lock))
        {
            int reader1 = context->reader1_count;
            int reader2 = context->reader2_count;
            int writer1 = context->writer1_count;

            context->writer2_count++;

            // The other writer, nor any readers should have the lock while we do
            EXPECT_EQ(reader1, context->reader1_count);
            EXPECT_EQ(reader2, context->reader2_count);
            EXPECT_EQ(writer1, context->writer1_count);

            ThreadAPI_Sleep(3);

            rwlock_release_write(&context->test_lock);
        }
        else
        {
            context->writer2_fail_count++;
        }

        ThreadAPI_Sleep(2);
    }
}

static int thread_writer2_threadproc(void *ctx)
{
    thread_writer2((threaded_rwlock_test_context_t *)ctx);
    return 0;
}

TEST(rwlock_ft, rwlock_threaded_test)
{
    /*

    This test creates four threads which have contention over a rwlock.

    Reader1:
        Tries to acquire a read lock every 2 ms
    Reader2:
        Uses the try acquire method to try getting the read lock every 3 ms
    Writer1:
        Gets and releases the write lock with a delay. It holds the lock for a while as well.
    Writer2:
        Uses the try acquire method to try getting the write lock every 5 ms

    Each thread checks for the known invariants of the behavior of the other threads. E.g. a read
    lock can't be acquired while the write lock is held.

     */
    THREAD_HANDLE r1, r2, w1, w2;
    threaded_rwlock_test_context_t test_context;
    memset(&test_context, 0, sizeof(test_context));
    test_context.run_test = true;

    rwlock_init(&test_context.test_lock);

    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&r1, thread_reader1_threadproc, &test_context));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&r2, thread_reader2_threadproc, &test_context));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&w1, thread_writer1_threadproc, &test_context));
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&w2, thread_writer2_threadproc, &test_context));

    ThreadAPI_Sleep(5000);

    test_context.run_test = false;

    ThreadAPI_Join(r1, NULL);
    ThreadAPI_Join(r2, NULL);
    ThreadAPI_Join(w1, NULL);
    ThreadAPI_Join(w2, NULL);

    rwlock_deinit(&test_context.test_lock);

    // Ensure all the threads have acquired the lock and hit failure contention
    ASSERT_NE(0, test_context.reader1_count);
    ASSERT_NE(0, test_context.reader2_count);
    ASSERT_NE(0, test_context.reader2_fail_count);
    ASSERT_NE(0, test_context.writer1_count);
    ASSERT_NE(0, test_context.writer2_count);
    ASSERT_NE(0, test_context.writer2_fail_count);

    GTEST_LOG_INFO << "reader1_count " << test_context.reader1_count << std::endl;
    GTEST_LOG_INFO << "reader2_count " << test_context.reader2_count << std::endl;
    GTEST_LOG_INFO << "reader2_fail_count " << test_context.reader2_fail_count << std::endl;
    GTEST_LOG_INFO << "writer1_count " << test_context.writer1_count << std::endl;
    GTEST_LOG_INFO << "writer2_count " << test_context.writer2_count << std::endl;
    GTEST_LOG_INFO << "writer2_fail_count " << test_context.writer2_fail_count << std::endl;
}
