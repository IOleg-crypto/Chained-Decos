#include "asset_resolution_system.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/material_asset.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/graphics/api/texture.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/scene/components/model_component.h"
#include "engine/scene/components/primitive_component.h"
#include "engine/scene/components/primitive_runtime.h"
#include "engine/scene/components/shader_component.h"
#include "engine/scene/components/sprite_component.h"
#include <filesystem>

namespace Chained::AssetResolutionSystem
{

	static void ResolveSprite(entt::registry& reg, entt::entity e)
	{
		auto& sprite = reg.get<SpriteComponent>(e);

		auto* assets = ServiceLocator::TryGet<AssetManager>();
		if (!assets)
		{
			return;
		}

		if (sprite.TextureHandle != AssetHandle(0))
		{
			auto currentAsset = assets->Get<TextureAsset>(sprite.TextureHandle);
			if (currentAsset && currentAsset->IsReady())
			{
				return;
			}
		}

		if (sprite.TextureUUID != 0)
		{
			auto asset = assets->GetByUUID<TextureAsset>(sprite.TextureUUID);
			if (asset)
			{
				if (asset->IsReady())
				{
					sprite.TextureHandle = asset->GetID();
					if (sprite.TexturePath.empty())
					{
						sprite.TexturePath = asset->GetPath();
					}
				}
				return;
			}
		}

		if (!sprite.TexturePath.empty())
		{
			auto asset = assets->Get<TextureAsset>(sprite.TexturePath);
			if (asset)
			{
				sprite.TextureUUID = asset->GetID();
				if (asset->IsReady())
				{
					sprite.TextureHandle = asset->GetID();
				}
			}
		}
	}

	static void ResolveShader(entt::registry& reg, entt::entity e)
	{
		auto& shader = reg.get<ShaderComponent>(e);

		auto* assets = ServiceLocator::TryGet<AssetManager>();
		if (!assets)
		{
			return;
		}

		if (shader.ShaderHandle != AssetHandle(0))
		{
			auto currentAsset = assets->Get<ShaderAsset>(shader.ShaderHandle);
			if (currentAsset && currentAsset->IsReady())
			{
				return;
			}
		}

		if (shader.ShaderUUID != 0)
		{
			auto asset = assets->GetByUUID<ShaderAsset>(shader.ShaderUUID);
			if (asset)
			{
				if (asset->IsReady())
				{
					shader.ShaderHandle = asset->GetID();
					if (shader.ShaderPath.empty())
					{
						shader.ShaderPath = asset->GetPath();
					}
				}
				return;
			}
		}

		if (!shader.ShaderPath.empty())
		{
			auto asset = assets->Get<ShaderAsset>(shader.ShaderPath);
			if (asset)
			{
				shader.ShaderUUID = asset->GetID();
				if (asset->IsReady())
				{
					shader.ShaderHandle = asset->GetID();
				}
			}
		}
	}

	static void ResolveModel(entt::registry& reg, entt::entity e)
	{
		auto& model = reg.get<ModelComponent>(e);

		auto* assets = ServiceLocator::TryGet<AssetManager>();
		if (!assets)
		{
			return;
		}

		if (model.ModelHandle != AssetHandle(0))
		{
			auto currentAsset = assets->Get<ModelAsset>(model.ModelHandle);
			if (currentAsset && currentAsset->IsReady())
			{
				return;
			}
		}

		if (model.ModelUUID != 0)
		{
			auto asset = assets->GetByUUID<ModelAsset>(model.ModelUUID);
			if (asset)
			{
				if (asset->IsReady())
				{
					model.ModelHandle = asset->GetID();
					if (model.ModelPath.empty())
					{
						model.ModelPath = asset->GetPath();
					}
				}
				return;
			}
		}

		// Inline ResolveModelPath logic (was previously in ComponentUtils::ResolveModelPath)
		if (model.ModelPath.empty())
		{
			model.ModelHandle = AssetHandle(0);
			return;
		}

		auto asset = assets->Get<ModelAsset>(model.ModelPath);
		if (asset && asset->GetState() == AssetState::Ready)
		{
			model.ModelHandle = asset->GetID();

			const auto& materials = asset->GetMaterials();
			if (model.MaterialPaths.empty() || model.MaterialPaths.size() != materials.size())
			{
				std::filesystem::path modelPath(model.ModelPath);
				std::string modelName = modelPath.stem().string();
				std::filesystem::path modelDir = modelPath.parent_path();

				model.MaterialPaths.resize(materials.size());

				for (int i = 0; i < (int)materials.size(); i++)
				{
					std::string matFileName = modelName + "_material_" + std::to_string(i) + ".chmat";
					model.MaterialPaths[i] = (modelDir / matFileName).generic_string();

					std::string resolvedMatPath = assets->ResolvePath(model.MaterialPaths[i]);
					if (!std::filesystem::exists(resolvedMatPath))
					{
						auto matAsset = std::make_shared<MaterialAsset>();
						matAsset->SetMaterial(materials[i]);
						matAsset->SaveToFile(resolvedMatPath);
					}
				}
			}
		}
		else
		{
			model.ModelHandle = AssetHandle(0);
		}
	}

