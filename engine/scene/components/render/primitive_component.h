#ifndef CH_PRIMITIVE_COMPONENT_H
#define CH_PRIMITIVE_COMPONENT_H

#include "engine/reflection/reflection.h"
#include "engine/reflection/reflection_rfl.h"
#include "engine/graphics/api/renderer_types.h"

namespace Chained
{

	enum class PrimitiveType : uint8_t
	{
		None = 0,
		Cube,
		Sphere,
		Plane,
		Cylinder,
		Cone,
		Torus,
		Knot,
		Hemisphere
	};

	struct PrimitiveComponent
	{
		PrimitiveType Type = PrimitiveType::None;

		// Geometry Parameters
		float Radius = 0.5f;
		float InnerRadius = 0.2f;
		float Height = 1.0f;
		int Slices = 16;
		int Stacks = 16;
		glm::vec3 Dimensions = {1.0f, 1.0f, 1.0f};

		// Material Properties (Serialized)
		std::string AlbedoPath;
		glm::vec4 AlbedoColor = {1.0f, 1.0f, 1.0f, 1.0f};
		std::string NormalPath;
		std::string MetallicRoughnessPath;
		std::string EmissivePath;
		glm::vec4 EmissiveColor = {0.0f, 0.0f, 0.0f, 1.0f};
		float EmissiveIntensity = 0.0f;
		float Metalness = 0.0f;
		float Roughness = 0.5f;
		bool Transparent = false;
		float Alpha = 1.0f;

		static const char* GetStaticName()
		{
			return "PrimitiveComponent";
		}

		struct UI
		{
			UIMeta Type = {.Hint = PropertyMeta::WidgetHint::Enum, .Tooltip = "Geometric shape of the primitive"};
			UIMeta Radius;
			UIMeta InnerRadius;
			UIMeta Height;
			UIMeta Slices;
			UIMeta Stacks;
			UIMeta Dimensions;
			// Material properties are edited in the Material Editor; serialized but hidden in the component inspector.
			UIMeta AlbedoPath = {.Hidden = true};
			UIMeta AlbedoColor = {.Hidden = true};
			UIMeta NormalPath = {.Hidden = true};
			UIMeta MetallicRoughnessPath = {.Hidden = true};
			UIMeta EmissivePath = {.Hidden = true};
			UIMeta EmissiveColor = {.Hidden = true};
			UIMeta EmissiveIntensity = {.Hidden = true};
			UIMeta Metalness = {.Hidden = true};
			UIMeta Roughness = {.Hidden = true};
			UIMeta Transparent = {.Hidden = true};
			UIMeta Alpha = {.Hidden = true};
		};

		Material GetMaterial() const
		{
			Material mat;
			mat.AlbedoPath = AlbedoPath;
			mat.AlbedoColor = AlbedoColor;
			mat.NormalPath = NormalPath;
			mat.MetallicRoughnessPath = MetallicRoughnessPath;
			mat.EmissivePath = EmissivePath;
			mat.EmissiveColor = EmissiveColor;
			mat.EmissiveIntensity = EmissiveIntensity;
			mat.Metalness = Metalness;
			mat.Roughness = Roughness;
			mat.Transparent = Transparent;
			mat.Alpha = Alpha;
			return mat;
		}

		void SetMaterial(const Material& mat)
		{
			AlbedoPath = mat.AlbedoPath;
			AlbedoColor = mat.AlbedoColor;
			NormalPath = mat.NormalPath;
			MetallicRoughnessPath = mat.MetallicRoughnessPath;
			EmissivePath = mat.EmissivePath;
			EmissiveColor = mat.EmissiveColor;
			EmissiveIntensity = mat.EmissiveIntensity;
			Metalness = mat.Metalness;
			Roughness = mat.Roughness;
			Transparent = mat.Transparent;
			Alpha = mat.Alpha;
		}
	};

	CH_MARK_RFL(PrimitiveComponent);
} // namespace Chained

#endif // CH_PRIMITIVE_COMPONENT_H
