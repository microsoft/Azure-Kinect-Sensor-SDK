// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4APIXEL_H
#define K4APIXEL_H

// Helper structs/typedefs to cast buffers to
//
namespace viewer
{

struct BgraPixel
{
    uint8_t Blue;
    uint8_t Green;
    uint8_t Red;
    uint8_t Alpha;
};

using DepthPixel = uint16_t;
} // namespace viewer

#endif
