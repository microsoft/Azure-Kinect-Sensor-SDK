// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "texture.h"

#include "viewerutil.h"

using namespace viewer;

Texture::Texture(int width, int height) : m_width(width), m_height(height)
{
    glGenTextures(1, &m_name);
    glBindTexture(GL_TEXTURE_2D, m_name);

    // Set filtering mode so the texture gets sampled properly
    //
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Reserve storage for the texture
    //
    glTexStorage2D(GL_TEXTURE_2D, // target
                   1,             // levels
                   GL_RGBA32F,    // internalformat
                   m_width,       // width
                   m_height);     // height

    CheckOpenGLErrors();
}

Texture::Texture(Texture &&other)
{
    *this = std::move(other);
}

Texture &Texture::operator=(Texture &&other)
{
    if (this == &other)
    {
        return *this;
    }

    DeleteTexture();

    m_name = other.m_name;
    m_height = other.m_height;
    m_width = other.m_width;

    other.m_name = InvalidTextureName;
    other.m_height = 0;
    other.m_width = 0;

    return *this;
}

Texture::~Texture()
{
    DeleteTexture();
}

void Texture::DeleteTexture()
{
    glDeleteTextures(1, &m_name);
}

void Texture::Update(const BgraPixel *data)
{
    glBindTexture(GL_TEXTURE_2D, m_name);
    glTexSubImage2D(GL_TEXTURE_2D,                           // target
                    0,                                       // level
                    0,                                       // xoffset
                    0,                                       // yoffset
                    m_width,                                 // width
                    m_height,                                // height
                    GL_BGRA,                                 // format
                    GL_UNSIGNED_BYTE,                        // type
                    reinterpret_cast<const GLvoid *>(data)); // pixels

    CheckOpenGLErrors();
}
