// ui_input_system.cpp
// Chained Engine — UI input processing for widget interaction.
// Maps ImGui click/hover state to UIControlComponent flags each frame.

#include "ui_input_system.h"
#include "engine/scene/components/control_component.h"
#include "imgui.h"
#include <algorithm>
#include <vector>

namespace Chained
{

void UIInputSystem::Update(entt::registry& registry, const UILayoutSystem& layout, int inputCooldown)
{
    ImVec2 mousePos = ImGui::GetMousePos();
    bool mouseDown = ImGui::IsMouseDown(0);
    bool mouseClicked = ImGui::IsMouseClicked(0);

    // Collect active entities and sort by ZOrder descending so the topmost widget
    // is processed first — this lets us consume the click on the first hit.
    auto view = registry.view<UIControlComponent, ControlComponent>();

    std::vector<entt::entity> sorted;
    sorted.reserve(view.size_hint());
    for (auto entityID : view)
    {
        if (view.get<ControlComponent>(entityID).IsActive)
            sorted.push_back(entityID);
    }
    std::sort(sorted.begin(), sorted.end(), [&](entt::entity a, entt::entity b) {
        return view.get<ControlComponent>(a).ZOrder > view.get<ControlComponent>(b).ZOrder;
    });

    // Reset all flags before processing so stale state never leaks across frames.
    for (auto entityID : view)
    {
        auto& widget = view.get<UIControlComponent>(entityID);
        widget.IsHovered = false;
        widget.IsDown = false;
        widget.PressedThisFrame = false;
    }

    bool clickConsumed = false;

    for (auto entityID : sorted)
    {
        auto& widget = view.get<UIControlComponent>(entityID);
        UIRect rect = layout.GetEntityRect(entityID);

        bool isOver = mousePos.x >= rect.x && mousePos.x <= rect.x + rect.width &&
                      mousePos.y >= rect.y && mousePos.y <= rect.y + rect.height;

        widget.IsHovered = isOver;
        widget.IsDown = isOver && mouseDown && inputCooldown == 0;

        if (isOver && mouseClicked && inputCooldown == 0 && !clickConsumed)
        {
            widget.PressedThisFrame = true;
            clickConsumed = true;
        }
    }
}

} // namespace Chained
