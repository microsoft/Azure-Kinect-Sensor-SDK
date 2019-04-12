// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/dynlib.h>

// Dependent libraries
#include <k4ainternal/logging.h>

// System dependencies
#include <windows.h>
#include <stdio.h>

#define TOSTRING(x) STRINGIFY(x)

typedef struct _dynlib_context_t
{
    HMODULE handle;
} dynlib_context_t;

K4A_DECLARE_CONTEXT(dynlib_t, dynlib_context_t);

static char *generate_file_name(const char *name, uint32_t major_ver, uint32_t minor_ver)
{
    size_t max_buffer_size = strlen(name) + strlen(TOSTRING(DYNLIB_MAX_MAJOR_VERSION)) +
                             strlen(TOSTRING(DYNLIB_MAX_MINOR_VERSION)) + strlen("_") + strlen("_") + 1;

    char *versioned_file_name = malloc(max_buffer_size);
    if (versioned_file_name == NULL)
    {
        LOG_ERROR("malloc failed with size %llu", max_buffer_size);
        return NULL;
    }
    versioned_file_name[0] = '\0';
    snprintf(versioned_file_name, max_buffer_size, "%s_%u_%u", name, major_ver, minor_ver);

    return versioned_file_name;
}

k4a_result_t dynlib_create(const char *name, uint32_t major_ver, uint32_t minor_ver, dynlib_t *dynlib_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, name == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, dynlib_handle == NULL);

    if (major_ver > DYNLIB_MAX_MAJOR_VERSION)
    {
        LOG_ERROR("Failed to load dynamic library %s. major_ver %u is too large to load. Max is %u\n",
                  name,
                  major_ver,
                  DYNLIB_MAX_MAJOR_VERSION);
        return K4A_RESULT_FAILED;
    }

    if (minor_ver > DYNLIB_MAX_MINOR_VERSION)
    {
        LOG_ERROR("Failed to load dynamic library %s. minor_ver %u is too large to load. Max is %u\n",
                  name,
                  minor_ver,
                  DYNLIB_MAX_MINOR_VERSION);
        return K4A_RESULT_FAILED;
    }

    char *versioned_name = generate_file_name(name, major_ver, minor_ver);
    if (versioned_name == NULL)
    {
        return K4A_RESULT_FAILED;
    }

    dynlib_context_t *dynlib = dynlib_t_create(dynlib_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(dynlib != NULL);

    if (K4A_SUCCEEDED(result))
    {
        dynlib->handle = LoadLibraryA(versioned_name);
        result = (dynlib->handle != NULL) ? K4A_RESULT_SUCCEEDED : K4A_RESULT_FAILED;

        if (K4A_FAILED(result))
        {
            LOG_ERROR("Failed to load DLL %s with error code: %u", versioned_name, GetLastError());
        }
    }

    if (versioned_name != NULL)
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

    void *ptr = (void *)GetProcAddress(dynlib->handle, symbol);
    result = K4A_RESULT_FROM_BOOL(ptr != NULL);

    if (K4A_SUCCEEDED(result))
    {
        *address = ptr;
    }
    else
    {
        LOG_ERROR("Failed to find symbol %s in dynamic library", symbol);
    }

    return result;
}

void dynlib_destroy(dynlib_t dynlib_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, dynlib_t, dynlib_handle);

    dynlib_context_t *dynlib = dynlib_t_get_context(dynlib_handle);

    BOOL freed = FreeLibrary(dynlib->handle);
    if (!freed)
    {
        LOG_ERROR("Failed to unload dynamic library");
    }

    dynlib_t_destroy(dynlib_handle);
    dynlib = NULL;
}