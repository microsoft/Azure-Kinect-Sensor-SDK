// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "k4afastcapture.h"
static k4afastcapture::K4AFastCapture Capturer;

#ifdef _WIN32
BOOL CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
        // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
        Capturer.Stop();
        return TRUE;

    default:
        return FALSE;
    }
}
#else
#include <signal.h>
void IntHandler(int);

void IntHandler(int)
{
    Capturer.Stop();
}
void PrintBasicUsage();
#endif

static int string_compare(const char *s1, const char *s2)
{
    assert(s1 != NULL);
    assert(s2 != NULL);

    while (tolower((unsigned char)*s1) == tolower((unsigned char)*s2))
    {
        if (*s1 == '\0')
        {
            return 0;
        }
        s1++;
        s2++;
    }
    // The return value shows the relations between s1 and s2.
    // Return value   Description
    //     < 0        s1 less than s2
    //       0        s1 identical to s2
    //     > 0        s1 greater than s2
    return (int)tolower((unsigned char)*s1) - (int)tolower((unsigned char)*s2);
}

void PrintBasicUsage()
{
    std::cout << "* fastcapture_streaming.exe Usage Info *\n" << std::endl;
    std::cout << "       fastcapture_streaming.exe \n"
                 "             [DirectoryPath_Options] [PcmShift_Options (default: 4)]\n"
                 "             [StreamingLength_Options (Limit the streaming to N seconds, default: 60)] \n"
                 "             [ExposureValue_Options (default: auto exposure)] \n"
              << std::endl;

    std::cout << "Examples:" << std::endl;
    std::cout << "       1 - fastcapture_streaming.exe -DirectoryPath C:\\data\\ \n" << std ::endl;
    std::cout << "       2 - fastcapture_streaming.exe -DirectoryPath C:\\data\\ -PcmShift 5 -StreamingLength 1000 "
                 "-ExposureValue -3 \n"
              << std::endl;
    std::cout << "       3 - fastcapture_streaming.exe -d C:\\data\\ -s 4 -l 60 -e -2 \n" << std::endl;
    return;
}
int main(int argc, char *argv[])
{

    // default values
    const char *fileDirectory = ".";
    int streamingLength = 60; // the length of time for streaming from the sensors until automatically exit.
    int shiftValue = 4;
    int absoluteExposureValue = 0;

    if (argc == 1)
    {
        std::cout << " ** fastcapture_streaming.exe -help for usage information" << std::endl;
        std::cout << " ** -----------------------------------------------------" << std::endl;
    }

    for (int i = 0; i < argc; i++)
    {
        if ((0 == string_compare("-PrintUsage", argv[i])) || (0 == string_compare("/PrintUsage", argv[i])) ||
            (0 == string_compare("-help", argv[i])) || (0 == string_compare("/help", argv[i])) ||
            (0 == string_compare("/?", argv[i])) || (0 == string_compare("/h", argv[i])) ||
            (0 == string_compare("-h", argv[i])))
        {
            PrintBasicUsage();
            return 0;
        }
        else if ((0 == string_compare("-DirectoryPath", argv[i])) || (0 == string_compare("-Directory", argv[i])) ||
                 (0 == string_compare("-d", argv[i])) || (0 == string_compare("/d", argv[i])))
        {
            i++;
            if (i >= argc)
            {
                return EINVAL;
            }
            fileDirectory = argv[i];
        }

        else if ((0 == string_compare("-StreamingLength", argv[i])) || (0 == string_compare("-Length", argv[i])) ||
                 (0 == string_compare("-l", argv[i])) || (0 == string_compare("/l", argv[i])))
        {
            i++;
            if (i >= argc)
            {
                return EINVAL;
            }
            streamingLength = atoi(argv[i]);
            if (streamingLength < 0)
            {
                std::wcout << "Recording length should be positive; Using 60s" << std::endl;
                streamingLength = 60;
            }
        }
        else if ((0 == string_compare("-ExposureValue", argv[i])) || (0 == string_compare("-Exposure", argv[i])) ||
                 (0 == string_compare("-e", argv[i])) || (0 == string_compare("/e", argv[i])))
        {
            i++;
            if (i >= argc)
            {
                return EINVAL;
            }
            int exposureValue = atoi(argv[i]);
            if (exposureValue < 2 && exposureValue > -12)
            {
                std::wcout << exposureValue << "  <exposure value>" << std::endl;
                absoluteExposureValue = (int32_t)(exp2f((float)exposureValue) * 1000000.0f);
            }
            else
            {
                std::wcout << " !! incorrect exposure value provided [ exposure value range -11 to 1].... Using Auto "
                              "exposure...."
                           << std::endl;
            }
        }
        else if ((0 == string_compare("-PcmShift", argv[i])) || (0 == string_compare("-Shift", argv[i])) ||
                 (0 == string_compare("-s", argv[i])) || (0 == string_compare("/s", argv[i])))
        {
            i++;
            if (i >= argc)
            {
                return EINVAL;
            }
            shiftValue = atoi(argv[i]);
            if (shiftValue < 9 && shiftValue > -1)
            {
                std::wcout << shiftValue << "  <pcm shift value>" << std::endl;
            }
            else
            {
                shiftValue = 4;
                std::wcout << " !! incorrect pcm shift value provided [ range 0 to 8].... Using 4 as pcm shift value"
                           << std::endl;
            }
        }
    }

    // Handle the CTRL-C signal.
#ifdef _WIN32
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
#else
    struct sigaction saHandle;
    sigemptyset(&saHandle.sa_mask);
    saHandle.sa_flags = 0;
    saHandle.sa_handler = IntHandler;
    sigaction(SIGINT, &saHandle, NULL);
#endif

    std::cout << fileDirectory << "  <Directory>" << std::endl;
    if (streamingLength > 0)
    {
        std::cout << streamingLength << "  <Streaming Length in seconds>" << std::endl;
    }

    if (Capturer.Configure(fileDirectory, absoluteExposureValue, shiftValue))
    {
        Capturer.Run(streamingLength);
    }
    else
        std::cout << "Configuration Failed." << std::endl;

    return 0;
}