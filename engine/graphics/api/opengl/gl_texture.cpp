#include "gl_texture.h"
#include <stb_image.h>

namespace Chained
{

static uint32_t CalculateMipLevels(uint32_t width, uint32_t height)
{
    return static_cast<uint32_t>(1 + std::floor(std::log2(std::max(width, height))));
}

GLTexture::GLTexture(uint32_t width, uint32_t height, TextureFormat format)
    : m_Width(width),
      m_Height(height),
      m_Format(format),
      m_Type(TextureType::Texture2D)
{
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;

    switch (format)
    {
    case TextureFormat::RGB8:
        m_InternalFormat = GL_RGB8;
        m_DataFormat = GL_RGB;
        break;
    case TextureFormat::RGBA8:
        m_InternalFormat = GL_RGBA8;
        m_DataFormat = GL_RGBA;
        break;
    case TextureFormat::RGB16F:
        m_InternalFormat = GL_RGB16F;
        m_DataFormat = GL_RGB;
        break;
    case TextureFormat::RGBA16F:
        m_InternalFormat = GL_RGBA16F;
        m_DataFormat = GL_RGBA;
        break;
    }

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    uint32_t mipLevels = CalculateMipLevels(m_Width, m_Height);
    glTexStorage2D(GL_TEXTURE_2D, mipLevels, m_InternalFormat, m_Width, m_Height);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    m_IsReady = true;
}

GLTexture::GLTexture(uint32_t size, TextureFormat format)
    : m_Width(size),
      m_Height(size),
      m_Format(format),
      m_Type(TextureType::Cubemap)
{
    m_InternalFormat = GL_RGB16F;
    m_DataFormat = GL_RGB;

    switch (format)
    {
    case TextureFormat::RGB16F:
        m_InternalFormat = GL_RGB16F;
        m_DataFormat = GL_RGB;
        break;
    case TextureFormat::RGBA16F:
        m_InternalFormat = GL_RGBA16F;
        m_DataFormat = GL_RGBA;
        break;
    }

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

    for (uint32_t i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat,
                     GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    m_IsReady = true;
}

GLTexture::GLTexture(uint32_t handle, uint32_t width, uint32_t height)
    : m_RendererID(handle),
      m_Width(width),
      m_Height(height),
      m_Format(TextureFormat::RGBA8),
      m_Type(TextureType::Texture2D),
      m_IsReady(true),
      m_OwnsResource(false)
{
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;
}

GLTexture::~GLTexture()
{
    if (m_OwnsResource) { glDeleteTextures(1, &m_RendererID); }
}

void GLTexture::SetData(void* data, uint32_t size)
{
    GLenum dataType =
        (m_Format == TextureFormat::RGB16F || m_Format == TextureFormat::RGBA16F) ? GL_FLOAT : GL_UNSIGNED_BYTE;

    if (m_Type == TextureType::Cubemap)
    {
        uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
        uint32_t faceSize = m_Width * m_Height * bpp;
        uint8_t* ptr = static_cast<uint8_t*>(data);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
        for (uint32_t i = 0; i < 6; ++i)
        {
            glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, m_Width, m_Height, m_DataFormat, dataType,
                            ptr + i * faceSize);
        }
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_DataFormat, dataType, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

void GLTexture::Bind(uint32_t slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(m_Type == TextureType::Cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, m_RendererID);
}

} // namespace Chained

