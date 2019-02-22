// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "color_mock_mediafoundation.h"
#include <Mferror.h>
#include <ksproxy.h>
#include <ksmedia.h>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace testing;

//
// Mock Media Source
//
class __declspec(uuid("E54F5569-8E95-4644-A4BF-CE6317A60F67")) IMockSource : public IUnknown
{
public:
    virtual LONG GetExposureTime() = 0;
};

class MockSourceActivate
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMFActivate, IMFMediaSource, IKsControl, IMockSource>
{
public:
    MockSourceActivate() {}

    virtual ~MockSourceActivate() {}

    HRESULT RuntimeClassInitialize()
    {
        HRESULT hr = S_OK;

        if (SUCCEEDED(hr = MFCreateAttributes(&m_spAttributes, 1)))
        {
            hr = m_spAttributes->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                           L"vid_045e&pid_097d");
        }

        return hr;
    }

    // IMFAttributes
    STDMETHOD(GetItem)(REFGUID guidKey, PROPVARIANT *pValue)
    {
        return m_spAttributes->GetItem(guidKey, pValue);
    }

    STDMETHOD(GetItemType)(REFGUID guidKey, MF_ATTRIBUTE_TYPE *pType)
    {
        return m_spAttributes->GetItemType(guidKey, pType);
    }

    STDMETHOD(CompareItem)(REFGUID guidKey, REFPROPVARIANT Value, BOOL *pbResult)
    {
        return m_spAttributes->CompareItem(guidKey, Value, pbResult);
    }

    STDMETHOD(Compare)(IMFAttributes *pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL *pbResult)
    {
        return m_spAttributes->Compare(pTheirs, MatchType, pbResult);
    }

    STDMETHOD(GetUINT32)(REFGUID guidKey, UINT32 *punValue)
    {
        return m_spAttributes->GetUINT32(guidKey, punValue);
    }

    STDMETHOD(GetUINT64)(REFGUID guidKey, UINT64 *punValue)
    {
        return m_spAttributes->GetUINT64(guidKey, punValue);
    }

    STDMETHOD(GetDouble)(REFGUID guidKey, double *pfValue)
    {
        return m_spAttributes->GetDouble(guidKey, pfValue);
    }

    STDMETHOD(GetGUID)(REFGUID guidKey, GUID *pguidValue)
    {
        return m_spAttributes->GetGUID(guidKey, pguidValue);
    }

    STDMETHOD(GetStringLength)(REFGUID guidKey, UINT32 *pcchLength)
    {
        return m_spAttributes->GetStringLength(guidKey, pcchLength);
    }

    STDMETHOD(GetString)(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32 *pcchLength)
    {
        return m_spAttributes->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
    }

    STDMETHOD(GetAllocatedString)(REFGUID guidKey, LPWSTR *ppwszValue, UINT32 *pcchLength)
    {
        return m_spAttributes->GetAllocatedString(guidKey, ppwszValue, pcchLength);
    }

    STDMETHOD(GetBlobSize)(REFGUID guidKey, UINT32 *pcbBlobSize)
    {
        return m_spAttributes->GetBlobSize(guidKey, pcbBlobSize);
    }

    STDMETHOD(GetBlob)(REFGUID guidKey, UINT8 *pBuf, UINT32 cbBufSize, UINT32 *pcbBlobSize)
    {
        return m_spAttributes->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
    }

    STDMETHOD(GetAllocatedBlob)(REFGUID guidKey, UINT8 **ppBuf, UINT32 *pcbSize)
    {
        return m_spAttributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
    }

    STDMETHOD(GetUnknown)(REFGUID guidKey, REFIID riid, LPVOID *ppv)
    {
        return m_spAttributes->GetUnknown(guidKey, riid, ppv);
    }

    STDMETHOD(SetItem)(REFGUID guidKey, REFPROPVARIANT Value)
    {
        return m_spAttributes->SetItem(guidKey, Value);
    }

    STDMETHOD(DeleteItem)(REFGUID guidKey)
    {
        return m_spAttributes->DeleteItem(guidKey);
    }

    STDMETHOD(DeleteAllItems)(void)
    {
        return m_spAttributes->DeleteAllItems();
    }

    STDMETHOD(SetUINT32)(REFGUID guidKey, UINT32 unValue)
    {
        return m_spAttributes->SetUINT32(guidKey, unValue);
    }

    STDMETHOD(SetUINT64)(REFGUID guidKey, UINT64 unValue)
    {
        return m_spAttributes->SetUINT64(guidKey, unValue);
    }

    STDMETHOD(SetDouble)(REFGUID guidKey, double fValue)
    {
        return m_spAttributes->SetDouble(guidKey, fValue);
    }

    STDMETHOD(SetGUID)(REFGUID guidKey, REFGUID guidValue)
    {
        return m_spAttributes->SetGUID(guidKey, guidValue);
    }

    STDMETHOD(SetString)(REFGUID guidKey, LPCWSTR wszValue)
    {
        return m_spAttributes->SetString(guidKey, wszValue);
    }

    STDMETHOD(SetBlob)(REFGUID guidKey, const UINT8 *pBuf, UINT32 cbBufSize)
    {
        return m_spAttributes->SetBlob(guidKey, pBuf, cbBufSize);
    }

    STDMETHOD(SetUnknown)(REFGUID guidKey, IUnknown *pUnknown)
    {
        return m_spAttributes->SetUnknown(guidKey, pUnknown);
    }

    STDMETHOD(LockStore)(void)
    {
        return m_spAttributes->LockStore();
    }

    STDMETHOD(UnlockStore)(void)
    {
        return m_spAttributes->UnlockStore();
    }

    STDMETHOD(GetCount)(UINT32 *pcItems)
    {
        return m_spAttributes->GetCount(pcItems);
    }

    STDMETHOD(GetItemByIndex)(UINT32 unIndex, GUID *pguidKey, PROPVARIANT *pValue)
    {
        return m_spAttributes->GetItemByIndex(unIndex, pguidKey, pValue);
    }

    STDMETHOD(CopyAllItems)(IMFAttributes *pDest)
    {
        return m_spAttributes->CopyAllItems(pDest);
    }

    // IMFActivate
    STDMETHOD(ActivateObject)(REFIID riid, void **ppv)
    {
        return QueryInterface(riid, ppv);
    }

    STDMETHOD(ShutdownObject)(void)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(DetachObject)(void)
    {
        return E_NOTIMPL;
    }

    // IMFMediaEventGenerator
    STDMETHOD(GetEvent)
    (DWORD,            // dwFlags
     IMFMediaEvent **) // ppEvent
    {
        return E_NOTIMPL;
    }

    STDMETHOD(BeginGetEvent)
    (IMFAsyncCallback *, // pCallback
     IUnknown *)         // punkState
    {
        return E_NOTIMPL;
    }

    STDMETHOD(EndGetEvent)
    (IMFAsyncResult *, // pResult
     IMFMediaEvent **) // ppEvent
    {
        return E_NOTIMPL;
    }

    STDMETHOD(QueueEvent)
    (MediaEventType,      // met
     REFGUID,             // guidExtendedType
     HRESULT,             // hrStatus
     const PROPVARIANT *) // pvValue
    {
        return E_NOTIMPL;
    }

    // IMFMediaSource
    STDMETHOD(GetCharacteristics)(DWORD *) // pdwCharacteristics
    {
        return E_NOTIMPL;
    }

    STDMETHOD(CreatePresentationDescriptor)(IMFPresentationDescriptor **) // ppPresentationDescriptor
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Start)
    (IMFPresentationDescriptor *, // pPresentationDescriptor
     const GUID *,                // pguidTimeFormat
     const PROPVARIANT *)         // pvarStartPosition
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Stop)(void)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Pause)(void)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Shutdown)(void)
    {
        return E_NOTIMPL;
    }

    // IKsControl
    STDMETHOD(KsProperty)
    (PKSPROPERTY Property, ULONG PropertyLength, LPVOID PropertyData, ULONG DataLength, ULONG *BytesReturned)
    {
        HRESULT hr = S_OK;
        if (Property == nullptr || PropertyData == nullptr)
        {
            return E_INVALIDARG;
        }

        if (Property->Set == PROPSETID_VIDCAP_CAMERACONTROL)
        {
            if (PropertyLength < sizeof(KSPROPERTY_CAMERACONTROL_S) || DataLength < sizeof(KSPROPERTY_CAMERACONTROL_S))
            {
                return HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            }
            PKSPROPERTY_CAMERACONTROL_S pCameraControl = (PKSPROPERTY_CAMERACONTROL_S)PropertyData;

            if (Property->Flags == KSPROPERTY_TYPE_SET)
            {
                exposure_time = pCameraControl->Value;
                exposure_flags = pCameraControl->Flags;
            }
            else if (Property->Flags == KSPROPERTY_TYPE_GET)
            {
                pCameraControl->Value = exposure_time;
                pCameraControl->Flags = exposure_flags;
            }
            else
            {
                return E_INVALIDARG;
            }

            if (BytesReturned)
            {
                *BytesReturned = sizeof(PKSPROPERTY_CAMERACONTROL_S);
            }
        }
        else
        {
            return E_INVALIDARG;
        }

        return hr;
    }

    STDMETHOD(KsMethod)
    (PKSMETHOD, // Method
     ULONG,     // MethodLength
     LPVOID,    // MethodData
     ULONG,     // DataLength
     ULONG *)   // BytesReturned
    {
        return E_NOTIMPL;
    }

    STDMETHOD(KsEvent)
    (PKSEVENT, // Event
     ULONG,    // EventLength
     LPVOID,   // EventData
     ULONG,    // DataLength
     ULONG *)  // BytesReturned
    {
        return E_NOTIMPL;
    }

    //
    // IMockSource
    //
    virtual LONG GetExposureTime() override
    {
        return exposure_time;
    }

