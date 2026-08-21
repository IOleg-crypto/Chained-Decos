#include "engine/graphics/api/texture.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/api/opengl/gl_texture.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/core/service_locator.h"

namespace Chained
{

	std::shared_ptr<Texture> Texture::Create(uint32_t width, uint32_t height, TextureFormat format)
	{
		switch (GraphicsDevice::GetAPI())
		{
		case GraphicsDevice::API::None:
			return nullptr;
		case GraphicsDevice::API::OpenGL:
			return std::make_shared<GLTexture>(width, height, format);
		}
		return nullptr;
	}

	std::shared_ptr<Texture> Texture::CreateCubemap(uint32_t size, TextureFormat format)
	{
		switch (GraphicsDevice::GetAPI())
		{
		case GraphicsDevice::API::None:
			return nullptr;
		case GraphicsDevice::API::OpenGL:
			return std::make_shared<GLTexture>(size, format);
		}
		return nullptr;
	}

	std::shared_ptr<Texture> Texture::CreateFromFile(const std::string& path)
	{
		auto* am = ServiceLocator::TryGet<AssetManager>();
		if (!am)
		{
			return nullptr;
		}

		auto texAsset = am->Get<TextureAsset>(path);
		if (texAsset && texAsset->GetTexture())
		{
			return texAsset->GetTexture();
		}
		return nullptr;
	}

} // namespace Chained
namespace Chained
{
	std::shared_ptr<Texture> Texture::WrapNative(uint32_t handle, uint32_t width, uint32_t height)
	{
		return std::make_shared<GLTexture>(handle, width, height);
	}
} // namespace Chained
