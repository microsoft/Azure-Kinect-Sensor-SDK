// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/global.h>

// Dependent libraries
#include <k4ainternal/capture.h>
#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/refcount.h>
#include <azure_c_shared_utility/threadapi.h>

// System dependencies
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
static INIT_ONCE g_InitOnce = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK InitGlobalFunction(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContext)
{
    UNREFERENCED_PARAMETER(InitOnce);
    UNREFERENCED_PARAMETER(lpContext);

    k4a_init_once_function_t *callback = (k4a_init_once_function_t *)Parameter;

    callback();

    return TRUE;
}
#endif

void global_init_once(k4a_init_once_t *init_once, k4a_init_once_function_t *init_function)
{

#ifdef _WIN32
    if (InitOnceExecuteOnce(init_once, InitGlobalFunction, (void *)init_function, NULL))
    {
        return;
    }
#else
    if (0 == pthread_once(init_once, init_function))
    {
        return;
    }
#endif

    assert(false);
    return;
}
