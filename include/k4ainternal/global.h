/** \file global.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <k4a/k4atypes.h>
#include <k4ainternal/rwlock.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
typedef INIT_ONCE k4a_init_once_t;
#define K4A_INIT_ONCE INIT_ONCE_STATIC_INIT
#else
typedef pthread_once_t k4a_init_once_t;
#define K4A_INIT_ONCE PTHREAD_ONCE_INIT
#endif

typedef void(k4a_init_once_function_t)();

void global_init_once(k4a_init_once_t *init_once, k4a_init_once_function_t *init_function);

#define K4A_DECLARE_GLOBAL(_global_type_, _init_function_)                                                             \
    static k4a_init_once_t g_##_global_type_##_init_once = K4A_INIT_ONCE;                                              \
    static _global_type_ _##_global_type_##_private;                                                                   \
    static void fn_##_global_type_##_init_function()                                                                   \
    {                                                                                                                  \
        memset(&_##_global_type_##_private, 0, sizeof(_##_global_type_##_private));                                    \
        _init_function_(&_##_global_type_##_private);                                                                  \
        return;                                                                                                        \
    }                                                                                                                  \
    static _global_type_ *_global_type_##_get()                                                                        \
    {                                                                                                                  \
        global_init_once(&g_##_global_type_##_init_once, fn_##_global_type_##_init_function);                          \
        return &_##_global_type_##_private;                                                                            \
    }

#ifdef __cplusplus
}
#endif

#endif /* GLOBAL_H */
