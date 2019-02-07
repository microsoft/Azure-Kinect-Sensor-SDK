#include <stdio.h>

#ifndef _WIN32
#define __declspec(arg)
#define __cdecl
#endif

__declspec(dllexport) void __cdecl say_hello(void);

__declspec(dllexport) void __cdecl say_hello(void)
{
    printf("Hello!\n");
}