// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/rwlock.h>

// System dependencies
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>

void rwlock_init(k4a_rwlock_t *lock)
{
    int err = pthread_rwlock_init(&lock->_lock, NULL);
    assert(err == 0);
    while (err != 0)
    {
    };
}
void rwlock_deinit(k4a_rwlock_t *lock)
{
    int err = pthread_rwlock_destroy(&lock->_lock);
    assert(err == 0);
    while (err != 0)
    {
    };
}

void rwlock_acquire_read(k4a_rwlock_t *lock)
{
    int err = pthread_rwlock_rdlock(&lock->_lock);
    assert(err == 0);
    while (err != 0)
    {
    };
}

bool rwlock_try_acquire_read(k4a_rwlock_t *lock)
{
    int err = pthread_rwlock_tryrdlock(&lock->_lock);
    return err == 0;
}

void rwlock_acquire_write(k4a_rwlock_t *lock)
{
    int err = pthread_rwlock_wrlock(&lock->_lock);
    assert(err == 0);
    while (err != 0)
    {
    };
}

bool rwlock_try_acquire_write(k4a_rwlock_t *lock)
{
    int err = pthread_rwlock_trywrlock(&lock->_lock);
    return err == 0;
}

void rwlock_release_read(k4a_rwlock_t *lock)
{
    pthread_rwlock_unlock(&lock->_lock);
}

void rwlock_release_write(k4a_rwlock_t *lock)
{
    pthread_rwlock_unlock(&lock->_lock);
}
