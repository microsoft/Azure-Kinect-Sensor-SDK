/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4APIXEL_H
#define K4APIXEL_H

// Associated header
//

// System headers
//
#include <memory>

// Library headers
//

// Project headers
//

namespace k4aviewer
{
struct RgbPixel
{
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
};

struct RgbaPixel
{
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
    uint8_t Alpha;
};

using DepthPixel = uint16_t;
} // namespace k4aviewer

#endif
