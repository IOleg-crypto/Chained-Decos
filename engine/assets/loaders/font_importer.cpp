#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/assets/loaders/asset_importer.h"
#include <fstream>
#include <glad/gl.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace Chained::AssetImporter
{
std::shared_ptr<FontAsset> ImportFont(AssetHandle handle, const AssetMetadata& metadata)
{
    std::filesystem::path fullPath = ServiceLocator::Get<AssetManager>()->GetAssetDirectory() / metadata.FilePath;
    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        return nullptr;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> rawFontData(size);
    if (!file.read(reinterpret_cast<char*>(rawFontData.data()), size))
    {
        return nullptr;
    }

    Font font;
    font.atlasWidth = 512;
    font.atlasHeight = 512;
    font.fontSize = 32.0f;

    std::vector<uint8_t> bitmap(font.atlasWidth * font.atlasHeight, 0);
    stbtt_bakedchar bakedChars[128];

    int result = stbtt_BakeFontBitmap(rawFontData.data(), 0, font.fontSize, bitmap.data(), font.atlasWidth,
                                      font.atlasHeight, 32, 96, bakedChars);

    for (int i = 0; i < 128; i++)
    {
        if (i >= 32 && i < 128)
        {
            const auto& bc = bakedChars[i - 32];
            font.chars[i].x0 = (float)bc.x0 / font.atlasWidth;
            font.chars[i].y0 = (float)bc.y0 / font.atlasHeight;
            font.chars[i].x1 = (float)bc.x1 / font.atlasWidth;
            font.chars[i].y1 = (float)bc.y1 / font.atlasHeight;
            font.chars[i].xoff = bc.xoff;
            font.chars[i].yoff = bc.yoff;
            font.chars[i].xadvance = bc.xadvance;
        }
        else
        {
            font.chars[i] = {0};
        }
    }

    glGenTextures(1, &font.textureId);
    glBindTexture(GL_TEXTURE_2D, font.textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font.atlasWidth, font.atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE,
                 bitmap.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    auto asset = std::make_shared<FontAsset>(handle);
    asset->SetPath(metadata.FilePath.string());
    asset->SetFont(font);
    asset->SetState(AssetState::Ready);
    return asset;
}
} // namespace Chained::AssetImporter
