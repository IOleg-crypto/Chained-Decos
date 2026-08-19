// ui_input_system.cpp
// Chained Engine — UI input processing for widget interaction.
// Maps ImGui click/hover state to UIControlComponent flags each frame.

#include "ui_input_system.h"
#include "engine/scene/components/ui/control_component.h"
#include "imgui.h"

namespace Chained
{

	void UpdateUIInput(entt::registry& registry, const UILayoutSystem& layout, bool suppress)
	{
		auto view = registry.view<UIControlComponent, ControlComponent>();

		// Reset per-frame flags. PrevIsDown is NOT reset here — it carries over so the
		// edge detector (!PrevIsDown && IsDown) works correctly the next frame.
		for (auto entityID : view)
		{
			auto& widget = view.get<UIControlComponent>(entityID);
			widget.IsHovered = false;
			widget.IsDown = false;
			widget.PressedThisFrame = false;
		}

		ImVec2 mousePos = ImGui::GetMousePos();
		bool mouseDown = ImGui::IsMouseDown(0);

		// Collect active entities for Z-order sorted processing.
		std::vector<entt::entity> sorted;
		sorted.reserve(view.size_hint());
		for (auto entityID : view)
		{
			if (view.get<ControlComponent>(entityID).IsActive)
			{
				sorted.push_back(entityID);
			}
		}

		// Sort by Z-order: highest (topmost) first.
		std::sort(sorted.begin(), sorted.end(), [&](entt::entity a, entt::entity b) {
			return view.get<ControlComponent>(a).ZOrder > view.get<ControlComponent>(b).ZOrder;
		});

		// Input consumption flags — only the topmost hovered element receives events.
		bool hoverConsumed = false;

		for (auto entityID : sorted)
		{
			auto& widget = view.get<UIControlComponent>(entityID);
			UIRect rect = layout.GetEntityRect(entityID);

			bool isOver = mousePos.x >= rect.x && mousePos.x <= rect.x + rect.width && mousePos.y >= rect.y &&
						  mousePos.y <= rect.y + rect.height;

			bool wasDown = widget.PrevIsDown;

			if (isOver && !hoverConsumed)
			{
				widget.IsHovered = true;
				hoverConsumed = true;

				if (mouseDown)
				{
					widget.IsDown = true;
				}

				// Own edge detection: click = rising edge of IsDown
				if (widget.IsDown && !wasDown)
				{
					widget.PressedThisFrame = true;
				}
			}

			// Suppress inputs globally at the end (e.g. crossing Edit->Play boundary).
			// This ensures PrevIsDown is still properly updated so human hold-overs
			// don't trigger false edges on the frame AFTER suppression ends.
			if (suppress)
			{
				widget.PressedThisFrame = false;
			}

			// Update previous state for next frame's edge detection.
			widget.PrevIsDown = widget.IsDown;
		}
	}

} // namespace Chained
