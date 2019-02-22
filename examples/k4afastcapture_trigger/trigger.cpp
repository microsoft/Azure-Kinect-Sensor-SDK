// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <synchapi.h>
#include <wrl/event.h>
#else
#include <fcntl.h> /* For O_* constants */
#include <sys/stat.h>
#include <semaphore.h>
#endif

int main(int argc, char *argv[])
{

#ifdef _WIN32
    // notify the streaming process to exit
    if (argc > 1 && strcmp(argv[1], "exit") == 0)
    {

        HANDLE captureExitEvent = OpenEventW(EVENT_ALL_ACCESS,             // request full access
                                             FALSE,                        // handle not inheritable
                                             L"Global\\captureExitEvent"); // object name
        if (captureExitEvent == NULL)
        {
            std::cout << "No streaming service running." << std::endl;
            return ESRCH;
        }
        else
        {
            SetEvent(captureExitEvent);
            std::cout << "Exit command is issued to streaming service." << std::endl;
            CloseHandle(captureExitEvent);
            return 0;
        }
    }

    HANDLE captureRequestedEvent = NULL;
    HANDLE captureDoneEvent = NULL;

    captureRequestedEvent = OpenEventW(EVENT_ALL_ACCESS,                  // request full access
                                       FALSE,                             // handle not inheritable
                                       L"Global\\captureRequestedEvent"); // object name

    captureDoneEvent = OpenEventW(EVENT_ALL_ACCESS,             // request full access
                                  FALSE,                        // handle not inheritable
                                  L"Global\\captureDoneEvent"); // object name

    if (captureRequestedEvent == NULL || captureDoneEvent == NULL)
    {
        // if the either of the handles cannot be accessed, the streaming service is unlikely running, so exit with 1.
        std::cout << "No streaming service is running." << std::endl;
        return ESRCH;
    }
    else
    {
        // send the capture request to fastcapture_streaming service
        ResetEvent(captureDoneEvent);
        SetEvent(captureRequestedEvent);

        // If the reuqested capture is done in fastcapture_streaming service, the captureDoneEvent should be set and the
        // fastcapture_trigger can exit as expected.
        auto result = WaitForSingleObject(captureDoneEvent, 1000);
        if (WAIT_OBJECT_0 == result)
        {
            CloseHandle(captureRequestedEvent);
            CloseHandle(captureDoneEvent);
            return 0;
        }
        else if (WAIT_TIMEOUT == result)
        {
            std::cout
                << "The capture takes longer than expected. Please check if the streaming service is still running."
                << std::endl;
            return ETIME;
        }
        else
        {
            std::cout << "Something is wrong with the streaming service." << std::endl;
            return EBUSY;
        }
    }

#else
    if (argc > 1 && strcmp(argv[1], "exit") == 0)
    {
        sem_t *captureExitSem = sem_open("/globalCaptureExitSem", 0);
        if (captureExitSem == NULL)
        {
            std::cout << "No streaming service running." << std::endl;
            return ESRCH;
        }
        else
        {
            sem_post(captureExitSem);
            std::cout << "Exit command is issued to streaming service." << std::endl;
            return 0;
        }
    }
    sem_t *captureRequestedSem = sem_open("/globalCaptureRequestedSem", 0);
    sem_t *captureDoneSem = sem_open("/globalCaptureDoneSem", 0);
    if (captureRequestedSem == SEM_FAILED || captureDoneSem == SEM_FAILED)
    {
        std::cout << "No streaming service running." << std::endl;
        return ESRCH;
    }
    sem_trywait(captureDoneSem);
    auto result = sem_trywait(captureRequestedSem);
    if (0 == result)
    {
        std::cout << "Something is wrong with the streaming service." << std::endl;
        return EBUSY;
    }
    else
    {
        sem_post(captureRequestedSem);
        sem_wait(captureDoneSem);
        return 0;
    }
#endif
}