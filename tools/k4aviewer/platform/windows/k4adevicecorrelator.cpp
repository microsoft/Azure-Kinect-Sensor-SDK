// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4adevicecorrelator.h"

// System headers
//
#include <codecvt>
#include <functional>
#include <locale>
#include <memory>
#include <map>
#include <regex>

// Library headers
//

// Shut off windows.h's min/max macros so they don't conflict with the STL min/max functions, which
// are used in some of the k4aviewer headers
//
#define NOMINMAX

#include <windows.h>
#include <combaseapi.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <setupapi.h>
#include <endpointvolume.h>
#include <devpkey.h>

// Project headers
//
#include "k4aviewererrormanager.h"
#include "k4aviewerutil.h"

#define RETURN_IF_FAILED(hr)                                                                                           \
    if (FAILED(hr))                                                                                                    \
    {                                                                                                                  \
        return hr;                                                                                                     \
    }

using namespace k4aviewer;

namespace
{
// Functor to clean up COM objects that we create
//
template<typename T> struct ComSafeDeleter
{
    void operator()(T *managedComObject)
    {
        if (managedComObject != nullptr)
        {
            managedComObject->Release();
            managedComObject = nullptr;
        }
    }
};

template<typename T> using ComUniquePtr = std::unique_ptr<T, ComSafeDeleter<T>>;

// GUID comparer to allow use of std::maps of GUIDs
//
struct GuidComparer
{
    bool operator()(const GUID &a, const GUID &b) const
    {
        return a.Data1 != b.Data1 ?
                   a.Data1 < b.Data1 :
                   a.Data2 != b.Data2 ? a.Data2 < b.Data2 :
                                        a.Data3 != b.Data3 ? a.Data3 < b.Data3 : memcmp(a.Data4, b.Data4, 8) < 0;
    }
};

// Adapted from https://docs.microsoft.com/en-us/windows/desktop/CoreAudio/device-properties
//
HRESULT GetContainerIdToWasapiIdMap(std::map<GUID, std::string, GuidComparer> *result)
{
    ComUniquePtr<IMMDeviceEnumerator> pEnumerator;
    {
        // clang-format off
        //
        // BCDE0395-E52F-467C-8E3D-C4579291692E
        //
        constexpr CLSID mmDeviceEnumeratorGuid =
            { 0xBCDE0395, 0xE52F, 0x467C, { 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E } };

        // A95664D2-9614-4F35-A746-DE8DB63617E6
        //
        constexpr IID immDeviceEnumeratorGuid =
            { 0xA95664D2, 0x9614, 0x4F35, { 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6 } };
        //
        // clang-format on

        IMMDeviceEnumerator *rawEnumerator = nullptr;
        RETURN_IF_FAILED(CoCreateInstance(mmDeviceEnumeratorGuid,
                                          nullptr,
                                          CLSCTX_ALL,
                                          immDeviceEnumeratorGuid,
                                          reinterpret_cast<void **>(&rawEnumerator)));
        pEnumerator.reset(rawEnumerator);
    }

    // Enumerate capture devices (i.e microphones)
    //
    ComUniquePtr<IMMDeviceCollection> pCollection;
    {
        IMMDeviceCollection *rawCollection = nullptr;
        RETURN_IF_FAILED(pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &rawCollection));
        pCollection.reset(rawCollection);
    }

    UINT count;
    RETURN_IF_FAILED(pCollection->GetCount(&count));

    for (ULONG i = 0; i < count; i++)
    {
        ComUniquePtr<IMMDevice> pEndpoint;
        {
            IMMDevice *rawEndpoint = nullptr;
            RETURN_IF_FAILED(pCollection->Item(i, &rawEndpoint));
            pEndpoint.reset(rawEndpoint);
        }

        // Get the endpoint ID string (this is what libsoundio wants)
        //
        LPWSTR idComString = nullptr;
        RETURN_IF_FAILED(pEndpoint->GetId(&idComString));

        std::wstring idWString = idComString;
        CoTaskMemFree(idComString);

        // Convert to an ASCII string, which is what libsoundio expects.
        // Note that this works by doing a narrowing conversion of the wchar_ts in the wstring
        // into to chars in the string.
        // This is not always safe.  However, in this case, it is, because we know that WASAPI identifiers
        // only use letters A-Z, numbers 0-9, dashes(-), periods(.), and curly braces ({}), all of which
        // have the same numeric value for chars and wchars and don't use the upper byte of the wchar.
        //
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::string idString = converter.to_bytes(idWString);

        ComUniquePtr<IPropertyStore> pProps;
        {
            IPropertyStore *rawProps = nullptr;
            RETURN_IF_FAILED(pEndpoint->OpenPropertyStore(STGM_READ, &rawProps));
            pProps.reset(rawProps);
        }

        // Get the endpoint's container ID
        PROPVARIANT containerIdProperty;
        PropVariantInit(&containerIdProperty);
        RETURN_IF_FAILED(pProps->GetValue(PKEY_Device_ContainerId, &containerIdProperty));

        GUID containerId = *containerIdProperty.puuid;

        PropVariantClear(&containerIdProperty);

        (*result)[containerId] = std::move(idString);
    }

