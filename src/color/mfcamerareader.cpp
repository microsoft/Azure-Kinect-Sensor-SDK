// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "mfcamerareader.h"
#include <k4ainternal/common.h>
#include <k4ainternal/capture.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <devpkey.h>
#include <assert.h>

#define COLOR_CAMERA_IDENTIFIER L"vid_045e&pid_097d"
#define MetadataId_FrameAlignInfo 0x80000001

#pragma pack(push, 1)
typedef struct tag_CUSTOM_METADATA_FrameAlignInfo
{
    KSCAMERA_METADATA_ITEMHEADER Header;
    ULONG Flags;
    ULONG Reserved;
    ULONGLONG FramePTS; // 8 bytes
    ULONG PTSReference;
    ULONGLONG USBSoFSeqNum; // 8 bytes
    ULONGLONG USBSoFPTS;    // 8 bytes
    ULONG Synced;
} CUSTOM_METADATA_FrameAlignInfo, *PKSCAMERA_CUSTOM_METADATA_FrameAlignInfo;
#pragma pack(pop)

using namespace Microsoft::WRL;

CFrameContext::CFrameContext(IMFSample *pSample)
{
    if (pSample)
    {
        HRESULT hr = S_OK;
        m_spSample = pSample;

        if (SUCCEEDED(hr = m_spSample->ConvertToContiguousBuffer(&m_spMediaBuffer)))
        {
            if (SUCCEEDED(hr = m_spMediaBuffer.As(&m_sp2DBuffer)))
            {
                LONG lPitch = 0;
                PBYTE pScanline = nullptr;

                if (FAILED(hr = m_sp2DBuffer->Lock2DSize(
                               MF2DBuffer_LockFlags_ReadWrite, &pScanline, &lPitch, &m_pBuffer, &m_bufferLength)))
                {
                    LOG_ERROR("failed to lock 2D frame buffer: 0x%08x", hr);
                    assert(0);
                }
            }
            else
            {
                m_sp2DBuffer = nullptr; // Make sure 2D buffer is not set.
                if (FAILED(hr = m_spMediaBuffer->Lock(&m_pBuffer, nullptr, &m_bufferLength)))
                {
                    LOG_ERROR("failed to lock frame buffer: 0x%08x", hr);
                    assert(0);
                }
            }
        }
        else
        {
            LOG_ERROR("failed to get color frame media buffer: 0x%08x", hr);
            assert(0);
        }

        // Get Metadata
        ComPtr<IMFAttributes> spCaptureMetaData;

        if (SUCCEEDED(hr = m_spSample->GetUnknown(MFSampleExtension_CaptureMetadata, IID_PPV_ARGS(&spCaptureMetaData))))
        {
            m_exposureTime = (uint64_t)(
                MFGetAttributeUINT64(spCaptureMetaData.Get(), MF_CAPTURE_METADATA_EXPOSURE_TIME, 0) *
                0.1f); // hns to micro-second

            m_whiteBalance = (uint32_t)(
                MFGetAttributeUINT32(spCaptureMetaData.Get(), MF_CAPTURE_METADATA_WHITEBALANCE, 0));

            m_isoSpeed = (uint32_t)(MFGetAttributeUINT32(spCaptureMetaData.Get(), MF_CAPTURE_METADATA_ISO_SPEED, 0));

            ComPtr<IMFMediaBuffer> spRawMetadataBuffer;
            if (SUCCEEDED(hr = spCaptureMetaData->GetUnknown(MF_CAPTURE_METADATA_FRAME_RAWSTREAM,
                                                             IID_PPV_ARGS(&spRawMetadataBuffer))))
            {
                PBYTE pRawMetaData = nullptr;
                DWORD dwMaxLength = 0;
                DWORD dwCurrentLength = 0;

                if (SUCCEEDED(hr = spRawMetadataBuffer->Lock(&pRawMetaData, &dwMaxLength, &dwCurrentLength)))
                {
                    LONG bufferLeft = (LONG)dwCurrentLength;
                    if (bufferLeft >= sizeof(KSCAMERA_METADATA_ITEMHEADER))
                    {
                        PKSCAMERA_METADATA_ITEMHEADER pItem = (PKSCAMERA_METADATA_ITEMHEADER)pRawMetaData;
                        while (bufferLeft > 0)
                        {
                            switch (pItem->MetadataId)
                            {
                            case MetadataId_FrameAlignInfo:
                            {
                                PKSCAMERA_CUSTOM_METADATA_FrameAlignInfo pFrameAlignInfo =
                                    (PKSCAMERA_CUSTOM_METADATA_FrameAlignInfo)pItem;
                                m_capturePTS = pFrameAlignInfo->FramePTS;
                            }
                            break;
                            default:
                                break;
                            }

                            if (pItem->Size == 0)
                            {
                                LOG_ERROR("frame metadata id %d has zero buffer size", pItem->MetadataId);
                                break;
                            }

                            bufferLeft -= (LONG)pItem->Size;
                            if (bufferLeft < sizeof(KSCAMERA_METADATA_ITEMHEADER))
                            {
                                break;
                            }

                            pItem = reinterpret_cast<PKSCAMERA_METADATA_ITEMHEADER>(reinterpret_cast<PBYTE>(pItem) +
                                                                                    pItem->Size);
                        }
                    }

                    if (FAILED(hr = spRawMetadataBuffer->Unlock()))
                    {
                        LOG_ERROR("failed to unlock metadata raw buffer: 0x%08x", hr);
                    }
                }
            }
        }
        else
        {
            LOG_WARNING("No metadata attached to the sample", 0);
        }
    }
}

