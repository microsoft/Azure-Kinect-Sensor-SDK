// This library
#include <k4ainternal/dynlib.h>

// Dependent libraries
#include <k4ainternal/logging.h>

// System dependencies
#include <stdlib.h>
#include <ctype.h>
#include <dlfcn.h>

typedef struct _dynlib_context_t
{
    void *handle;
} dynlib_context_t;

K4A_DECLARE_CONTEXT(dynlib_t, dynlib_context_t);

k4a_result_t dynlib_create(const char *name, dynlib_t *dynlib_handle)
{
    // Note: A nullptr is allowed on linux systems for "name"
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, dynlib_handle == NULL);

    dynlib_context_t *dynlib = dynlib_t_create(dynlib_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(dynlib != NULL);

    if (K4A_SUCCEEDED(result))
    {
        dynlib->handle = dlopen(name, RTLD_NOW);

        if ((dynlib->handle == NULL) && name && isalpha(name[0]))
        {
            // try to load "lib" + name + ".so" this emulates Windows
            // automatically adding .DLL to name
            const char *lib_prefix = "lib";
            const char *lib_suffix = ".so";
            char *full_name = malloc(strlen(lib_prefix) + strlen(name) + strlen(lib_suffix) + 1);
            full_name[0] = '\0';
            strcat(full_name, lib_prefix);
            strcat(full_name, name);
            strcat(full_name, lib_suffix);

            dynlib->handle = dlopen(full_name, RTLD_NOW);
            free(full_name);
        }

        result = (dynlib->handle != NULL) ? K4A_RESULT_SUCCEEDED : K4A_RESULT_FAILED;

        if (K4A_FAILED(result))
        {
            LOG_ERROR("Failed to load shared object %s with error: %s", name, dlerror());
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
