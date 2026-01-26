#pragma once
#include "asset.h"
#include "engine/core/base.h"
#include <raylib.h>
#include <string>

namespace CHEngine
{
class TextureAsset : public Asset
{
public:
    static std::shared_ptr<TextureAsset> Load(const std::string &path);
    static void LoadAsync(const std::string &path); // Background CPU

    TextureAsset() = default;
    virtual ~TextureAsset();

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

    void UploadToGPU(); // Main Thread GPU

private:
    Texture2D m_Texture = {0};
    Image m_PendingImage = {0};
    bool m_HasPendingImage = false;
};
} // namespace CHEngine