CFrameContext::~CFrameContext()
{
    if (m_pBuffer)
    {
        HRESULT hr = S_OK;

        if (m_sp2DBuffer)
        {
            if (FAILED(hr = m_sp2DBuffer->Unlock2D()))
            {
                LOG_ERROR("failed to unlock 2D frame buffer: 0x%08x", hr);
                assert(0);
            }
        }
        else
        {
            if (FAILED(hr = m_spMediaBuffer->Unlock()))
            {
                LOG_ERROR("failed to unlock frame buffer: 0x%08x", hr);
                assert(0);
            }
        }
    }
}

typedef struct _mf_buffer_wrapper_t
{
    void *allocator_context;
    CFrameContext *pFrameContext;
} mf_buffer_wrapper_t;

void FrameDestroyCallback(void *frame, void *context)
{
    (void)frame;
    mf_buffer_wrapper_t *wrapper = (mf_buffer_wrapper_t *)context;

    delete wrapper->pFrameContext;
    allocator_free(wrapper, wrapper->allocator_context);
}

CMFCameraReader::CMFCameraReader()
{
    const char *copy_to_new_buffer = environment_get_variable("K4A_MF_COPY_TO_NEW_BUFFER");
    if (copy_to_new_buffer != NULL && copy_to_new_buffer[0] != '\0' && copy_to_new_buffer[0] != '0')
    {
        m_use_mf_buffer = false;
    }
}

CMFCameraReader::~CMFCameraReader()
{
    if (m_mfStarted)
    {
        HRESULT hr = MFShutdown();
        if (FAILED(hr))
        {
            LOG_ERROR("MFShutdown failed: 0x%08x", hr);
        }
    }

    if (m_hStreamFlushed != NULL && m_hStreamFlushed != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hStreamFlushed);
    }
}

HRESULT CMFCameraReader::RuntimeClassInitialize(GUID *containerId)
{
    HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
    if (SUCCEEDED(hr))
    {
        m_mfStarted = true;
        ComPtr<IMFActivate> spDevice;
        ComPtr<IMFMediaSource> spColorSource;
        ComPtr<IMFAttributes> spAttributes;

        // Find Color Camera
        if (FAILED(hr = FindEdenColorCamera(containerId, spDevice)))
        {
            LOG_ERROR("Failed to find color camera in open camera: 0x%08x", hr);
            return hr;
        }

        // Activate Color Media Source
        if (FAILED(hr = spDevice->ActivateObject(IID_PPV_ARGS(&spColorSource))))
        {
            LOG_ERROR("Failed to activate source in open camera: 0x%08x", hr);
            return hr;
        }

        // Create Source Reader
        if (FAILED(hr = MFCreateAttributes(&spAttributes, 3)))
        {
            LOG_ERROR("Failed to create attribute bag in open camera: 0x%08x", hr);
            return hr;
        }

        if (FAILED(hr = spAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this)))
        {
            LOG_ERROR("Failed to register callback in open camera: 0x%08x", hr);
            return hr;
        }

        if (FAILED(hr = spAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, 1)))
        {
            LOG_ERROR("Failed to set Advanced video processing: 0x%08x", hr);
            return hr;
        }

        if (FAILED(hr = spAttributes->SetUINT32(MF_XVP_DISABLE_FRC, 1)))
        {
            LOG_ERROR("Failed to disable frame rate control: 0x%08x", hr);
            return hr;
        }

        if (FAILED(
                hr = MFCreateSourceReaderFromMediaSource(spColorSource.Get(), spAttributes.Get(), &m_spSourceReader)))
        {
            LOG_ERROR("Failed to create reader in open camera: 0x%08x", hr);
            return hr;
        }

        if (FAILED(hr = m_spSourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE)))
        {
            LOG_ERROR("Failed to deselect stream in open camera: 0x%08x", hr);
            return hr;
        }

        // Get KsControl to control sensor
        if (FAILED(hr = m_spSourceReader->GetServiceForStream((DWORD)MF_SOURCE_READER_MEDIASOURCE,
                                                              GUID_NULL,
                                                              IID_PPV_ARGS(&m_spKsControl))))
        {
            LOG_ERROR("Failed to get camera control interface in open camera: 0x%08x", hr);
            return hr;
        }

        m_hStreamFlushed = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_hStreamFlushed == NULL || m_hStreamFlushed == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ERROR("Failed to create event in open camera: 0x%08x", hr);
            return hr;
        }
    }

    return hr;
}

