// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AFASTCAPTURE_H
#define K4AFASTCAPTURE_H

#include <vector>
#include <k4a/k4a.h>
#include <math.h>
#include <string.h>
#include "assert.h"

#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <wincodec.h>
#include <wrl/event.h>
#else
#include <fcntl.h> /* For O_* constants */
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#endif

namespace k4afastcapture
{
class K4AFastCapture
{
public:
    K4AFastCapture();
    ~K4AFastCapture();
    bool Configure(const char *filePathPrefix, int32_t exposureValue, int pcmShiftValue);
    void Run(int streamingLength);
    void Stop();

private:
    long WriteToFile(const char *fileName, void *buffer, size_t bufferSize);

    // SavePcmToImage function encodes the pcm frame into lossless PNG format, which depends on Windows Imaging
    // Component. It's only available on Windows.
    void SavePcmToImage(const char *fileName, unsigned int width, unsigned int height, uint8_t *data, size_t dataSize);

    std::string m_colorFileDirectory;
    std::string m_depthFileDirectory;
    int m_frameRequestedNum;
    int m_pcmShiftValue; // when converting the 16-bit pixel value to 8-bit format, the pixel value is shifted by
                         // m_pcmShiftValue, which is normally set as 4 or 5. The pcm frame would become darker if the
                         // shift value is higher.
    unsigned int m_pcmOutputHeight;
    unsigned int m_pcmOutputWidth;
    bool m_streaming;

    k4a_device_t m_device;
    k4a_capture_t m_capture;
    k4a_device_configuration_t m_deviceConfig;
    std::vector<char> m_pcmImg;

#ifdef _WIN32
    Microsoft::WRL::Wrappers::Event m_captureRequestedEvent;
    Microsoft::WRL::Wrappers::Event m_captureDoneEvent;
    Microsoft::WRL::Wrappers::Event m_captureExitEvent;
#else
    sem_t *m_captureRequestedSem;
    sem_t *m_captureDoneSem;
    sem_t *m_captureExitSem;
#endif
};
} // namespace k4afastcapture

#endif
