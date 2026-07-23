#define STB_TRUETYPE_IMPLEMENTATION
#include "engine/assets/loaders/font_loader.h"
#include "engine/assets/types/font_asset.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"
#include <filesystem>
#include <fstream>
#include <glad/gl.h>
#include <stb_truetype.h>
#include <vector>

namespace Chained
{
std::shared_ptr<Asset> FontLoader::Create()
{
    return std::make_shared<FontAsset>();
}

bool FontLoader::Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError)
{

    auto fail = [&](const std::string& msg, bool logError = true) {
        std::string fullMsg = "FontLoader: " + msg;
        if (logError)
        {
            CH_CORE_ERROR("{0}", fullMsg);
        }
        if (outError)
        {
            *outError = fullMsg;
        }
        return false;
    };

    if (resolvedPath.empty())
    {
        return fail("empty path", false);
    }

    // Read font data: pack first, then filesystem
    std::vector<unsigned char> buffer;
    auto* assetManager = ServiceLocator::TryGet<AssetManager>();
    bool usePack = assetManager && assetManager->IsPacked();

    if (usePack)
    {
        auto data = assetManager->ReadAssetData(resolvedPath);
        if (data.empty())
        {
            return fail("font not found in pack '" + resolvedPath + "'");
        }
        buffer.assign(data.begin(), data.end());
    }
    else
    {
        std::filesystem::path fullPath(resolvedPath);
        if (!std::filesystem::exists(fullPath))
        {
            return fail("file not found '" + resolvedPath + "'");
        }

        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return fail("failed to open font file '" + resolvedPath + "'");
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        buffer.resize(static_cast<size_t>(size));

        if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        {
            return fail("failed to read font file '" + resolvedPath + "'");
        }
    }

    NativeFont font;
    font.fontSize = 24.0f;
    font.atlasWidth = 512;
    font.atlasHeight = 512;

    std::vector<unsigned char> pixels(font.atlasWidth * font.atlasHeight);
    stbtt_bakedchar chardata[128];

    int charsBaked = stbtt_BakeFontBitmap(buffer.data(), 0, font.fontSize, pixels.data(), font.atlasWidth,
                                          font.atlasHeight, 32, 128, chardata);
    if (charsBaked <= 0)
    {
        return fail("stbtt_BakeFontBitmap failed for '" + resolvedPath + "'");
    }

    std::vector<unsigned char> rgbaPixels(font.atlasWidth * font.atlasHeight * 4);
    for (size_t i = 0; i < pixels.size(); i++)
    {
        rgbaPixels[i * 4 + 0] = 255;       // R
        rgbaPixels[i * 4 + 1] = 255;       // G
        rgbaPixels[i * 4 + 2] = 255;       // B
        rgbaPixels[i * 4 + 3] = pixels[i]; // A
    }

    font.textureAtlas = Texture::Create(font.atlasWidth, font.atlasHeight, TextureFormat::RGBA8);
    font.textureAtlas->SetData(rgbaPixels.data(), rgbaPixels.size());

    float invWidth = 1.0f / font.atlasWidth;
    float invHeight = 1.0f / font.atlasHeight;

    for (int i = 0; i < 128; i++)
    {
        font.chars[i].x0 = chardata[i].x0 * invWidth;
        font.chars[i].y0 = chardata[i].y0 * invHeight;
        font.chars[i].x1 = chardata[i].x1 * invWidth;
        font.chars[i].y1 = chardata[i].y1 * invHeight;
        font.chars[i].xoff = chardata[i].xoff;
        font.chars[i].yoff = chardata[i].yoff;
        font.chars[i].xadvance = chardata[i].xadvance;
    }

    std::static_pointer_cast<FontAsset>(asset)->SetFont(font);

    // Fallback to NativeHandle just for logging
    uint32_t handleId = font.textureAtlas ? font.textureAtlas->GetNativeHandle() : 0;
    CH_CORE_INFO("FontLoader: Imported font atlas for {} (ID={})", resolvedPath, handleId);

    return true;
}
} // namespace Chained