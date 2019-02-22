// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COLOR_MOCK_WINDOWS_H
#define COLOR_MOCK_WINDOWS_H

#include <utcommon.h>

#include <Windows.h>
#include <devpkey.h>
#include <cfgmgr32.h>

#include <wrl.h>

class MockWindowsSystem
{
public:
    // Mock CM APIs
    MOCK_METHOD6(CM_Get_Device_Interface_PropertyW,
                 CONFIGRET(LPCWSTR pszDeviceInterface,
                           CONST DEVPROPKEY *PropertyKey,
                           DEVPROPTYPE *PropertyType,
                           PBYTE PropertyBuffer,
                           PULONG PropertyBufferSize,
                           ULONG ulFlags));

    MOCK_METHOD3(CM_Locate_DevNodeW, CONFIGRET(PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags));

    MOCK_METHOD6(CM_Get_DevNode_PropertyW,
                 CONFIGRET(DEVINST dnDevInst,
                           CONST DEVPROPKEY *PropertyKey,
                           DEVPROPTYPE *PropertyType,
                           PBYTE PropertyBuffer,
                           PULONG PropertyBufferSize,
                           ULONG ulFlags));
};

#ifdef __cplusplus
extern "C" {
#endif

CMAPI void WINAPI SetWindowsSystemMock(MockWindowsSystem *pMockWindowsSystem);

//
// CM Mock
//
void EXPECT_CM_Get_Device_Interface_PropertyW(MockWindowsSystem &mockWindows)
{
    EXPECT_CALL(mockWindows,
                CM_Get_Device_Interface_PropertyW(testing::_, // LPCWSTR pszDeviceInterface
                                                  testing::_, // CONST DEVPROPKEY *PropertyKey
                                                  testing::_, // DEVPROPTYPE *PropertyType
                                                  testing::_, // PBYTE PropertyBuffer
                                                  testing::_, // PULONG PropertyBufferSize
                                                  testing::_  // ULONG ulFlags
                                                  ))
        .WillRepeatedly(testing::Invoke([](LPCWSTR pszDeviceInterface,
                                           CONST DEVPROPKEY *PropertyKey,
                                           DEVPROPTYPE *PropertyType,
                                           PBYTE PropertyBuffer,
                                           PULONG PropertyBufferSize,
                                           ULONG ulFlags) {
            UNREFERENCED_PARAMETER(pszDeviceInterface);
            UNREFERENCED_PARAMETER(PropertyKey);
            UNREFERENCED_PARAMETER(PropertyType);
            UNREFERENCED_PARAMETER(PropertyBuffer);
            UNREFERENCED_PARAMETER(PropertyBufferSize);
            UNREFERENCED_PARAMETER(ulFlags);
            return CR_SUCCESS;
        }));
}

void EXPECT_CM_Locate_DevNodeW(MockWindowsSystem &mockWindows)
{
    EXPECT_CALL(mockWindows,
                CM_Locate_DevNodeW(testing::_, // PDEVINST pdnDevInst
                                   testing::_, // DEVINSTID_W pDeviceID
                                   testing::_  // ULONG ulFlags
                                   ))
        .WillRepeatedly(testing::Invoke([](PDEVINST pdnDevInst, DEVINSTID_W pDeviceID, ULONG ulFlags) {
            UNREFERENCED_PARAMETER(pdnDevInst);
            UNREFERENCED_PARAMETER(pDeviceID);
            UNREFERENCED_PARAMETER(ulFlags);
            return CR_SUCCESS;
        }));
}

void EXPECT_CM_Get_DevNode_PropertyW(MockWindowsSystem &mockWindows, const GUID fakeContainerId)
{
    EXPECT_CALL(mockWindows,
                CM_Get_DevNode_PropertyW(testing::_, // DEVINST dnDevInst
                                         testing::_, // CONST DEVPROPKEY *PropertyKey
                                         testing::_, // DEVPROPTYPE *PropertyType
                                         testing::_, // PBYTE PropertyBuffer
                                         testing::_, // PULONG PropertyBufferSize
                                         testing::_  // ULONG ulFlags
                                         ))
        .WillRepeatedly(testing::Invoke([=](DEVINST dnDevInst,
                                            CONST DEVPROPKEY *PropertyKey,
                                            DEVPROPTYPE *PropertyType,
                                            PBYTE PropertyBuffer,
                                            PULONG PropertyBufferSize,
                                            ULONG ulFlags) {
            UNREFERENCED_PARAMETER(dnDevInst);
            UNREFERENCED_PARAMETER(PropertyKey);
            UNREFERENCED_PARAMETER(PropertyType);
            UNREFERENCED_PARAMETER(PropertyBuffer);
            UNREFERENCED_PARAMETER(PropertyBufferSize);
            UNREFERENCED_PARAMETER(ulFlags);

            if (*PropertyKey != DEVPKEY_Device_ContainerId)
            {
                return CR_NO_SUCH_VALUE;
            }

            if (PropertyBuffer == nullptr)
            {
                if (PropertyBufferSize)
                {
                    *PropertyBufferSize = sizeof(GUID);
                }

                return CR_BUFFER_SMALL;
            }
            else if (*PropertyBufferSize >= sizeof(GUID))
            {
                memcpy(PropertyBuffer, &fakeContainerId, sizeof(GUID));
            }
            else
            {
                return CR_BUFFER_SMALL;
            }

            return CR_SUCCESS;
        }));
}

#ifdef __cplusplus
}
#endif

#endif // COLOR_MOCK_WINDOWS_H
