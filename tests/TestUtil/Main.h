// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

//************************ Includes *****************************

//**************Symbolic Constant Macros (defines)  *************

//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************
#ifdef __cplusplus
extern "C" {
#endif
k4a_device_t get_k4a_handle(uint32_t index);
void open_k4a(void);
void close_k4a(void);

#ifdef __cplusplus
}
#endif
