/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

// Associated header
//
#include "opengltexture.h"

// System headers
//

// Library headers
//

// Project headers
//

using namespace k4aviewer;

OpenGlTexture::OpenGlTexture(const ImageDimensions dimensions, const GLenum format, const GLenum type) :
    m_dimensions(dimensions),
    m_format(format),
    m_type(type),
    m_textureId(0)
{
}

void OpenGlTexture::SetTextureActive()
{
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

GLenum OpenGlTexture::UpdateTexture(const uint8_t *data)
{
    SetTextureActive();

    glTexSubImage2D(GL_TEXTURE_2D,                             // target
                    0,                                         // level
                    0,                                         // xoffset
                    0,                                         // yoffset
                    static_cast<GLsizei>(m_dimensions.Width),  // width
                    static_cast<GLsizei>(m_dimensions.Height), // height
                    m_format,                                  // format
                    m_type,                                    // type
                    data                                       // data
    );

    const GLenum status = glGetError();
    return status;
}

GLenum OpenGlTextureFactory::CreateTexture(std::shared_ptr<OpenGlTexture> &out,
                                           uint8_t *data,
                                           const ImageDimensions dimensions,
                                           const GLenum format,
                                           const GLenum internalFormat,
                                           const GLenum type)
{
    std::shared_ptr<OpenGlTexture> newTexture(new OpenGlTexture(dimensions, format, type));

    glGenTextures(1, &newTexture->m_textureId);
    newTexture->SetTextureActive();

    glTexImage2D(GL_TEXTURE_2D,                           // target
                 0,                                       // level
                 static_cast<GLint>(internalFormat),      // internalformat
                 static_cast<GLsizei>(dimensions.Width),  // width
                 static_cast<GLsizei>(dimensions.Height), // height
                 0,                                       // border
                 format,                                  // format
                 type,                                    // type
                 data                                     // data
    );

    const GLenum status = glGetError();

    if (status == GL_NO_ERROR)
    {
        out = newTexture;
    }
    else
    {
        out.reset();
    }

    return status;
}

OpenGlTexture::operator ImTextureID() const
{
    return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(m_textureId));
}

OpenGlTexture::operator GLuint() const
{
    return m_textureId;
}

OpenGlTexture::~OpenGlTexture()
{
    if (m_textureId)
    {
        glDeleteTextures(1, &m_textureId);
    }
}