k4a_result_t CMFCameraReader::Start(const UINT32 width,
                                    const UINT32 height,
                                    const float fps,
                                    const k4a_image_format_t imageFormat,
                                    color_cb_stream_t *pCallback,
                                    void *pCallbackContext)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, pCallback == nullptr);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, pCallbackContext == nullptr);
    HRESULT hr = S_OK;
    GUID guidDeviceSubType = GUID_NULL;
    GUID guidOutputSubType = GUID_NULL;

    switch (imageFormat)
    {
    case K4A_IMAGE_FORMAT_COLOR_NV12:
        guidDeviceSubType = MFVideoFormat_NV12;
        guidOutputSubType = MFVideoFormat_NV12;
        break;
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
        guidDeviceSubType = MFVideoFormat_YUY2;
        guidOutputSubType = MFVideoFormat_YUY2;
        break;
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
        guidDeviceSubType = MFVideoFormat_MJPG;
        guidOutputSubType = MFVideoFormat_MJPG;
        break;
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        if (width == 1280 && height == 720)
        {
            guidDeviceSubType = MFVideoFormat_NV12;
        }
        else
        {
            guidDeviceSubType = MFVideoFormat_MJPG;
        }
        guidOutputSubType = MFVideoFormat_ARGB32;
        break;
    default:
        LOG_ERROR("Image Format %d is invalid", imageFormat);
        return K4A_RESULT_FAILED;
    }

    if (!m_started)
    {
        bool typeFound = false;
        DWORD typeIndex = 0;
        ComPtr<IMFMediaType> spMediaType;

        // Find target media type
        while (!typeFound)
        {
            hr = m_spSourceReader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                      typeIndex,
                                                      &spMediaType);

            if (SUCCEEDED(hr))
            {
                UINT32 tempWidth = 0;
                UINT32 tempHeight = 0;
                UINT32 tempFpsNumerator = 0;
                UINT32 tempFpsDenominator = 0;
                GUID tempSubType = GUID_NULL;

                if (FAILED(hr = MFGetAttributeSize(spMediaType.Get(), MF_MT_FRAME_SIZE, &tempWidth, &tempHeight)))
                {
                    LOG_ERROR("Failed to get available frame size at start: 0x%08x", hr);
                    return k4aResultFromHRESULT(hr);
                }
                if (FAILED(hr = MFGetAttributeRatio(spMediaType.Get(),
                                                    MF_MT_FRAME_RATE,
                                                    &tempFpsNumerator,
                                                    &tempFpsDenominator)))
                {
                    LOG_ERROR("Failed to get available frame rate at start: 0x%08x", hr);
                    return k4aResultFromHRESULT(hr);
                }
                if (FAILED(hr = spMediaType->GetGUID(MF_MT_SUBTYPE, &tempSubType)))
                {
                    LOG_ERROR("Failed to get available color format at start: 0x%08x", hr);
                    return k4aResultFromHRESULT(hr);
                }

                if (width == tempWidth && height == tempHeight &&
                    fps == ((float)tempFpsNumerator / (float)tempFpsDenominator) && guidDeviceSubType == tempSubType)
                {
                    typeFound = true;
                }
            }
            else if (hr == MF_E_NO_MORE_TYPES)
            {
                break;
            }
            else
            {
                LOG_ERROR("Failed to enumerate media type at start: 0x%08x", hr);
                return k4aResultFromHRESULT(hr);
            }

            typeIndex++;
        }

        if (typeFound)
        {
            ComPtr<IMFSourceReaderEx> spSourceReaderEx;
            ComPtr<IMFMediaType> spOutputMediaType;

            if (FAILED(hr = m_spSourceReader.As(&spSourceReaderEx)))
            {
                LOG_ERROR("Failed to get source reader extension at start: 0x%08x", hr);
                return k4aResultFromHRESULT(hr);
            }
            DWORD dwStreamFlags = 0;

            if (FAILED(hr = spSourceReaderEx->SetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                                 spMediaType.Get(),
                                                                 &dwStreamFlags)))
            {
                LOG_ERROR("Failed to set media type at start: 0x%08x", hr);
                return k4aResultFromHRESULT(hr);
            }

            if (guidDeviceSubType != guidOutputSubType)
            {
                // Create Output Media type
                if (FAILED(hr = MFCreateMediaType(&spOutputMediaType)))
                {
                    LOG_ERROR("Failed to create output media type at start: 0x%08x", hr);
                    return k4aResultFromHRESULT(hr);
                }

                // Copy all items from device type to output type
                if (FAILED(hr = spMediaType->CopyAllItems(spOutputMediaType.Get())))
                {
                    LOG_ERROR("Failed to copy device type to output type at start: 0x%08x", hr);
                    return k4aResultFromHRESULT(hr);
                }

                // Modify output media type
                if (FAILED(hr = spOutputMediaType->SetGUID(MF_MT_SUBTYPE, guidOutputSubType)))
                {
                    LOG_ERROR("Failed to set output subtype at start: 0x%08x", hr);
                    return k4aResultFromHRESULT(hr);
                }
            }
            else
            {
                spOutputMediaType = spMediaType;
            }

            if (FAILED(hr = m_spSourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                                  nullptr,
                                                                  spOutputMediaType.Get())))
            {
                LOG_ERROR("Failed to set output media type at start: 0x%08x", hr);
                return k4aResultFromHRESULT(hr);
            }
        }
        else
        {
            LOG_ERROR("Can not find requested sensor mode", 0);
            return K4A_RESULT_FAILED;
        }

        if (SUCCEEDED(hr = m_spSourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE)))
        {
            auto lock = m_lock.LockExclusive();

            m_width_pixels = width;
            m_height_pixels = height;
            m_image_format = imageFormat;

            m_pCallback = pCallback;
            m_pCallbackContext = pCallbackContext;

            if (SUCCEEDED(hr = m_spSourceReader->ReadSample(
                              (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr, nullptr, nullptr)))
            {
                m_started = true;
            }
            else
            {
                LOG_ERROR("Failed to request first sample at start: 0x%08x", hr);
            }
        }
        else
        {
            LOG_ERROR("Failed to select stream at start: 0x%08x", hr);
        }
    }
    else
    {
        LOG_WARNING("Start request in started state", 0);
    }

    return k4aResultFromHRESULT(hr);
}

