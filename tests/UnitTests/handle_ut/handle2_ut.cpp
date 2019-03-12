// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include "handle_ut.h"

int is_handle_in_2nd_file_valid(dual_defined_t handle)
{
    dual_defined_context_t *context = dual_defined_t_get_context(handle);

    return (context != NULL);
}
