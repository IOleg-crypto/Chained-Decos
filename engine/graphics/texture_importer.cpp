#include "texture_importer.h"
#include "engine/core/log.h"
#include "raylib.h"

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

        // Ensure we always have RGBA for consistency and GLFW/Raylib compatibility
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
}
