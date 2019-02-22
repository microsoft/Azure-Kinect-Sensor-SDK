// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*--------------- Includes --------------*/
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "Cli.h"

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_LINE_HISTORY 2
#define MAX_MENUS 100

/*-------------- Typedefs ----------------*/
typedef enum _CLI_ESC_CTRL_T
{
    CLI_ESC_NONE,
    CLI_ESC_PENDING1,
    CLI_ESC_PENDING2,
    CLI_ESC_UP
} CLI_ESC_CTRL_T;

typedef struct _CLI_COMMAND_LIST
{
    CLI_COMMAND_T Cmd;
    struct _CLI_COMMAND_LIST *Next;
} CLI_COMMAND_LIST;

/*-- Declarations (Statics and globals) --*/
static char *Prompt = CLI_MAIN_MENU;
static CLI_COMMAND_LIST *RegisteredCmds = NULL;
static bool ShellOpen = false;

/*--------- Function Prototypes ----------*/
void Help(int argc, char **argv);
bool StrHasUpper(const char *pszStr);

/*-------------- Functions ---------------*/
/**
 *  Converts a string to a UINT32 value
 *
 *  @param str
 *   Pointer to the string to conver
 *
 *  @return
 *  TRUE     The data was correctly formated and converted
 *  FALSE    The data MAY have been converted but it was incorrectly formated
 *
 */
bool CliGetStrVal(const char *str, uint32_t *pOut)
{
    size_t len = strlen(str);

    // Default to decimal argument.
    *pOut = 0;
    int32_t base = 10;
    uint32_t start = 0;

    if (len == 0)
        return false;

    // If find '0x' or '0X', change to hexadecimal argument.
    if (len > 1 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        start = 2;
        base = 16;
    }

    *pOut = (uint32_t)strtoul(&str[start], NULL, base);
    return true;
}

/**
 *  function to extract binary data from space separated string
 *
 *  @param pSrc  Pointer to the string source
 *  @param pDst  Pointer to where the binary data will be placed
 *  @param Size  Size of the Destination pointed to by pDst
 *
 *  @return
 *       UINT8   Numb of bytes converted
 */
uint8_t CliGetBin(char *pSrc, uint8_t *pDst, uint32_t Size)
{
    uint8_t Index = 0;
    uint32_t Value;

    // Go through the input string and parse to binary data
    while ((*pSrc != 0) && (Index < Size))
    {
        // skip white spaces
        while ((*pSrc < 48) && (*pSrc != 0))
        {
            pSrc++;
        }
        if (*pSrc != 0)
        {
            CliGetStrVal((const char *)pSrc, &Value);
            pDst[Index++] = Value & 0xFF;
            // move pointer beyond value
            while ((*pSrc > 47) && (*pSrc != 0))
            {
                pSrc++;
            }
        }
    }
    return Index;
}

/**
 *   This function detects if an escape character
 *   is entered.
 *
 *  @param Ch   Character to include for esc check
 *
 *   @return
 *       CLI_ESC_NONE        No characters detected
 *       CLI_ESC_PENDING1    First stage of escape character detected
 *       CLI_ESC_PENDING2    Second stage of escape character detected
 *       CLI_ESC_UP      -   Up arrow detected
 */
static CLI_ESC_CTRL_T CliEsc(char Ch)
{
    static CLI_ESC_CTRL_T EscChar = CLI_ESC_NONE;

    if (Ch == 0x1B)
    {
        EscChar = CLI_ESC_PENDING1;
    }

    else if ((EscChar == CLI_ESC_PENDING1) && (Ch == '['))
    {
        EscChar = CLI_ESC_PENDING2;
    }

    else if (EscChar == CLI_ESC_PENDING2 && Ch == 'A')
    {
        EscChar = CLI_ESC_UP;
    }
    else
    {
        EscChar = CLI_ESC_NONE;
    }

    return EscChar;
}

/**
 *  Get Line from serial stream
 *
 *  @param pBuffer   Pointer to the buffer the will hold the line
 *
 *  @param Length    The size of the buffer handed in
 *
 *  @return
 *   uint16_t  number of bytes received
 *
 */
