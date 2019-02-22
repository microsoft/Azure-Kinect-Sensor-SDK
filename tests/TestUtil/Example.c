// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//************************ Includes *****************************
#include <k4a/k4a.h>
#include "Cli.h"
#include "Example.h"

//**************Symbolic Constant Macros (defines)  *************

//************************ Typedefs *****************************

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************

//*********************** Functions *****************************

/**
 *  Example command for displaying "bob" from 'example menu.
 *
 *  @param Argc  Number of variables handed in
 *
 *  @param Argv  Pointer to variable array
 *
 *  @return
 *   CLI_SUCCESS    If executed properly
 *   CLI_ERROR      If error was detected
 *
 */
static CLI_STATUS ExampleCmdBob(int Argc, char **Argv)
{
    (void)Argc; // unused
    (void)Argv; // unused
    printf("Hello, I'm Bob\n");
    return CLI_SUCCESS;
}

/**
 *  Example unit test file
 *
 */
void ExampleInit(void)
{
    CliRegister("example", "bob", ExampleCmdBob, "Display Bob", "Example for commands on main menu\nExample: bob");
}
