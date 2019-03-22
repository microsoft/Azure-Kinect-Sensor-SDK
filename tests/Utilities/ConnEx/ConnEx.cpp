// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ConnEx.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>

#include <k4ainternal/logging.h>

#define CONNEX_CMD_PORT "port"
#define CONNEX_CMD_VOLTS "volts"
#define CONNEX_CMD_AMPS "amps"
#define CONNEX_CMD_VERSION "version"

// This is the version expected to come back from the connection exerciser.
// This consists of the hardcoded HMD Validation Kit version and the type of shield.
#define CONN_EX_VERSION "0108"

typedef struct connection_exerciser_internal_t
{
    HANDLE serialHandle;
} connection_exerciser_internal_t;

static HANDLE OpenComPort(LPCSTR comPort)
{
    // Open serial port
    HANDLE serialHandle;

    serialHandle = CreateFile(comPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (serialHandle == INVALID_HANDLE_VALUE || serialHandle == NULL)
    {
        DWORD lastError = GetLastError();

        if (lastError != ERROR_FILE_NOT_FOUND)
        {
            LOG_ERROR("Failed to open %s. Last Error=%d", comPort, lastError);
        }
        return INVALID_HANDLE_VALUE;
    }

    // Do some basic settings
    DCB serialParams = { 0 };
    serialParams.DCBlength = sizeof(serialParams);

    if (!GetCommState(serialHandle, &serialParams))
    {
        LOG_ERROR("GetCommState Failed. Last Error=%d", comPort, GetLastError());
        CloseHandle(serialHandle);
        return INVALID_HANDLE_VALUE;
    }

    serialParams.BaudRate = CBR_9600;
    serialParams.ByteSize = 8;
    serialParams.StopBits = ONESTOPBIT;
    serialParams.Parity = NOPARITY;
    serialParams.fDtrControl = DTR_CONTROL_DISABLE;
    serialParams.fRtsControl = RTS_CONTROL_DISABLE;
    if (!SetCommState(serialHandle, &serialParams))
    {
        LOG_ERROR("SetCommState Failed. Last Error=%d", comPort, GetLastError());
        CloseHandle(serialHandle);
        return INVALID_HANDLE_VALUE;
    }

    // Set timeouts
    COMMTIMEOUTS timeout = { 0 };
    if (!GetCommTimeouts(serialHandle, &timeout))
    {
        LOG_ERROR("GetCommTimeouts Failed. Last Error=%d", comPort, GetLastError());
        CloseHandle(serialHandle);
        return INVALID_HANDLE_VALUE;
    }

    timeout.ReadTotalTimeoutConstant = 500;
    if (!SetCommTimeouts(serialHandle, &timeout))
    {
        LOG_ERROR("SetCommTimeouts Failed. Last Error=%d", comPort, GetLastError());
        CloseHandle(serialHandle);
        return INVALID_HANDLE_VALUE;
    }

    return serialHandle;
}

connection_exerciser::connection_exerciser()
{
    state = new (std::nothrow) connection_exerciser_internal_t();
}

connection_exerciser::~connection_exerciser()
{
    if (state->serialHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(state->serialHandle);
    }
    delete state;
}

k4a_result_t connection_exerciser::set_usb_port(int port)
{
    char usbPort[16] = { 0 };
    if (sprintf_s(usbPort, "%d", port) == -1)
    {
        LOG_ERROR("Failed to generate the USB port command.");
        return K4A_RESULT_FAILED;
    }

    return TRACE_CALL(send_command(CONNEX_CMD_PORT, usbPort));
}

int connection_exerciser::get_usb_port()
{
    char buffer[16] = { 0 };
    if (K4A_FAILED(TRACE_CALL(send_command(CONNEX_CMD_PORT, nullptr, buffer))))
    {
        return -1;
    }

    int rawValue;
    sscanf_s(buffer, "%d", &rawValue);
    return rawValue;
}

float connection_exerciser::get_voltage_reading()
{
    char buffer[16] = { 0 };
    if (K4A_FAILED(TRACE_CALL(send_command(CONNEX_CMD_VOLTS, nullptr, buffer))))
    {
        return -1;
    }

    int rawValue;
    if (sscanf_s(buffer, "%d", &rawValue) == -1)
    {
        LOG_ERROR("Failed to parse response.");
        return -1;
    }
    return (float)(rawValue / 100.0);
}

float connection_exerciser::get_current_reading()
{
    char buffer[16] = { 0 };
    if (K4A_FAILED(TRACE_CALL(send_command(CONNEX_CMD_AMPS, nullptr, buffer))))
    {
        return -1;
    }

    int rawValue;
    if (sscanf_s(buffer, "%d", &rawValue) == -1)
    {
        LOG_ERROR("Failed to parse response.");
        return -1;
    }

    // The "1000" digit appears to be a sign digit. A negative number would be 1000 + the value. With this scheme
    // "negative zero" is possible and needs to be converted to 0.
    if (rawValue >= 1000)
    {
        rawValue = 1000 - rawValue;
    }

    return (float)(rawValue / 100.0);
}

template<size_t size>
inline k4a_result_t connection_exerciser::send_command(const char *command,
                                                       const char *parameter,
                                                       char (&response)[size])
{
    return send_command(command, parameter, response, size);
}

inline k4a_result_t connection_exerciser::send_command(const char *command, const char *parameter)
{
    return send_command(command, parameter, nullptr, 0);
}

k4a_result_t connection_exerciser::send_command(const char *command, const char *parameter, char *response, size_t size)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, command == NULL);

    DWORD command_length = 0;
    DWORD bytes_transfered = 0;
    char buffer[1024] = { 0 };

    if (parameter != nullptr)
    {
        if ((command_length = sprintf_s(buffer, "%s %s\r\n", command, parameter)) == -1)
        {
            LOG_ERROR("Failed to generate the command.");
            return K4A_RESULT_FAILED;
        }
    }
    else
    {
        if ((command_length = sprintf_s(buffer, "%s\r\n", command)) == -1)
        {
            LOG_ERROR("Failed to generate the command.");
            return K4A_RESULT_FAILED;
        }
    }

    if (WriteFile(state->serialHandle, buffer, command_length, nullptr, nullptr) == 0)
    {
        LOG_ERROR("WriteFile failed with %d", GetLastError());
        return K4A_RESULT_FAILED;
    }
    buffer[0] = '\0';

    if (ReadFile(state->serialHandle, buffer, sizeof(buffer), &bytes_transfered, nullptr) == 0)
    {
        LOG_ERROR("ReadFile failed with %d", GetLastError());
        return K4A_RESULT_FAILED;
    }

    buffer[bytes_transfered] = '\0';

    if (response != nullptr)
    {
        strcpy_s(response, size, buffer);
        for (size_t i = strlen(response) - 1; i >= 0; --i)
        {
            if (response[i] == '\n' || response[i] == '\r')
            {
                response[i] = '\0';
            }
            else
            {
                break;
            }
        }
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t connection_exerciser::find_connection_exerciser()
{
    LOG_INFO("Searching for a connection exerciser...", 0);

    char comPort[16] = { 0 };
    char buffer[1024] = { 0 };

    if (state->serialHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(state->serialHandle);
        state->serialHandle = INVALID_HANDLE_VALUE;
    }

    for (int i = 1; i < 10; ++i)
    {
        sprintf_s(comPort, "COM%d", i);
        LOG_INFO("Opening %s", comPort);
        state->serialHandle = OpenComPort(comPort);

        if (state->serialHandle != INVALID_HANDLE_VALUE)
        {
            if (K4A_SUCCEEDED(TRACE_CALL(send_command(CONNEX_CMD_VERSION, nullptr, buffer))) &&
                strcmp(buffer, CONN_EX_VERSION) == 0)
            {
                return K4A_RESULT_SUCCEEDED;
            }
            else
            {
                CloseHandle(state->serialHandle);
                state->serialHandle = INVALID_HANDLE_VALUE;
            }
        }
    }

    return K4A_RESULT_FAILED;
}
