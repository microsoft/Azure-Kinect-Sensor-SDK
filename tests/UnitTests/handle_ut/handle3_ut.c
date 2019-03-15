// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <inttypes.h>
#include "handle_ut.h"

// K4A_DECLARE_HANDLE(dual_defined_t) is in the header path for this file, so it is created with this C file and in CPP
// files from other source code in this test. They should not be usable
int is_handle_in_c_file_valid(dual_defined_t handle)
{
    dual_defined_context_t *context = dual_defined_t_get_context(handle);

    return (context != NULL);
}