static uint16_t CliGetLine(char *pBuffer, uint8_t Length)
{
    char Ch;
    uint16_t NumChar = 0;
    static char Line[MAX_LINE_HISTORY][MAX_LINE_LENGTH];
    static uint8_t LineCount;
    static uint8_t LinePosition;
    CLI_ESC_CTRL_T EscCtrl;

    while (NumChar < Length)
    {
        Ch = (char)getchar();

        // Check for LF or CR.
        if (Ch == '\n' || Ch == '\r')
        {
            pBuffer[NumChar] = 0;

            // Exit loop
            break;
        }

        // Check for full cmd buffer.
        if (NumChar >= Length - 1)
        {
            continue;
        }

        // Check for ESC sequence: <ESC> [ 1 3 ~
        EscCtrl = CliEsc(Ch);
        if (EscCtrl != CLI_ESC_NONE)
        {
            if ((EscCtrl == CLI_ESC_UP) && (LineCount > 0))
            {
                // Last command
                // go back 1 cmd position
                if (LinePosition != 0)
                {
                    LinePosition--;
                }
                else
                {
                    LinePosition = LineCount - 1;
                }

                // backspace current number of characters
                while (NumChar > 0)
                {
                    NumChar--;
                }

                // Copy last command to buffer
                NumChar = (uint16_t)strlen(Line[LinePosition]);
                if (NumChar > Length)
                {
                    NumChar = Length;
                }

                memcpy(pBuffer, Line[LinePosition], NumChar);
            }
            continue;
        }

        // Check for backspace or del character.
        else if ((Ch == '\b') || (Ch == 0x7F))
        {
            if (NumChar > 0)
            {
                NumChar--;
            }
        }

        // Check for uppercase character.
        else if (Ch >= 'A' && Ch <= 'Z')
        {
            pBuffer[NumChar++] = Ch - 'A' + 'a';
        }

        // Check for printable character.
        else
        {
            pBuffer[NumChar++] = Ch;
        }
    }

    if (NumChar > 0)
    {
        // Copy to buffer history
        memcpy(Line[LinePosition], pBuffer, strlen(pBuffer));

        LinePosition++;
        if (LinePosition > LineCount)
        {
            LineCount = LinePosition;
        }
        if (LinePosition >= MAX_LINE_HISTORY)
        {
            LinePosition = 0; // loop through available buffer
        }
    }

    return NumChar;
}

/**
 *  This utility function will parse the input line by separating
 *  the arguments by NULLs.
 *
 *  @param pLine   Pointer to the string to parse
 *
 *  @param Argv    Pointer to the pointer array for arguments
 *
 *  @param Length  Maximum number of arguments to parse
 *
 *  @return
 *   int           Number of arguments found
 *
 */
static int ParseLine(char *pLine, char **Argv, uint8_t Length)
{
    int Argc;
    char Ch;
    bool StringVar = false;

    for (Argc = 0; Argc < Length;)
    {
        // Skip whitespace.
        while ((Ch = *pLine) != 0)
        {
            if (Ch == '"')
            {
                StringVar = true;
            }
            if (Ch != ' ' && Ch != '\t' && Ch != '"')
            {
                break;
            }
            pLine++;
        }
        if (Ch == 0)
            break;

        // Record start of current argument.
        Argv[Argc] = pLine;
        Argc++;

        // Skip up to next whitespace.
        while ((Ch = *pLine) != 0)
        {
            if (StringVar)
            {
                if (Ch == '"')
                {
                    StringVar = false;
                    break;
                }
            }
            else
            {
                if (Ch == ' ' || Ch == '\t')
                    break;
            }
            pLine++;
        }

        // Terminate argument string.
        if (Ch != 0)
        {
            *pLine++ = 0;
        }
    }
    return Argc;
}

/**
 *   Shell loop for processing commands.
 *
 */
static void CliShell(void)
{
    char Buffer[MAX_LINE_LENGTH];
    static int MyArgc;
    static char *MyArgv[MAX_ARGS];

    ShellOpen = true;
    printf("%s>", Prompt);

    while (ShellOpen)
    {
        CliGetLine(Buffer, MAX_LINE_LENGTH);

        MyArgc = ParseLine(Buffer, MyArgv, MAX_ARGS);

        // Parse and execute command.
        CliExecute(MyArgc, &MyArgv[0]);
    }
}

/**
 *   Exit command for shell
 *
 */
static void CliExit(void)
{
    ShellOpen = false;
}