private:
    ComPtr<IMFAttributes> m_spAttributes;
    LONG exposure_time = 0;
    ULONG exposure_flags = 0;
};

//
// Mock Sample Async Callback
//
class MockAsyncSampleCallback : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMFAsyncCallback>
{
public:
    MockAsyncSampleCallback() {}
    virtual ~MockAsyncSampleCallback() {}

    HRESULT RuntimeClassInitialize(IMFSourceReaderCallback *pCallback,
                                   IMFMediaSource *pMediaSource,
                                   IMFMediaType *pMediaType)
    {
        m_spSourceReaderCallback = pCallback;
        m_spMediaSource = pMediaSource;
        m_spMediaType = pMediaType;

        return S_OK;
    }

    STDMETHOD(GetParameters)
    (DWORD *, // pdwFlags
     DWORD *) // pdwQueue
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Invoke)(IMFAsyncResult *)
    {
        HRESULT hr = S_OK;
        ComPtr<IMFSample> spSample;
        ComPtr<IMFMediaBuffer> spBuffer;
        UINT32 width = 0;
        UINT32 height = 0;
        GUID guidSubType = GUID_NULL;
        DWORD length = 0;

        if (FAILED(hr = MFGetAttributeSize(m_spMediaType.Get(), MF_MT_FRAME_SIZE, &width, &height)))
        {
            return hr;
        }

        if (FAILED(hr = m_spMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubType)))
        {
            return hr;
        }

        if (guidSubType == MFVideoFormat_NV12)
        {
            length = width * height * 3 / 2;
        }
        else if (guidSubType == MFVideoFormat_YUY2)
        {
            length = width * height * 2;
        }
        else if (guidSubType == MFVideoFormat_MJPG)
        {
            length = width * height;
        }
        else if (guidSubType == MFVideoFormat_RGB24)
        {
            length = width * height * 3;
        }
        else
        {
            return MF_E_INVALIDMEDIATYPE;
        }

        // Create Sample
        if (FAILED(hr = MFCreateSample(&spSample)))
        {
            return hr;
        }

        // Create Buffer
        if (FAILED(hr = MFCreateMemoryBuffer(length, &spBuffer)))
        {
            return hr;
        }

        // Set Buffer Length
        if (FAILED(hr = spBuffer->SetCurrentLength(length)))
        {
            return hr;
        }

        // Add buffer to the sample
        if (FAILED(hr = spSample->AddBuffer(spBuffer.Get())))
        {
            return hr;
        }

        // Add Metadata to the sample
        ComPtr<IMFAttributes> spMetadata;
        if (FAILED(hr = MFCreateAttributes(&spMetadata, 1)))
        {
            return hr;
        }

        ComPtr<IMockSource> spMockSource;
        if (FAILED(hr = m_spMediaSource.As(&spMockSource)))
        {
            return hr;
        }

        UINT64 exposure_time = (UINT64)(exp2f((float)spMockSource->GetExposureTime()) * 10000000.0f);

        if (FAILED(hr = spMetadata->SetUINT64(MF_CAPTURE_METADATA_EXPOSURE_TIME, exposure_time)))
        {
            return hr;
        }

        if (FAILED(hr = spSample->SetUnknown(MFSampleExtension_CaptureMetadata, spMetadata.Get())))
        {
            return hr;
        }

        hr = m_spSourceReaderCallback->OnReadSample(S_OK,
                                                    (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                    0,
                                                    0,
                                                    spSample.Get());

        return hr;
    }

