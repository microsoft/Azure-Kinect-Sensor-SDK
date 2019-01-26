// This library
#include <k4ainternal/dynlib.h>

// Dependent libraries
#include <k4ainternal/logging.h>

// System dependencies
#include <windows.h>

typedef struct _dynlib_context_t
{
    HMODULE handle;
} dynlib_context_t;

K4A_DECLARE_CONTEXT(dynlib_t, dynlib_context_t);

k4a_result_t dynlib_create(const char *name, dynlib_t *dynlib_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, name == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, dynlib_handle == NULL);

    dynlib_context_t *dynlib = dynlib_t_create(dynlib_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(dynlib != NULL);

    if (K4A_SUCCEEDED(result))
    {
        dynlib->handle = LoadLibraryA(name);
        result = K4A_RESULT_FROM_BOOL(dynlib->handle != NULL);

        if (K4A_FAILED(result))
        {
            LOG_ERROR("Failed to load DLL %s with error code: %u", name, GetLastError());
        }
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