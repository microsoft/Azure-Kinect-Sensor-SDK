/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4AIMUSAMPLESOURCE_H
#define K4AIMUSAMPLESOURCE_H

// System headers
//
#include <mutex>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//
#include "ik4aobserver.h"
#include "k4aringbuffer.h"

namespace k4aviewer
{
class K4AImuSampleSource : public IK4AImuObserver
{
public:
    void NotifyData(const k4a_imu_sample_t &data) override;
    void NotifyTermination() override;

    bool PopSample(k4a_imu_sample_t &out);

    bool IsFailed() const
    {
        return m_failed;
    }

    ~K4AImuSampleSource() override = default;

private:
    K4ARingBuffer<k4a_imu_sample_t, 1000> m_sampleBuffer;
    bool m_failed = false;

    mutable std::mutex m_mutex;
};
} // namespace k4aviewer

#endif
