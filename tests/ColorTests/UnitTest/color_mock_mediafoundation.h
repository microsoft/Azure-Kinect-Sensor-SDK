// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COLOR_MOCK_MEDIAFOUNDATION_H
#define COLOR_MOCK_MEDIAFOUNDATION_H
#include <utcommon.h>

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <ks.h>

class MockMediaFoundation
{
public:
    MOCK_METHOD3(MFEnumDeviceSources,
                 HRESULT(IMFAttributes *pAttributes, IMFActivate ***pppSourceActivate, UINT32 *pcSourceActivate));
    MOCK_METHOD3(MFCreateSourceReaderFromMediaSource,
                 HRESULT(IMFMediaSource *pMediaSource, IMFAttributes *pAttributes, IMFSourceReader **ppSourceReader));
};

#ifdef __cplusplus
extern "C" {
#endif
extern MockMediaFoundation *g_mockMediaFoundation;

void EXPECT_MFEnumDeviceSources(MockMediaFoundation &mockMediaFoundation);

void EXPECT_MFCreateSourceReaderFromMediaSource(MockMediaFoundation &mockMediaFoundation);

#ifdef __cplusplus
}
#endif

#endif // COLOR_MOCK_MEDIAFOUNDATION_H
