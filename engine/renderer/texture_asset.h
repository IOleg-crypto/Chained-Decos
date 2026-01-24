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
    static Ref<TextureAsset> Load(const std::string &path);

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

private:
    Texture2D m_Texture = {0};
};
} // namespace CHEngine
