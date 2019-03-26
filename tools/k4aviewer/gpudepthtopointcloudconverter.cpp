// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "gpudepthtopointcloudconverter.h"

// System headers
//
#include <algorithm>
#include <sstream>

// Library headers
//

// Project headers
//

using namespace k4aviewer;

namespace
{

constexpr char const ComputeShader[] =
    R"(
#version 430

layout(location=0, rgba32f) writeonly uniform image2D destTex;

layout(location=1, r16ui) readonly uniform uimage2D depthImage;
layout(location=2, rg32f) readonly uniform image2D xyTable;

layout(local_size_x = 1, local_size_y = 1) in;

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);

    float vertexValue = float(imageLoad(depthImage, pixel));
    vec2 xyValue = imageLoad(xyTable, pixel).xy;

    float alpha = 1.0f;
    vec3 vertexPosition = vec3(vertexValue * xyValue.x, vertexValue * xyValue.y, vertexValue);

    // Invalid pixels have their XY table values set to 0.
    // Set the rest of their values to 0 so clients can pick them out.
    //
    if (xyValue.x == 0.0f && xyValue.y == 0.0f)
    {
        alpha = 0.0f;
        vertexValue = 0.0f;
    }

    // Vertex positions are in millimeters, but everything else is in meters, so we need to convert
    //
    vertexPosition /= 1000.0f;

    // OpenGL and K4A have different conventions on which direction is positive -
    // we need to flip the X coordinate.
    //
    vertexPosition.x *= -1;

    imageStore(destTex, pixel, vec4(vertexPosition, alpha));
}
)";

// Texture formats
//
constexpr GLenum depthImageInternalFormat = GL_R16UI;
constexpr GLenum depthImageDataFormat = GL_RED_INTEGER;
constexpr GLenum depthImageDataType = GL_UNSIGNED_SHORT;

constexpr GLenum xyTableInternalFormat = GL_RG32F;
constexpr GLenum xyTableDataFormat = GL_RG;
constexpr GLenum xyTableDataType = GL_FLOAT;

} // namespace

GpuDepthToPointCloudConverter::GpuDepthToPointCloudConverter()
{
    OpenGL::Shader shader(GL_COMPUTE_SHADER, ComputeShader);

    m_shaderProgram.AttachShader(std::move(shader));
    m_shaderProgram.Link();

    m_destTexId = glGetUniformLocation(m_shaderProgram.Id(), "destTex");
    m_xyTableId = glGetUniformLocation(m_shaderProgram.Id(), "xyTable");
    m_depthImageId = glGetUniformLocation(m_shaderProgram.Id(), "depthImage");
}

GLenum GpuDepthToPointCloudConverter::Convert(const k4a::image &depth, OpenGL::Texture *outputTexture)
{
    if (!m_xyTableTexture)
    {
        throw std::logic_error("You must call SetActiveXyTable at least once before calling Convert!");
    }

    // Create output texture if it doesn't already exist
    //
    // We don't use the alpha channel, but it turns out OpenGL doesn't
    // actually have a 3-component (i.e. RGB32F) format - you get
    // 1, 2, or 4 components.
    //
    const int width = depth.get_width_pixels();
    const int height = depth.get_height_pixels();
    if (!*outputTexture)
    {
        outputTexture->Init();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTexture->Id());

        glTexStorage2D(GL_TEXTURE_2D, 1, PointCloudTextureFormat, width, height);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Upload data to our uniform texture
    //
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_depthImagePixelBuffer.Id());
    glBindTexture(GL_TEXTURE_2D, m_depthImageTexture.Id());

    const GLuint numBytes = static_cast<GLuint>(width * height) * sizeof(uint16_t);

    GLubyte *textureMappedBuffer = reinterpret_cast<GLubyte *>(
        glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, numBytes, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    if (!textureMappedBuffer)
    {
        return glGetError();
    }

    const GLubyte *depthSrc = reinterpret_cast<const GLubyte *>(depth.get_buffer());
    std::copy(depthSrc, depthSrc + numBytes, textureMappedBuffer);
    if (!glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER))
    {
        return glGetError();
    }

    glTexSubImage2D(GL_TEXTURE_2D,        // target
                    0,                    // level
                    0,                    // xoffset
                    0,                    // yoffset
                    width,                // width
                    height,               // height
                    depthImageDataFormat, // format
                    depthImageDataType,   // type
                    nullptr);             // data
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glUseProgram(m_shaderProgram.Id());

    // Bind textures that we're going to pass to the shader
    //
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outputTexture->Id());
    glBindImageTexture(0, outputTexture->Id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, PointCloudTextureFormat);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_depthImageTexture.Id());
    glBindImageTexture(1, m_depthImageTexture.Id(), 0, GL_FALSE, 0, GL_READ_ONLY, depthImageInternalFormat);
    glUniform1i(m_depthImageId, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_xyTableTexture.Id());
    glBindImageTexture(2, m_xyTableTexture.Id(), 0, GL_FALSE, 0, GL_READ_ONLY, xyTableInternalFormat);
    glUniform1i(m_xyTableId, 2);

    // Render point cloud
    //
    glDispatchCompute(static_cast<GLuint>(depth.get_width_pixels()), static_cast<GLuint>(depth.get_height_pixels()), 1);

    // Wait for the rendering to finish before allowing reads to the texture we just wrote
    //
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    GLenum status = glGetError();
    return status;
}