void CMFCameraReader::Shutdown()
{
    if (m_started)
    {
        Stop();
    }

    m_spSourceReader.Reset();
    m_spKsControl.Reset();
}

void CMFCameraReader::Stop()
{
    HRESULT hr = S_OK;
    auto lock = m_lock.LockExclusive();

    if (m_started)
    {
        // Set flag to not handle sample anymore
        m_started = false;

        if (SUCCEEDED(hr = m_spSourceReader->Flush((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM)))
        {
            m_flushing = true;
        }
        else
        {
            LOG_ERROR("Failed to request flush for stop: 0x%08x", hr);
        }

        lock.Unlock(); // Wait without lock
        do
        {
            // Wait until async operations are terminated for 10 sec
            switch (WaitForSingleObject(m_hStreamFlushed, 10000))
            {
            case WAIT_OBJECT_0:
                // Flushing completed
                return;
            case WAIT_TIMEOUT:
                LOG_ERROR("Timeout waiting for m_hStreamFlushed");
                break;
            case WAIT_FAILED:
                LOG_ERROR("WaitForSingleObject on m_hStreamFlushed failed (%d)", GetLastError());
                assert(false);
                break;
            default:
                break;
            }
        } while (1);
    }
}

k4a_result_t CMFCameraReader::GetCameraControl(const k4a_color_control_command_t command,
                                               k4a_color_control_mode_t *mode,
                                               int32_t *pValue)
{
    HRESULT hr = S_OK;
    LONG propertyValue = 0;
    ULONG flags = 0;
    ULONG capability = 0;

    // clear return
    *mode = K4A_COLOR_CONTROL_MODE_MANUAL;
    *pValue = 0;

    switch (command)
    {
    case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE:
    {
        hr = GetCameraControlValue(KSPROPERTY_CAMERACONTROL_EXPOSURE, &propertyValue, &flags, &capability);

        // Convert KSProperty exposure time value to micro-second unit
        propertyValue = (LONG)(exp2f((float)propertyValue) * 1000000.0f);
    }
    break;
    case K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY:
    {
        hr =
            GetCameraControlValue(KSPROPERTY_CAMERACONTROL_AUTO_EXPOSURE_PRIORITY, &propertyValue, &flags, &capability);
    }
    break;
    case K4A_COLOR_CONTROL_BRIGHTNESS:
    {
        hr = GetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS, &propertyValue, &flags, &capability);
    }
    break;
    case K4A_COLOR_CONTROL_CONTRAST:
    {
        hr = GetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_CONTRAST, &propertyValue, &flags, &capability);
    }
    break;
    case K4A_COLOR_CONTROL_SATURATION:
    {
        hr = GetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_SATURATION, &propertyValue, &flags, &capability);
    }
    break;
    case K4A_COLOR_CONTROL_SHARPNESS:
    {
        hr = GetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_SHARPNESS, &propertyValue, &flags, &capability);
    }
    break;
    case K4A_COLOR_CONTROL_WHITEBALANCE:
    {
        hr = GetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE, &propertyValue, &flags, &capability);
    }
    break;
    case K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION:
    {
        hr = GetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION, &propertyValue, &flags, &capability);
    }
    break;
    case K4A_COLOR_CONTROL_GAIN:
    {
        hr = GetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_GAIN, &propertyValue, &flags, &capability);
    }
    break;
    case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY:
    {
        hr = GetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY, &propertyValue, &flags, &capability);
    }
    break;
    default:
        return K4A_RESULT_FAILED;
    }

    if (SUCCEEDED(hr))
    {
        if (flags & KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL)
        {
            *mode = K4A_COLOR_CONTROL_MODE_MANUAL;
        }
        else if (flags & KSPROPERTY_CAMERACONTROL_FLAGS_AUTO)
        {
            *mode = K4A_COLOR_CONTROL_MODE_AUTO;
        }

        *pValue = (int32_t)propertyValue;
    }

    return k4aResultFromHRESULT(hr);
}

