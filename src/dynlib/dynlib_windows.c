// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/dynlib.h>

// Dependent libraries
#include <k4ainternal/logging.h>

// System dependencies
#include <windows.h>
#include <stdio.h>
#include <pathcch.h>

#define TOSTRING(x) STRINGIFY(x)

typedef struct _dynlib_context_t
{
    HMODULE handle;
} dynlib_context_t;

K4A_DECLARE_CONTEXT(dynlib_t, dynlib_context_t);

static char *generate_file_name(const char *name, uint32_t version)
{
    // Format of the depth engine name is: depthengine_<K4A_PLUGIN_VERSION>_0
    //                                     depthengine_2_0
    size_t max_buffer_size = strlen(name) + strlen(TOSTRING(DYNLIB_MAX_VERSION)) + strlen("_0") + strlen("_") + 1;

    char *versioned_file_name = malloc(max_buffer_size);
    if (versioned_file_name == NULL)
    {
        LOG_ERROR("malloc failed with size %llu", max_buffer_size);
        return NULL;
    }
    versioned_file_name[0] = '\0';

    // NOTE: _0 is appended to the name for legacy reasons, a time when the depth engine plugin version was tracked with
    // major and minor versions
    snprintf(versioned_file_name, max_buffer_size, "%s_%u_0", name, version);

    return versioned_file_name;
}

static DLL_DIRECTORY_COOKIE add_current_module_to_search()
{
    wchar_t path[MAX_PATH];
    HMODULE hModule = NULL;

    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)add_current_module_to_search, &hModule) == 0)
    {
        LOG_WARNING("Failed to get current module (%d).", GetLastError());
        return NULL;
    }

    if (GetModuleFileNameW(hModule, path, _countof(path)) == 0)
    {
        LOG_WARNING("Failed to get current module file name (%d).", GetLastError());
        return NULL;
    }

    HRESULT result = PathCchRemoveFileSpec(path, _countof(path));
    if (result != S_OK)
    {
        LOG_WARNING("Failed to remove the file name from the path (%d).", result);
        return NULL;
    }

    // This adds the directory of the current module (k4a.dll) to the loader's path.
    // The loader for C code only loads from the path of the current executable, not the current
    // module. By adding the current module path, this will mimic how C# and Linux loads DLLs.
    DLL_DIRECTORY_COOKIE dllDirectory = AddDllDirectory(path);
    if (dllDirectory == 0)
    {
        LOG_WARNING("Failed to add the directory to the DLL search path (%d).", GetLastError());
    }

    return dllDirectory;
}

k4a_result_t dynlib_create(const char *name, uint32_t version, dynlib_t *dynlib_handle)
{
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

    DLL_DIRECTORY_COOKIE dllDirectory = add_current_module_to_search();

    dynlib_context_t *dynlib = dynlib_t_create(dynlib_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(dynlib != NULL);

    if (K4A_SUCCEEDED(result))
    {
        dynlib->handle = LoadLibraryExA(versioned_name,
                                        NULL,
                                        LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS);
        result = (dynlib->handle != NULL) ? K4A_RESULT_SUCCEEDED : K4A_RESULT_FAILED;

        if (K4A_FAILED(result))
        {
            LOG_ERROR("Failed to load DLL %s with error code: %u", versioned_name, GetLastError());
        }
    }

    if (dynlib->handle)
    {
        char file_path[MAX_PATH];

        DWORD ret = GetModuleFileNameA(dynlib->handle, file_path, sizeof(file_path));
        result = K4A_RESULT_FROM_BOOL(ret <= sizeof(file_path));
        if (K4A_FAILED(result))
        {
            LOG_ERROR("Failed calling GetModuleFileNameA %x", GetLastError());
        }
        else
        {
            LOG_INFO("Depth Engine loaded %s", file_path);
        }
    }

    if (dllDirectory != NULL)
    {
        if (RemoveDllDirectory(dllDirectory) == 0)
        {
            LOG_WARNING("Failed to remove the directory from the DLL search path (%d).", GetLastError());
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
        LOG_ERROR("Failed to find symbol %s in dynamic library, GLE is 0x%08x", symbol, GetLastError());
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
