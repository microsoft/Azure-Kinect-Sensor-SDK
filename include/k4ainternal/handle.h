/** \file handle.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef K4A_INTERNAL_HANDLE_H
#define K4A_INTERNAL_HANDLE_H

#include <k4ainternal/logging.h>
#include <k4ainternal/common.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IF_LOGGER(x) x

#ifdef _WIN32
#define KSELECTANY __declspec(selectany)
#else
#define KSELECTANY __attribute__((weak))
#endif

#ifdef __cplusplus
#define ALLOCATE(type) (type *)(::new (std::nothrow) type()) /* init to zero */
#define DESTROY(ptr) ::delete ptr
#define PRIV_HANDLE_TYPE(type) _handle_##type##_cpp
#define PUB_HANDLE_TYPE(type) type##_wrapper_##_cpp
#define STR_INTERNAL_CONTEXT_TYPE(type) STRINGIFY(type##_cpp)
#else
#define ALLOCATE(type) (type *)(calloc(sizeof(type), 1)) /*Zero initialized*/
#define DESTROY(ptr) free(ptr)
#define PRIV_HANDLE_TYPE(type) _handle_##type##_c
#define PUB_HANDLE_TYPE(type) type##_wrapper_##_c
#define STR_INTERNAL_CONTEXT_TYPE(type) STRINGIFY(type##_c)
#endif

/* K4A_DECLARE_CONTEXT creates type matched C functions to create, destroy and get the context. The create and destroy
functions will ensure matched CPP constructor and destructor are called. To protext against the create function being
used with CPP and destroy being used with C, or vise-vesa, the types get c or cpp appended to them. */
#define K4A_DECLARE_CONTEXT(_public_handle_name_, _internal_context_type_)                                             \
    extern char PRIV_HANDLE_TYPE(_public_handle_name_)[];                                                              \
    KSELECTANY char PRIV_HANDLE_TYPE(_public_handle_name_)[] = STR_INTERNAL_CONTEXT_TYPE(_internal_context_type_);     \
    typedef struct PUB_HANDLE_TYPE(_public_handle_name_)                                                               \
    {                                                                                                                  \
        char *handleType;                                                                                              \
        _internal_context_type_ context;                                                                               \
    } PUB_HANDLE_TYPE(_public_handle_name_);                                                                           \
                                                                                                                       \
    /* Define "context_t* handle_t_create(handle_t* handle)" function */                                               \
    static inline _internal_context_type_ *_public_handle_name_##_create(_public_handle_name_ *handle)                 \
    {                                                                                                                  \
        PUB_HANDLE_TYPE(_public_handle_name_) * pContextWrapper;                                                       \
        *handle = NULL;                                                                                                \
        pContextWrapper = ALLOCATE(PUB_HANDLE_TYPE(_public_handle_name_));                                             \
        if (pContextWrapper == NULL)                                                                                   \
        {                                                                                                              \
            IF_LOGGER(LOG_ERROR("Failed to allocate " #_public_handle_name_, 0);) return NULL;                         \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            IF_LOGGER(LOG_TRACE("Created   " #_public_handle_name_ " %p", pContextWrapper);)                           \
        }                                                                                                              \
        pContextWrapper->handleType = PRIV_HANDLE_TYPE(_public_handle_name_);                                          \
        *handle = (_public_handle_name_)pContextWrapper;                                                               \
        return &pContextWrapper->context;                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    /* Define "context_t* handle_t_get_context(handle_t handle)" function */                                           \
    static inline _internal_context_type_ *_public_handle_name_##_get_context(_public_handle_name_ handle)             \
    {                                                                                                                  \
        if ((handle == NULL) ||                                                                                        \
            ((PUB_HANDLE_TYPE(_public_handle_name_) *)handle)->handleType != PRIV_HANDLE_TYPE(_public_handle_name_))   \
        {                                                                                                              \
            IF_LOGGER(LOG_ERROR("Invalid " #_public_handle_name_ " %p", handle);)                                      \
            return NULL;                                                                                               \
        }                                                                                                              \
        return &(((PUB_HANDLE_TYPE(_public_handle_name_) *)handle)->context);                                          \
    }                                                                                                                  \
                                                                                                                       \
    /* Define "void handle_t_destroy(handle_t handle) function */                                                      \
    static inline void _public_handle_name_##_destroy(_public_handle_name_ handle)                                     \
    {                                                                                                                  \
        (void)_public_handle_name_##_get_context(handle);                                                              \
        IF_LOGGER(LOG_TRACE("Destroyed " #_public_handle_name_ " %p", handle);)                                        \
        ((PUB_HANDLE_TYPE(_public_handle_name_) *)handle)->handleType = NULL;                                          \
        DESTROY((PUB_HANDLE_TYPE(_public_handle_name_) *)handle);                                                      \
    }

/*
 * Example:

// Declare the handle in your public header
K4A_DECLARE_HANDLE(foo_t);

// Declare your context struct in your private C implementation file
typedef struct {
    int my;
    int data;
} context_t;
K4A_DECLARE_CONTEXT(foo_t, context_t );

foo_t foo_create()
{
    // Allocates a new handle and context
    foo_t handle;
    context_t* c = foo_t_create(&handle);

    // operate on your context
    c->my = 1;

    return handle;
}

foo_do_things(foo_t handle)
{
    // Get the context from the handle
    // Asserts failure if the handle is not the correct type or is null
    context_t* c = foo_t_get_context(handle);

    c->data = 1;
}

foo_destroy(foo_t handle)
{
    // Frees the handle and context
    foo_t_destroy(handle);
}

*/

#ifdef __cplusplus
}
#endif

#endif /* K4A_INTERNAL_HANDLE_H */
