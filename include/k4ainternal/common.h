/** \file COMMON.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef COMMON_H
#define COMMON_H

#include <k4a/k4atypes.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _guid_t
{
    uint8_t id[16];
} guid_t;

#define K4A_IMU_SAMPLE_RATE 1666 // +/- 2%

#define MAX_FPS_IN_MS (33) // 30 FPS

#define COUNTOF(x) (sizeof(x) / sizeof(x[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define STRINGIFY(string) #string

// Clock tick runs 90kHz and convert sec to micro sec
#define K4A_90K_HZ_TICK_TO_USEC(x) ((uint64_t)(x)*100 / 9)
#define K4A_USEC_TO_90K_HZ_TICK(x) ((x)*9 / 100)

#define MAX_SERIAL_NUMBER_LENGTH                                                                                       \
    (13 * 2) // Current schema is for 12 digits plus NULL, the extra size is in case that grows in the future.

#define HZ_TO_PERIOD_MS(Hz) (1000 / Hz)
#define HZ_TO_PERIOD_US(Hz) (1000000 / Hz)
#define HZ_TO_PERIOD_NS(Hz) (1000000000 / Hz)

/* Copy struct to a result pointer, not exceeding the size specified in the result pointer
Assumes size comes before version */
#define SAFE_COPY_STRUCT(result, temp)                                                                                 \
    {                                                                                                                  \
        size_t offset = ((char *)(&(temp)->struct_version) - (char *)(temp)) + sizeof((temp)->struct_version);         \
        size_t size = sizeof(*temp);                                                                                   \
        if (size > (result)->struct_size)                                                                              \
            size = (result)->struct_size;                                                                              \
        if (size > offset)                                                                                             \
        {                                                                                                              \
            memcpy((char *)result + offset, (char *)(temp) + offset, size - offset);                                   \
        }                                                                                                              \
    }

/* Macro for checking if a struct member is within the limits set by size */
#define HAS_MEMBER(S, M) (((char *)(&((S)->M)) - (char *)(S)) + sizeof((S)->M) <= (S)->struct_size)

/* Safe setting a value in a struct */
#define SAFE_SET_MEMBER(S, M, NEW_VALUE)                                                                               \
    if (HAS_MEMBER(S, M))                                                                                              \
    (S)->M = (NEW_VALUE)

/* Safe getting a value from a struct */
#define SAFE_GET_MEMBER(S, M, DEFAULT_VALUE) (HAS_MEMBER(S, M) ? ((S)->M) : (DEFAULT_VALUE))

inline static bool k4a_is_version_greater_or_equal(k4a_version_t *fw_version_l, k4a_version_t *fw_version_r)
{
    typedef enum
    {
        FW_OK,
        FW_TOO_LOW,
        FW_UNKNOWN
    } fw_check_state_t;

    fw_check_state_t fw = FW_UNKNOWN;

    // Check major version
    if (fw_version_l->major > fw_version_r->major)
    {
        fw = FW_OK;
    }
    else if (fw_version_l->major < fw_version_r->major)
    {
        fw = FW_TOO_LOW;
    }

    // Check minor version
    if (fw == FW_UNKNOWN)
    {
        if (fw_version_l->minor > fw_version_r->minor)
        {
            fw = FW_OK;
        }
        else if (fw_version_l->minor < fw_version_r->minor)
        {
            fw = FW_TOO_LOW;
        }
    }

    // Check iteration version
    if (fw == FW_UNKNOWN)
    {
        fw = FW_TOO_LOW;
        if (fw_version_l->iteration >= fw_version_r->iteration)
        {
            fw = FW_OK;
        }
    }

    return (fw == FW_OK);
}

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
