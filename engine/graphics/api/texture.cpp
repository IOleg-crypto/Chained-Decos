#include "texture.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/api/renderer_api.h"
#include "engine/graphics/api/opengl/opengl_texture.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/texture_asset.h"


namespace Chained
{

std::shared_ptr<Texture> Texture::Create(uint32_t width, uint32_t height, TextureFormat format)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture>(width, height, format);
    }
    return nullptr;
}

std::shared_ptr<Texture> Texture::CreateCubemap(uint32_t size, TextureFormat format)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture>(size, format);
    }
    return nullptr;
}

std::shared_ptr<Texture> Texture::CreateFromFile(const std::string& path)
{
    auto handle = ServiceLocator::Get<AssetManager>()->ImportAsset(path);
    auto asset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>(handle);
    return asset ? asset->GetTexture() : nullptr;
}

} // namespace Chained