	static void MarkPrimitiveDirty(entt::registry& reg, entt::entity e)
	{
		reg.get_or_emplace<PrimitiveRuntimeState>(e).Dirty = true;
	}

	static bool ResolveTexture(AssetManager* assets, const std::string& path, std::shared_ptr<Texture>& outMap)
	{
		if (path.empty())
		{
			return false;
		}
		assets->LoadAsset(path, TextureAsset::GetStaticType());
		auto texAsset = assets->Get<TextureAsset>(path);
		if (texAsset && texAsset->IsReady())
		{
			outMap = texAsset->GetTexture();
			return false;
		}
		return true;
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

		auto applyMaterialTextures = [&](Material& mat) -> bool {
			bool anyPending = false;
			if (auto* assets = ServiceLocator::TryGet<AssetManager>())
			{
				anyPending |= ResolveTexture(assets, mat.AlbedoPath, mat.AlbedoMap);
				anyPending |= ResolveTexture(assets, mat.NormalPath, mat.NormalMap);
				anyPending |= ResolveTexture(assets, mat.MetallicRoughnessPath, mat.MetallicRoughnessMap);
				anyPending |= ResolveTexture(assets, mat.EmissivePath, mat.EmissiveMap);
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

		anyPending |= ResolveTexture(assets, mat.AlbedoPath, mat.AlbedoMap);
		anyPending |= ResolveTexture(assets, mat.NormalPath, mat.NormalMap);
		anyPending |= ResolveTexture(assets, mat.MetallicRoughnessPath, mat.MetallicRoughnessMap);
		anyPending |= ResolveTexture(assets, mat.EmissivePath, mat.EmissiveMap);

		mats[0] = mat;
		rt->TexturesPending = anyPending;
	}

	void RegisterObservers(entt::registry& reg)
	{
		reg.on_construct<SpriteComponent>().connect<&ResolveSprite>();
		reg.on_update<SpriteComponent>().connect<&ResolveSprite>();

		reg.on_construct<ShaderComponent>().connect<&ResolveShader>();
		reg.on_update<ShaderComponent>().connect<&ResolveShader>();

		reg.on_construct<ModelComponent>().connect<&ResolveModel>();
		reg.on_update<ModelComponent>().connect<&ResolveModel>();

		reg.on_construct<PrimitiveComponent>().connect<&MarkPrimitiveDirty>();
		reg.on_update<PrimitiveComponent>().connect<&MarkPrimitiveDirty>();
	}

	void Update(entt::registry& reg)
	{
		CH_PROFILE_FUNCTION();

		reg.view<SpriteComponent>().each([&](auto entity, auto& sprite) {
			if (sprite.TextureHandle == AssetHandle(0) && (sprite.TextureUUID != 0 || !sprite.TexturePath.empty()))
			{
				ResolveSprite(reg, entity);
			}
		});

		reg.view<ShaderComponent>().each([&](auto entity, auto& shader) {
			if (shader.ShaderHandle == AssetHandle(0) && (shader.ShaderUUID != 0 || !shader.ShaderPath.empty()))
			{
				ResolveShader(reg, entity);
			}
		});

		reg.view<ModelComponent>().each([&](auto entity, auto& model) {
			if (model.ModelHandle == AssetHandle(0) && (model.ModelUUID != 0 || !model.ModelPath.empty()))
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
