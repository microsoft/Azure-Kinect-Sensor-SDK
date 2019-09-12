// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "test_dynlib_export.h"

#include <stdio.h>

#ifndef _WIN32
#define __declspec(arg)
#define __cdecl
#endif

TEST_DYNLIB_EXPORT void __cdecl say_hello(void);

TEST_DYNLIB_EXPORT void __cdecl say_hello(void)
{
    printf("Hello!\n");
}
