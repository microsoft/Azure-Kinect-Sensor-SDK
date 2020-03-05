// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "k4afastcapture.h"
#include <k4ainternal/common.h>
#include <string>

using namespace k4afastcapture;

#ifdef _WIN32
void VERIFY_HR(HRESULT hr)
{
    if (FAILED(hr))
    {
        std::cout << "DS >> ERROR: " << hr << std::endl;
        exit(hr);
    }
}
#endif

K4AFastCapture::K4AFastCapture() :
    m_colorFileDirectory(""),
    m_depthFileDirectory(""),
    m_frameRequestedNum(0),
    m_pcmShiftValue(4),
    m_pcmOutputHeight(1024),
    m_pcmOutputWidth(1024),
    m_streaming(true),
    m_device(NULL),
    m_capture(NULL),
    m_deviceConfig(K4A_DEVICE_CONFIG_INIT_DISABLE_ALL)
{

#ifdef _WIN32
    m_captureRequestedEvent = Microsoft::WRL::Wrappers::Event(
        CreateEventW(false, false, false, L"Global\\captureRequestedEvent"));
    m_captureDoneEvent = Microsoft::WRL::Wrappers::Event(
        CreateEventW(false, false, false, L"Global\\captureDoneEvent"));
    m_captureExitEvent = Microsoft::WRL::Wrappers::Event(
        CreateEventW(false, false, false, L"Global\\captureExitEvent"));

    if (m_captureRequestedEvent == NULL || m_captureDoneEvent == NULL || m_captureExitEvent == NULL)
    {
        throw std::exception("Create Event failed");
    }
#else
    m_captureRequestedSem = sem_open("/globalCaptureRequestedSem", O_CREAT, 0644, 0);
    m_captureDoneSem = sem_open("/globalCaptureDoneSem", O_CREAT, 0644, 1);
    m_captureExitSem = sem_open("/globalCaptureExitSem", O_CREAT, 0644, 0);
    if (m_captureRequestedSem == SEM_FAILED || m_captureExitSem == SEM_FAILED || m_captureDoneSem == SEM_FAILED)
    {
        std::cout << "Create Semaphore failed " << std::endl;
        throw std::exception();
    }
#endif
}

K4AFastCapture::~K4AFastCapture()
{

    if (m_device != NULL)
    {
        k4a_device_close(m_device);
    }
#ifndef _WIN32
    sem_close(m_captureRequestedSem);
    sem_close(m_captureDoneSem);
    sem_close(m_captureExitSem);
#endif
    std::cout << "[Streaming Service] Stopped." << std::endl;
}

bool K4AFastCapture::Configure(const char *fileDirectory, int32_t exposureValue, int pcmShiftValue)
{
    unsigned int deviceCount = k4a_device_get_installed_count();
    if (deviceCount == 0)
    {
        std::cout << "[Streaming Service] No K4A devices found" << std::endl;
        return false;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_device_open(K4A_DEVICE_DEFAULT, &m_device))
    {
        std::cout << "[Streaming Service] Failed to open device" << std::endl;
        return false;
    }

    m_deviceConfig.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
    m_deviceConfig.color_resolution = K4A_COLOR_RESOLUTION_3072P;
    m_deviceConfig.depth_mode = K4A_DEPTH_MODE_PASSIVE_IR;
    m_deviceConfig.camera_fps = K4A_FRAMES_PER_SECOND_15;
    m_deviceConfig.synchronized_images_only = true;

    if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(m_device, &m_deviceConfig))
    {
        std::cout << "[Streaming Service] Failed to start cameras" << std::endl;
        return false;
    }

    if (exposureValue != 0)
    {
        if (K4A_RESULT_SUCCEEDED != k4a_device_set_color_control(m_device,
                                                                 K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                                                 K4A_COLOR_CONTROL_MODE_MANUAL,
                                                                 exposureValue))
        {
            std::cout << "[Streaming Service] Failed to set the exposure" << std::endl;
            return false;
        }
    }
    else
    {
        if (K4A_RESULT_SUCCEEDED != k4a_device_set_color_control(m_device,
                                                                 K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE,
                                                                 K4A_COLOR_CONTROL_MODE_AUTO,
                                                                 0))
        {
            std::cout << "[Streaming Service] Failed to set auto exposure" << std::endl;
            return false;
        }
    }

    if (pcmShiftValue != -1)
    {
        m_pcmShiftValue = pcmShiftValue;
    }

    m_depthFileDirectory = fileDirectory;
    m_colorFileDirectory = fileDirectory;

    // create the directories for storing the captures
#ifdef _WIN32
    m_depthFileDirectory += "\\D0\\";
    m_colorFileDirectory += "\\PV0\\";
#else
    m_depthFileDirectory += "/D0/";
    m_colorFileDirectory += "/PV0/";
#endif

