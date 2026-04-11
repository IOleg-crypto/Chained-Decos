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

    /** Cached GPU texture metadata tracked by the texture system. */
    struct TextureInfo
    {
        /** The uploaded GPU texture, or nullptr until the asset is ready. */
        std::shared_ptr<Texture> GPUTexture;
        /** Resolved source path used to identify and reload the texture. */
        std::string Path;
        /** True when the source data is HDR encoded. */
        bool IsHDR = false;
        /** True when the texture was loaded as a cubemap. */
        bool IsCubemap = false;

        /** Returns true when the GPU texture object exists. */
        bool IsReady() const { return GPUTexture != nullptr; }
    };

    /** Owns texture cache entries and provides lookup/unload helpers. */
    class TextureSystem
    {
    public:
        /** Returns the global texture system singleton. */
        static TextureSystem& Get()
        {
            static TextureSystem instance;
            return instance;
        }

        /** Loads a texture synchronously and returns its handle. */
        TextureHandle LoadTexture(const std::string& filepath, bool isCubemap = false);

        /** Returns true when the texture handle exists in the cache. */
        bool IsLoaded(TextureHandle handle) const;
        /** Returns the renderer-specific GPU texture ID, or 0 if missing. */
        uint32_t GetRendererID(TextureHandle handle) const;
        /** Returns the cached texture object for the given handle. */
        std::shared_ptr<Texture> GetTexture(TextureHandle handle) const;
        /** Returns true when the cached texture was loaded as HDR. */
        bool IsHDR(TextureHandle handle) const;
        /** Returns true when the cached texture was loaded as a cubemap. */
        bool IsCubemap(TextureHandle handle) const;
        
        /** Removes one texture from the cache. */
        void Unload(TextureHandle handle);
        /** Clears the entire texture cache. */
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
