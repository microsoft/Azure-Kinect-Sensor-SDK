/** \file matroska_common.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure Record SDK.
 */

#ifndef RECORD_COMMON_H
#define RECORD_COMMON_H

#include <k4arecord/types.h>
#include <k4ainternal/handle.h>
#include <list>
#include <fstream>
#include <memory>
#include <thread>

#if defined(__clang__)

#pragma clang diagnostic push
// -Wheader-hygiene: using namespace directive in global context in header
// -Wconversion: implicit conversion loses integer precision: 'unsigned long' to 'uint8'
// -Wsign-conversion: implicit conversion changes signedness: 'int' to 'unsigned long'
// -Wfloat-equal: comparing floating point with == or != is unsafe
// -Wcast-qual: cast from 'const unsigned char *' to 'unsigned char *' drops const qualifier
#pragma clang diagnostic ignored "-Wheader-hygiene"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wcast-qual"

#elif defined(_MSC_VER)

#pragma warning(push)
// EBML header warnings
// 4267: 'return': conversion from 'size_t' to 'uint8', possible loss of data
// 4138: '*/' found outside of comment
// 4245: 'return': conversion from 'int' to 'uint64', signed/unsigned mismatch
#pragma warning(disable : 4267 4138 4245)

#endif

#include "ebml/EbmlHead.h"
#include "ebml/EbmlStream.h"
#include "ebml/EbmlSubHead.h"
#include "ebml/EbmlVoid.h"
#include "matroska/KaxSegment.h"
#include "matroska/KaxCluster.h"
#include "matroska/KaxSeekHead.h"
#include "matroska/KaxCuesData.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

// Helpers to easily specify time in nanoseconds
constexpr uint64_t operator"" _us(unsigned long long x)
{
    return x * 1000;
}
constexpr uint64_t operator"" _ms(unsigned long long x)
{
    return x * 1000000;
}
constexpr uint64_t operator"" _s(unsigned long long x)
{
    return x * 1000000000;
}

#ifndef MAX_CLUSTER_LENGTH_NS
#define MAX_CLUSTER_LENGTH_NS 32_ms
#endif

#ifndef MATROSKA_TIMESCALE_NS
// 1 unit = 1us
#define MATROSKA_TIMESCALE_NS 1_us
#endif

#ifndef CLUSTER_WRITE_DELAY_NS
#define CLUSTER_WRITE_DELAY_NS 2_s
#endif

#ifndef CLUSTER_WRITE_QUEUE_WARNING_NS
// If a cluster is in the qeuue too long, warn about disk write speed.
#define CLUSTER_WRITE_QUEUE_WARNING_NS CLUSTER_WRITE_DELAY_NS + 2_s
#endif

#ifndef CUE_ENTRY_GAP_NS
#define CUE_ENTRY_GAP_NS 1_s
#endif

#ifndef CLUSTER_READ_AHEAD_COUNT
#define CLUSTER_READ_AHEAD_COUNT 2
#endif

static_assert(MAX_CLUSTER_LENGTH_NS < INT16_MAX * MATROSKA_TIMESCALE_NS, "Cluster length must fit in a 16 bit int");
static_assert(CLUSTER_WRITE_DELAY_NS >= MAX_CLUSTER_LENGTH_NS * 2, "Cluster write delay is shorter than 2 clusters");

#define RETURN_IF_ERROR(_call_)                                                                                        \
    {                                                                                                                  \
        k4a_result_t retval = TRACE_CALL(_call_);                                                                      \
        if (K4A_FAILED(retval))                                                                                        \
        {                                                                                                              \
            return retval;                                                                                             \
        }                                                                                                              \
    }

