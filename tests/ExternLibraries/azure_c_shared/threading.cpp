// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/lock.h"

#define TEST_RETURN_VALUE (22)
#define TEST_ASSIGN_VALUE (33)

struct ThreadData
{
    int value;
    LOCK_HANDLE lock;
};

static int test_thread_proc(void *h)
{
    // This function is not a TEST, and cannot use ASSERT_* or EXPECT_* macros

    ThreadData *pThreadData = (ThreadData *)h;

    // When the thread is created, the lock should already be held
    Lock(pThreadData->lock);

    // Sleep once the lock is aquired
    ThreadAPI_Sleep(50);

    // Update the value
    pThreadData->value = TEST_ASSIGN_VALUE;
    Unlock(pThreadData->lock);

    return TEST_RETURN_VALUE;
}

TEST(azure_c_shared_threading, ThreadAPI)
{
    ThreadData data;

    data.value = 0;
    data.lock = Lock_Init();

    // Lock before creating the thread
    ASSERT_EQ(LOCK_OK, Lock(data.lock));

    // Create the thread
    THREAD_HANDLE th = NULL;
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Create(&th, test_thread_proc, &data));

    // Sleep for 100ms, then unlock
    ThreadAPI_Sleep(100);
    ASSERT_EQ(LOCK_OK, Unlock(data.lock));

    // Wait for the thread to terminate
    int result = 0;
    ASSERT_EQ(THREADAPI_OK, ThreadAPI_Join(th, &result));

    // Verify the return value
    ASSERT_EQ(result, TEST_RETURN_VALUE);

    // This thread should have slept for at least 100 ms.

    // The test_thread_proc thread should have slept for at least 50 ms after the
    // lock was released.

    // We should expect the join to have taken at least 150 ms from the time the
    // thread was started/

    EXPECT_EQ(TEST_ASSIGN_VALUE, data.value);

    ASSERT_EQ(LOCK_OK, Lock_Deinit(data.lock));
}
