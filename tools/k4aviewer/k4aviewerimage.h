// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4AVIEWERIMAGE_H
#define K4AVIEWERIMAGE_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a.hpp>
#include "k4aimgui_all.h"

// Project headers
//
#include "k4apixel.h"
#include "openglhelpers.h"

namespace k4aviewer
{
struct ImageDimensions
{
    ImageDimensions() = default;
    constexpr ImageDimensions(int w, int h) : Width(w), Height(h) {}
    constexpr ImageDimensions(const std::pair<int, int> &pair) : Width(pair.first), Height(pair.second) {}

    int Width;
    int Height;
};

// An image renderable by K4AViewer UI controls.
// Essentially an OpenGL texture with a bit of extra metadata.
//
class K4AViewerImage
{
public:
    static GLenum Create(std::shared_ptr<K4AViewerImage> *out,
                         const uint8_t *data,
                         ImageDimensions dimensions,
                         GLenum format = GL_BGRA,
                         GLenum internalFormat = GL_RGBA8);

    explicit operator ImTextureID() const;
    explicit operator GLuint() const;

    ImageDimensions GetDimensions() const
    {
        return m_dimensions;
    }

    GLenum UpdateTexture(const uint8_t *data);

    ~K4AViewerImage() = default;

    K4AViewerImage(const K4AViewerImage &) = delete;
    K4AViewerImage(const K4AViewerImage &&) = delete;
    K4AViewerImage &operator=(const K4AViewerImage &) = delete;
    K4AViewerImage &operator=(const K4AViewerImage &&) = delete;

private:
    K4AViewerImage(ImageDimensions dimensions, GLenum format);

    ImageDimensions m_dimensions;
    GLenum m_format;

    OpenGL::Texture m_texture = OpenGL::Texture(true);
    OpenGL::Buffer m_textureBuffer = OpenGL::Buffer(true);

    GLuint m_textureBufferSize;
};

} // namespace k4aviewer

#endif
