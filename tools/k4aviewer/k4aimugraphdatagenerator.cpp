// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aimugraphdatagenerator.h"

// System headers
//

// Library headers
//

// Project headers
//

using namespace k4aviewer;

void K4AImuGraphDataGenerator::NotifyData(const k4a_imu_sample_t &sample)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (int i = 0; i < 3; ++i)
    {
        m_accAccumulator.v[i] += sample.acc_sample.v[i];
        m_gyroAccumulator.v[i] += sample.gyro_sample.v[i];
    }

    ++m_accumulatorCount;

    // 'Commit' the samples we've accumulated to the graph
    //
    if (m_accumulatorCount >= SamplesPerAggregateSample)
    {
        size_t insertOffset = static_cast<size_t>(m_graphData.StartOffset);
        m_graphData.StartOffset = (m_graphData.StartOffset + 1) % K4AImuGraphData::GraphSampleCount;

        for (int i = 0; i < 3; ++i)
        {
            m_graphData.AccData[insertOffset].v[i] = m_accAccumulator.v[i] / SamplesPerAggregateSample;
            m_graphData.GyroData[insertOffset].v[i] = m_gyroAccumulator.v[i] / SamplesPerAggregateSample;
        }

        m_graphData.AccTimestamp = sample.acc_timestamp_usec;
        m_graphData.GyroTimestamp = sample.gyro_timestamp_usec;
        m_graphData.LastTemperature = sample.temperature;

        ResetAccumulators();
    }
}

void K4AImuGraphDataGenerator::NotifyTermination()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_failed = true;
}

void K4AImuGraphDataGenerator::ClearData()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ResetAccumulators();
    std::fill(m_graphData.AccData.begin(), m_graphData.AccData.end(), k4a_float3_t{ { 0.f, 0.f, 0.f } });
    std::fill(m_graphData.GyroData.begin(), m_graphData.GyroData.end(), k4a_float3_t{ { 0.f, 0.f, 0.f } });

    m_graphData.AccTimestamp = 0;
    m_graphData.GyroTimestamp = 0;

    m_graphData.StartOffset = 0;
    m_graphData.LastTemperature = std::numeric_limits<float>::quiet_NaN();
}

K4AImuGraphDataGenerator::K4AImuGraphDataGenerator()
{
    ClearData();
    ResetAccumulators();
}

void K4AImuGraphDataGenerator::ResetAccumulators()
{
    m_gyroAccumulator = { { 0.f, 0.f, 0.f } };
    m_accAccumulator = { { 0.f, 0.f, 0.f } };
    m_accumulatorCount = 0;
}
