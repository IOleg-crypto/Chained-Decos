#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include "font_importer.h"
#include "engine/graphics/assets/font_asset.h"
#include "engine/core/log.h"
#include <fstream>
#include <vector>
#include <glad/gl.h>
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<FontAsset> FontImporter::ImportFont(const std::string& path)
{
    if (path.empty()) return nullptr;

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("FontImporter: File not found: {}", path);
        return nullptr;
    }

    // Read file into buffer
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return nullptr;
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(size);
    if (!file.read((char*)buffer.data(), size)) {
        CH_CORE_ERROR("FontImporter: Failed to read font file: {}", path);
        return nullptr;
    }

    auto asset = std::make_shared<FontAsset>();
    asset->SetPath(path);
    
    NativeFont font;
    font.fontSize = 32.0f;
    font.atlasWidth = 512;
    font.atlasHeight = 512;

    std::vector<unsigned char> pixels(font.atlasWidth * font.atlasHeight);
    stbtt_bakedchar chardata[128]; 
    
    stbtt_BakeFontBitmap(buffer.data(), 0, font.fontSize, pixels.data(), font.atlasWidth, font.atlasHeight, 32, 128, chardata);

    // Convert to GL texture
    glGenTextures(1, &font.textureId);
    glBindTexture(GL_TEXTURE_2D, font.textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font.atlasWidth, font.atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
    
    // GL_RED texture needs swizzling or shader adjustment to behave like white texture with alpha
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

    asset->SetFont(font);
    asset->SetState(AssetState::Ready);

    CH_CORE_INFO("FontImporter: Imported font atlas for {} (ID={})", path, font.textureId);

    return asset;
}
} // namespace CHEngine
