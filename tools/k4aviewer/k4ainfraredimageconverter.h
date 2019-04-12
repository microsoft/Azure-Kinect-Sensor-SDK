// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AINFRAREDIMAGECONVERTER_H
#define K4AINFRAREDIMAGECONVERTER_H

// System headers
//

// Library headers
//

// Project headers
//
#include "k4adepthimageconverterbase.h"
#include "k4adepthpixelcolorizer.h"

namespace k4aviewer
{
class K4AInfraredImageConverter
    : public K4ADepthImageConverterBase<K4A_IMAGE_FORMAT_IR16, K4ADepthPixelColorizer::ColorizeGreyscale>
{
public:
    explicit K4AInfraredImageConverter(k4a_depth_mode_t depthMode) :
        K4ADepthImageConverterBase<K4A_IMAGE_FORMAT_IR16, K4ADepthPixelColorizer::ColorizeGreyscale>(depthMode,
                                                                                                     GetIrLevels(
                                                                                                         depthMode)){};

    ~K4AInfraredImageConverter() override = default;

    K4AInfraredImageConverter(const K4AInfraredImageConverter &) = delete;
    K4AInfraredImageConverter(const K4AInfraredImageConverter &&) = delete;
    K4AInfraredImageConverter &operator=(const K4AInfraredImageConverter &) = delete;
    K4AInfraredImageConverter &operator=(const K4AInfraredImageConverter &&) = delete;
};
} // namespace k4aviewer

#endif
