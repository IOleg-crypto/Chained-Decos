#define STB_TRUETYPE_IMPLEMENTATION
#include "engine/graphics/loaders/font_loader.h"
#include "engine/core/log.h"
#include <filesystem>
#include <fstream>
#include <glad/gl.h>
#include <stb_truetype.h>
#include <vector>

namespace CHEngine
{
std::shared_ptr<Asset> FontLoader::Create() const
{
    return std::make_shared<FontAsset>();
}

bool FontLoader::Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError)
{
    auto fontAsset = std::dynamic_pointer_cast<FontAsset>(asset);
    if (!fontAsset)
    {
        if (outError)
        {
            *outError = "FontLoader: Invalid asset type";
        }
        return false;
    }

    if (ctx.ResolvedPath.empty())
    {
        if (outError)
        {
            *outError = "FontLoader: empty path";
        }
        return false;
    }

    std::filesystem::path fullPath(ctx.ResolvedPath);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("FontLoader: File not found: {}", ctx.ResolvedPath);
        if (outError)
        {
            *outError = "FontLoader: file not found '" + ctx.ResolvedPath + "'";
        }
        return false;
    }

    // Read file into buffer
    std::ifstream file(ctx.ResolvedPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        if (outError)
        {
            *outError = "FontLoader: failed to open font file '" + ctx.ResolvedPath + "'";
        }
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(size);
    if (!file.read((char*)buffer.data(), size))
    {
        CH_CORE_ERROR("FontLoader: Failed to read font file: {}", ctx.ResolvedPath);
        if (outError)
        {
            *outError = "FontLoader: failed to read font file '" + ctx.ResolvedPath + "'";
        }
        return false;
    }

    Font font;
    font.fontSize = 32.0f;
    font.atlasWidth = 512;
    font.atlasHeight = 512;

    std::vector<unsigned char> pixels(font.atlasWidth * font.atlasHeight);
    stbtt_bakedchar chardata[128];

    stbtt_BakeFontBitmap(buffer.data(), 0, font.fontSize, pixels.data(), font.atlasWidth, font.atlasHeight, 32, 128,
                         chardata);

    // Convert to GL texture
    glGenTextures(1, &font.textureId);
    glBindTexture(GL_TEXTURE_2D, font.textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font.atlasWidth, font.atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE,
                 pixels.data());

    GLint swizzleMask[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    for (int i = 0; i < 128; i++)
    {
        font.chars[i].x0 = (float)chardata[i].x0 / (float)font.atlasWidth;
        font.chars[i].y0 = (float)chardata[i].y0 / (float)font.atlasHeight;
        font.chars[i].x1 = (float)chardata[i].x1 / (float)font.atlasWidth;
        font.chars[i].y1 = (float)chardata[i].y1 / (float)font.atlasHeight;
        font.chars[i].xoff = chardata[i].xoff;
        font.chars[i].yoff = chardata[i].yoff;
        font.chars[i].xadvance = chardata[i].xadvance;
    }

    fontAsset->SetFont(font);
    CH_CORE_INFO("FontLoader: Imported font atlas for {} (ID={})", ctx.ResolvedPath, font.textureId);

    return true;
}
} // namespace CHEngine
