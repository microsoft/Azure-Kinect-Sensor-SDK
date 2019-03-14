/** \file handle.h
 * A C++ modified version of handle.h in Kinect For Azure SDK.
 */

#ifndef K4A_INTERNAL_HANDLE_CPP_H
#define K4A_INTERNAL_HANDLE_CPP_H

#include <k4ainternal/logging.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K4A_DECLARE_CONTEXT_CPP(_public_handle_name_, _internal_context_type_)                                         \
    static const char _handle_##_public_handle_name_[] = #_internal_context_type_;                                     \
    typedef struct                                                                                                     \
    {                                                                                                                  \
        const char *handleType;                                                                                        \
        _internal_context_type_ context;                                                                               \
    } _public_handle_name_##_wrapper;                                                                                  \
                                                                                                                       \
    /* Define "context_t* handle_t_create(handle_t* handle)" function */                                               \
    static inline _internal_context_type_ *_public_handle_name_##_create(_public_handle_name_ *handle)                 \
    {                                                                                                                  \
        _public_handle_name_##_wrapper *pContextWrapper = new (std::nothrow) _public_handle_name_##_wrapper;           \
        if (pContextWrapper == NULL)                                                                                   \
        {                                                                                                              \
            IF_LOGGER(logger_error(LOGGER_K4A, "Failed to allocate " #_public_handle_name_);) return NULL;             \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            IF_LOGGER(logger_trace(LOGGER_K4A, "Created   " #_public_handle_name_ " %p", pContextWrapper);)            \
        }                                                                                                              \
        pContextWrapper->handleType = _handle_##_public_handle_name_;                                                  \
        *handle = (_public_handle_name_)pContextWrapper;                                                               \
        return &pContextWrapper->context;                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    /* Define "context_t* handle_t_get_context(handle_t handle)" function */                                           \
    static inline _internal_context_type_ *_public_handle_name_##_get_context(_public_handle_name_ handle)             \
    {                                                                                                                  \
        if ((handle == NULL) ||                                                                                        \
            ((_public_handle_name_##_wrapper *)handle)->handleType != _handle_##_public_handle_name_)                  \
        {                                                                                                              \
            IF_LOGGER(logger_error(LOGGER_K4A, "Invalid " #_public_handle_name_ " %p", handle);)                       \
            return NULL;                                                                                               \
        }                                                                                                              \
        return &((_public_handle_name_##_wrapper *)handle)->context;                                                   \
    }                                                                                                                  \
                                                                                                                       \
    /* Define "void handle_t_destroy(handle_t handle) function */                                                      \
    static inline void _public_handle_name_##_destroy(_public_handle_name_ handle)                                     \
    {                                                                                                                  \
        (void)_public_handle_name_##_get_context(handle);                                                              \
        IF_LOGGER(logger_trace(LOGGER_K4A, "Destroyed " #_public_handle_name_ " %p", handle);)                         \
        ((_public_handle_name_##_wrapper *)handle)->handleType = NULL;                                                 \
        delete (_public_handle_name_##_wrapper *)handle;                                                               \
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
K4A_DECLARE_CONTEXT_CPP(foo_t, context_t );

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

#endif /* K4A_INTERNAL_HANDLE_CPP_H */
