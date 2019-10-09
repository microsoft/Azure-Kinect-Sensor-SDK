/** \file global.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void *k4a_init_once_t;
#define K4A_INIT_ONCE                                                                                                  \
    {                                                                                                                  \
        0                                                                                                              \
    }

typedef void(k4a_init_once_function_t)(void);

void global_init_once(k4a_init_once_t *init_once, k4a_init_once_function_t *init_function);

#ifdef __cplusplus
#define DEFAULT_INIT(type, field) field = type()
#else
#define DEFAULT_INIT(type, field) memset(&field, 0, sizeof(field))
#endif

/** Declares an initialized global context
 *
 * \param _global_type_
 * The structure type of the global context
 *
 * \param _init_function_
 * An initialization function that returns void and takes a pointer to a _global_type_ structure.
 * This function will be called once to initialize the global structure.
 *
 * \remarks
 * This macro creates a new function with the name of the global type followed by _get(). This function
 * returns a pointer to an initialized singleton instance of the global structure. Initialization will only
 * happen once per module and is safe across multiple threads attempting to initialize at the same time.
 *
 */
#define K4A_DECLARE_GLOBAL(_global_type_, _init_function_)                                                             \
    static k4a_init_once_t g_##_global_type_##_init_once = K4A_INIT_ONCE;                                              \
    static _global_type_ _##_global_type_##_private;                                                                   \
    static void fn_##_global_type_##_init_function(void)                                                               \
    {                                                                                                                  \
        DEFAULT_INIT(_global_type_, _##_global_type_##_private);                                                       \
        _init_function_(&_##_global_type_##_private);                                                                  \
        return;                                                                                                        \
    }                                                                                                                  \
    static _global_type_ *_global_type_##_get()                                                                        \
    {                                                                                                                  \
        global_init_once(&g_##_global_type_##_init_once, &fn_##_global_type_##_init_function);                         \
        return &_##_global_type_##_private;                                                                            \
    }

#ifdef __cplusplus
}
#endif

#endif /* GLOBAL_H */
