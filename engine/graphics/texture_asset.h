#ifndef CH_TEXTURE_ASSET_H
#define CH_TEXTURE_ASSET_H

#include "engine/graphics/asset.h"
#include <memory>
#include <raylib.h>
#include <string>

namespace CHEngine
{

class TextureAsset : public Asset
{
public:
    TextureAsset() = default;
    virtual ~TextureAsset();

    static std::shared_ptr<TextureAsset> Load(const std::string &path);
    static void LoadAsync(const std::string &path);

    void UploadToGPU();
    void LoadFromFile(const std::string &path);

    virtual AssetType GetType() const override
    {
        return AssetType::Texture;
    }

    Texture2D &GetTexture()
    {
        return m_Texture;
    }
    void SetTexture(Texture2D tex)
    {
        m_Texture = tex;
    }

private:
    Texture2D m_Texture = {0};
    Image m_PendingImage = {0};
    bool m_HasPendingImage = false;
};
} // namespace CHEngine

#endif // CH_TEXTURE_ASSET_H
