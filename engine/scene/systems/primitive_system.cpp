#include "primitive_system.h"

#include "engine/assets/asset_manager.h"
#include "engine/assets/loaders/model_loader.h"
#include "engine/assets/model_data.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/scene/components/core/id_component.h"
#include "engine/scene/components/core/tag_component.h"
#include "engine/scene/components/render/model_component.h"
#include "engine/scene/components/render/primitive_component.h"

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Chained::PrimitiveSystem
{

	// ---------------------------------------------------------------------------
	// Internal helpers
	// ---------------------------------------------------------------------------

	static std::string s_PrimitiveDir;

	/// Maps PrimitiveType enum → the marker string used by GeometryGenerator.
	static const char* TypeMarker(PrimitiveType t)
	{
		switch (t)
		{
		case PrimitiveType::Cube:
			return ":cube:";
		case PrimitiveType::Sphere:
			return ":sphere:";
		case PrimitiveType::Plane:
			return ":plane:";
		case PrimitiveType::Cylinder:
			return ":cylinder:";
		case PrimitiveType::Cone:
			return ":cone:";
		case PrimitiveType::Torus:
			return ":torus:";
		case PrimitiveType::Knot:
			return ":knot:";
		case PrimitiveType::Hemisphere:
			return ":hemisphere:";
		default:
			return nullptr;
		}
	}

	static std::string TypeName(PrimitiveType t)
	{
		switch (t)
		{
		case PrimitiveType::Cube:
			return "Cube";
		case PrimitiveType::Sphere:
			return "Sphere";
		case PrimitiveType::Plane:
			return "Plane";
		case PrimitiveType::Cylinder:
			return "Cylinder";
		case PrimitiveType::Cone:
			return "Cone";
		case PrimitiveType::Torus:
			return "Torus";
		case PrimitiveType::Knot:
			return "Knot";
		case PrimitiveType::Hemisphere:
			return "Hemisphere";
		default:
			return "Primitive";
		}
	}

	static std::string SanitizeName(const std::string& name)
	{
		std::string result;
		for (char c : name)
		{
			if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')
			{
				result += c;
			}
			else if (c == ' ' || c == '.' || c == ':')
			{
				result += '_';
			}
		}
		return result.empty() ? "Primitive" : result;
	}

	/// Build human-readable project-relative key stored in ModelComponent.ModelPath.
	/// Format: primitives/<Name>.chmesh (e.g. primitives/Cube.chmesh)
	static std::string MakeRelativePath(entt::registry& reg, entt::entity e, PrimitiveType type)
	{
		std::string baseName;
		if (auto* tag = reg.try_get<TagComponent>(e))
		{
			if (!tag->Tag.empty() && tag->Tag != "Entity")
			{
				baseName = SanitizeName(tag->Tag);
			}
		}
		if (baseName.empty())
		{
			baseName = TypeName(type);
		}

		return "primitives/" + baseName + ".chmesh";
	}

	/// Serialize PendingModelData to disk using the same ChainedAssetHeader+cereal
	/// format that ModelLoader already knows how to read.
	static bool WriteChasset(const std::string& absPath, const PendingModelData& data)
	{
		try
		{
			std::filesystem::create_directories(std::filesystem::path(absPath).parent_path());

			std::ostringstream dataStream(std::ios::binary);
			{
				cereal::BinaryOutputArchive dataArchive(dataStream);
				dataArchive(data);
			}
			std::string serializedData = dataStream.str();

			ChainedAssetHeader header;
			header.sourceHash = 0; // procedural — no source file to hash
			header.compressed = false;
			header.compressedSize = 0;
			header.uncompressedSize = serializedData.size();

			std::ofstream os(absPath, std::ios::binary | std::ios::trunc);
			if (!os.is_open())
			{
				CH_CORE_ERROR("PrimitiveSystem: cannot open '{}' for writing", absPath);
				return false;
			}
			cereal::BinaryOutputArchive archive(os);
			archive(header);
			os.write(serializedData.data(), static_cast<std::streamsize>(serializedData.size()));
			return os.good();
		} catch (const std::exception& e)
		{
			CH_CORE_ERROR("PrimitiveSystem: failed to write '{}': {}", absPath, e.what());
			return false;
		}
	}

	/// Generate the mesh and write it to disk. Returns the relative path on success,
	/// or an empty string on failure.
	static std::string BakeMesh(entt::registry& reg, entt::entity e)
	{
		auto* prim = reg.try_get<PrimitiveComponent>(e);
		if (!prim || prim->Type == PrimitiveType::None)
		{
			return {};
		}

		const char* marker = TypeMarker(prim->Type);
		if (!marker)
		{
			return {};
		}

		ProceduralParameters params;
		params.Radius = prim->Radius;
		params.InnerRadius = prim->InnerRadius;
		params.Height = prim->Height;
		params.Slices = prim->Slices;
		params.Stacks = prim->Stacks;
		params.Dimensions = prim->Dimensions;

		PendingModelData data = GeometryGenerator::GeneratePrimitivePendingData(marker, params);
		if (!data.isValid)
		{
			CH_CORE_WARN("PrimitiveSystem: GeometryGenerator returned invalid data for entity {}", (uint32_t)e);
			return {};
		}

		std::string rel;
		if (!prim->MeshPath.empty() && (prim->MeshPath.ends_with(".chmesh") || prim->MeshPath.ends_with(".chasset")))
		{
			rel = prim->MeshPath;
		}
		else
		{
			rel = MakeRelativePath(reg, e, prim->Type);
		}

		auto* am = ServiceLocator::TryGet<AssetManager>();
		std::string abs = am ? am->ResolvePath(rel) : (s_PrimitiveDir.empty() ? rel : s_PrimitiveDir + "/" + rel);

		if (!WriteChasset(abs, data))
		{
			return {};
		}

		return rel;
	}

	/// Invalidate the asset in AssetManager so ModelLoader re-reads from disk.
	static void InvalidateAsset(const std::string& relPath)
	{
		if (auto* am = ServiceLocator::TryGet<AssetManager>())
		{
			am->Invalidate(relPath);
		}
	}

	// ---------------------------------------------------------------------------
	// Observer callbacks
	// ---------------------------------------------------------------------------

	/// Called when PrimitiveComponent is constructed or updated (on_construct + on_update).
	static void OnPrimitiveChanged(entt::registry& reg, entt::entity e)
	{
		auto* prim = reg.try_get<PrimitiveComponent>(e);
		if (!prim)
		{
			return;
		}

		if (prim->Type == PrimitiveType::None)
		{
			// Remove ModelComponent if the primitive is set to None.
			reg.remove<ModelComponent>(e);
			prim->MeshPath.clear();
			return;
		}

		std::string newRelPath = BakeMesh(reg, e);
		if (newRelPath.empty())
		{
			return;
		}

		// Invalidate cached asset so the loader picks up the new .chasset bytes.
		if (!prim->MeshPath.empty() && prim->MeshPath != newRelPath)
		{
			InvalidateAsset(prim->MeshPath);
		}
		else if (!newRelPath.empty())
		{
			InvalidateAsset(newRelPath);
		}

		prim->MeshPath = newRelPath;

		// Ensure ModelComponent exists and points at the generated file.
		auto& mc = reg.get_or_emplace<ModelComponent>(e);
		if (mc.ModelPath != newRelPath)
		{
			mc.ModelPath = newRelPath;
			mc.ModelHandle = AssetHandle(0); // force re-resolve
			// Trigger AssetResolutionSystem observer so the model gets loaded.
			reg.patch<ModelComponent>(e, [](ModelComponent&) {});
		}
	}

	/// Called when PrimitiveComponent is destroyed.
	static void OnPrimitiveDestroyed(entt::registry& reg, entt::entity e)
	{
		// Remove the generated ModelComponent — per the design decision:
		// deleting PrimitiveComponent also removes the mesh from the scene.
		reg.remove<ModelComponent>(e);
	}

	// ---------------------------------------------------------------------------
	// Public API
	// ---------------------------------------------------------------------------

	void RegisterObservers(entt::registry& reg, const std::string& primitiveDir)
	{
		s_PrimitiveDir = primitiveDir;
		reg.on_construct<PrimitiveComponent>().connect<&OnPrimitiveChanged>();
		reg.on_update<PrimitiveComponent>().connect<&OnPrimitiveChanged>();
		reg.on_destroy<PrimitiveComponent>().connect<&OnPrimitiveDestroyed>();
	}

	void UnregisterObservers(entt::registry& reg)
	{
		reg.on_construct<PrimitiveComponent>().disconnect<&OnPrimitiveChanged>();
		reg.on_update<PrimitiveComponent>().disconnect<&OnPrimitiveChanged>();
		reg.on_destroy<PrimitiveComponent>().disconnect<&OnPrimitiveDestroyed>();
	}

} // namespace Chained::PrimitiveSystem
