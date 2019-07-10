// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Stub.h"
#include "StubImplementation.h"

std::map<std::string, FARPROC> functionLookup;
std::map<std::string, int> callCount;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" {

STUB_EXPORT int Stub_RegisterRedirect(char *functionName, FARPROC implementation)
{
    functionLookup[functionName] = implementation;

    return 0;
}

STUB_EXPORT int Stub_GetCallCount(char *functionName)
{
    if (callCount.find(functionName) == callCount.end())
    {
        return 0;
    }
    return callCount[functionName];
}

STUB_EXPORT FARPROC Stub_GetFunctionPointer(std::string functionName)
{
    std::map<std::string, FARPROC>::iterator it = functionLookup.find(functionName);
    if (it == functionLookup.end() || it->second == NULL)
    {
        STUB_FAIL("Stubbed function has no implementation");
    }
    return it->second;
}

STUB_EXPORT void Stub_RecordCall(char *functionName)
{
    callCount[functionName]++;
}

} // extern "C"