// make_unique is C++14 only
template<typename T, typename... Args> std::unique_ptr<T> make_unique(Args &&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// C++11 Cross-platform implementation of countof()
template<typename T, size_t N> constexpr size_t arraysize(T const (&)[N]) noexcept
{
    return N;
}

// Function for switching between big and little-endian 16-bit ints
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif
constexpr uint16_t swap_bytes_16(uint16_t input)
{
#if __has_builtin(__builtin_bswap16)
    return __builtin_bswap16(input);
#else
    return input >> 8 | input << 8;
#endif
}

namespace k4arecord
{
/**
 * EBML IO handler compatible with file sizes larger than 32-bit
 */
class LargeFileIOCallback : public libebml::IOCallback
{
public:
    LargeFileIOCallback(const char *path, const open_mode mode);
    ~LargeFileIOCallback() override;

    uint32 read(void *buffer, size_t size) override;
    void setFilePointer(int64 offset, libebml::seek_mode mode = libebml::seek_beginning) override;
    size_t write(const void *buffer, size_t size) override;
    uint64 getFilePointer() override;
    void close() override;
    void setOwnerThread();

private:
    std::fstream m_stream;
    std::thread::id m_owner;
};

// Struct matches https://docs.microsoft.com/en-us/windows/desktop/wmdm/-bitmapinfoheader
struct BITMAPINFOHEADER
{
    uint32_t biSize = sizeof(BITMAPINFOHEADER);
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes = 1;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPelsPerMeter = 0;
    uint32_t biYPelsPerMeter = 0;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
};

#pragma pack(push, 1)
// Used to serialize imu samples to disk. The struct padding and size must be exact.
struct matroska_imu_sample_t
{
    uint64_t acc_timestamp_ns;
    float acc_data[3];
    uint64_t gyro_timestamp_ns;
    float gyro_data[3];
};
#pragma pack(pop)

// Sample is stored as [acc_timestamp, acc_data[3], gyro_timestamp, gyro_data[3]]
static_assert(sizeof(matroska_imu_sample_t) == (sizeof(float) * 6 + sizeof(uint64_t) * 2),
              "matroska_imu_sample_t size does not match expected padding.");

// If the size of an IMU sample changes in the SDK, the file format will need to be updated as well.
static_assert(sizeof(matroska_imu_sample_t) ==
                  sizeof(k4a_imu_sample_t().acc_sample.v) + sizeof(k4a_imu_sample_t().acc_timestamp_usec) +
                      sizeof(k4a_imu_sample_t().gyro_timestamp_usec) + sizeof(k4a_imu_sample_t().gyro_sample.v),
              "Size of IMU data structure has changed from on-disk format.");

static const k4a_color_resolution_t color_resolutions[] = { K4A_COLOR_RESOLUTION_720P,  K4A_COLOR_RESOLUTION_1080P,
                                                            K4A_COLOR_RESOLUTION_1440P, K4A_COLOR_RESOLUTION_1536P,
                                                            K4A_COLOR_RESOLUTION_2160P, K4A_COLOR_RESOLUTION_3072P };

static const std::pair<k4a_depth_mode_t, std::string> depth_modes[] =
    { { K4A_DEPTH_MODE_NFOV_2X2BINNED, "NFOV_2X2BINNED" },
      { K4A_DEPTH_MODE_NFOV_UNBINNED, "NFOV_UNBINNED" },
      { K4A_DEPTH_MODE_WFOV_2X2BINNED, "WFOV_2X2BINNED" },
      { K4A_DEPTH_MODE_WFOV_UNBINNED, "WFOV_UNBINNED" },
      { K4A_DEPTH_MODE_PASSIVE_IR, "PASSIVE_IR" } };

static const std::pair<k4a_wired_sync_mode_t, std::string> external_sync_modes[] =
    { { K4A_WIRED_SYNC_MODE_STANDALONE, "STANDALONE" },
      { K4A_WIRED_SYNC_MODE_MASTER, "MASTER" },
      { K4A_WIRED_SYNC_MODE_SUBORDINATE, "SUBORDINATE" } };

} // namespace k4arecord

#endif /* RECORD_COMMON_H */