k4a_result_t CMFCameraReader::SetCameraControl(const k4a_color_control_command_t command,
                                               const k4a_color_control_mode_t mode,
                                               int32_t newValue)
{
    HRESULT hr = S_OK;
    ULONG flags = KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;

    if (mode == K4A_COLOR_CONTROL_MODE_AUTO)
    {
        if (command == K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE || command == K4A_COLOR_CONTROL_WHITEBALANCE)
        {
            flags = KSPROPERTY_CAMERACONTROL_FLAGS_AUTO;
        }
        else
        {
            LOG_ERROR("K4A_COLOR_CONTROL_MODE_AUTO is not supported for %d color control", command);
            return K4A_RESULT_FAILED;
        }
    }

    switch (command)
    {
    case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE:
    {
        // Convert micro-second unit to KSProperty exposure time value
        hr = SetCameraControlValue(KSPROPERTY_CAMERACONTROL_EXPOSURE, (LONG)log2f((float)newValue * 0.000001f), flags);
    }
    break;
    case K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY:
    {
        if (newValue == 0 || newValue == 1)
        {
            hr = SetCameraControlValue(KSPROPERTY_CAMERACONTROL_AUTO_EXPOSURE_PRIORITY, (LONG)newValue, flags);
        }
        else
        {
            LOG_ERROR("K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY only accept 0 or 1. Actual = %d", newValue);
            return K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_BRIGHTNESS:
    {
        hr = SetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS, (LONG)newValue, flags);
    }
    break;
    case K4A_COLOR_CONTROL_CONTRAST:
    {
        hr = SetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_CONTRAST, (LONG)newValue, flags);
    }
    break;
    case K4A_COLOR_CONTROL_SATURATION:
    {
        hr = SetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_SATURATION, (LONG)newValue, flags);
    }
    break;
    case K4A_COLOR_CONTROL_SHARPNESS:
    {
        hr = SetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_SHARPNESS, (LONG)newValue, flags);
    }
    break;
    case K4A_COLOR_CONTROL_WHITEBALANCE:
    {
        hr = SetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE, (LONG)newValue, flags);
    }
    break;
    case K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION:
    {
        hr = SetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION, (LONG)newValue, flags);
    }
    break;
    case K4A_COLOR_CONTROL_GAIN:
    {
        hr = SetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_GAIN, (LONG)newValue, flags);
    }
    break;
    case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY:
    {
        hr = SetVideoProcAmpValue(KSPROPERTY_VIDEOPROCAMP_POWERLINE_FREQUENCY, (LONG)newValue, flags);
    }
    break;
    default:
        return K4A_RESULT_FAILED;
    }

    return k4aResultFromHRESULT(hr);
}

int CMFCameraReader::GetStride()
{
    int stride;
    switch (m_image_format)
    {
    case K4A_IMAGE_FORMAT_COLOR_NV12:
        stride = m_width_pixels;
        break;
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
        stride = m_width_pixels * 2;
        break;
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        stride = m_width_pixels * 4;
        break;
    default:
        stride = 0; // MJPG does not have stride
        break;
    }
    return stride;
}

k4a_result_t CMFCameraReader::CreateImage(CFrameContext *pFrameContext, k4a_image_t *image)
{
    void *context;
    mf_buffer_wrapper_t *wrapper;

    // We use a wrapper here so that we can keep track of the MF frame buffers in use; ALLOCATION_SOURCE_COLOR will
    // count outstanding allocations. If too many are used then MF stops receiving images over USB until the captures
    // are released.
    wrapper = (mf_buffer_wrapper_t *)allocator_alloc(ALLOCATION_SOURCE_COLOR, sizeof(mf_buffer_wrapper_t), &context);

    wrapper->allocator_context = context;
    wrapper->pFrameContext = pFrameContext;

    return TRACE_CALL(image_create_from_buffer(m_image_format,
                                               m_width_pixels,
                                               m_height_pixels,
                                               GetStride(),
                                               pFrameContext->GetBuffer(),
                                               pFrameContext->GetFrameSize(),
                                               FrameDestroyCallback,
                                               wrapper,
                                               image));
}

