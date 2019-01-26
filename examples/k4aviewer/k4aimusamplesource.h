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

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//
#include <ik4aobserver.h>

namespace k4aviewer
{
class K4AImuSampleSource : public IK4AImuObserver
{
public:
    void NotifyData(const k4a_imu_sample_t &data) override;
    void NotifyTermination() override;

    const k4a_imu_sample_t &GetLastSample() const
    {
        return m_lastSample;
    }

    bool IsFailed() const
    {
        return m_failed;
    }

    ~K4AImuSampleSource() override = default;

private:
    k4a_imu_sample_t m_lastSample{};
    bool m_failed = false;
};
} // namespace k4aviewer

#endif
