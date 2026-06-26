#ifndef CH_TEXTURE_ASSET_H
#define CH_TEXTURE_ASSET_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/texture.h"
#include <cstdint>
#include <memory>

namespace Chained
{

class TextureAsset : public Asset
{
public:
    // ── Constructors ──────────────────────────────────────────────────────────
    TextureAsset()
        : Asset(GetStaticType()) {}

    TextureAsset(AssetHandle handle, std::shared_ptr<Texture> texture = nullptr)
        : Asset(GetStaticType(), handle), m_Texture(texture)
    {
        if (m_Texture)
        {
            m_IsCubemap = m_Texture->GetType() == TextureType::Cubemap;
            SetState(AssetState::Ready);
        }
    }

    virtual ~TextureAsset();

    static AssetType GetStaticType() { return AssetType::Texture; }

    // ── Thread-safe (call from worker thread after stbi_load) ─────────────────
    // Stores the raw decoded pixel buffer. GPU upload is deferred to main thread.
    // Takes ownership of 'data' (will call stbi_image_free when done).
    void SetRawData(void* data, int width, int height, int channels, bool isHDR)
    {
        m_RawData       = data;
        m_RawWidth      = width;
        m_RawHeight     = height;
        m_RawChannels   = channels;
        m_IsHDR         = isHDR;
        m_HasPendingImage = true;
    }

    // ── Main thread only ──────────────────────────────────────────────────────
    // Uploads raw CPU data to GPU and marks asset Ready.
    // Called by AssetManager::Update() every frame until done.
    bool Finalize();

    // ── Accessors ─────────────────────────────────────────────────────────────
    std::shared_ptr<Texture> GetTexture() const { return m_Texture; }

    uint32_t GetWidth()      const { return m_Texture ? m_Texture->GetWidth()      : 0; }
    uint32_t GetHeight()     const { return m_Texture ? m_Texture->GetHeight()     : 0; }
    uint32_t GetRendererID() const { return m_Texture ? m_Texture->GetRendererID() : 0; }

    void Bind(uint32_t slot = 0) const { if (m_Texture) m_Texture->Bind(slot); }

    bool IsCubemap()    const { return m_IsCubemap; }
    bool IsHDR()        const { return m_IsHDR; }
    bool HasPending()   const { return m_HasPendingImage; }

    void SetIsCubemap(bool v) { m_IsCubemap = v; }
    void SetIsHDR(bool v)     { m_IsHDR = v; }

    // Legacy compat: path stored as string
    void SetPath(const std::string& p) { m_FilePath = p; Asset::SetPath(p); }

    size_t GetMemoryUsage() const override
    {
        return m_Texture ? (size_t)m_Texture->GetWidth() * m_Texture->GetHeight() * 4 : 0;
    }

// Allow TextureLoader::Finalize() to access internals without a friend declaration
// by using public helpers. Alternatively kept as fields here (tight coupling is acceptable
// for a loader inside the same module).
    void* m_RawData           = nullptr;
    int   m_RawWidth          = 0;
    int   m_RawHeight         = 0;
    int   m_RawChannels       = 4;
    bool  m_HasPendingImage   = false;

private:
    std::shared_ptr<Texture> m_Texture;
    std::string              m_FilePath;
    bool                     m_IsCubemap = false;
    bool                     m_IsHDR     = false;
};

} // namespace Chained

#endif // CH_TEXTURE_ASSET_H