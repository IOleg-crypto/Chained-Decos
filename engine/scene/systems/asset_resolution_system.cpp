#include "asset_resolution_system.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/scene/components/component_utils.h"
#include "engine/scene/components/model_component.h"
#include "engine/scene/components/primitive_component.h"
#include "engine/scene/components/primitive_runtime.h"
#include "engine/scene/components/shader_component.h"
#include "engine/scene/components/sprite_component.h"

namespace Chained::AssetResolutionSystem
{

static void ResolveSprite(entt::registry& reg, entt::entity e)
{
    auto& sprite = reg.get<SpriteComponent>(e);
    if (sprite.TextureHandle != 0)
    {
        return;
    }

    auto* assets = ServiceLocator::TryGet<AssetManager>();
    if (!assets)
    {
        return;
    }

    // Try UUID first
    if (sprite.TextureUUID != 0)
    {
        auto asset = assets->GetByUUID<TextureAsset>(sprite.TextureUUID);
        if (asset && asset->IsReady())
        {
            sprite.TextureHandle = asset->GetID();
            return;
        }
    }

    // Fallback to path
    if (!sprite.TexturePath.empty())
    {
        assets->LoadAsset(sprite.TexturePath, TextureAsset::GetStaticType());
        auto asset = assets->Get<TextureAsset>(sprite.TexturePath);
        if (asset && asset->IsReady())
        {
            sprite.TextureHandle = asset->GetID();
            sprite.TextureUUID = asset->GetID();
        }
    }
}

static void ResolveShader(entt::registry& reg, entt::entity e)
{
    auto& shader = reg.get<ShaderComponent>(e);
    if (shader.ShaderHandle != 0)
    {
        return;
    }

    auto* assets = ServiceLocator::TryGet<AssetManager>();
    if (!assets)
    {
        return;
    }

    // Try UUID first
    if (shader.ShaderUUID != 0)
    {
        auto asset = assets->GetByUUID<ShaderAsset>(shader.ShaderUUID);
        if (asset && asset->IsReady())
        {
            shader.ShaderHandle = asset->GetID();
            return;
        }
    }

    // Fallback to path
    if (!shader.ShaderPath.empty())
    {
        assets->LoadAsset(shader.ShaderPath, ShaderAsset::GetStaticType());
        auto asset = assets->Get<ShaderAsset>(shader.ShaderPath);
        if (asset && asset->IsReady())
        {
            shader.ShaderHandle = asset->GetID();
            shader.ShaderUUID = asset->GetID();
        }
    }
}

static void ResolveModel(entt::registry& reg, entt::entity e)
{
    auto& model = reg.get<ModelComponent>(e);

    // Already resolved
    if (model.ModelHandle != 0)
    {
        return;
    }

    auto* assets = ServiceLocator::TryGet<AssetManager>();
    if (!assets)
    {
        return;
    }

    // Try UUID first
    if (model.ModelUUID != 0)
    {
        auto asset = assets->GetByUUID<ModelAsset>(model.ModelUUID);
        if (asset && asset->IsReady())
        {
            model.ModelHandle = asset->GetID();
            return;
        }
    }

    // Fallback to path
    ComponentUtils::ResolveModelPath(model);

    // Remember UUID for next time
    if (model.ModelHandle != 0 && model.ModelUUID == 0)
    {
        auto asset = assets->Get<ModelAsset>(model.ModelPath);
        if (asset)
        {
            model.ModelUUID = asset->GetID();
        }
    }
}

static void MarkPrimitiveDirty(entt::registry& reg, entt::entity e)
{
    reg.get_or_emplace<PrimitiveRuntimeState>(e).Dirty = true;
}

static void ResolvePrimitive(entt::registry& reg, entt::entity e)
{
    auto& prim = reg.get<PrimitiveComponent>(e);
    auto& rt = reg.get_or_emplace<PrimitiveRuntimeState>(e);

    const char* typeMarker = nullptr;
    switch (prim.Type)
    {
    case PrimitiveType::Cube:
        typeMarker = ":cube:";
        break;
    case PrimitiveType::Sphere:
        typeMarker = ":sphere:";
        break;
    case PrimitiveType::Plane:
        typeMarker = ":plane:";
        break;
    case PrimitiveType::Cylinder:
        typeMarker = ":cylinder:";
        break;
    case PrimitiveType::Cone:
        typeMarker = ":cone:";
        break;
    case PrimitiveType::Torus:
        typeMarker = ":torus:";
        break;
    case PrimitiveType::Knot:
        typeMarker = ":knot:";
        break;
    case PrimitiveType::Hemisphere:
        typeMarker = ":hemisphere:";
        break;
    case PrimitiveType::None:
    default:
        rt.Dirty = false;
        return;
    }

    ProceduralParameters params;
    params.Radius = prim.Radius;
    params.InnerRadius = prim.InnerRadius;
    params.Height = prim.Height;
    params.Slices = prim.Slices;
    params.Stacks = prim.Stacks;
    params.Dimensions = prim.Dimensions;

    PendingModelData data = GeometryGenerator::GeneratePrimitivePendingData(typeMarker, params);
    if (!data.isValid)
    {
        rt.Dirty = false;
        return;
    }

    bool hadAsset = (rt.Asset != nullptr);
    std::vector<Material> editedMaterials;
    if (hadAsset)
    {
        editedMaterials = rt.Asset->GetMaterials();
    }

    if (!rt.Asset)
    {
        rt.Asset = std::make_shared<ModelAsset>();
    }
    rt.Asset->SetPendingData(std::move(data));
    rt.Asset->OnLoaded();

    auto resolveTexture = [](AssetManager* assets, const std::string& path, std::shared_ptr<Texture>& outTex) -> bool {
        if (path.empty())
        {
            return false;
        }
        assets->LoadAsset(path, TextureAsset::GetStaticType());
        auto texAsset = assets->Get<TextureAsset>(path);
        if (texAsset && texAsset->IsReady())
        {
            outTex = texAsset->GetTexture();
            return false;
        }
        return true;
    };

    auto applyMaterialTextures = [&](Material& mat) -> bool {
        bool anyPending = false;
        if (auto* assets = ServiceLocator::TryGet<AssetManager>())
        {
            anyPending |= resolveTexture(assets, mat.AlbedoPath, mat.AlbedoMap);
            anyPending |= resolveTexture(assets, mat.NormalPath, mat.NormalMap);
            anyPending |= resolveTexture(assets, mat.MetallicRoughnessPath, mat.MetallicRoughnessMap);
            anyPending |= resolveTexture(assets, mat.EmissivePath, mat.EmissiveMap);
        }
        return anyPending;
    };

    auto& regenerated = rt.Asset->GetMaterials();
    bool anyPending = false;

    if (!editedMaterials.empty())
    {
        for (size_t i = 0; i < regenerated.size() && i < editedMaterials.size(); ++i)
        {
            anyPending |= applyMaterialTextures(editedMaterials[i]);
            regenerated[i] = editedMaterials[i];
        }
        if (!regenerated.empty())
        {
            prim.SetMaterial(regenerated[0]);
        }
    }
    else
    {
        if (!regenerated.empty())
        {
            Material mat = prim.GetMaterial();
            anyPending = applyMaterialTextures(mat);
            regenerated[0] = mat;
        }
    }

    if (anyPending)
    {
        rt.TexturesPending = true;
        rt.Dirty = false;
        return;
    }

    rt.TexturesPending = false;
    rt.Dirty = false;
}

static void ApplyPrimitiveTextures(entt::registry& reg, entt::entity e)
{
    auto* rt = reg.try_get<PrimitiveRuntimeState>(e);
    auto* prim = reg.try_get<PrimitiveComponent>(e);
    if (!rt || !rt->Asset || !prim)
    {
        return;
    }

    auto& mats = rt->Asset->GetMaterials();
    if (mats.empty())
    {
        return;
    }

    auto* assets = ServiceLocator::TryGet<AssetManager>();
    if (!assets)
    {
        return;
    }

    Material mat = mats[0];
    bool anyPending = false;

    auto resolveTexture = [&](const std::string& path, std::shared_ptr<Texture>& outTex) -> bool {
        if (path.empty())
        {
            return false;
        }
        assets->LoadAsset(path, TextureAsset::GetStaticType());
        auto texAsset = assets->Get<TextureAsset>(path);
        if (texAsset && texAsset->IsReady())
        {
            outTex = texAsset->GetTexture();
            return false;
        }
        return true;
    };

    anyPending |= resolveTexture(mat.AlbedoPath, mat.AlbedoMap);
    anyPending |= resolveTexture(mat.NormalPath, mat.NormalMap);
    anyPending |= resolveTexture(mat.MetallicRoughnessPath, mat.MetallicRoughnessMap);
    anyPending |= resolveTexture(mat.EmissivePath, mat.EmissiveMap);

    mats[0] = mat;
    rt->TexturesPending = anyPending;
}

void RegisterObservers(entt::registry& reg)
{
    reg.on_construct<SpriteComponent>().connect<&ResolveSprite>();
    reg.on_update<SpriteComponent>().connect<&ResolveSprite>();

    reg.on_construct<ShaderComponent>().connect<&ResolveShader>();
    reg.on_update<ShaderComponent>().connect<&ResolveShader>();

    reg.on_construct<PrimitiveComponent>().connect<&MarkPrimitiveDirty>();
    reg.on_update<PrimitiveComponent>().connect<&MarkPrimitiveDirty>();
}

void Update(entt::registry& reg)
{
    CH_PROFILE_FUNCTION();

    reg.view<SpriteComponent>().each([&](auto entity, auto& sprite) {
        if (sprite.TextureHandle == 0 && (sprite.TextureUUID != 0 || !sprite.TexturePath.empty()))
        {
            ResolveSprite(reg, entity);
        }
    });

    reg.view<ShaderComponent>().each([&](auto entity, auto& shader) {
        if (shader.ShaderHandle == 0 && (shader.ShaderUUID != 0 || !shader.ShaderPath.empty()))
        {
            ResolveShader(reg, entity);
        }
    });

    reg.view<ModelComponent>().each([&](auto entity, auto& model) {
        if (model.ModelHandle == 0 && (model.ModelUUID != 0 || !model.ModelPath.empty()))
        {
            ResolveModel(reg, entity);
        }
    });

    reg.view<PrimitiveComponent>().each([&](auto entity, auto& prim) {
        auto& rt = reg.get_or_emplace<PrimitiveRuntimeState>(entity);
        if (prim.Type != PrimitiveType::None && (!rt.Asset || rt.Dirty))
        {
            ResolvePrimitive(reg, entity);
        }
        else if (rt.TexturesPending && rt.Asset)
        {
            ApplyPrimitiveTextures(reg, entity);
        }
    });
}

} // namespace Chained::AssetResolutionSystem