/**
 *  This function is execute the input from the user
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
int CliExecute(int Argc, char **Argv)
{
    CLI_COMMAND_LIST *pCmdEntry = RegisteredCmds;
    CLI_COMMAND_T *pCmd;

    CLI_STATUS Status = CLI_SUCCESS;

    // Search for command.
    printf("\n");

    // check for help
    if ((Argc == 0) || (strcmp(Argv[0], "?") == 0))
    {
        // Print help information
        Help(Argc, &Argv[0]);
        printf("%s>", Prompt);
        return (int)Status;
    }

    // check for shell
    if (strcmp(Argv[0], "shell") == 0)
    {
        // Go into command line shell
        CliShell();
        return (int)Status;
    }

    // check for exit
    if (strcmp(Argv[0], "exit") == 0)
    {
        // Exit shell if in it
        CliExit();
        return (int)Status;
    }

    // check for main menu
    if (strcmp(Argv[0], "..") == 0)
    {
        // Go to main menu
        Prompt = CLI_MAIN_MENU;
        printf("%s>", Prompt);
        return (int)Status;
    }

    // Find command
    while (pCmdEntry != NULL)
    {
        pCmd = &(pCmdEntry->Cmd);
        if ((strcmp(Argv[0], pCmd->Menu) == 0) && (Argc == 1))
        {
            // Change prompts and menu level
            Prompt = pCmd->Menu;
            break;
        }

        if ((strcmp(Prompt, pCmd->Menu) == 0) && (strcmp(Argv[0], pCmd->Command) == 0))
        {
            Status = pCmd->Routine(Argc, &Argv[0]);
            if (Status == CLI_SUCCESS)
            {
                printf("Ok\n");
            }
            else
            {
                printf("ERROR\n");
            }
            break;
        }
        else if ((Argc > 1) && (strcmp(Argv[0], pCmd->Menu) == 0) && (strcmp(Argv[1], pCmd->Command) == 0))
        {
            Status = pCmd->Routine(Argc - 1, &Argv[1]);
            if (Status == CLI_SUCCESS)
            {
                printf("Ok\n");
            }
            else
            {
                printf("ERROR\n");
            }
            break;
        }
        pCmdEntry = pCmdEntry->Next;
    };

    if (pCmdEntry == NULL)
    {
        // No commands found to match
        printf("'%s' Invalid command.  Type ? for help.\n", Argv[0]);
        Status = CLI_ERROR;
    }

    printf("%s>", Prompt);
    return (int)Status;
}

/**
 *  This function is used to display help information
 *
 *  @param Argc  Number of variables handed in
 *
 *  @param Argv  Pointer to variable array
 *
 */
void Help(int Argc, char **Argv)
{
    CLI_COMMAND_LIST *pCmdEntry = RegisteredCmds;
    CLI_COMMAND_T *pCmd;
    uint32_t y;

    char *Menus[MAX_MENUS] = { NULL };

    // Display menu or usage information
    while (pCmdEntry != NULL)
    {
        pCmd = &(pCmdEntry->Cmd);
        // Extract list of menus
        for (y = 0; y < MAX_MENUS; y++)
        {
            if (Menus[y] == NULL)
            {
                Menus[y] = pCmd->Menu;
                break;
            }
            else if (strcmp(Menus[y], pCmd->Menu) == 0)
            {
                break;
            }
        }
        if (Argc == 1)
        {
            // List all command at current level
            if (strcmp(Prompt, pCmd->Menu) == 0)
            {
                printf("%s", pCmd->Command);
                printf("%s", &"                "[strlen(pCmd->Command) & 0xF]);
                printf("%s\n", pCmd->Description);
            }
        }
        else if (Argc == 2)
        {
            // Display help on item at current level
            if ((strcmp(Prompt, pCmd->Menu) == 0) && (strcmp(Argv[1], pCmd->Command) == 0) && (pCmd->Usage != NULL))
            {
                printf("%s\n", pCmd->Usage);
                break;
            }
        }
        else if (Argc == 3)
        {
            // Display sub menu item help
            if ((strcmp(Argv[1], pCmd->Menu) == 0) && (strcmp(Argv[2], pCmd->Command) == 0) && (pCmd->Usage != NULL))
            {
                printf("%s\n", pCmd->Usage);
                break;
            }
        }

        pCmdEntry = pCmdEntry->Next;
        if (pCmdEntry == NULL)
        {
            // Print out default commands in menu list
            if (strncmp(Prompt, CLI_MAIN_MENU, sizeof(CLI_MAIN_MENU)) != 0)
            {
                printf("..              Goto Main Menu\n");
            }
            else
            {
                // print out available sub-menus
                for (y = 0; y < MAX_MENUS; y++)
                {
                    if ((Menus[y] != 0) && (*Menus[y] != 0))
                    {
                        printf("%s", Menus[y]);
                        printf("%s", &"                "[strlen(Menus[y]) & 0xF]);
                        printf("Menu\n");
                    }
                }
            }

            printf("?               Display menu or usage information\n");
            break;
        }
    }
}

