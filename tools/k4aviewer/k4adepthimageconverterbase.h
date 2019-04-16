// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ADEPTHIMAGECONVERTERBASE_H
#define K4ADEPTHIMAGECONVERTERBASE_H

// System headers
//
#include <memory>
#include <vector>

// Library headers
//

// Project headers
//
#include "ik4aimageconverter.h"
#include "k4adepthpixelcolorizer.h"
#include "k4astaticimageproperties.h"
#include "k4aviewerutil.h"
#include "perfcounter.h"

namespace k4aviewer
{
template<k4a_image_format_t ImageFormat, DepthPixelVisualizationFunction VisualizationFunction>
class K4ADepthImageConverterBase : public IK4AImageConverter<ImageFormat>
{
public:
    explicit K4ADepthImageConverterBase(const k4a_depth_mode_t depthMode,
                                        const std::pair<DepthPixel, DepthPixel> expectedValueRange) :
        m_dimensions(GetDepthDimensions(depthMode)),
        m_expectedValueRange(expectedValueRange),
        m_expectedBufferSize(static_cast<size_t>(m_dimensions.Width * m_dimensions.Height) * sizeof(BgraPixel))
    {
    }

    ImageConversionResult ConvertImage(const k4a::image &srcImage, k4a::image *bgraImage) override
    {
        const size_t srcImageSize = static_cast<size_t>(m_dimensions.Width * m_dimensions.Height) * sizeof(DepthPixel);

        if (srcImage.get_size() != srcImageSize)
        {
            return ImageConversionResult::InvalidBufferSizeError;
        }

        if (bgraImage->get_width_pixels() != srcImage.get_width_pixels() ||
            bgraImage->get_height_pixels() != srcImage.get_height_pixels())
        {
            return ImageConversionResult::InvalidBufferSizeError;
        }

        const uint8_t *src = srcImage.get_buffer();

        static PerfCounter render(std::string("Depth sensor<T") + std::to_string(int(ImageFormat)) + "> render");
        PerfSample renderSample(&render);
        RenderImage(src,
                    static_cast<size_t>(m_dimensions.Width * m_dimensions.Height) * sizeof(DepthPixel),
                    bgraImage->get_buffer());
        renderSample.End();

        return ImageConversionResult::Success;
    }

    ImageDimensions GetImageDimensions() const override
    {
        return m_dimensions;
    }

    ~K4ADepthImageConverterBase() override = default;

    K4ADepthImageConverterBase(const K4ADepthImageConverterBase &) = delete;
    K4ADepthImageConverterBase(const K4ADepthImageConverterBase &&) = delete;
    K4ADepthImageConverterBase &operator=(const K4ADepthImageConverterBase &) = delete;
    K4ADepthImageConverterBase &operator=(const K4ADepthImageConverterBase &&) = delete;

private:
    void RenderImage(const uint8_t *src, const size_t srcSize, uint8_t *dst)
    {
        const uint8_t *currentSrc = src;
        const uint8_t *srcEnd = src + srcSize;
        while (currentSrc < srcEnd)
        {
            const DepthPixel pixelValue = *reinterpret_cast<const DepthPixel *>(currentSrc);

            BgraPixel *outputPixel = reinterpret_cast<BgraPixel *>(dst);
            *outputPixel = VisualizationFunction(pixelValue, m_expectedValueRange.first, m_expectedValueRange.second);

            dst += sizeof(BgraPixel);
            currentSrc += sizeof(DepthPixel);
        }
    }

    const ImageDimensions m_dimensions;
    const std::pair<DepthPixel, DepthPixel> m_expectedValueRange;
    const size_t m_expectedBufferSize;
};

} // namespace k4aviewer

#endif
