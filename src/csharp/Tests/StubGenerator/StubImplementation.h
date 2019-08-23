// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

typedef void(RaiseError)(char *szFile, int line, char *expression);

RaiseError *g_ErrorHandler = NULL;

// These functions are exported by the Stub module itself and are callable by either
// The test application or the stub implementations.
extern "C" {
__declspec(dllexport) extern void Stub_SetErrorFunction(RaiseError *pfnErrorHandler);
}

void Stub_SetErrorFunction(RaiseError *pfnErrorHandler)
{
    g_ErrorHandler = pfnErrorHandler;
}

inline void Stub_Assert(char *szFile, int line, bool expressionValue, char *expression)
{
    if (!expressionValue)
    {
        if (g_ErrorHandler)
        {
            g_ErrorHandler(szFile, line, expression);
        }
        else
        {
            throw 0;
        }
    }
}

#define STUB_ASSERT(_expr_) Stub_Assert(__FILE__, __LINE__, (_expr_), #_expr_)

#define STUB_FAIL(_message_) Stub_Assert(__FILE__, __LINE__, (false), _message_)
