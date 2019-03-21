// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ADEPTHSENSORFRAMEBASEVISUALIZER_H
#define K4ADEPTHSENSORFRAMEBASEVISUALIZER_H

// System headers
//
#include <memory>
#include <vector>

// Library headers
//

// Project headers
//
#include "ik4aframevisualizer.h"
#include "k4adepthpixelcolorizer.h"
#include "k4astaticimageproperties.h"
#include "k4aviewerutil.h"
#include "perfcounter.h"

namespace k4aviewer
{
template<k4a_image_format_t ImageFormat, DepthPixelVisualizationFunction VisualizationFunction>
class K4ADepthSensorFrameBaseVisualizer : public IK4AFrameVisualizer<ImageFormat>
{
public:
    explicit K4ADepthSensorFrameBaseVisualizer(const k4a_depth_mode_t depthMode,
                                               const std::pair<DepthPixel, DepthPixel> expectedValueRange) :
        m_dimensions(GetDepthDimensions(depthMode)),
        m_expectedValueRange(expectedValueRange),
        m_expectedBufferSize(static_cast<size_t>(m_dimensions.Width * m_dimensions.Height) * sizeof(BgraPixel))
    {
    }

    GLenum InitializeTexture(std::shared_ptr<K4AViewerImage> &texture) override
    {
        return K4AViewerImage::Create(&texture, nullptr, m_dimensions, GL_BGRA);
    }

    void InitializeBuffer(K4ATextureBuffer<ImageFormat> &buffer) override
    {
        buffer.Data.resize(m_expectedBufferSize);
    }

    ImageVisualizationResult ConvertImage(const k4a::image &image, K4ATextureBuffer<ImageFormat> &buffer) override
    {
        const size_t srcImageSize = static_cast<size_t>(m_dimensions.Width * m_dimensions.Height) * sizeof(DepthPixel);

        if (image.get_size() != srcImageSize)
        {
            return ImageVisualizationResult::InvalidBufferSizeError;
        }

        // Convert the image into an OpenGL texture
        //
        const uint8_t *src = image.get_buffer();

        static PerfCounter render(std::string("Depth sensor<T") + std::to_string(int(ImageFormat)) + "> render");
        PerfSample renderSample(&render);
        RenderImage(src,
                    static_cast<size_t>(m_dimensions.Width * m_dimensions.Height) * sizeof(DepthPixel),
                    &buffer.Data[0]);
        renderSample.End();

        buffer.SourceImage = image;
        return ImageVisualizationResult::Success;
    }

    ImageVisualizationResult UpdateTexture(const K4ATextureBuffer<ImageFormat> &buffer,
                                           K4AViewerImage &texture) override
    {
        static PerfCounter upload(std::string("Depth sensor<T") + std::to_string(int(ImageFormat)) + "> upload");
        PerfSample uploadSample(&upload);
        return GLEnumToImageVisualizationResult(texture.UpdateTexture(&buffer.Data[0]));
    }

    ~K4ADepthSensorFrameBaseVisualizer() override = default;

    K4ADepthSensorFrameBaseVisualizer(const K4ADepthSensorFrameBaseVisualizer &) = delete;
    K4ADepthSensorFrameBaseVisualizer(const K4ADepthSensorFrameBaseVisualizer &&) = delete;
    K4ADepthSensorFrameBaseVisualizer &operator=(const K4ADepthSensorFrameBaseVisualizer &) = delete;
    K4ADepthSensorFrameBaseVisualizer &operator=(const K4ADepthSensorFrameBaseVisualizer &&) = delete;

protected:
    const ImageDimensions &GetDimensions() const
    {
        return m_dimensions;
    }

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
