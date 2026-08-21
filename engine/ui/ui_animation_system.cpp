#include "ui_animation_system.h"

namespace Chained
{

	static Color LerpColor(Color c1, Color c2, float t)
	{
		return {(uint8_t)(c1.r + (c2.r - c1.r) * t), (uint8_t)(c1.g + (c2.g - c1.g) * t),
				(uint8_t)(c1.b + (c2.b - c1.b) * t), (uint8_t)(c1.a + (c2.a - c1.a) * t)};
	}

	void UIAnimationSystem::Update(entt::registry& registry, float dt)
	{
		auto view = registry.view<UIControlComponent>();
		for (auto entity : view)
		{
			auto& widget = view.get<UIControlComponent>(entity);
			UpdateStyle(widget.BoxStyle, widget.IsHovered, widget.IsDown, dt);
		}
	}

	void UIAnimationSystem::UpdateStyle(UIStyle& style, bool isHovered, bool isDown, float dt)
	{
		float target = isDown ? 1.0f : (isHovered ? 0.5f : 0.0f);

		if (style.TransitionSpeed > 0.0f)
		{
			style.State.AnimationAlpha +=
				(target - style.State.AnimationAlpha) * std::min(1.0f, dt / style.TransitionSpeed);
		}
		else
		{
			style.State.AnimationAlpha = target;
		}

		float a = style.State.AnimationAlpha;
		style.State.CurrentScale =
			1.0f + (isDown ? (style.PressedScale - 1.0f) : (isHovered ? (style.HoverScale - 1.0f) : 0.0f)) * a;

		style.State.CurrentColor = (a <= 0.5f) ? LerpColor(style.BackgroundColor, style.HoverColor, a * 2.0f)
											   : LerpColor(style.HoverColor, style.PressedColor, (a - 0.5f) * 2.0f);
	}

} // namespace Chained