private:
    ComPtr<IMFSourceReaderCallback> m_spSourceReaderCallback;
    ComPtr<IMFMediaSource> m_spMediaSource;
    ComPtr<IMFMediaType> m_spMediaType;
};

//
// Mock Flush Async Callback
//
class MockAsyncFlushCallback : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMFAsyncCallback>
{
public:
    MockAsyncFlushCallback() {}
    virtual ~MockAsyncFlushCallback() {}

    HRESULT RuntimeClassInitialize(IMFSourceReaderCallback *pCallback)
    {
        m_spSourceReaderCallback = pCallback;

        return S_OK;
    }

    STDMETHOD(GetParameters)
    (DWORD *, // pdwFlags
     DWORD *) // pdwQueue
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Invoke)(IMFAsyncResult *) // pAsyncResult
    {
        return m_spSourceReaderCallback->OnFlush((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM);
    }

private:
    ComPtr<IMFSourceReaderCallback> m_spSourceReaderCallback;
};

//
// Mock Media Foundation Source Reader
//
class MockSourceReader
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IMFSourceReaderEx, IMFSourceReader>>
{
public:
    MockSourceReader() {}
    virtual ~MockSourceReader() {}

    HRESULT RuntimeClassInitialize(IMFMediaSource *pMediaSource, IMFAttributes *pAttributes)
    {
        HRESULT hr = S_OK;

        if (pMediaSource == nullptr || pAttributes == nullptr)
        {
            return E_INVALIDARG;
        }

        m_spMediaSource = pMediaSource;

        hr = pAttributes->GetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, IID_PPV_ARGS(&m_spSourceReaderCallback));
        if (SUCCEEDED(hr))
        {
            if (m_spSourceReaderCallback == nullptr)
            {
                return E_INVALIDARG;
            }
        }

        return hr;
    }

    // IMFSourceReader
    STDMETHOD(GetStreamSelection)(DWORD dwStreamIndex, BOOL *pfSelected)
    {
        if (dwStreamIndex != (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM)
        {
            return MF_E_INVALIDSTREAMNUMBER;
        }

        if (pfSelected)
        {
            *pfSelected = m_fStreamSelected;
        }
        else
        {
            return E_POINTER;
        }

        return S_OK;
    }

    STDMETHOD(SetStreamSelection)(DWORD dwStreamIndex, BOOL fSelected)
    {
        if (dwStreamIndex != (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM &&
            dwStreamIndex != (DWORD)MF_SOURCE_READER_ALL_STREAMS)
        {
            return MF_E_INVALIDSTREAMNUMBER;
        }

        m_fStreamSelected = fSelected;

        return S_OK;
    }

    STDMETHOD(GetNativeMediaType)(DWORD dwStreamIndex, DWORD dwMediaTypeIndex, IMFMediaType **ppMediaType)
    {
        HRESULT hr = S_OK;
        ComPtr<IMFMediaType> spMediaType;

        if (dwStreamIndex != (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM)
        {
            return MF_E_INVALIDSTREAMNUMBER;
        }

        if (dwMediaTypeIndex >= _countof(m_arrSupportedType))
        {
            return MF_E_NO_MORE_TYPES;
        }

        if (ppMediaType)
        {
            // Create Media Type
            if (FAILED(hr = MFCreateMediaType(&spMediaType)))
            {
                return hr;
            }

            if (FAILED(hr = MFSetAttributeSize(spMediaType.Get(),
                                               MF_MT_FRAME_SIZE,
                                               m_arrSupportedType[dwMediaTypeIndex].width,
                                               m_arrSupportedType[dwMediaTypeIndex].height)))
            {
                return hr;
            }

            if (FAILED(hr = MFSetAttributeRatio(spMediaType.Get(),
                                                MF_MT_FRAME_RATE,
                                                m_arrSupportedType[dwMediaTypeIndex].framerateNumerator,
                                                m_arrSupportedType[dwMediaTypeIndex].framerateDenominator)))
            {
                return hr;
            }

            if (FAILED(hr = spMediaType->SetGUID(MF_MT_SUBTYPE, m_arrSupportedType[dwMediaTypeIndex].subType)))
            {
                return hr;
            }

            if (FAILED(hr = spMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)))
            {
                return hr;
            }

            *ppMediaType = spMediaType.Detach();
        }
        else
        {
            hr = E_POINTER;
        }

        return hr;
    }

    STDMETHOD(GetCurrentMediaType)
    (DWORD,           // dwStreamIndex
     IMFMediaType **) // ppMediaType
    {
        return E_NOTIMPL;
    }

    STDMETHOD(SetCurrentMediaType)
    (DWORD dwStreamIndex, DWORD *pdwReserved, IMFMediaType *pMediaType)
    {
        HRESULT hr = S_OK;
        UNREFERENCED_PARAMETER(pdwReserved);

        if (dwStreamIndex != (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM)
        {
            return MF_E_INVALIDSTREAMNUMBER;
        }

        if (pMediaType == nullptr)
        {
            return E_INVALIDARG;
        }

        if (m_spNativeMediaType)
        {
            BOOL result = FALSE;

            hr = m_spNativeMediaType->Compare(pMediaType,
                                              MF_ATTRIBUTES_MATCH_TYPE::MF_ATTRIBUTES_MATCH_ALL_ITEMS,
                                              &result);
            if (FAILED(hr))
            {
                return hr;
            }

            if (result)
            {
                m_spCurrentMediaType = pMediaType;
            }
            else
            {
                ComPtr<IMFMediaType> spNativeTypeTemp;
                GUID guidCurrentSubType = GUID_NULL;

                hr = MFCreateMediaType(&spNativeTypeTemp);
                if (FAILED(hr))
                {
                    return hr;
                }

                hr = m_spNativeMediaType->CopyAllItems(spNativeTypeTemp.Get());
                if (FAILED(hr))
                {
                    return hr;
                }

                hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &guidCurrentSubType);
                if (FAILED(hr))
                {
                    return hr;
                }

                hr = spNativeTypeTemp->SetGUID(MF_MT_SUBTYPE, guidCurrentSubType);
                if (FAILED(hr))
                {
                    return hr;
                }

                hr = spNativeTypeTemp->Compare(pMediaType,
                                               MF_ATTRIBUTES_MATCH_TYPE::MF_ATTRIBUTES_MATCH_ALL_ITEMS,
                                               &result);
                if (FAILED(hr))
                {
                    return hr;
                }

                if (result)
                {
                    m_spCurrentMediaType = pMediaType;
                }
                else
                {
                    return MF_E_INVALIDTYPE;
                }
            }
        }
        else
        {
            m_spCurrentMediaType = pMediaType;
        }

        return hr;
    }

    STDMETHOD(SetCurrentPosition)
    (REFGUID,        // guidTimeFormat
     REFPROPVARIANT) // varPosition
    {
        return E_NOTIMPL;
    }

    STDMETHOD(ReadSample)
    (DWORD dwStreamIndex,
     DWORD dwControlFlags,
     DWORD *pdwActualStreamIndex,
     DWORD *pdwStreamFlags,
     LONGLONG *pllTimestamp,
     IMFSample **ppSample)
    {
        HRESULT hr = S_OK;
        MFWORKITEM_KEY cancelKey;
        ComPtr<IMFAsyncCallback> spCallback;
        UNREFERENCED_PARAMETER(dwControlFlags);

        if (dwStreamIndex != (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM)
        {
            return MF_E_INVALIDSTREAMNUMBER;
        }

        if (pdwActualStreamIndex || pdwStreamFlags || pllTimestamp || ppSample)
        {
            // Source reader must be run as Async mode with all nullptr of these parameters
            return E_INVALIDARG;
        }

        if (m_fStreamSelected == false)
        {
            return MF_E_INVALIDREQUEST;
        }

        // Schedule a sample callback
        if (FAILED(hr = MakeAndInitialize<MockAsyncSampleCallback>(&spCallback,
                                                                   m_spSourceReaderCallback.Get(),
                                                                   m_spMediaSource.Get(),
                                                                   m_spCurrentMediaType.Get())))
        {
            return hr;
        }

        if (FAILED(hr = MFScheduleWorkItem(spCallback.Get(), nullptr, -33, &cancelKey)))
        {
            return hr;
        }

        return hr;
    }

    STDMETHOD(Flush)(DWORD dwStreamIndex)
    {
        HRESULT hr = S_OK;
        ComPtr<IMFAsyncCallback> spCallback;
        if (dwStreamIndex != (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM)
        {
            return MF_E_INVALIDSTREAMNUMBER;
        }

        // Schedule a flush callback
        if (FAILED(hr = MakeAndInitialize<MockAsyncFlushCallback>(&spCallback, m_spSourceReaderCallback.Get())))
        {
            return hr;
        }

        if (FAILED(hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, spCallback.Get(), nullptr)))
        {
            return hr;
        }

        return hr;
    }

    STDMETHOD(GetServiceForStream)(DWORD dwStreamIndex, REFGUID guidService, REFIID riid, LPVOID *ppvObject)
    {
        if (dwStreamIndex != (DWORD)MF_SOURCE_READER_MEDIASOURCE)
        {
            return MF_E_INVALIDSTREAMNUMBER;
        }

        if (guidService != GUID_NULL)
        {
            return E_INVALIDARG;
        }

        return m_spMediaSource.CopyTo(riid, ppvObject);
    }

    STDMETHOD(GetPresentationAttribute)
    (DWORD,         // dwStreamIndex
     REFGUID,       // guidAttribute
     PROPVARIANT *) // pvarAttribute
    {
        return E_NOTIMPL;
    }

    // IMFSourceReaderEx
    STDMETHOD(SetNativeMediaType)(DWORD dwStreamIndex, IMFMediaType *pMediaType, DWORD *pdwStreamFlags)
    {
        HRESULT hr = S_OK;
        UINT32 width = 0;
        UINT32 height = 0;
        GUID guidSubType = GUID_NULL;
        UINT32 fpsNumerator = 0;
        UINT32 fpsDenominator = 0;

        if (dwStreamIndex != (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM)
        {
            return MF_E_INVALIDSTREAMNUMBER;
        }

        if (pMediaType == nullptr)
        {
            return E_INVALIDARG;
        }

        if (pdwStreamFlags == nullptr)
        {
            return E_POINTER;
        }

        if (FAILED(hr = MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height)))
        {
            return hr;
        }

        if (FAILED(hr = MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &fpsNumerator, &fpsDenominator)))
        {
            return hr;
        }

        if (FAILED(hr = pMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubType)))
        {
            return hr;
        }

        for (int i = 0; i < _countof(m_arrSupportedType); i++)
        {
            if (m_arrSupportedType[i].width == width && m_arrSupportedType[i].height == height &&
                m_arrSupportedType[i].subType == guidSubType &&
                m_arrSupportedType[i].framerateNumerator == fpsNumerator &&
                m_arrSupportedType[i].framerateDenominator == fpsDenominator)
            {
                m_spNativeMediaType = pMediaType;
                return S_OK;
            }
        }

        return MF_E_INVALIDMEDIATYPE;
    }

    STDMETHOD(AddTransformForStream)
    (DWORD,      // dwStreamIndex
     IUnknown *) // pTransformOrActivate
    {
        return E_NOTIMPL;
    }

    STDMETHOD(RemoveAllTransformsForStream)(DWORD) // dwStreamIndex
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetTransformForStream)
    (DWORD,           // dwStreamIndex
     DWORD,           // dwTransformIndex
     GUID *,          // pGuidCategory
     IMFTransform **) // ppTransform
    {
        return E_NOTIMPL;
    }

