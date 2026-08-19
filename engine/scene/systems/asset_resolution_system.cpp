#include "asset_resolution_system.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/material_asset.h"
#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/shader_asset.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/scene/components/render/model_component.h"
#include "engine/scene/components/render/shader_component.h"
#include "engine/scene/components/render/sprite_component.h"

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

	void RegisterObservers(entt::registry& reg)
	{
		reg.on_construct<SpriteComponent>().connect<&ResolveSprite>();
		reg.on_update<SpriteComponent>().connect<&ResolveSprite>();

		reg.on_construct<ShaderComponent>().connect<&ResolveShader>();
		reg.on_update<ShaderComponent>().connect<&ResolveShader>();

		reg.on_construct<ModelComponent>().connect<&ResolveModel>();
		reg.on_update<ModelComponent>().connect<&ResolveModel>();
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
	}

} // namespace Chained::AssetResolutionSystem
