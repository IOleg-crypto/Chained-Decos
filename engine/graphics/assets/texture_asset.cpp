#include "engine/graphics/assets/texture_asset.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include <glad/gl.h>
#include <filesystem>
#include <algorithm>

namespace CHEngine
{

void TextureAsset::UploadToGPU()

{
    AssetState currentState = GetState();
    if (currentState == AssetState::Ready || currentState == AssetState::Failed)
    {
        return;
    }

    if (m_HasPendingImage && m_PendingImage.data != nullptr)
    {
        if (m_Texture.id > 0)
        {
            glDeleteTextures(1, &m_Texture.id);
        }

        glGenTextures(1, &m_Texture.id);
        glBindTexture(GL_TEXTURE_2D, m_Texture.id);

        m_Texture.width = m_PendingImage.width;
        m_Texture.height = m_PendingImage.height;
        m_Texture.format = m_PendingImage.format;

        GLenum internalFormat = GL_RGBA8;
        GLenum dataFormat = GL_RGBA;
        GLenum dataType = GL_UNSIGNED_BYTE;

        if (m_PendingImage.isHDR)
        {
            dataType = GL_FLOAT;
            if (m_PendingImage.channels == 3)
            {
                internalFormat = GL_RGB16F;
                dataFormat = GL_RGB;
            }
            else
            {
                internalFormat = GL_RGBA16F;
                dataFormat = GL_RGBA;
            }
        }
        else
        {
            dataType = GL_UNSIGNED_BYTE;
            if (m_PendingImage.channels == 3)
            {
                internalFormat = GL_RGB8;
                dataFormat = GL_RGB;
            }
            else
            {
                internalFormat = GL_RGBA8;
                dataFormat = GL_RGBA;
            }
        }


        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Texture.width, m_Texture.height, 0, dataFormat, dataType, m_PendingImage.data);
        
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Free pending image data (stbi_image_free or free depending on loader)
        // For now we assume free() if we use stbi_load
        free(m_PendingImage.data);
        m_PendingImage.data = nullptr;
        m_HasPendingImage = false;

        SetState(AssetState::Ready);
    }
}

TextureAsset::~TextureAsset()
{
    Unload();
}

void TextureAsset::Unload()
{
    if (m_Texture.id > 0)
    {
        glDeleteTextures(1, &m_Texture.id);
        m_Texture.id = 0;
    }
    if (m_PendingImage.data != nullptr)
    {
        free(m_PendingImage.data);
        m_PendingImage.data = nullptr;
    }
}

} // namespace CHEngine