private:
    typedef struct _MediaType
    {
        UINT32 width;
        UINT32 height;
        GUID subType;
        UINT32 framerateNumerator;
        UINT32 framerateDenominator;
    } MediaType;

    BOOL m_fStreamSelected = true; // Default stream is selected by default

    ComPtr<IMFMediaSource> m_spMediaSource;
    ComPtr<IMFSourceReaderCallback> m_spSourceReaderCallback;

    ComPtr<IMFMediaType> m_spNativeMediaType;
    ComPtr<IMFMediaType> m_spCurrentMediaType;
    MediaType m_arrSupportedType[23] = // Color FW 1.2.14 supported media types
        { { 1280, 720, MFVideoFormat_NV12, 30, 1 },  { 1280, 720, MFVideoFormat_YUY2, 30, 1 },
          { 3840, 2160, MFVideoFormat_MJPG, 30, 1 }, { 2560, 1440, MFVideoFormat_MJPG, 30, 1 },
          { 1920, 1080, MFVideoFormat_MJPG, 30, 1 }, { 1280, 720, MFVideoFormat_MJPG, 30, 1 },
          { 2048, 1536, MFVideoFormat_MJPG, 30, 1 },

          { 1280, 720, MFVideoFormat_NV12, 15, 1 },  { 1280, 720, MFVideoFormat_YUY2, 15, 1 },
          { 3840, 2160, MFVideoFormat_MJPG, 15, 1 }, { 2560, 1440, MFVideoFormat_MJPG, 15, 1 },
          { 1920, 1080, MFVideoFormat_MJPG, 15, 1 }, { 1280, 720, MFVideoFormat_MJPG, 15, 1 },
          { 4096, 3072, MFVideoFormat_MJPG, 15, 1 }, { 2048, 1536, MFVideoFormat_MJPG, 15, 1 },

          { 1280, 720, MFVideoFormat_NV12, 5, 1 },   { 1280, 720, MFVideoFormat_YUY2, 5, 1 },
          { 3840, 2160, MFVideoFormat_MJPG, 5, 1 },  { 2560, 1440, MFVideoFormat_MJPG, 5, 1 },
          { 1920, 1080, MFVideoFormat_MJPG, 5, 1 },  { 1280, 720, MFVideoFormat_MJPG, 5, 1 },
          { 4096, 3072, MFVideoFormat_MJPG, 5, 1 },  { 2048, 1536, MFVideoFormat_MJPG, 5, 1 } };
};

