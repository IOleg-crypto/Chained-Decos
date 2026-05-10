#include "asset_resolution_system.h"
#include "engine/scene/scene.h"
#include "engine/scene/components/sprite_component.h"
#include "engine/scene/components/shader_component.h"
#include "engine/scene/components/mesh_component.h"
#include "engine/scene/components/component_utils.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/graphics/assets/shader_asset.h"

namespace CHEngine
{

void AssetResolutionSystem::RegisterObservers(entt::registry& reg)
{
    reg.on_construct<SpriteComponent>().connect<&AssetResolutionSystem::OnSpriteChanged>(this);
    reg.on_update<SpriteComponent>().connect<&AssetResolutionSystem::OnSpriteChanged>(this);

    reg.on_construct<ShaderComponent>().connect<&AssetResolutionSystem::OnShaderChanged>(this);
    reg.on_update<ShaderComponent>().connect<&AssetResolutionSystem::OnShaderChanged>(this);

    reg.on_construct<ModelComponent>().connect<&AssetResolutionSystem::OnModelChanged>(this);
    reg.on_update<ModelComponent>().connect<&AssetResolutionSystem::OnModelChanged>(this);
}

void AssetResolutionSystem::OnUpdate(Scene* scene, Timestep ts)
{
    auto& reg = scene->GetRegistry();

    // Re-attempt resolution for assets that weren't ready during construction/update events.
    // This handles asynchronous asset loading gracefully.

    // 1. Sprites
    reg.view<SpriteComponent>().each([&](auto entity, auto& sprite) {
        if (!sprite.TexturePath.empty() && sprite.TextureHandle == 0)
        {
            OnSpriteChanged(reg, entity);
        }
    });

    // 2. Shaders
    reg.view<ShaderComponent>().each([&](auto entity, auto& shader) {
        if (!shader.ShaderPath.empty() && shader.ShaderHandle == 0)
        {
            OnShaderChanged(reg, entity);
        }
    });

    // 3. Models
    reg.view<ModelComponent>().each([&](auto entity, auto& model) {
        if (!model.ModelPath.empty() && (model.ModelHandle == 0 || !model.MaterialsInitialized))
        {
            OnModelChanged(reg, entity);
        }
    });
}

void AssetResolutionSystem::OnSpriteChanged(entt::registry& reg, entt::entity e)
{
    auto& sprite = reg.get<SpriteComponent>(e);
    if (!sprite.TexturePath.empty() && sprite.TextureHandle == 0)
    {
        auto& assetManager = ServiceLocator::Get<AssetManager>();
        auto handle = assetManager.ResolveToHandle(sprite.TexturePath, TextureAsset::GetStaticType());
        auto asset = assetManager.Get<TextureAsset>(handle);
        if (asset && asset->IsReady())
        {
            sprite.TextureHandle = asset->GetID();
        }
    }
}

void AssetResolutionSystem::OnShaderChanged(entt::registry& reg, entt::entity e)
{
    auto& shader = reg.get<ShaderComponent>(e);
    if (!shader.ShaderPath.empty() && shader.ShaderHandle == 0)
    {
        auto& assetManager = ServiceLocator::Get<AssetManager>();
        auto handle = assetManager.ResolveToHandle(shader.ShaderPath, ShaderAsset::GetStaticType());
        auto asset = assetManager.Get<ShaderAsset>(handle);
        if (asset && asset->IsReady())
        {
            shader.ShaderHandle = asset->GetID();

            // Sync uniforms from the asset (.chshader metadata)
            const auto& assetUniforms = asset->GetUniforms();
            for (const auto& u : assetUniforms)
            {
                auto it = std::find_if(shader.Uniforms.begin(), shader.Uniforms.end(),
                                       [&](const auto& current) { return current.Name == u.Name; });
                if (it == shader.Uniforms.end())
                {
                    shader.Uniforms.push_back(u);
                }
            }
        }
    }
}

void AssetResolutionSystem::OnModelChanged(entt::registry& reg, entt::entity e)
{
    auto& model = reg.get<ModelComponent>(e);
    ComponentUtils::ResolveModelPath(model);
}

} // namespace CHEngine
