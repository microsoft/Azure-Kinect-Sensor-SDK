// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aimusamplesource.h"

// System headers
//

// Library headers
//

// Project headers
//

using namespace k4aviewer;

void K4AImuSampleSource::NotifyData(const k4a_imu_sample_t &data)
{
    if (!m_sampleBuffer.BeginInsert())
    {
        // Buffer overflowed; drop the sample.
        //
        return;
    }

    *m_sampleBuffer.InsertionItem() = data;

    m_sampleBuffer.EndInsert();
}

void K4AImuSampleSource::NotifyTermination()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_failed = true;
}

bool K4AImuSampleSource::PopSample(k4a_imu_sample_t &out)
{
    if (!m_sampleBuffer.AdvanceRead())
    {
        return false;
    }

    out = *m_sampleBuffer.CurrentItem();
    return true;
}
