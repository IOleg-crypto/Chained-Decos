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

    auto view = registry.view<WidgetComponent, ControlComponent>();
    for (auto entityID : view)
    {
        auto& control = view.get<ControlComponent>(entityID);
        if (!control.IsActive)
        {
            continue;
        }

        auto& widget = view.get<WidgetComponent>(entityID);
        UIRect rect = layout.GetEntityRect(entityID);

        bool isOver = mousePos.x >= rect.x && mousePos.x <= rect.x + rect.width &&
                      mousePos.y >= rect.y && mousePos.y <= rect.y + rect.height;

        widget.IsHovered = isOver;
        widget.IsDown = isOver && mouseDown && inputCooldown == 0;
        
        // Reset PressedThisFrame is handled by UIRenderer per frame, 
        // but let's be explicit here if it's the start of the frame.
        if (isOver && mouseClicked && inputCooldown == 0)
        {
            widget.PressedThisFrame = true;
        }
    }
}

} // namespace Chained
