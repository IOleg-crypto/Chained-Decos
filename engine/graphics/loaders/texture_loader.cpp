#include "engine/graphics/loaders/texture_loader.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/graphics/api/texture.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "stb_image.h"
#include <filesystem>
#include <algorithm>

namespace CHEngine
{
    std::shared_ptr<Asset> TextureLoader::Create() const
    {
        return std::make_shared<TextureAsset>();
    }

    bool TextureLoader::Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError)
    {
        auto texAsset = std::dynamic_pointer_cast<TextureAsset>(asset);
        if (!texAsset)
        {
            if (outError) *outError = "TextureLoader: Invalid asset type";
            return false;
        }

        if (ctx.ResolvedPath.empty())
        {
            if (outError) *outError = "TextureLoader: empty path";
            return false;
        }

        int width, height, channels;
        bool isHDR = stbi_is_hdr(ctx.ResolvedPath.c_str());
        stbi_set_flip_vertically_on_load(!isHDR);

        void* data = nullptr;
        if (isHDR)
        {
            data = stbi_loadf(ctx.ResolvedPath.c_str(), &width, &height, &channels, 0);
        }
        else
        {
            data = stbi_load(ctx.ResolvedPath.c_str(), &width, &height, &channels, 4);
            channels = 4;
        }

        if (data == nullptr)
        {
            const char* reason = stbi_failure_reason();
            CH_CORE_ERROR("TextureLoader: Failed to load image {}. Reason: {}", ctx.ResolvedPath, reason ? reason : "Unknown");
            if (outError)
            {
                *outError = "TextureLoader: failed to load image '" + ctx.ResolvedPath + "'. Reason: " + (reason ? reason : "Unknown");
            }
            return false;
        }

        texAsset->SetIsHDR(isHDR);

        texAsset->SetRawData(data, width, height, channels, isHDR);
        return true;
    }

    void TextureLoader::Finalize(std::shared_ptr<TextureAsset> asset)
    {
        CH_PROFILE_FUNCTION();

        if (asset->GetState() == AssetState::Ready || asset->GetState() == AssetState::Failed)
        {
            return;
        }

        if (asset->m_HasPendingImage && asset->m_RawData != nullptr)
        {
            TextureFormat format = TextureFormat::RGBA8;
            if (asset->m_IsHDR)
            {
                format = (asset->m_RawChannels == 3) ? TextureFormat::RGB16F : TextureFormat::RGBA16F;
            }
            else
            {
                format = (asset->m_RawChannels == 3) ? TextureFormat::RGB8 : TextureFormat::RGBA8;
            }

            if (asset->m_IsCubemap)
            {
                asset->m_Texture = Texture::CreateCubemap(asset->m_RawWidth, format);
            }
            else
            {
                asset->m_Texture = Texture::Create(asset->m_RawWidth, asset->m_RawHeight, format);
                if (asset->m_Texture)
                {
                    asset->m_Texture->SetData(asset->m_RawData, 0); 
                }
            }

            if (asset->m_RawData != nullptr)
            {
                stbi_image_free(asset->m_RawData);
                asset->m_RawData = nullptr;
            }
            asset->m_HasPendingImage = false;

            asset->SetState(AssetState::Ready);
        }
    }
} // namespace CHEngine
