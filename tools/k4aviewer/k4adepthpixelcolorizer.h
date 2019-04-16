// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ADEPTHPIXELCOLORIZER_H
#define K4ADEPTHPIXELCOLORIZER_H

// System headers
//
#include <algorithm>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//
#include "k4apixel.h"

namespace k4aviewer
{

using DepthPixelVisualizationFunction = BgraPixel(const DepthPixel &value,
                                                  const DepthPixel &min,
                                                  const DepthPixel &max);

// Functions that provide ways to take depth images and turn them into color representations
// suitable for showing to humans.
//
class K4ADepthPixelColorizer
{
public:
    // Computes a color representation of a depth pixel on the blue-red spectrum, using min
    // as the value for blue and max as the value for red.
    //
    static inline BgraPixel ColorizeBlueToRed(const DepthPixel &depthPixel,
                                              const DepthPixel &min,
                                              const DepthPixel &max)
    {
        constexpr uint8_t PixelMax = std::numeric_limits<uint8_t>::max();

        // Default to opaque black.
        //
        BgraPixel result = { 0, 0, 0, PixelMax };

        // If the pixel is actual zero and not just below the min value, make it black
        //
        if (depthPixel == 0)
        {
            return result;
        }

        uint16_t clampedValue = depthPixel;
        clampedValue = std::min(clampedValue, max);
        clampedValue = std::max(clampedValue, min);

        // Normalize to [0, 1]
        //
        float hue = (clampedValue - min) / static_cast<float>(max - min);

        // The 'hue' coordinate in HSV is a polar coordinate, so it 'wraps'.
        // Purple starts after blue and is close enough to red to be a bit unclear,
        // so we want to go from blue to red.  Purple starts around .6666667,
        // so we want to normalize to [0, .6666667].
        //
        constexpr float range = 2.f / 3.f;
        hue *= range;

        // We want blue to be close and red to be far, so we need to reflect the
        // hue across the middle of the range.
        //
        hue = range - hue;

        float fRed = 0.f;
        float fGreen = 0.f;
        float fBlue = 0.f;
        ImGui::ColorConvertHSVtoRGB(hue, 1.f, 1.f, fRed, fGreen, fBlue);

        result.Red = static_cast<uint8_t>(fRed * PixelMax);
        result.Green = static_cast<uint8_t>(fGreen * PixelMax);
        result.Blue = static_cast<uint8_t>(fBlue * PixelMax);

        return result;
    }

    // Computes a greyscale representation of a depth pixel.
    //
    static inline BgraPixel ColorizeGreyscale(const DepthPixel &value, const DepthPixel &min, const DepthPixel &max)
    {
        // Clamp to max
        //
        DepthPixel pixelValue = std::min(value, max);

        constexpr uint8_t PixelMax = std::numeric_limits<uint8_t>::max();
        const auto normalizedValue = static_cast<uint8_t>((pixelValue - min) * (double(PixelMax) / (max - min)));

        // All color channels are set the same (image is greyscale)
        //
        return BgraPixel{ normalizedValue, normalizedValue, normalizedValue, PixelMax };
    }
};
} // namespace k4aviewer

#endif
