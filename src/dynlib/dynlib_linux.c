// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE // ldaddr() extention in dlfcn.h

// This library
#include <k4ainternal/dynlib.h>

// Dependent libraries
#include <k4ainternal/logging.h>

// System dependencies
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <dlfcn.h>

#define TOSTRING(x) STRINGIFY(x)

typedef struct _dynlib_context_t
{
    void *handle;
} dynlib_context_t;

K4A_DECLARE_CONTEXT(dynlib_t, dynlib_context_t);
static char *generate_file_name(const char *name, uint32_t version)
{
    const char *lib_prefix = "lib";
    const char *lib_suffix = "so";
    // Format of the depth engine name is: libdepthengine.so.<K4A_PLUGIN_VERSION>.0
    //                                     libdepthengine.so.2.0
    size_t max_buffer_size = strlen(name) + strlen(TOSTRING(DYNLIB_MAX_VERSION)) + strlen(".0") + strlen(".") +
                             strlen(".") + strlen(lib_suffix) + strlen(lib_prefix) + 1;

    char *versioned_file_name = malloc(max_buffer_size);
    if (versioned_file_name == NULL)
    {
        LOG_ERROR("malloc failed with size %llu", max_buffer_size);
        return NULL;
    }
    versioned_file_name[0] = '\0';
    // NOTE: 0 is appended to the name for legacy reasons, a time when the depth engine plugin version was tracked with
    // major and minor versions
    snprintf(versioned_file_name, max_buffer_size, "%s%s.%s.%u.0", lib_prefix, name, lib_suffix, version);

    return versioned_file_name;
}

k4a_result_t dynlib_create(const char *name, uint32_t version, dynlib_t *dynlib_handle)
{
    // Note: A nullptr is allowed on linux systems for "name", however, this is
    // functionality we do not support
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, name == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, dynlib_handle == NULL);

    if (version > DYNLIB_MAX_VERSION)
    {
        LOG_ERROR("Failed to load dynamic library %s. version %u is too large to load. Max is %u\n",
                  name,
                  version,
                  DYNLIB_MAX_VERSION);
        return K4A_RESULT_FAILED;
    }

    char *versioned_name = generate_file_name(name, version);
    if (versioned_name == NULL)
    {
        return K4A_RESULT_FAILED;
    }

    dynlib_context_t *dynlib = dynlib_t_create(dynlib_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(dynlib != NULL);

    if (K4A_SUCCEEDED(result))
    {
        dynlib->handle = dlopen(versioned_name, RTLD_NOW);

        result = (dynlib->handle != NULL) ? K4A_RESULT_SUCCEEDED : K4A_RESULT_FAILED;

        if (K4A_FAILED(result))
        {
            LOG_ERROR("Failed to load shared object %s with error: %s", versioned_name, dlerror());
        }
    }

    if (versioned_name)
    {
        free(versioned_name);
    }

    if (K4A_FAILED(result))
    {
        dynlib_t_destroy(*dynlib_handle);
        *dynlib_handle = NULL;
    }
    return result;
}

k4a_result_t dynlib_find_symbol(dynlib_t dynlib_handle, const char *symbol, void **address)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, dynlib_t, dynlib_handle);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, symbol == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, address == NULL);

    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    dynlib_context_t *dynlib = dynlib_t_get_context(dynlib_handle);

    void *ptr = dlsym(dynlib->handle, symbol);
    result = K4A_RESULT_FROM_BOOL(ptr != NULL);

    if (K4A_SUCCEEDED(result))
    {
        *address = ptr;
    }
    else
    {
        LOG_ERROR("Failed to find symbol %s in dynamic library. Error: ", symbol, dlerror());
    }

    if (K4A_SUCCEEDED(result))
    {
        Dl_info info;
        if (dladdr(*address, &info) != 0)
        {
            LOG_INFO("Depth Engine loaded %s", info.dli_fname);
        }
        else
        {
            LOG_ERROR("Failed calling dladdr %x", dlerror());
        }
    }

    return result;
}

void dynlib_destroy(dynlib_t dynlib_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, dynlib_t, dynlib_handle);

    dynlib_context_t *dynlib = dynlib_t_get_context(dynlib_handle);

    dlclose(dynlib->handle);

    dynlib_t_destroy(dynlib_handle);
    dynlib = NULL;
}
