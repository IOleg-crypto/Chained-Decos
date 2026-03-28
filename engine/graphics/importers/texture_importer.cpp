#include "engine/graphics/importers/texture_importer.h"
#include "engine/core/log.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#include <algorithm>
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<TextureAsset> TextureImporter::ImportTexture(const std::filesystem::path& path)
{
    CH_CORE_INFO("TextureImporter: Importing texture from {}", path.string());

    auto asset = std::make_shared<TextureAsset>();
    asset->SetPath(path.string());

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    bool isHDR = stbi_is_hdr(path.string().c_str());
    void* data = nullptr;

    if (isHDR)
    {
        data = stbi_loadf(path.string().c_str(), &width, &height, &channels, 0);
    }
    else
    {
        data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
        channels = 4; // We requested 4 channels
    }
    
    if (data == nullptr)
    {
        CH_CORE_ERROR("TextureImporter: Failed to load image {}", path.string());
        asset->SetState(AssetState::Failed);
        return asset;
    }

    RawImage rawImage;
    rawImage.data = data;
    rawImage.width = width;
    rawImage.height = height;
    rawImage.channels = channels;
    rawImage.isHDR = isHDR;
    rawImage.format = isHDR ? 11 : 7; // Keep compatibility with existing format definitions
    rawImage.mipmaps = 1;

    asset->SetPendingImage(rawImage);
    asset->UploadToGPU();

    return asset;
}

RawImage TextureImporter::LoadImageFromDisk(const std::filesystem::path& path)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    
    bool isHDR = stbi_is_hdr(path.string().c_str());
    void* data = nullptr;

    if (isHDR)
        data = stbi_loadf(path.string().c_str(), &width, &height, &channels, 0);
    else
    {
        data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
        channels = 4;
    }
    
    RawImage rawImage;
    rawImage.data = data;
    rawImage.width = width;
    rawImage.height = height;
    rawImage.channels = channels;
    rawImage.isHDR = isHDR;
    rawImage.format = isHDR ? 11 : 7;
    rawImage.mipmaps = 1;
    return rawImage;
}
} // namespace CHEngine