k4a_result_t CMFCameraReader::CreateImageCopy(CFrameContext *pFrameContext, k4a_image_t *image)
{

    size_t size = pFrameContext->GetFrameSize();
    void *context;
    k4a_result_t result;
    uint8_t *buffer = allocator_alloc(ALLOCATION_SOURCE_COLOR, size, &context);
    result = K4A_RESULT_FROM_BOOL(buffer != NULL);

    if (K4A_SUCCEEDED(result))
    {
        memcpy(buffer, pFrameContext->GetBuffer(), size);

        result = TRACE_CALL(image_create_from_buffer(m_image_format,
                                                     m_width_pixels,
                                                     m_height_pixels,
                                                     GetStride(),
                                                     buffer,
                                                     size,
                                                     allocator_free,
                                                     context,
                                                     image));
    }
    else
    {
        // cleanup if there was an error
        allocator_free(buffer, context);
    }
    return result;
}

STDMETHODIMP CMFCameraReader::OnReadSample(HRESULT hrStatus,
                                           DWORD dwStreamIndex,
                                           DWORD dwStreamFlags,
                                           LONGLONG llTimestamp,
                                           IMFSample *pSample)
{
    HRESULT hr = S_OK;
    UNREFERENCED_PARAMETER(dwStreamIndex);
    UNREFERENCED_PARAMETER(dwStreamFlags);
    UNREFERENCED_PARAMETER(llTimestamp);

    if (SUCCEEDED(hrStatus))
    {
        auto lock = m_lock.LockExclusive();

        if (m_started && !m_flushing)
        {
            if (pSample && m_pCallback && m_pCallbackContext)
            {
                k4a_capture_t capture = NULL;
                k4a_image_t image = NULL;
                k4a_result_t result;
                bool dropped = false;
                bool FrameContextRefd = false;

                CFrameContext *pFrameContext = new (std::nothrow) CFrameContext(pSample);
                result = K4A_RESULT_FROM_BOOL(pFrameContext != NULL);

                if (K4A_SUCCEEDED(result) && pFrameContext->GetPTSTime() == 0)
                {
                    dropped = true;
                    result = K4A_RESULT_FAILED;
                    LOG_INFO("Dropping color image due to ts:%lld", pFrameContext->GetPTSTime());
                }

                if (K4A_SUCCEEDED(result))
                {
                    if (m_use_mf_buffer)
                    {
                        result = CreateImage(pFrameContext, &image);
                        FrameContextRefd = true;
                    }
                    else
                    {
                        result = CreateImageCopy(pFrameContext, &image);
                    }
                }

                if (K4A_SUCCEEDED(result))
                {
                    result = TRACE_CALL(capture_create(&capture));
                }

                if (K4A_SUCCEEDED(result))
                {
                    image_set_timestamp_usec(image, K4A_90K_HZ_TICK_TO_USEC(pFrameContext->GetPTSTime()));

                    // Set metadata
                    image_set_exposure_time_usec(image, pFrameContext->GetExposureTime());
                    image_set_white_balance(image, pFrameContext->GetWhiteBalance());
                    image_set_iso_speed(image, pFrameContext->GetISOSpeed());

                    capture_set_color_image(capture, image);
                }

                if (dropped == false)
                {
                    m_pCallback(result, capture, m_pCallbackContext);
                }

                if (!FrameContextRefd)
                {
                    delete pFrameContext;
                }

                if (image)
                {
                    image_dec_ref(image);
                }
                if (capture)
                {
                    // We guarantee that capture is valid for the duration of the callback function, if someone
                    // needs it to live longer, then they need to add a ref
                    capture_dec_ref(capture);
                }
            }

            // Request next sample
            if (FAILED(hr = m_spSourceReader->ReadSample(
                           (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr, nullptr, nullptr)))
            {
                LOG_ERROR("Failed to request sample: 0x%08x", hr);
            }
        }
    }
    else
    {
        hr = hrStatus;
        LOG_ERROR("Pipeline propagate error to callback: 0x%08x", hr);
        // Notify upper layers of failed status
        m_pCallback(K4A_RESULT_FAILED, NULL, m_pCallbackContext);
    }

    if (FAILED(hr) && m_pCallback && m_pCallbackContext)
    {
        m_pCallback(K4A_RESULT_FAILED, NULL, m_pCallbackContext);

        // Stop and clean up.
        Stop();
    }

    return hr;
}

STDMETHODIMP CMFCameraReader::OnFlush(DWORD dwStreamIndex)
{
    UNREFERENCED_PARAMETER(dwStreamIndex);
    HRESULT hr = S_OK;
    auto lock = m_lock.LockShared();

    if (m_started == false)
    {
        // Flushed. Deselect stream if it's not streaming
        if (SUCCEEDED(hr = m_spSourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, FALSE)))
        {
            m_pCallback = nullptr;
            m_pCallbackContext = nullptr;
        }
        else
        {
            LOG_ERROR("Failed to deselect stream for stop: 0x%08x", hr);
        }
    }

    if (SetEvent(m_hStreamFlushed) == 0)
    {
        LOG_ERROR("Failed to set flushed event after flushing: 0x%08x", HRESULT_FROM_WIN32(GetLastError()));
    }

    m_flushing = false;

    return hr;
}

