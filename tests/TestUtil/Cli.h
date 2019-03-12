// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
/*----------- Nested includes ------------*/
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_ARGS 64
#define MAX_LINE_LENGTH 128
#define CLI_MAIN_MENU ""

/*-------------- Typedefs ----------------*/
typedef enum _CLI_STATUS
{
    CLI_SUCCESS,
    CLI_ERROR
} CLI_STATUS;

typedef struct _CLI_COMMAND_T
{
    char *Menu;
    const char *Command;
    CLI_STATUS (*Routine)(int argc, char **argv);
    const char *Description;
    const char *Usage;
} CLI_COMMAND_T;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

int CliExecute(int Argc, char **Argv);
void CliRegister(char *pMenu,
                 const char *pSyntax,
                 CLI_STATUS (*Routine)(int argc, char **argv),
                 const char *pDescription,
                 const char *pUsage);
void CliRegisterTable(CLI_COMMAND_T *pTable);
bool CliGetStrVal(const char *str, uint32_t *pOut);
uint8_t CliGetBin(char *pSrc, uint8_t *pDst, uint32_t Size);
uint16_t CliRead(char *pPrompt, char *pString, uint8_t Length);
void CliDisplayUsage(CLI_STATUS (*Routine)(int argc, char **argv));
uint32_t CliGetChar(char *Ch);

#ifdef __cplusplus
}
#endif
