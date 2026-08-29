#ifndef CH_LIGHT_COMPONENT_H
#define CH_LIGHT_COMPONENT_H

#include "engine/reflection/reflection_rfl.h"

namespace Chained
{
	enum class LightType
	{
		Point = 0,
		Spot = 1,
		Directional = 2
	};

	struct LightComponent
	{
		LightType Type = LightType::Point;
		Color LightColor = Color::White();
		float Intensity = 100.0f;
		float Radius = 100.0f;	   // Also used as Range for Spot lights
		float InnerCutoff = 15.0f; // Spot light only (degrees)
		float OuterCutoff = 20.0f; // Spot light only (degrees)
		bool Shadows = false;	   // Future proofing

		static const char* GetStaticName()
		{
			return "LightComponent";
		}

		struct UI
		{
			UIMeta Type = {.Hint = PropertyMeta::WidgetHint::Checkbox,
						   .Tooltip = "Type of light source: Directional, Point, or Spot"};
			UIMeta LightColor = {.Hint = PropertyMeta::WidgetHint::ColorPicker, .Tooltip = "Color of the light"};
			UIMeta Intensity = {
				.Min = 0.0f, .Max = 10000.0f, .Speed = 5.0f, .Tooltip = "Light brightness intensity multiplier"};
			UIMeta Radius = {
				.Min = 0.0f, .Max = 1000.0f, .Speed = 1.0f, .Tooltip = "Maximum range of the light (Point and Spot)"};
			UIMeta InnerCutoff = {
				.Min = 0.0f, .Max = 90.0f, .Speed = 0.5f, .Tooltip = "Inner cone angle for Spot light (degrees)"};
			UIMeta OuterCutoff = {
				.Min = 0.0f, .Max = 90.0f, .Speed = 0.5f, .Tooltip = "Outer cone angle for Spot light (degrees)"};
		};
	};

	CH_MARK_RFL(LightComponent);

} // namespace Chained

#endif // CH_LIGHT_COMPONENT_H
