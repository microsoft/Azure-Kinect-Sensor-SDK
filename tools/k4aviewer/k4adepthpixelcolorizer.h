// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ADEPTHPIXELCOLORIZER_H
#define K4ADEPTHPIXELCOLORIZER_H

// System headers
//
#include <algorithm>

// Library headers
//

// Project headers
//
#include "k4apixel.h"
#include "k4aviewerutil.h"

namespace k4aviewer
{

using DepthPixelVisualizationFunction = RgbPixel(const ExpectedValueRange &validValueRange, const DepthPixel &value);

class K4ADepthPixelColorizer
{
public:
    // Colorizing strategy that uses a rainbow gradient where blue is near and red is far
    //
    static inline RgbPixel ColorizeRedToBlue(const ExpectedValueRange &validValueRange, const DepthPixel &value)
    {
        float c[3] = { 1.0, 1.0, 1.0 }; // white

        // If the pixel is actual zero and not just below the min value, make it black
        //
        if (value == 0)
        {
            return RgbPixel{ 0, 0, 0 };
        }

        DepthPixel clampedValue = value;
        clampedValue = std::min(clampedValue, validValueRange.Max);
        clampedValue = std::max(clampedValue, validValueRange.Min);

        const auto dv = static_cast<float>(validValueRange.Max - validValueRange.Min);
        const float index = (clampedValue - validValueRange.Min) / dv; // [0, 1]
        // If we're less than min, in the middle, or greater than max, then do B, G, R, respectively
        if (index <= 0.0f)
        {
            c[0] = 0.0f;
            c[1] = 0.0f;
            c[2] = 1.0f; // Blue
        }
        else if (index == 0.5f)
        {
            c[0] = 0.0f;
            c[1] = 1.0f; // Green
            c[2] = 0.0f;
        }
        else if (index >= 1.0f)
        {
            c[0] = 1.0f; // Red
            c[1] = 0.0f;
            c[2] = 0.0f;
        }
        // The in between colors
        else if (index < 0.5f)
        {
            // Adjust only Blue and Green
            const float colorToSubtract = index - 0.25f; // [-.25, .25]
            c[0] = 0.0f;                                 // Red
            if (colorToSubtract < 0.0f)
            {
                // If < 0, then subtract from Green
                c[1] = 1.0f + (4.0f * colorToSubtract);
                c[2] = 1.0f;
            }
            else
            {
                // If > 0, then subtract from Blue
                c[1] = 1.0f;
                c[2] = 1.0f - (4.0f * colorToSubtract);
            }
        }
        else
        {
            // Adjust only Green and Red
            const float colorToSubtract = index - 0.75f; // [-.25, .25]
            c[2] = 0.0f;                                 // Blue
            if (colorToSubtract < 0.0f)
            {
                // If < 0, then subtract from Red
                c[0] = 1.0f + (4.0f * colorToSubtract);
                c[1] = 1.0f;
            }
            else
            {
                // If > 0, then subtract from Green
                c[0] = 1.0f;
                c[1] = 1.0f - (4.0f * colorToSubtract);
            }
        }

        return RgbPixel{ static_cast<uint8_t>(c[0] * std::numeric_limits<uint8_t>::max()),
                         static_cast<uint8_t>(c[1] * std::numeric_limits<uint8_t>::max()),
                         static_cast<uint8_t>(c[2] * std::numeric_limits<uint8_t>::max()) };
    }

    static inline RgbPixel ColorizeGreyscale(const ExpectedValueRange &expectedValueRange, const DepthPixel &value)
    {
        // Clamp to max
        //
        DepthPixel pixelValue = std::min(value, expectedValueRange.Max);

        const auto normalizedValue = static_cast<uint8_t>(
            (pixelValue - expectedValueRange.Min) *
            (double(std::numeric_limits<uint8_t>::max()) / (expectedValueRange.Max - expectedValueRange.Min)));

        // All color channels (RGB) are set the same (image is greyscale)
        //
        return RgbPixel{ normalizedValue, normalizedValue, normalizedValue };
    }
};
} // namespace k4aviewer

#endif