#ifdef __cplusplus
extern "C" {
#endif

MockMediaFoundation *g_mockMediaFoundation = nullptr;

//
// Mock MFEnumDeviceSources
//
STDAPI MFEnumDeviceSources(IMFAttributes *pAttributes, IMFActivate ***pppSourceActivate, UINT32 *pcSourceActivate)
{
    return g_mockMediaFoundation->MFEnumDeviceSources(pAttributes, pppSourceActivate, pcSourceActivate);
}

void EXPECT_MFEnumDeviceSources(MockMediaFoundation &mockMediaFoundation)
{
    EXPECT_CALL(mockMediaFoundation,
                MFEnumDeviceSources(_, // IMFAttributes* pAttributes,
                                    _, // IMFActivate*** pppSourceActivate,
                                    _  // UINT32* pcSourceActivate
                                    ))
        .WillRepeatedly(
            Invoke([](IMFAttributes *pAttributes, IMFActivate ***pppSourceActivate, UINT32 *pcSourceActivate) {
                HRESULT hr = S_OK;

                if (pppSourceActivate == nullptr || pcSourceActivate == nullptr)
                {
                    hr = E_POINTER;
                    goto Exit;
                }
                else if (pAttributes == nullptr)
                {
                    hr = E_INVALIDARG;
                }
                else
                {
                    GUID guidCategory = GUID_NULL;
                    GUID guidSourceType = GUID_NULL;

                    (void)pAttributes->GetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &guidSourceType);
                    (void)pAttributes->GetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_CATEGORY, &guidCategory);

                    if (guidSourceType != MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID ||
                        guidCategory != KSCATEGORY_VIDEO_CAMERA)
                    {
                        return E_FAIL;
                    }

                    *pcSourceActivate = 1;
                    *pppSourceActivate = (IMFActivate **)CoTaskMemAlloc((*pcSourceActivate) * sizeof(IMFActivate *));
                    if (*pppSourceActivate == nullptr)
                    {
                        return E_OUTOFMEMORY;
                    }

                    hr = MakeAndInitialize<MockSourceActivate>(*pppSourceActivate);
                }

            Exit:
                if (FAILED(hr) && *pppSourceActivate)
                {
                    CoTaskMemFree(*pppSourceActivate);
                    *pppSourceActivate = nullptr;
                }
                return hr;
            }));
}

