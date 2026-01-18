#ifndef CH_TEXTURE_ASSET_H
#define CH_TEXTURE_ASSET_H

#include "engine/core/base.h"
#include <raylib.h>
#include <string>

namespace CHEngine
{
class TextureAsset
{
public:
    static Ref<TextureAsset> Load(const std::string &path);

    TextureAsset() = default;
    ~TextureAsset();

    Texture2D &GetTexture()
    {
        return m_Texture;
    }
    const std::string &GetPath() const
    {
        return m_Path;
    }

    void SetTexture(Texture2D tex)
    {
        m_Texture = tex;
    }
    void SetPath(const std::string &path)
    {
        m_Path = path;
    }

private:
    Texture2D m_Texture = {0};
    std::string m_Path;
};
} // namespace CHEngine

#endif // CH_TEXTURE_ASSET_H
