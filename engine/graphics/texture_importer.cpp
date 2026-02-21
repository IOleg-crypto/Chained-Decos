#include "texture_importer.h"
#include "engine/core/log.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<TextureAsset> TextureImporter::ImportTexture(const std::filesystem::path& path)
{
    CH_CORE_INFO("TextureImporter: Importing texture from {}", path.string());

    auto asset = std::make_shared<TextureAsset>();
    asset->SetPath(path.string());

    Image image = LoadImage(path.string().c_str());
    if (image.data == nullptr)
    {
        CH_CORE_ERROR("TextureImporter: Failed to load image {}", path.string());
        asset->SetState(AssetState::Failed);
        return asset;
    }

    // HDR images: apply tone mapping on CPU, then convert to RGBA8
    bool isHDR = (image.format == PIXELFORMAT_UNCOMPRESSED_R32G32B32 ||
                  image.format == PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
    if (isHDR)
    {
        int channels = (image.format == PIXELFORMAT_UNCOMPRESSED_R32G32B32) ? 3 : 4;
        int pixelCount = image.width * image.height;
        float* pixels = (float*)image.data;
        for (int i = 0; i < pixelCount; i++)
        {
            for (int c = 0; c < 3; c++)
            {
                float v = pixels[i * channels + c];
                float mapped = (v * (2.51f * v + 0.03f)) /
                               (v * (2.43f * v + 0.59f) + 0.14f);
                if (mapped < 0.0f) mapped = 0.0f;
                if (mapped > 1.0f) mapped = 1.0f;
                mapped = powf(mapped, 1.0f / 2.2f);
                pixels[i * channels + c] = mapped;
            }
        }
    }
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);

    if (texture.id == 0)
    {
        CH_CORE_ERROR("TextureImporter: Failed to create GPU texture from {}", path.string());
        asset->SetState(AssetState::Failed);
        return asset;
    }

    asset->SetTexture(texture);
    asset->SetState(AssetState::Ready);
    return asset;
}

Image TextureImporter::LoadImageFromDisk(const std::filesystem::path& path)
{
    return LoadImage(path.string().c_str());
}
} // namespace CHEngine