#if defined(_WIN32)
    if (0 == CreateDirectory(fileDirectory, NULL) && ERROR_PATH_NOT_FOUND == GetLastError())
    {
        std::cout << fileDirectory << " directory creation failed. Exiting..." << std::endl;
        return false;
    }

    if (0 == CreateDirectory(m_depthFileDirectory.c_str(), NULL) && ERROR_PATH_NOT_FOUND == GetLastError())
    {
        std::cout << m_depthFileDirectory << " Depth directory creation failed. Exiting..." << std::endl;
        return false;
    }
    if (0 == CreateDirectory(m_colorFileDirectory.c_str(), NULL) && ERROR_PATH_NOT_FOUND == GetLastError())
    {
        std::cout << m_colorFileDirectory << " Color directory creation failed. Exiting..." << std::endl;
        return false;
    }
#else
    mode_t nMode = 0733; // UNIX style permissions
    int nError = 0;
    nError = mkdir(m_depthFileDirectory.c_str(), nMode); // can be used on non-Windows
    if (nError != 0)
    {
        std::cout << m_depthFileDirectory << " Directory creation failed. Exiting..." << std::endl;
        return false;
    }

    nError = mkdir(m_colorFileDirectory.c_str(), nMode); // can be used on non-Windows
    if (nError != 0)
    {
        std::cout << m_colorFileDirectory << " Directory creation failed. Exiting..." << std::endl;
        return false;
    }
#endif
    try
    {
        m_pcmImg.resize(m_pcmOutputHeight * m_pcmOutputWidth);
    }
    catch (std::bad_alloc const &)
    {
        std::cout << "Memory allocation failed. " << std::endl;
        throw std::exception();
    }
    return true;
}

void K4AFastCapture::Run(int streamingLength)
{
    std::string colorFileName;
    std::string depthFileName;
    k4a_image_t ir_image = NULL;
    k4a_image_t depth_image = NULL;
    k4a_image_t color_image = NULL;

    uint32_t camera_fps = k4a_convert_fps_to_uint(m_deviceConfig.camera_fps);
    uint32_t remainingFrames = UINT32_MAX;
    if (streamingLength >= 0)
    {
        remainingFrames = (uint32_t)streamingLength * camera_fps;
    }

    // Wait for the first capture before starting streaming.
    if (k4a_device_get_capture(m_device, &m_capture, 60000) != K4A_WAIT_RESULT_SUCCEEDED)
    {
        std::cerr << "[Streaming Service] Runtime error: k4a_device_get_capture() failed" << std::endl;
        return;
    }
    k4a_capture_release(m_capture);

    int32_t timeout_ms = HZ_TO_PERIOD_MS(camera_fps);
    std::cout << "[Streaming Service] Streaming from sensors..." << std::endl;
    while (remainingFrames-- > 0 && m_streaming)
    {
        depthFileName = m_depthFileDirectory;
        depthFileName += std::to_string(m_frameRequestedNum);

        colorFileName = m_colorFileDirectory;
        colorFileName += std::to_string(m_frameRequestedNum);
        colorFileName += ".jpg";

        // Get the capture
        switch (k4a_device_get_capture(m_device, &m_capture, timeout_ms))
        {
        case K4A_WAIT_RESULT_SUCCEEDED:
            break;
        case K4A_WAIT_RESULT_TIMEOUT:
            // std::cout << "[Streaming Service] Timed out waiting for the capture" << std::endl;
            continue;
        case K4A_WAIT_RESULT_FAILED:
            std::cout << "[Streaming Service] Failed to get the capture" << std::endl;
            return;
        }

        // check the capture request signal from fastcapture_trigger app. Store the current capture to disk once the
        // signal is received.
#ifdef _WIN32
        auto result = WaitForSingleObject(m_captureRequestedEvent.Get(), 0);
        if (WAIT_OBJECT_0 == result)
#else
        auto result = sem_trywait(m_captureRequestedSem);
        if (0 == result)
#endif
        {
            if (m_deviceConfig.depth_mode == K4A_DEPTH_MODE_PASSIVE_IR)
            {
                // for the passive IR mode, there is no depth image. Only IR image is available in the capture.
                ir_image = k4a_capture_get_ir_image(m_capture);
                assert(ir_image != NULL); // Because m_deviceConfig.synchronized_images_only == true

                // Do work with the frames
                // write captures to disk
#ifdef _WIN32
                depthFileName += ".png";
                // On Windows, write the IR image to .png file.
                // SavePcmToImage function encodes the pcm frame into lossless PNG format, which depends on Windows
                // Imaging Component and it's only available on Windows.
                SavePcmToImage(depthFileName.c_str(),
                               m_pcmOutputHeight,
                               m_pcmOutputWidth,
                               k4a_image_get_buffer(ir_image),
                               k4a_image_get_size(ir_image));
#else
                // For other platforms, write IR image to .bin file.
                depthFileName += ".bin";
                WriteToFile(depthFileName.c_str(), k4a_image_get_buffer(ir_image), k4a_image_get_size(ir_image));
#endif
            }
            else
            {
                depth_image = k4a_capture_get_depth_image(m_capture);
                assert(depth_image != NULL); // Because m_deviceConfig.synchronized_images_only == true

                // write depth frame to .bin
                depthFileName += ".bin";
                WriteToFile(depthFileName.c_str(), k4a_image_get_buffer(depth_image), k4a_image_get_size(depth_image));
            }

            color_image = k4a_capture_get_color_image(m_capture);
            assert(color_image != NULL); // Because m_deviceConfig.synchronized_images_only == true

            WriteToFile(colorFileName.c_str(), k4a_image_get_buffer(color_image), k4a_image_get_size(color_image));

            m_frameRequestedNum++;

#ifdef _WIN32
            SetEvent(m_captureDoneEvent.Get());
            ResetEvent(m_captureRequestedEvent.Get());
#else
            sem_post(m_captureDoneSem);
#endif

            if (depth_image)
            {
                k4a_image_release(depth_image);
            }
            if (color_image)
            {
                k4a_image_release(color_image);
            }
        }
        // release frame
        k4a_capture_release(m_capture);

        // check if exit command is received from fastcapture_trigger app
#ifdef _WIN32
        result = WaitForSingleObject(m_captureExitEvent.Get(), 0);
        if (WAIT_OBJECT_0 == result)
#else
        result = sem_trywait(m_captureExitSem);
        if (0 == result)
#endif
        {
            break;
        }
    }
    std::cout << "[Streaming Service] Exiting as requested..." << std::endl;
    return;
}