//
// Mock MFCreateSourceReaderFromMediaSource
//
STDAPI MFCreateSourceReaderFromMediaSource(IMFMediaSource *pMediaSource,
                                           IMFAttributes *pAttributes,
                                           IMFSourceReader **ppSourceReader)
{
    return g_mockMediaFoundation->MFCreateSourceReaderFromMediaSource(pMediaSource, pAttributes, ppSourceReader);
}

void EXPECT_MFCreateSourceReaderFromMediaSource(MockMediaFoundation &mockMediaFoundation)
{
    EXPECT_CALL(mockMediaFoundation,
                MFCreateSourceReaderFromMediaSource(_, // IMFMediaSource *pMediaSource,
                                                    _, // IMFAttributes *pAttributes,
                                                    _  // IMFSourceReader **ppSourceReader
                                                    ))
        .WillRepeatedly(
            Invoke([](IMFMediaSource *pMediaSource, IMFAttributes *pAttributes, IMFSourceReader **ppSourceReader) {
                // pMediaSource must be valid pointer
                if (pMediaSource == nullptr)
                {
                    return E_INVALIDARG;
                }

                // pAttributes must be valid pointer for setting async callback
                if (pAttributes == nullptr)
                {
                    return E_INVALIDARG;
                }

                // returning ppSourceReader must be valid pointer
                if (ppSourceReader == nullptr)
                {
                    return E_POINTER;
                }

                return MakeAndInitialize<MockSourceReader>(ppSourceReader, pMediaSource, pAttributes);
            }));
}

#ifdef __cplusplus
}
#endif