    return S_OK;
}

HRESULT GetSerialNumberToContainerIdMap(std::map<std::string, GUID> *result)
{
    // WinUSB Device {88BAE032-5A81-49F0-BC3D-A4FF138216D6}
    // see http://msdn.microsoft.com/en-us/library/windows/hardware/ff553426%28v=vs.85%29.aspx
    //
    // clang-format off
    constexpr GUID winUsbDeviceClassGuid =
        { 0x88BAE032, 0x5A81, 0x49F0, { 0xBC, 0x3D, 0xA4, 0xFF, 0x13, 0x82, 0x16, 0xD6 } };
    // clang-format on

    // Get list of USB devices
    //
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&winUsbDeviceClassGuid, "USB", nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    CleanupGuard hDevInfoGuard([&hDevInfo]() { SetupDiDestroyDeviceInfoList(hDevInfo); });

    // Loop through the devices from the USB class
    //
    SP_DEVINFO_DATA deviceInfo{};
    deviceInfo.cbSize = sizeof(deviceInfo);
    DWORD currentDeviceId = 0;
    while (SetupDiEnumDeviceInfo(hDevInfo, currentDeviceId, &deviceInfo))
    {
        currentDeviceId++;

        // The device path shouldn't be > this for any of the devices that we care about
        //
        char devicePathBuffer[500];
        DWORD devicePathLength = 0;

        if (!SetupDiGetDeviceInstanceId(hDevInfo,
                                        &deviceInfo,
                                        devicePathBuffer,
                                        sizeof(devicePathBuffer),
                                        &devicePathLength))
        {
            continue;
        }

        // Example string: USB\VID_045E&PID_097C\EV1-014
        //
        const std::regex vidPidRegex(R"(USB\\VID_([0-9A-F]{4})&PID_([0-9A-F]{4})\\(.*))");

        std::cmatch match;
        if (!std::regex_match(devicePathBuffer, match, vidPidRegex))
        {
            continue;
        }

        // Extract vid/pid/serial number
        //
        // match[0] is the whole match; captures start at 1
        //
        std::string vidStr = match[1];
        std::string pidStr = match[2];
        std::string serialNumber = match[3];

        constexpr int baseHex = 16;
        const auto vid = uint16_t(std::stoul(vidStr, nullptr, baseHex));
        const auto pid = uint16_t(std::stoul(pidStr, nullptr, baseHex));

        constexpr uint16_t depthCameraVid = 0x045E;
        constexpr uint16_t depthCameraPid = 0x097C;

        if (vid != depthCameraVid || pid != depthCameraPid)
        {
            continue;
        }

        DEVPROPTYPE propType;
        GUID containerId{};
        if (!SetupDiGetDevicePropertyW(hDevInfo,
                                       &deviceInfo,
                                       &DEVPKEY_Device_ContainerId,
                                       &propType,
                                       reinterpret_cast<BYTE *>(&containerId),
                                       sizeof(containerId),
                                       nullptr,
                                       0))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        (*result)[serialNumber] = containerId;
    }

    // SetupDiEnumDeviceInfo is expected to set the last error to ERROR_NO_MORE_ITEMS when
    // it completes successfully.  If it fails for any other reason, we have a problem and want
    // to report failure.
    //
    const DWORD lastError = GetLastError();
    if (lastError != ERROR_NO_MORE_ITEMS)
    {
        return HRESULT_FROM_WIN32(lastError);
    }

    return S_OK;
}
} // namespace

bool K4ADeviceCorrelator::GetSoundIoBackendIdToSerialNumberMapping(SoundIo *soundio,
                                                                   std::map<std::string, std::string> *result)
{
    (void)soundio;

    std::map<std::string, GUID> serialNumberToContainerIdMap;
    std::map<GUID, std::string, GuidComparer> containerIdToWasapiIdMap;

    if (FAILED(GetSerialNumberToContainerIdMap(&serialNumberToContainerIdMap)) ||
        FAILED(GetContainerIdToWasapiIdMap(&containerIdToWasapiIdMap)))
    {
        return false;
    }

    result->clear();

    for (auto &serialNumberToContainerIdMapping : serialNumberToContainerIdMap)
    {
        auto containerIdToWasapiIdMapping = containerIdToWasapiIdMap.find(serialNumberToContainerIdMapping.second);
        if (containerIdToWasapiIdMapping != containerIdToWasapiIdMap.end())
        {
            (*result)[containerIdToWasapiIdMapping->second] = serialNumberToContainerIdMapping.first;
        }
    }

    return true;
}
