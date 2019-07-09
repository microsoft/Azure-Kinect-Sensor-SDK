/** \file rwlock.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef RWLOCK_H 
#define RWLOCK_H

#include <k4a/k4atypes.h>

#include <stddef.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
#ifdef _WIN32
    SRWLOCK _lock;
#else
    pthread_rwlock_t _lock;
#endif
} k4a_rwlock_t;

void rwlock_init(k4a_rwlock_t* lock);
void rwlock_deinit(k4a_rwlock_t* lock);

void rwlock_acquire_read(k4a_rwlock_t* lock);
bool rwlock_try_acquire_read(k4a_rwlock_t* lock);

void rwlock_acquire_write(k4a_rwlock_t* lock);
bool rwlock_try_acquire_write(k4a_rwlock_t* lock);

void rwlock_release_read(k4a_rwlock_t* lock);
void rwlock_release_write(k4a_rwlock_t* lock);

#ifdef __cplusplus
}
#endif

#endif /* GLOBAL_H */
