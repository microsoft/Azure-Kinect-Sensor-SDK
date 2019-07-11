// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

#include <k4ainternal/global.h>
#include <k4ainternal/rwlock.h>
#include <k4ainternal/common.h>
#include <gtest/gtest.h>

#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/refcount.h>

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}

#define GTEST_LOG_INFO std::cout << "[     INFO ] "

static int g_GlobalCounter1 = 0;

typedef struct
{
    int valueToInit;
} test_global_t;

static void globalInitFunction(test_global_t *global)
{
    ASSERT_EQ(0, global->valueToInit);
    ASSERT_EQ(0, g_GlobalCounter1);

    g_GlobalCounter1++;
    global->valueToInit = 1;

    // Sleep to simulate an init function that takes some time
    ThreadAPI_Sleep(50);
}

K4A_DECLARE_GLOBAL(test_global_t, globalInitFunction);

static volatile long g_GlobalCounter2 = 0;

typedef struct
{
    int value1;
    int value2;
} test_global2_t;

static void globalInitFunction2(test_global2_t *global)
{
    ASSERT_EQ(0, global->value1);
    ASSERT_EQ(0, global->value2);
    ASSERT_EQ(0, g_GlobalCounter2);

    ASSERT_EQ(1, INC_REF_VAR(g_GlobalCounter2));

    global->value1 = 1;

    // Sleep to simulate an init function that takes some time
    ThreadAPI_Sleep(100);
    global->value2 = 1;

    ASSERT_EQ(1, global->value1);
    ASSERT_EQ(1, global->value2);
}

K4A_DECLARE_GLOBAL(test_global2_t, globalInitFunction2);

TEST(global_ft, global_init_singlethread)
{
    // We should start uninitialized
    ASSERT_EQ(0, g_GlobalCounter1);

    // Get the global context, and verify that it is initalized
    test_global_t *g_test = test_global_t_get();

    ASSERT_EQ(1, g_GlobalCounter1);
    ASSERT_EQ(1, g_test->valueToInit);

    // Get it again and verify initialization hasn't been run more than once
    test_global_t *g_test2 = test_global_t_get();

    ASSERT_EQ(1, g_GlobalCounter1);
    ASSERT_EQ(1, g_test2->valueToInit);

    // The global is a singleton
    ASSERT_EQ(g_test, g_test2);
}

static void thread_testinit(k4a_rwlock_t *lock)
{
    ASSERT_EQ(0, g_GlobalCounter2);

    // Block on the lock to attempt simultaneous release of threads
    rwlock_acquire_read(lock);

    test_global2_t *g_test = test_global2_t_get();

    ASSERT_EQ(1, g_GlobalCounter2);
    ASSERT_EQ(1, g_test->value1);
    ASSERT_EQ(1, g_test->value2);

    rwlock_release_read(lock);
}

static int thread_testinit_threadproc(void *ctx)
{
    thread_testinit((k4a_rwlock_t *)ctx);

    return 0;
}

#define THREAD_COUNT 10
TEST(global_ft, global_init_multithread)
{
    THREAD_HANDLE thread[THREAD_COUNT];
    // Try spinning up a series of threads to simultaneously try initialization

    k4a_rwlock_t lock;
    rwlock_init(&lock);

    rwlock_acquire_write(&lock);

    // Create the threads
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&thread[i], thread_testinit_threadproc, &lock));
        GTEST_LOG_INFO << "Created thread " << i << " (" << thread[i] << ")" << std::endl;
    }

    // Wait for the threads to block
    ThreadAPI_Sleep(200);

    // Allow the threads to run all at once
    rwlock_release_write(&lock);

    // Wait for the threads to complete
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        GTEST_LOG_INFO << "Waiting on thread " << i << std::endl;
        ThreadAPI_Join(thread[i], NULL);
    }

    rwlock_deinit(&lock);

    // Verify initialization happened once
    test_global2_t *g_test = test_global2_t_get();

    ASSERT_EQ(1, g_GlobalCounter2);
    ASSERT_EQ(1, g_test->value1);
    ASSERT_EQ(1, g_test->value2);
}
