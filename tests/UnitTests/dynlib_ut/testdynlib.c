// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "test_dynlib_export.h"

#include <stdio.h>

TEST_DYNLIB_EXPORT void say_hello(void);

TEST_DYNLIB_EXPORT void say_hello(void)
{
    printf("Hello!\n");
}
