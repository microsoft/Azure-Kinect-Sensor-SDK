/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

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
    m_lastSample = data;
}

void K4AImuSampleSource::NotifyTermination()
{
    m_failed = true;
}