GLenum GpuDepthToPointCloudConverter::SetActiveXyTable(const k4a::image &xyTable)
{
    const int width = xyTable.get_width_pixels();
    const int height = xyTable.get_height_pixels();

    // Upload the XY table as a texture so we can use it as a uniform
    //
    m_xyTableTexture.Init();

    glBindTexture(GL_TEXTURE_2D, m_xyTableTexture.Id());
    glTexStorage2D(GL_TEXTURE_2D, 1, xyTableInternalFormat, width, height);
    glTexSubImage2D(GL_TEXTURE_2D,                                          // target
                    0,                                                      // level
                    0,                                                      // xoffset
                    0,                                                      // yoffset
                    width,                                                  // width
                    height,                                                 // height
                    xyTableDataFormat,                                      // format
                    xyTableDataType,                                        // type
                    reinterpret_cast<const float *>(xyTable.get_buffer())); // data

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Pre-allocate a texture for the depth images so we don't have to
    // reallocate on every frame
    //
    m_depthImageTexture.Init();
    m_depthImagePixelBuffer.Init();

    const GLuint depthImageSizeBytes = static_cast<GLuint>(width * height) * sizeof(uint16_t);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_depthImagePixelBuffer.Id());
    glBufferData(GL_PIXEL_UNPACK_BUFFER, depthImageSizeBytes, nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, m_depthImageTexture.Id());

    glTexStorage2D(GL_TEXTURE_2D, 1, depthImageInternalFormat, width, height);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum status = glGetError();
    return status;
}

k4a::image GpuDepthToPointCloudConverter::GenerateXyTable(const k4a::calibration &calibration,
                                                          k4a_calibration_type_t calibrationType)
{
    const k4a_calibration_camera_t &cameraCalibration = calibrationType == K4A_CALIBRATION_TYPE_COLOR ?
                                                            calibration.color_camera_calibration :
                                                            calibration.depth_camera_calibration;

    k4a::image xyTable = k4a::image::create(K4A_IMAGE_FORMAT_CUSTOM,
                                            cameraCalibration.resolution_width,
                                            cameraCalibration.resolution_height,
                                            cameraCalibration.resolution_width *
                                                static_cast<int>(sizeof(k4a_float2_t)));

    k4a_float2_t *tableData = reinterpret_cast<k4a_float2_t *>(xyTable.get_buffer());

    int width = cameraCalibration.resolution_width;
    int height = cameraCalibration.resolution_height;

    k4a_float2_t p;
    k4a_float3_t ray;

    for (int y = 0, idx = 0; y < height; y++)
    {
        p.xy.y = static_cast<float>(y);
        for (int x = 0; x < width; x++, idx++)
        {
            p.xy.x = static_cast<float>(x);

            if (calibration.convert_2d_to_3d(p, 1.f, calibrationType, calibrationType, &ray))
            {
                tableData[idx].xy.x = ray.xyz.x;
                tableData[idx].xy.y = ray.xyz.y;
            }
            else
            {
                // The pixel is invalid.
                //
                tableData[idx].xy.x = 0.0f;
                tableData[idx].xy.y = 0.0f;
            }
        }
    }

    return xyTable;
}
