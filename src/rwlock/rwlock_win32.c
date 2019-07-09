// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/rwlock.h>

// System dependencies
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <windows.h>

void rwlock_init(k4a_rwlock_t* lock)
{
    InitializeSRWLock(&lock->_lock);
}

void rwlock_deinit(k4a_rwlock_t* lock)
{
    UNREFERENCED_PARAMETER(lock);
    // No work
}

void rwlock_acquire_read(k4a_rwlock_t* lock)
{
    AcquireSRWLockShared(&lock->_lock);
}

bool rwlock_try_acquire_read(k4a_rwlock_t* lock)
{
    return TryAcquireSRWLockShared(&lock->_lock);
}

void rwlock_acquire_write(k4a_rwlock_t* lock)
{
    AcquireSRWLockExclusive(&lock->_lock);
}

bool rwlock_try_acquire_write(k4a_rwlock_t* lock)
{
    return TryAcquireSRWLockExclusive(&lock->_lock);
}

void rwlock_release_read(k4a_rwlock_t* lock)
{
    ReleaseSRWLockShared(&lock->_lock);
}

void rwlock_release_write(k4a_rwlock_t* lock)
{
    ReleaseSRWLockExclusive(&lock->_lock);
}

