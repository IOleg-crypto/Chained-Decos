#include "engine/graphics/loaders/texture_loader.h"
#include "engine/core/log.h"

#include "stb_image.h"
#include <filesystem>
#include <algorithm>

namespace CHEngine
{
    std::shared_ptr<Asset> TextureLoader::Create()
    {
        return std::make_shared<TextureAsset>();
    }

    bool TextureLoader::Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError)
    {
        auto texAsset = std::static_pointer_cast<TextureAsset>(asset);

        if (resolvedPath.empty())
        {
            if (outError)
            {
                *outError = "TextureLoader: empty path";
            }
            return false;
        }
        
        std::string ext = std::filesystem::path(resolvedPath).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        // Handle HDR khusus (procedural or direct load on main thread can be handled here if needed)
        // For now, we follow the logic from the previous Importer
        
        int width, height, channels;

        bool isHDR = stbi_is_hdr(resolvedPath.c_str());
        stbi_set_flip_vertically_on_load(!isHDR);

        void* data = nullptr;

        if (isHDR)
        {
            data = stbi_loadf(resolvedPath.c_str(), &width, &height, &channels, 0);
        }
        else
        {
            data = stbi_load(resolvedPath.c_str(), &width, &height, &channels, 4);
            channels = 4;
        }

        if (data == nullptr)
        {
            const char* reason = stbi_failure_reason();
            CH_CORE_ERROR("TextureLoader: Failed to load image {}. Reason: {}", resolvedPath, reason ? reason : "Unknown");
            if (outError)
            {
                *outError = "TextureLoader: failed to load image '" + resolvedPath + "'. Reason: " + (reason ? reason : "Unknown");
            }
            return false;
        }

        texAsset->SetIsHDR(isHDR);

        RawImage rawImage;
        rawImage.data = data;
        rawImage.width = width;
        rawImage.height = height;
        rawImage.channels = channels;
        rawImage.isHDR = isHDR;
        rawImage.format = isHDR ? 11 : 7; // Matching previous constants
        rawImage.mipmaps = 1;

        texAsset->SetPendingImage(rawImage);
        return true;
    }
} // namespace CHEngine
