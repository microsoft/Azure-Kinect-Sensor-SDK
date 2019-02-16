/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

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
#include "assertionexception.h"
#include "ik4aframevisualizer.h"
#include "k4adepthpixelcolorizer.h"
#include "k4aviewerutil.h"
#include "perfcounter.h"

namespace k4aviewer
{
template<k4a_image_format_t ImageFormat, DepthPixelVisualizationFunction VisualizationFunction>
class K4ADepthSensorFrameBaseVisualizer : public IK4AFrameVisualizer<ImageFormat>
{
public:
    explicit K4ADepthSensorFrameBaseVisualizer(const k4a_depth_mode_t depthMode,
                                               const ExpectedValueRange expectedValueRange) :
        m_dimensions(GetImageDimensionsForDepthMode(depthMode)),
        m_expectedValueRange(expectedValueRange),
        m_expectedBufferSize(static_cast<size_t>(m_dimensions.Width * m_dimensions.Height) * sizeof(RgbPixel))
    {
    }

    GLenum InitializeTexture(std::shared_ptr<OpenGlTexture> &texture) override
    {
        return OpenGlTextureFactory::CreateTexture(texture, nullptr, m_dimensions, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
    }

    void InitializeBuffer(K4ATextureBuffer<ImageFormat> &buffer) override
    {
        buffer.Data.resize(m_expectedBufferSize);
    }

    ImageVisualizationResult ConvertImage(const std::shared_ptr<K4AImage<ImageFormat>> &image,
                                          K4ATextureBuffer<ImageFormat> &buffer) override
    {
        const size_t srcImageSize = static_cast<size_t>(m_dimensions.Width * m_dimensions.Height) * sizeof(DepthPixel);

        if (image->GetSize() != srcImageSize)
        {
            return ImageVisualizationResult::InvalidBufferSizeError;
        }

        // Convert the image into an OpenGL texture
        //
        const uint8_t *src = image->GetBuffer();

        static PerfCounter render(std::string("Depth sensor<T") + std::to_string(int(ImageFormat)) + "> render");
        PerfSample renderSample(&render);
        RenderImage(src,
                    static_cast<size_t>(m_dimensions.Width * m_dimensions.Height) * sizeof(DepthPixel),
                    &buffer.Data[0]);
        renderSample.End();

        buffer.SourceImage = image;
        return ImageVisualizationResult::Success;
    }

    ImageVisualizationResult UpdateTexture(const K4ATextureBuffer<ImageFormat> &buffer, OpenGlTexture &texture) override
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

            RgbPixel *outputPixel = reinterpret_cast<RgbPixel *>(dst);
            *outputPixel = VisualizationFunction(m_expectedValueRange, pixelValue);

            dst += sizeof(RgbPixel);
            currentSrc += sizeof(DepthPixel);
        }
    }

    static ImageDimensions GetImageDimensionsForDepthMode(const k4a_depth_mode_t depthMode)
    {
        switch (depthMode)
        {
        case K4A_DEPTH_MODE_NFOV_2X2BINNED:
            return { 320, 288 };
        case K4A_DEPTH_MODE_NFOV_UNBINNED:
            return { 640, 576 };
        case K4A_DEPTH_MODE_WFOV_2X2BINNED:
            return { 512, 512 };
        case K4A_DEPTH_MODE_WFOV_UNBINNED:
            return { 1024, 1024 };
        case K4A_DEPTH_MODE_PASSIVE_IR:
            return { 1024, 1024 };

        default:
            throw AssertionException("Invalid depth dimensions value");
        }
    }

    const ImageDimensions m_dimensions;
    const ExpectedValueRange m_expectedValueRange;
    const size_t m_expectedBufferSize;
};

} // namespace k4aviewer

#endif
