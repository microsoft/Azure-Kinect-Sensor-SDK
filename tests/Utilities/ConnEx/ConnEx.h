// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef CONNECTION_EXERCISER_H
#define CONNECTION_EXERCISER_H

#include <k4a/k4atypes.h>

// The connection exerciser has 4 active ports plus a everything disconnected "port".
// Port 0 is the disconnected port. Ports 1-4 are the active ports.
#define CONN_EX_MAX_NUM_PORTS 5

typedef struct connection_exerciser_internal_t *pconnection_exerciser_internal_t;

class connection_exerciser
{
public:
    connection_exerciser();
    ~connection_exerciser();

    k4a_result_t find_connection_exerciser();

    k4a_result_t set_usb_port(int port);

    // Returns -1 on failure.
    int get_usb_port();

    // Returns -1 on failure.
    float get_voltage_reading();

    // Returns -1 on failure.
    float get_current_reading();

private:
    k4a_result_t send_command(const char *command, const char *parameter);
    k4a_result_t send_command(const char *command, const char *parameter, char *response, size_t size);
    template<size_t size> k4a_result_t send_command(const char *command, const char *parameter, char (&response)[size]);

    pconnection_exerciser_internal_t state;
};

#endif /* CONNECTION_EXERCISER_H */
