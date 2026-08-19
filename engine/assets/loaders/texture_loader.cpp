#include "engine/assets/loaders/texture_loader.h"

#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/texture_asset.h"

#include "stb_image_impl.h"

namespace Chained
{
	namespace
	{
		void FlipImageVertically(void* pixels, int width, int height, int channels, size_t bytesPerChannel)
		{
			if (pixels == nullptr || width <= 0 || height <= 0 || channels <= 0)
			{
				return;
			}

			const size_t rowBytes = static_cast<size_t>(width) * static_cast<size_t>(channels) * bytesPerChannel;
			std::vector<unsigned char> temp(rowBytes);
			auto* bytes = static_cast<unsigned char*>(pixels);

			for (int row = 0; row < height / 2; ++row)
			{
				const size_t topOffset = static_cast<size_t>(row) * rowBytes;
				const size_t bottomOffset = static_cast<size_t>(height - row - 1) * rowBytes;

				std::memcpy(temp.data(), bytes + topOffset, rowBytes);
				std::memcpy(bytes + topOffset, bytes + bottomOffset, rowBytes);
				std::memcpy(bytes + bottomOffset, temp.data(), rowBytes);
			}
		}
	} // namespace

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

		int width, height, channels;

		// Check if we should read from pack or filesystem
		auto* assetManager = ServiceLocator::TryGet<AssetManager>();
		bool usePack = assetManager && assetManager->IsPacked();

		// Determine if HDR by checking file header or extension.
		// This is intentionally per-asset and not a single global rule: UI and editor
		// textures often need their source pixels kept in the original orientation,
		// while scene/game textures are typically loaded to OpenGL's bottom-left UV convention.
		bool isHDR = (ext == ".hdr" || ext == ".exr");

		const auto isUiLikePath = [](const std::string& path) {
			const std::string lower = [&]() {
				std::string result = path;
				std::transform(result.begin(), result.end(), result.begin(), ::tolower);
				return result;
			}();

			return lower.find("/ui/") != std::string::npos || lower.find("\\ui\\") != std::string::npos ||
				   lower.find("/icons/") != std::string::npos || lower.find("\\icons\\") != std::string::npos ||
				   lower.find("/font/") != std::string::npos || lower.find("\\font\\") != std::string::npos ||
				   lower.find("logo") != std::string::npos || lower.find("menu") != std::string::npos;
		};

		const TextureUsage usage = isUiLikePath(resolvedPath) ? TextureUsage::UI : TextureUsage::Scene;
		texAsset->SetUsage(usage);

		const bool shouldFlipVertically = !isHDR && usage == TextureUsage::Scene &&
										  (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
										   ext == ".tga" || ext == ".gif" || ext == ".webp");
		texAsset->SetFlipYOnLoad(shouldFlipVertically);

		void* data = nullptr;

		if (usePack)
		{
			auto fileData = assetManager->ReadAssetData(resolvedPath);
			if (!fileData.empty())
			{
				if (isHDR)
				{

					data = stbi_loadf_from_memory(fileData.data(), (int)fileData.size(), &width, &height, &channels, 0);
				}
				else
				{
					data = stbi_load_from_memory(fileData.data(), (int)fileData.size(), &width, &height, &channels, 4);
					channels = 4;
				}
			}
		}
		else
		{
			isHDR = stbi_is_hdr(resolvedPath.c_str());
			const TextureUsage usage = isUiLikePath(resolvedPath) ? TextureUsage::UI : TextureUsage::Scene;
			texAsset->SetUsage(usage);

			const bool shouldFlipVertically = !isHDR && usage == TextureUsage::Scene &&
											  (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
											   ext == ".tga" || ext == ".gif" || ext == ".webp");
			texAsset->SetFlipYOnLoad(shouldFlipVertically);

			if (isHDR)
			{
				data = stbi_loadf(resolvedPath.c_str(), &width, &height, &channels, 0);
			}
			else
			{
				data = stbi_load(resolvedPath.c_str(), &width, &height, &channels, 4);
				channels = 4;
			}
		}

		if (shouldFlipVertically && data != nullptr)
		{
			if (isHDR)
			{
				FlipImageVertically(data, width, height, channels > 0 ? channels : 4, sizeof(float));
			}
			else
			{
				FlipImageVertically(data, width, height, channels > 0 ? channels : 4, sizeof(unsigned char));
			}
		}

		if (data == nullptr)
		{
			const char* reason = stbi_failure_reason();
			CH_CORE_ERROR("TextureLoader: Failed to load image {}. Reason: {}", resolvedPath,
						  reason ? reason : "Unknown");
			if (outError)
			{
				*outError = "TextureLoader: failed to load image '" + resolvedPath +
							"'. Reason: " + (reason ? reason : "Unknown");
			}
			return false;
		}

		texAsset->SetIsHDR(isHDR);

		DecodedImage rawImage;
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
} // namespace Chained