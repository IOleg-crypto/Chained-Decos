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

		/// Path to the generated .chmesh file (managed by PrimitiveSystem, not edited by user).
		std::string MeshPath;

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
			// MeshPath is managed by PrimitiveSystem — not shown in inspector.
			UIMeta MeshPath = {.Hidden = true};
		};
	};

	template <> struct FieldVisibilityOverride<PrimitiveComponent>
	{
		static bool IsVisible(std::string_view field, const PrimitiveComponent& comp)
		{
			using PT = PrimitiveType;

			// MeshPath is always hidden (managed internally by PrimitiveSystem)
			if (field == "MeshPath")
			{
				return false;
			}

			// Dimensions only for Cube / Plane
			if (field == "Dimensions")
			{
				return comp.Type == PT::Cube || comp.Type == PT::Plane;
			}

			// Radius shown for all except Cube, Plane, None
			if (field == "Radius")
			{
				return comp.Type != PT::None && comp.Type != PT::Cube && comp.Type != PT::Plane;
			}

			// InnerRadius only for Torus and Knot
			if (field == "InnerRadius")
			{
				return comp.Type == PT::Torus || comp.Type == PT::Knot;
			}

			// Height only for Cylinder and Cone
			if (field == "Height")
			{
				return comp.Type == PT::Cylinder || comp.Type == PT::Cone;
			}

			// Slices / Stacks shown for sphere-like and ring shapes (not Cube, Plane, None)
			if (field == "Slices" || field == "Stacks")
			{
				return comp.Type != PT::None && comp.Type != PT::Cube && comp.Type != PT::Plane;
			}

			return true;
		}
	};

	CH_MARK_RFL(PrimitiveComponent);
} // namespace Chained

#endif // CH_PRIMITIVE_COMPONENT_H
