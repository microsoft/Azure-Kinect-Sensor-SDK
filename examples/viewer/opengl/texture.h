// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TEXTURE_H
#define TEXTURE_H

#include <utility>

#include "k4aimgui_all.h"
#include "k4apixel.h"

namespace viewer
{

// A simple wrapper for OpenGL textures.
// Textures are not resizeable, but can have their contents updated any number of times.
//
class Texture
{
public:
    // Updates the image stored in the texture with data.
    // The data is expected to be formatted as Width * Height pixels formatted
    // as BGRA pixels represented by uint8_t's.
    //
    // The data you pass to it must be of the same size as the dimensions used
    // to construct the texture.
    //
    void Update(const BgraPixel *data);

    // Get the OpenGL texture name.
    // OpenGL texture names are just uints that OpenGL uses as opaque pointers.
    //
    inline GLuint Name() const
    {
        return m_name;
    }

    // Gets the width of the texture.
    //
    inline int Width() const
    {
        return m_width;
    }

    // Gets the height of the texture.
    //
    inline int Height() const
    {
        return m_height;
    }

    ~Texture();

    // In the interest of simplicity, we don't refcount textures, so
    // copying doesn't work.
    //
    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    // Textures are moveable.
    //
    Texture(Texture &&other);
    Texture &operator=(Texture &&other);

private:
    // Creating a texture before the viewer window has been initialized will fail,
    // so we make the viewer responsible for creating texture instances.
    //
    // Creates a texture with the specified dimensions, including allocating space
    // on the GPU.  Use Update() to update the image stored in the texture.
    //
    friend class ViewerWindow;
    Texture(int width, int height);

    // Deletes the wrapped texture.
    //
    void DeleteTexture();

    static constexpr GLuint InvalidTextureName = 0;

    GLuint m_name = InvalidTextureName;
    int m_width = 0;
    int m_height = 0;
};

} // namespace viewer

#endif