/**
 *  Dynamic registration API for CLI commands
 *
 *  @param pMenu        String pointer for menu syntax
 *
 *  @param pSyntax      String pointer for command syntax
 *
 *  @param Routine      Function pointer associated with the command
 *
 *  @param pDescription String pointer describing the command
 *
 *  @param pUsage       Usage string
 *
 */
void CliRegister(char *pMenu,
                 const char *pSyntax,
                 CLI_STATUS (*Routine)(int argc, char **argv),
                 const char *pDescription,
                 const char *pUsage)
{
    assert((pSyntax != NULL) && (Routine != NULL) && (pMenu != NULL));
    CLI_COMMAND_LIST *pCmdEntry = RegisteredCmds;
    CLI_COMMAND_LIST *pNewEntry;

    assert(!StrHasUpper(pMenu));
    assert(!StrHasUpper(pSyntax));

    pNewEntry = malloc(sizeof(CLI_COMMAND_LIST));
    assert(pNewEntry != NULL);
    pNewEntry->Cmd.Menu = pMenu;
    pNewEntry->Cmd.Command = pSyntax;
    pNewEntry->Cmd.Routine = Routine;
    pNewEntry->Cmd.Description = pDescription;
    pNewEntry->Cmd.Usage = pUsage;
    pNewEntry->Next = NULL;

    // Fist command?
    if (RegisteredCmds == NULL)
    {
        RegisteredCmds = pNewEntry;
    }
    else
    {
        // Find last command
        while (pCmdEntry->Next != NULL)
        {
            pCmdEntry = pCmdEntry->Next;
        }
        pCmdEntry->Next = pNewEntry;
    }
}

/**
 *  Dynamic registration API for CLI commands
 *
 *  @param pTable        Pointer to the array of commands to register with
 *                       the command engine.
 *
 */
void CliRegisterTable(CLI_COMMAND_T *pTable)
{
    while (pTable->Command != NULL)
    {
        CliRegister(pTable->Menu, pTable->Command, pTable->Routine, pTable->Description, pTable->Usage);
        pTable++;
    }
}

/**
 *  Method for prompting information from the user
 *
 *  @param pPrompt       The prompt string that will be displayed to the
 *                       user when this function is called.  If NULL, no
 *                       prompt will be displayed.
 *
 *  @param pBuffer       Pointer to the location used to hold
 *                       the input string
 *
 *  @param Length        Size of buffer handed in
 *
 *  @return
 *       uint16_t          Number of bytes that were placed in pBuffer
 */
uint16_t CliRead(char *pPrompt, char *pBuffer, uint8_t Length)
{
    if (pPrompt != NULL)
    {
        printf("%s", pPrompt);
    }
    return CliGetLine(pBuffer, Length);
}

/**
 *  Method to display the usage string of the function  associated with the
 *  command syntax
 *
 *  @param Routine       Function pointer to the function associated
 *                       with the command syntax.
 *
 */
void CliDisplayUsage(CLI_STATUS (*Routine)(int argc, char **argv))
{
    CLI_COMMAND_LIST *pCmdEntry = RegisteredCmds;

    while (pCmdEntry != NULL)
    {
        if ((pCmdEntry->Cmd.Routine == Routine) && (pCmdEntry->Cmd.Usage != NULL))
        {
            printf("%s\n", pCmdEntry->Cmd.Usage);
            break;
        }
        pCmdEntry = pCmdEntry->Next;
    }
}

/**
 *  Check if a string has any upper case charater
 *
 *  @param pszStr       pointer to a NULL terminated string
 *
 *  @return
 *       true if an upper case character is found in the string
 *       false if no upper case character is found in the string
 */
bool StrHasUpper(const char *pszStr)
{
    while (pszStr && *pszStr)
    {
        if (isupper(*pszStr++))
        {
            return true;
        }
    }
    return false;
}
