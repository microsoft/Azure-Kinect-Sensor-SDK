// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4aviewerimage.h"

// System headers
//

// Library headers
//

// Project headers
//
#include "k4aviewerutil.h"

using namespace k4aviewer;

namespace
{

// Gets the pixel size in elements for a given OpenGL format.
//
GLuint GetFormatPixelElementCount(GLenum format)
{
    switch (format)
    {
    case GL_RED:
        return 1;
    case GL_RG:
        return 2;
    case GL_RGB:
    case GL_BGR:
        return 3;
    case GL_RGBA:
    case GL_BGRA:
        return 4;
    default:
        throw std::logic_error("Invalid OpenGL format!");
    }
}
} // namespace

K4AViewerImage::K4AViewerImage(const ImageDimensions dimensions, const GLenum format) :
    m_dimensions(dimensions),
    m_format(format)
{
    m_textureBufferSize = static_cast<GLuint>(dimensions.Width * dimensions.Height) *
                          GetFormatPixelElementCount(format);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_textureBuffer.Id());
    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_textureBufferSize, nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

GLenum K4AViewerImage::UpdateTexture(const uint8_t *data)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_textureBuffer.Id());
    glBindTexture(GL_TEXTURE_2D, m_texture.Id());

    CleanupGuard bufferCleanupGuard([]() { glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); });

    uint8_t *buffer = reinterpret_cast<uint8_t *>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,
                                                                   0,
                                                                   m_textureBufferSize,
                                                                   GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));

    if (!buffer)
    {
        return glGetError();
    }

    if (data)
    {
        std::copy(data, data + m_textureBufferSize, buffer);
    }
    else
    {
        std::fill(buffer, buffer + m_textureBufferSize, static_cast<uint8_t>(0));
    }

    if (!glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER))
    {
        return glGetError();
    }

    glTexSubImage2D(GL_TEXTURE_2D,       // target
                    0,                   // level
                    0,                   // xoffset
                    0,                   // yoffset
                    m_dimensions.Width,  // width
                    m_dimensions.Height, // height
                    m_format,            // format
                    GL_UNSIGNED_BYTE,    // type
                    nullptr);            // data

    return glGetError();
}

GLenum K4AViewerImage::Create(std::shared_ptr<K4AViewerImage> *out,
                              const uint8_t *data,
                              const ImageDimensions dimensions,
                              const GLenum format,
                              const GLenum internalFormat)
{
    std::shared_ptr<K4AViewerImage> newTexture(new K4AViewerImage(dimensions, format));

    glBindTexture(GL_TEXTURE_2D, newTexture->m_texture.Id());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glTexStorage2D(GL_TEXTURE_2D,
                   1,
                   internalFormat,
                   static_cast<GLsizei>(dimensions.Width),
                   static_cast<GLsizei>(dimensions.Height));

    GLenum status = newTexture->UpdateTexture(data);

    if (status == GL_NO_ERROR)
    {
        *out = std::move(newTexture);
    }
    else
    {
        out->reset();
    }

    return status;
}

K4AViewerImage::operator ImTextureID() const
{
    return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(m_texture.Id()));
}

K4AViewerImage::operator GLuint() const
{
    return m_texture.Id();
}