STDMETHODIMP CMFCameraReader::OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent)
{
    UNREFERENCED_PARAMETER(dwStreamIndex);
    UNREFERENCED_PARAMETER(pEvent);
    return S_OK;
}

HRESULT CMFCameraReader::FindEdenColorCamera(GUID *containerId, ComPtr<IMFActivate> &spDevice)
{
    HRESULT hr = S_OK;
    UINT32 count = 0;
    ComPtr<IMFAttributes> spConfig;
    IMFActivate **ppDevices = nullptr;
    REFGUID vidcapCategoryGuid = KSCATEGORY_VIDEO_CAMERA;

    // Create an attribute store to hold the search criteria.
    hr = MFCreateAttributes(&spConfig, 2);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create attribute bag to find color camera: 0x%08x", hr);
        goto exit;
    }

    // Request video capture devices.
    hr = spConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to set video capture attribute to find color camera: 0x%08x", hr);
        goto exit;
    }

    // Request video capture devices that belong to the requested category
    hr = spConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_CATEGORY, vidcapCategoryGuid);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to set video category to find color camera: 0x%08x", hr);
        goto exit;
    }

    // Enumerate the devices,
    hr = MFEnumDeviceSources(spConfig.Get(), &ppDevices, &count);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to enumerate device: 0x%08x", hr);
        goto exit;
    }

    hr = MF_E_NOT_FOUND;
    for (DWORD i = 0; i < count; i++)
    {
        if (SUCCEEDED(ppDevices[i]->GetItem(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, nullptr)))
        {
            WCHAR *symbolicLink = nullptr;
            UINT32 length = 0;
            UINT32 outLength = 0;

            if (SUCCEEDED(
                    ppDevices[i]->GetStringLength(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &length)))
            {
                symbolicLink = new (std::nothrow) WCHAR[length + 1];
                if (symbolicLink &&
                    SUCCEEDED(ppDevices[i]->GetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                                      (LPWSTR)symbolicLink,
                                                      length + 1,
                                                      &outLength)))
                {
                    // Search vender and product id.
                    if (wcsstr(symbolicLink, COLOR_CAMERA_IDENTIFIER))
                    {
                        ULONG cbWritten = 0;
                        GUID guidContainerId = GUID_NULL;

                        if (SUCCEEDED(GetDeviceProperty(symbolicLink,
                                                        &DEVPKEY_Device_ContainerId,
                                                        (PBYTE)&guidContainerId,
                                                        sizeof(guidContainerId),
                                                        &cbWritten)))
                        {
                            if (*containerId == guidContainerId)
                            {
                                spDevice = ppDevices[i];
                                delete[] symbolicLink;
                                symbolicLink = nullptr;
                                hr = S_OK;

                                break;
                            }
                        }
                    }
                }
            }
            if (symbolicLink)
            {
                delete[] symbolicLink;
            }
        }
    }

exit:
    if (ppDevices)
    {
        for (DWORD i = 0; i < count; i++)
        {
            ppDevices[i]->Release();
        }
        CoTaskMemFree(ppDevices);
    }

    return hr;
}

HRESULT CMFCameraReader::GetDeviceProperty(LPCWSTR DeviceSymbolicName,
                                           const DEVPROPKEY *pKey,
                                           PBYTE pbBuffer,
                                           ULONG cbBuffer,
                                           ULONG *pcbData)
{
    HRESULT hr = S_OK;
    CONFIGRET cr = CR_SUCCESS;
    wchar_t wz[MAX_PATH] = { 0 };
    ULONG cb = sizeof(wz);
    DEVPROPTYPE propType;
    DEVINST hDevInstance = 0;

    if (DeviceSymbolicName == nullptr || pcbData == nullptr)
    {
        return E_INVALIDARG;
    }
    *pcbData = 0;

    cr =
        CM_Get_Device_Interface_PropertyW(DeviceSymbolicName, &DEVPKEY_Device_InstanceId, &propType, (PBYTE)wz, &cb, 0);
    if (cr != CR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA));
    }

    cr = CM_Locate_DevNodeW(&hDevInstance, wz, CM_LOCATE_DEVNODE_NORMAL);
    if (cr != CR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA));
    }

    cb = 0;
    cr = CM_Get_DevNode_PropertyW(hDevInstance, pKey, &propType, nullptr, &cb, 0);
    if (cr == CR_BUFFER_SMALL)
    {
        if (pbBuffer == nullptr || cbBuffer == 0)
        {
            *pcbData = cb;
            goto done;
        }
        if (cb > cbBuffer)
        {
            *pcbData = cb;
            return MF_E_INSUFFICIENT_BUFFER;
        }
        cb = cbBuffer;
        cr = CM_Get_DevNode_PropertyW(hDevInstance, pKey, &propType, pbBuffer, &cb, 0);
    }
    if (cr != CR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA));
    }

