// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Mock for Windows System calls
#include <initguid.h>
#define _CFGMGR32_
#include "color_mock_windows.h"

using namespace Microsoft::WRL;
using namespace testing;

static MockWindowsSystem *g_mockWindows = nullptr;

#ifdef __cplusplus
extern "C" {
#endif

CMAPI void WINAPI SetWindowsSystemMock(MockWindowsSystem *pMockWindowsSystem)
{
    g_mockWindows = pMockWindowsSystem;
}

// CM_Get_Device_Interface_PropertyW
CMAPI CONFIGRET WINAPI CM_Get_Device_Interface_PropertyW(LPCWSTR pszDeviceInterface,
                                                         CONST DEVPROPKEY *PropertyKey,
                                                         DEVPROPTYPE *PropertyType,
                                                         PBYTE PropertyBuffer,
                                                         PULONG PropertyBufferSize,
                                                         ULONG ulFlags)
{
    return g_mockWindows->CM_Get_Device_Interface_PropertyW(pszDeviceInterface,
                                                            PropertyKey,
                                                            PropertyType,
                                                            PropertyBuffer,
                                                            PropertyBufferSize,
                                                            ulFlags);
}

// CM_Locate_DevNodeW
CMAPI CONFIGRET WINAPI CM_Locate_DevNodeW(PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags)
{
    return g_mockWindows->CM_Locate_DevNodeW(pdnDevInst, pDeviceID, ulFlags);
}

// CM_Get_DevNode_PropertyW
CMAPI CONFIGRET WINAPI CM_Get_DevNode_PropertyW(DEVINST dnDevInst,
                                                CONST DEVPROPKEY *PropertyKey,
                                                DEVPROPTYPE *PropertyType,
                                                PBYTE PropertyBuffer,
                                                PULONG PropertyBufferSize,
                                                ULONG ulFlags)
{
    return g_mockWindows
        ->CM_Get_DevNode_PropertyW(dnDevInst, PropertyKey, PropertyType, PropertyBuffer, PropertyBufferSize, ulFlags);
}

#ifdef __cplusplus
}
#endif
