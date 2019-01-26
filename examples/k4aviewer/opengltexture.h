/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef OPENGLTEXTURE_H
#define OPENGLTEXTURE_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a.h>
#include "k4aimgui_all.h"

// Project headers
//
#include "k4apixel.h"

namespace k4aviewer
{
struct ImageDimensions
{
    int Width;
    int Height;
};

class OpenGlTexture
{
public:
    explicit operator ImTextureID() const;
    explicit operator GLuint() const;

    ImageDimensions GetDimensions() const
    {
        return m_dimensions;
    }

    GLenum UpdateTexture(uint8_t *data);

    ~OpenGlTexture();

    OpenGlTexture(const OpenGlTexture &) = delete;
    OpenGlTexture(const OpenGlTexture &&) = delete;
    OpenGlTexture &operator=(const OpenGlTexture &) = delete;
    OpenGlTexture &operator=(const OpenGlTexture &&) = delete;

private:
    explicit OpenGlTexture(ImageDimensions dimensions, GLenum format, GLenum type);
    void SetTextureActive();

    ImageDimensions m_dimensions;
    GLenum m_format;
    GLenum m_type;

    GLuint m_textureId;

    friend class OpenGlTextureFactory;
};

class OpenGlTextureFactory
{
public:
    static GLenum CreateTexture(std::shared_ptr<OpenGlTexture> &out,
                                uint8_t *data,
                                ImageDimensions dimensions,
                                GLenum format,
                                GLenum internalFormat,
                                GLenum type);
};
} // namespace k4aviewer

#endif