void K4AFastCapture::Stop()
{
    m_streaming = false;
}

#ifdef _WIN32
void K4AFastCapture::SavePcmToImage(const char *fileName,
                                    unsigned int height,
                                    unsigned int width,
                                    uint8_t *data,
                                    size_t dataSize)
{
    assert(height > 0);
    assert(width > 0);
    assert(dataSize > 0);
    assert(data != NULL);

    VERIFY_HR(CoInitialize(nullptr));
    Microsoft::WRL::ComPtr<IWICImagingFactory> imageFactory;
    VERIFY_HR(CoCreateInstance(CLSID_WICImagingFactory,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               __uuidof(IWICImagingFactory),
                               reinterpret_cast<void **>(imageFactory.GetAddressOf())));

    Microsoft::WRL::ComPtr<IWICStream> stream;
    Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder;
    Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frame;
    VERIFY_HR(imageFactory->CreateStream(&stream));
    wchar_t wString[4096];
    MultiByteToWideChar(CP_ACP, 0, fileName, -1, wString, 4096);
    VERIFY_HR(stream->InitializeFromFilename(wString, GENERIC_WRITE));
    VERIFY_HR(imageFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.GetAddressOf()));
    VERIFY_HR(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache));

    Microsoft::WRL::ComPtr<IPropertyBag2> props;
    VERIFY_HR(encoder->CreateNewFrame(frame.GetAddressOf(), props.GetAddressOf()));
    VERIFY_HR(frame->Initialize(props.Get()));

    GUID pixelFormat = GUID_WICPixelFormat8bppGray;
    VERIFY_HR(frame->SetSize(width, height));
    VERIFY_HR(frame->SetPixelFormat(&pixelFormat));

    // convert the 16 bit pixel to 8 bit format
    UINT16 *pcm = reinterpret_cast<UINT16 *>(data);
    dataSize = dataSize >> 1;
    for (unsigned int i = 0; i < height * width && i < dataSize; i++)
    {
        if (pcm[i] < 0)
        {
            pcm[i] = 0;
        }
        m_pcmImg[i] = static_cast<char>(min(pcm[i] >> m_pcmShiftValue, 0xFF));
    }

    VERIFY_HR(frame->WritePixels(height, width * sizeof(char), height * width * sizeof(char), (BYTE *)m_pcmImg.data()));
    VERIFY_HR(frame->Commit());
    VERIFY_HR(encoder->Commit());
    CoUninitialize();
    std::cout << "[Streaming Service] Depth frame is stored in " << fileName << std::endl;
}
#endif

long K4AFastCapture::WriteToFile(const char *fileName, void *buffer, size_t bufferSize)
{
    assert(buffer != NULL);

    std::ofstream hFile;
    hFile.open(fileName, std::ios::out | std::ios::trunc | std::ios::binary);
    if (hFile.is_open())
    {
        hFile.write((char *)buffer, static_cast<std::streamsize>(bufferSize));
        hFile.close();
    }
    std::cout << "[Streaming Service] Color frame is stored in " << fileName << std::endl;

    return 0;
}