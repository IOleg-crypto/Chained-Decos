#include "texture_system.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include <stb_image.h>
#include <filesystem>
#include <algorithm>

namespace CHEngine
{
    TextureSystem::TextureSystem()
    {
    }

    TextureSystem::~TextureSystem()
    {
        UnloadAll();
    }

    TextureHandle TextureSystem::LoadTexture(const std::string& filepath, bool isCubemap)
    {
        if (filepath.empty())
            return TextureHandle(0);

        std::filesystem::path resolvedPath = Project::GetAbsolutePath(filepath);
        if (resolvedPath.empty() || !std::filesystem::exists(resolvedPath))
            return TextureHandle(0);

        std::string cacheKey = resolvedPath.generic_string();

        std::lock_guard<std::mutex> lock(m_Mutex);

        auto cachedPathIt = m_PathMap.find(cacheKey);
        if (cachedPathIt != m_PathMap.end())
        {
            return cachedPathIt->second;
        }

        int width, height, channels;

        bool isHDR = stbi_is_hdr(cacheKey.c_str());
        stbi_set_flip_vertically_on_load(true);

        void* data = nullptr;
        if (isHDR)
        {
            data = stbi_loadf(cacheKey.c_str(), &width, &height, &channels, 0);
        }
        else
        {
            data = stbi_load(cacheKey.c_str(), &width, &height, &channels, 4);
            channels = 4;
        }

        if (!data)
        {
            const char* reason = stbi_failure_reason();
            CH_CORE_ERROR("TextureSystem: Failed to load image {}. Reason: {}", cacheKey, reason ? reason : "Unknown");
            return TextureHandle(0);
        }

        TextureFormat format = TextureFormat::RGBA8;
        if (isHDR)
        {
            format = (channels == 3) ? TextureFormat::RGB16F : TextureFormat::RGBA16F;
        }
        else
        {
            format = (channels == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
        }

        std::shared_ptr<Texture> texture;
        if (isCubemap)
        {
            // For now, we assume the input is an equirectangular map that needs to be treated as a cubemap
            // or a single face. In a full engine, we'd have a converter here.
            texture = Texture::CreateCubemap(width, format);
            if (texture)
            {
                texture->SetData(data, 0); // This should handle conversion or faces
            }
        }
        else
        {
            texture = Texture::Create(width, height, format);
            if (texture)
            {
                texture->SetData(data, 0);
            }
        }

        stbi_image_free(data);

        if (!texture)
        {
            return TextureHandle(0);
        }

        TextureHandle newHandle = UUID();
        m_Registry.emplace(newHandle, TextureInfo{texture, cacheKey, isHDR, isCubemap});
        m_PathMap.emplace(cacheKey, newHandle);

        return newHandle;
    }

    bool TextureSystem::IsLoaded(TextureHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Registry.find(handle) != m_Registry.end();
    }

    uint32_t TextureSystem::GetRendererID(TextureHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(handle);
        if (it != m_Registry.end() && it->second.GPUTexture)
        {
            return it->second.GPUTexture->GetRendererID();
        }
        return 0;
    }

    std::shared_ptr<Texture> TextureSystem::GetTexture(TextureHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(handle);
        if (it != m_Registry.end())
        {
            return it->second.GPUTexture;
        }
        return nullptr;
    }

    bool TextureSystem::IsHDR(TextureHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(handle);
        return it != m_Registry.end() && it->second.IsHDR;
    }

    bool TextureSystem::IsCubemap(TextureHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(handle);
        return it != m_Registry.end() && it->second.IsCubemap;
    }

    void TextureSystem::Unload(TextureHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Registry.find(handle);
        if (it != m_Registry.end())
        {
            m_PathMap.erase(it->second.Path);
            m_Registry.erase(it);
        }
    }

    void TextureSystem::UnloadAll()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Registry.clear();
        m_PathMap.clear();
    }
}
