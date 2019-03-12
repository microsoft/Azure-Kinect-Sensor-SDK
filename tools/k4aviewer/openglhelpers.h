// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef OPENGLHELPERS_H
#define OPENGLHELPERS_H

// System headers
//
#include <functional>
#include <list>
#include <sstream>

// Library headers
//
#include "k4aimgui_all.h"

// Project headers
//

namespace k4aviewer
{
namespace OpenGL
{

class Shader
{
public:
    Shader(GLenum shaderType, const GLchar *source)
    {
        m_id = glCreateShader(shaderType);
        glShaderSource(m_id, 1, &source, nullptr);
        glCompileShader(m_id);

        GLint success = GL_FALSE;
        char infoLog[512];
        glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(m_id, 512, nullptr, infoLog);
            std::stringstream errorBuilder;
            errorBuilder << "Shader compilation error: " << std::endl << infoLog;
            throw std::logic_error(errorBuilder.str().c_str());
        }
    }

    Shader(Shader &&other) : m_id(other.m_id)
    {
        other.m_id = 0;
    }

    Shader &operator=(Shader &&other)
    {
        if (this != &other)
        {
            m_id = other.m_id;
            other.m_id = 0;
        }
        return *this;
    }

    GLuint Id()
    {
        return m_id;
    }

    ~Shader()
    {
        glDeleteShader(m_id);
    }

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

private:
    GLuint m_id = 0;
};

class Program
{
public:
    Program()
    {
        m_id = glCreateProgram();
    }

    void AttachShader(Shader &&newShader)
    {
        glAttachShader(m_id, newShader.Id());
        m_shaders.emplace_back(std::move(newShader));
    }

    void Link()
    {
        glLinkProgram(m_id);
        GLint success = GL_FALSE;
        char infoLog[512];
        glGetProgramiv(m_id, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(m_id, 512, nullptr, infoLog);
            std::stringstream errorBuilder;
            errorBuilder << "Shader program linking error: " << std::endl << infoLog;
            throw std::logic_error(errorBuilder.str().c_str());
        }
    }

    GLint GetUniformLocation(const GLchar *name)
    {
        return glGetUniformLocation(m_id, name);
    }

    GLuint Id()
    {
        return m_id;
    }

    ~Program()
    {
        // Reset the active shader if we're about to delete it
        //
        GLint currentProgramId;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgramId);
        if (m_id == static_cast<GLuint>(currentProgramId))
        {
            glUseProgram(0);
        }

        glDeleteProgram(m_id);
    }

    Program(const Program &) = delete;
    Program &operator=(const Program &) = delete;

private:
    GLuint m_id = 0;
    std::list<Shader> m_shaders;
};

namespace Internal
{

// Shaders and programs are a little weird, but other than those,
// most OpenGL types have more-or-less the same create/delete pattern,
// so we have a common wrapper to get RAII semantics for those
//
template<void (*createFn)(GLsizei, GLuint *), void (*deleteFn)(GLsizei, GLuint *)> class BasicWrapper
{
public:
    BasicWrapper() = default;

    // Convenience constructor to automatically init the OpenGL object
    //
    BasicWrapper(bool init)
    {
        if (init)
        {
            Init();
        }
    }

    GLuint Id() const
    {
        return m_id;
    }

    operator bool() const
    {
        return m_id != 0;
    }

    void Init()
    {
        Reset();
        createFn(1, &m_id);
    }

    void Reset(BasicWrapper &&other)
    {
        Reset();
        m_id = other.m_id;
        other.Reset();
    }

    void Reset()
    {
        if (m_id != 0)
        {
            deleteFn(1, &m_id);
            m_id = 0;
        }
    }

    BasicWrapper(BasicWrapper &&other)
    {
        Reset(std::move(other));
    }

    BasicWrapper &operator=(BasicWrapper &&other)
    {
        Reset(std::move(other));
        return *this;
    }

    ~BasicWrapper()
    {
        Reset();
    }

    BasicWrapper(const BasicWrapper &) = delete;
    BasicWrapper &operator=(const BasicWrapper &) = delete;

private:
    GLuint m_id = 0;
};

// The OpenGL functions exported by GL3W are not actually functions,
// they're macros, so we need to wrap them in functions so we can pass them as
// template args to BasicWrapper.
//
inline void GenBuffers(GLsizei n, GLuint *buffers)
{
    glGenBuffers(n, buffers);
}

inline void DeleteBuffers(GLsizei n, GLuint *buffers)
{
    glDeleteBuffers(n, buffers);
}

inline void GenVertexArrays(GLsizei n, GLuint *vertexArrays)
{
    glGenVertexArrays(n, vertexArrays);
}

inline void DeleteVertexArrays(GLsizei n, GLuint *vertexArrays)
{
    glDeleteVertexArrays(n, vertexArrays);
}

inline void GenFramebuffers(GLsizei n, GLuint *framebuffers)
{
    glGenFramebuffers(n, framebuffers);
}

inline void DeleteFramebuffers(GLsizei n, GLuint *framebuffers)
{
    glDeleteFramebuffers(n, framebuffers);
}

inline void GenRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
    glGenRenderbuffers(n, renderbuffers);
}

inline void DeleteRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
    glDeleteRenderbuffers(n, renderbuffers);
}

inline void GenTextures(GLsizei n, GLuint *textures)
{
    glGenTextures(n, textures);
}

inline void DeleteTextures(GLsizei n, GLuint *textures)
{
    glDeleteTextures(n, textures);
}
} // namespace Internal

using Buffer = Internal::BasicWrapper<Internal::GenBuffers, Internal::DeleteBuffers>;
using VertexArray = Internal::BasicWrapper<Internal::GenVertexArrays, Internal::DeleteVertexArrays>;
using Framebuffer = Internal::BasicWrapper<Internal::GenFramebuffers, Internal::DeleteFramebuffers>;
using Renderbuffer = Internal::BasicWrapper<Internal::GenRenderbuffers, Internal::DeleteRenderbuffers>;
using Texture = Internal::BasicWrapper<Internal::GenTextures, Internal::DeleteTextures>;

} // namespace OpenGL
} // namespace k4aviewer

#endif