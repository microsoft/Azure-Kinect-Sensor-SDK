// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include <iostream>
#include <map>
#include <functional>

#ifdef StubExport
#define STUB_EXPORT __declspec(dllexport)
#else
#define STUB_EXPORT __declspec(dllimport)
#endif

// These functions are exported by the Stub module itself and are callable by either
// The test application or the stub implementations.
extern "C" {
STUB_EXPORT extern FARPROC Stub_GetFunctionPointer(std::string functionName);
STUB_EXPORT extern void Stub_RecordCall(char *szFunction);
}

template<typename Signature> std::function<Signature> farproc_to_function(FARPROC f)
{
    return std::function<Signature>(reinterpret_cast<Signature *>(f));
}

template<typename Signature, typename T, typename... Arguments> T redirect(std::string functionName, Arguments... args)
{
    auto r = farproc_to_function<Signature>(Stub_GetFunctionPointer(functionName));
    return r(args...);
}
