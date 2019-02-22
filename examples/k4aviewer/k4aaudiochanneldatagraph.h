// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AAUDIOCHANNELDATAGRAPH_H
#define K4AAUDIOCHANNELDATAGRAPH_H

// System headers
//
#include <array>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4amicrophonelistener.h"

namespace k4aviewer
{
class K4AAudioChannelDataGraph
{
public:
    explicit K4AAudioChannelDataGraph(const char *name);

    void AddSample(float sample);
    void Show(ImVec2 graphSize, float scale);

private:
    // We need to keep track of the min and max separately to produce graphs
    //
    class SignedAudioDataAccumulator
    {
    public:
        void AddSample(float sample);
        void Reset();
        float GetAbsMax() const;

        // Gets the root-mean-square of the samples that have been given to the accumulator,
        // which is intended to be an estimation of the loudness of the sound
        //
        float GetRms() const;
        size_t GetSampleCount() const;

    private:
        size_t m_sampleCount = 0;
        float m_rmsAccumulator = 0;
        float m_absMax = 0;
    };

    struct DataPoint
    {
        float Max;
        float PositiveRms;
        float NegativeRms;
        float Min;

        DataPoint(const float max, const float positiveRms, const float negativeRms, const float min) :
            Max(max),
            PositiveRms(positiveRms),
            NegativeRms(negativeRms),
            Min(min)
        {
        }

        DataPoint() : Max(0), PositiveRms(0), NegativeRms(0), Min(0) {}
    };

    static constexpr size_t AudioChannelGraphSampleCount = 120;
    std::array<DataPoint, AudioChannelGraphSampleCount> m_graphData = {};

    // We're targeting 60FPS, so we want to do our sample math approximately
    // often enough that we trigger an update to the graph every frame.
    //
    static constexpr size_t AudioSamplesPerGraphSample = K4AMicrophoneSampleRate / 60;
    size_t m_nextGraphPointIndex = 0;
    SignedAudioDataAccumulator m_positiveDataAccumulator;
    SignedAudioDataAccumulator m_negativeDataAccumulator;
    std::string m_name = "Unknown channel";
    std::string m_zoomSliderLabel;
};
} // namespace k4aviewer

#endif
