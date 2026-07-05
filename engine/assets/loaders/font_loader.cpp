#define STB_TRUETYPE_IMPLEMENTATION
#include "engine/assets/loaders/font_loader.h"
#include "engine/assets/types/font_asset.h"
#include "engine/core/log.h"
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
    std::vector<unsigned char> buffer(size);

    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        return fail("failed to read font file '" + resolvedPath + "'");
    }

    NativeFont font;
    font.fontSize = 24.0f;
    font.atlasWidth = 512;
    font.atlasHeight = 512;

    std::vector<unsigned char> pixels(font.atlasWidth * font.atlasHeight);
    stbtt_bakedchar chardata[128];

    stbtt_BakeFontBitmap(buffer.data(), 0, font.fontSize, pixels.data(), font.atlasWidth, font.atlasHeight, 32, 128,
                         chardata);

    
    glGenTextures(1, &font.textureId);
    glBindTexture(GL_TEXTURE_2D, font.textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font.atlasWidth, font.atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE,
                 pixels.data());

    GLint swizzleMask[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    
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
    CH_CORE_INFO("FontLoader: Imported font atlas for {} (ID={})", resolvedPath, font.textureId);

    return true;
}
} // namespace Chained