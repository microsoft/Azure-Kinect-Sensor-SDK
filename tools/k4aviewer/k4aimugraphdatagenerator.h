// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AIMUGRAPHDATAGENERATOR_H
#define K4AIMUGRAPHDATAGENERATOR_H

// System headers
//
#include <mutex>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//
#include "ik4aobserver.h"
#include "k4aringbuffer.h"

namespace k4aviewer
{

struct K4AImuGraphData
{
public:
    static constexpr int GraphSampleCount = 150;

    using AccumulatorArray = std::array<k4a_float3_t, GraphSampleCount>;

    AccumulatorArray AccData;
    AccumulatorArray GyroData;

    uint64_t AccTimestamp;
    uint64_t GyroTimestamp;

    float LastTemperature;

    int StartOffset;
};

class K4AImuGraphDataGenerator : public IK4AImuObserver
{
public:
    void NotifyData(const k4a_imu_sample_t &sample) override;
    void NotifyTermination() override;
    void ClearData() override;

    struct GraphReader
    {
        std::unique_lock<std::mutex> Lock;
        const K4AImuGraphData *Data;
    };

    // Returns a pointer-to-graph-data and a lock that guarantees
    // that the graph data won't be modified.  You must release the lock
    // in a timely fashion or the graph generator will hang.
    //
    GraphReader GetGraphData() const
    {
        return GraphReader{ std::unique_lock<std::mutex>(m_mutex), &m_graphData };
    }

    bool IsFailed() const
    {
        return m_failed;
    }

    K4AImuGraphDataGenerator();
    ~K4AImuGraphDataGenerator() override = default;

    static constexpr int SamplesPerAggregateSample = 20;
    static constexpr int SamplesPerGraph = SamplesPerAggregateSample * K4AImuGraphData::GraphSampleCount;

private:
    void ResetAccumulators();

    K4AImuGraphData m_graphData;

    bool m_failed = false;

    k4a_float3_t m_gyroAccumulator;
    k4a_float3_t m_accAccumulator;
    int m_accumulatorCount = 0;

    mutable std::mutex m_mutex;
};
} // namespace k4aviewer

#endif
