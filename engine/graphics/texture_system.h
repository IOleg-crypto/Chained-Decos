#ifndef CH_TEXTURE_SYSTEM_H
#define CH_TEXTURE_SYSTEM_H

#include "engine/core/uuid.h"
#include "engine/graphics/api/texture.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>

namespace CHEngine
{
    using TextureHandle = UUID;

    struct TextureInfo
    {
        std::shared_ptr<Texture> GPUTexture;
        std::string Path;
        bool IsHDR = false;
        bool IsCubemap = false;

        bool IsReady() const { return GPUTexture != nullptr; }
    };

    class TextureSystem
    {
    public:
        static TextureSystem& Get()
        {
            static TextureSystem instance;
            return instance;
        }

        // synchronous loading
        TextureHandle LoadTexture(const std::string& filepath, bool isCubemap = false);

        bool IsLoaded(TextureHandle handle) const;
        uint32_t GetRendererID(TextureHandle handle) const;
        std::shared_ptr<Texture> GetTexture(TextureHandle handle) const;
        bool IsHDR(TextureHandle handle) const;
        bool IsCubemap(TextureHandle handle) const;
        
        void Unload(TextureHandle handle);
        void UnloadAll();

    private:
        TextureSystem() = default;
        ~TextureSystem() { UnloadAll(); }

        TextureSystem(const TextureSystem&) = delete;
        TextureSystem& operator=(const TextureSystem&) = delete;

    private:
        mutable std::mutex m_Mutex;
        std::unordered_map<TextureHandle, TextureInfo> m_Registry;
        std::unordered_map<std::string, TextureHandle> m_PathMap;
    };
}

#endif // CH_TEXTURE_SYSTEM_H
