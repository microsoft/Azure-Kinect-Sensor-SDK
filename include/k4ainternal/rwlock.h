/** \file rwlock.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef RWLOCK_H
#define RWLOCK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef void *k4a_rwlock_t;
#else
#include <pthread.h>
typedef pthread_rwlock_t k4a_rwlock_t;
#endif

void rwlock_init(k4a_rwlock_t *lock);
void rwlock_deinit(k4a_rwlock_t *lock);

void rwlock_acquire_read(k4a_rwlock_t *lock);
bool rwlock_try_acquire_read(k4a_rwlock_t *lock);

void rwlock_acquire_write(k4a_rwlock_t *lock);
bool rwlock_try_acquire_write(k4a_rwlock_t *lock);

void rwlock_release_read(k4a_rwlock_t *lock);
void rwlock_release_write(k4a_rwlock_t *lock);

#ifdef __cplusplus
}
#endif

#endif /* GLOBAL_H */