done:
    return hr;
}

HRESULT CMFCameraReader::GetVideoProcAmpValue(const KSPROPERTY_VIDCAP_VIDEOPROCAMP PropertyId,
                                              LONG *pValue,
                                              ULONG *pFlags,
                                              ULONG *pCapability)
{
    HRESULT hr = S_OK;
    KSPROPERTY_VIDEOPROCAMP_S videoCamp = {};
    videoCamp.Property.Set = PROPSETID_VIDCAP_VIDEOPROCAMP;
    videoCamp.Property.Id = PropertyId;
    videoCamp.Property.Flags = KSPROPERTY_TYPE_GET;
    videoCamp.Value = -1;
    ULONG retSize = 0;

    if (SUCCEEDED(hr = m_spKsControl->KsProperty(
                      (PKSPROPERTY)&videoCamp, sizeof(videoCamp), &videoCamp, sizeof(videoCamp), &retSize)))
    {
        if (pValue)
        {
            *pValue = videoCamp.Value;
        }

        if (pFlags)
        {
            *pFlags = videoCamp.Flags;
        }

        if (pCapability)
        {
            *pCapability = videoCamp.Capabilities;
        }
    }

    return hr;
}

HRESULT CMFCameraReader::SetVideoProcAmpValue(const KSPROPERTY_VIDCAP_VIDEOPROCAMP PropertyId,
                                              LONG newValue,
                                              ULONG newFlags)
{
    KSPROPERTY_VIDEOPROCAMP_S videoCamp = {};
    videoCamp.Property.Set = PROPSETID_VIDCAP_VIDEOPROCAMP;
    videoCamp.Property.Id = PropertyId;
    videoCamp.Property.Flags = KSPROPERTY_TYPE_SET;
    videoCamp.Value = newValue;
    videoCamp.Flags = newFlags;
    ULONG retSize = 0;

    return m_spKsControl->KsProperty((PKSPROPERTY)&videoCamp,
                                     sizeof(videoCamp),
                                     &videoCamp,
                                     sizeof(videoCamp),
                                     &retSize);
}

HRESULT CMFCameraReader::GetCameraControlValue(const KSPROPERTY_VIDCAP_CAMERACONTROL PropertyId,
                                               LONG *pValue,
                                               ULONG *pFlags,
                                               ULONG *pCapability)
{
    HRESULT hr = S_OK;
    KSPROPERTY_CAMERACONTROL_S videoControl = {};
    videoControl.Property.Set = PROPSETID_VIDCAP_CAMERACONTROL;
    videoControl.Property.Id = PropertyId;
    videoControl.Property.Flags = KSPROPERTY_TYPE_GET;
    videoControl.Value = -1;
    ULONG retSize = 0;

    if (SUCCEEDED(hr = m_spKsControl->KsProperty(
                      (PKSPROPERTY)&videoControl, sizeof(videoControl), &videoControl, sizeof(videoControl), &retSize)))
    {
        if (pValue)
        {
            *pValue = videoControl.Value;
        }

        if (pFlags)
        {
            *pFlags = videoControl.Flags;
        }

        if (pCapability)
        {
            *pCapability = videoControl.Capabilities;
        }
    }

    return hr;
}

HRESULT CMFCameraReader::SetCameraControlValue(const KSPROPERTY_VIDCAP_CAMERACONTROL PropertyId,
                                               LONG newValue,
                                               ULONG newFlags)
{
    KSPROPERTY_CAMERACONTROL_S videoControl = {};
    videoControl.Property.Set = PROPSETID_VIDCAP_CAMERACONTROL;
    videoControl.Property.Id = PropertyId;
    videoControl.Property.Flags = KSPROPERTY_TYPE_SET;
    videoControl.Value = newValue;
    videoControl.Flags = newFlags;
    ULONG retSize = 0;

    return m_spKsControl->KsProperty((PKSPROPERTY)&videoControl,
                                     sizeof(videoControl),
                                     &videoControl,
                                     sizeof(videoControl),
                                     &retSize);
}
