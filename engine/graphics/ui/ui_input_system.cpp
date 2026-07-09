// ui_input_system.cpp
// Chained Engine — UI input processing for widget interaction.
// Maps ImGui click/hover state to UIControlComponent flags each frame.

#include "ui_input_system.h"
#include "engine/scene/components/control_component.h"
#include "imgui.h"

namespace Chained
{

void UIInputSystem::Update(entt::registry& registry, const UILayoutSystem& layout, int inputCooldown)
{
    ImVec2 mousePos = ImGui::GetMousePos();
    bool mouseDown = ImGui::IsMouseDown(0);
    bool mouseClicked = ImGui::IsMouseClicked(0);

    auto view = registry.view<UIControlComponent, ControlComponent>();
    for (auto entityID : view)
    {
        auto& control = view.get<ControlComponent>(entityID);
        if (!control.IsActive)
        {
            continue;
        }

        auto& widget = view.get<UIControlComponent>(entityID);
        UIRect rect = layout.GetEntityRect(entityID);

        bool isOver = mousePos.x >= rect.x && mousePos.x <= rect.x + rect.width &&
                      mousePos.y >= rect.y && mousePos.y <= rect.y + rect.height;

        widget.IsHovered = isOver;
        widget.IsDown = isOver && mouseDown && inputCooldown == 0;

        // Reset each frame so a stale click cannot re-fire across frames.
        widget.PressedThisFrame = false;

        if (isOver && mouseClicked && inputCooldown == 0)
        {
            widget.PressedThisFrame = true;
        }
    }
}

} // namespace Chained
