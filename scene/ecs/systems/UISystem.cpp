#include "UISystem.h"
#include "core/Log.h"
#include "scene/ecs/components/UIComponents.h"
#include <raylib.h>

namespace CHEngine
{

UISystem::UISystem()
{
    CD_CORE_INFO("[UISystem] Initialized");
}

void UISystem::ProcessUIEvents(entt::registry &registry)
{
    ProcessButtons(registry);
}

void UISystem::RegisterButtonHandler(const std::string &eventId, ButtonCallback callback)
{
    m_ButtonHandlers[eventId] = callback;
    CD_CORE_TRACE("[UISystem] Registered handler for event: %s", eventId.c_str());
}

void UISystem::UnregisterButtonHandler(const std::string &eventId)
{
    m_ButtonHandlers.erase(eventId);
}

void UISystem::ProcessButtons(entt::registry &registry)
{
    auto view = registry.view<RectTransform, UIButton>();

    for (auto entity : view)
    {
        auto &transform = view.get<RectTransform>(entity);
        auto &button = view.get<UIButton>(entity);

        // Check if button is hovered
        bool hovered = IsButtonHovered(transform);

        // Check if button is clicked
        if (hovered && IsButtonClicked())
        {
            // Find and execute handler
            auto it = m_ButtonHandlers.find(button.eventId);
            if (it != m_ButtonHandlers.end())
            {
                CD_CORE_INFO("[UISystem] Button clicked: %s", button.eventId.c_str());
                it->second(); // Execute callback
            }
            else
            {
                CD_CORE_WARN("[UISystem] No handler for button event: %s", button.eventId.c_str());
            }
        }
    }
}

bool UISystem::IsButtonHovered(const RectTransform &transform)
{
    Vector2 mousePos = GetMousePosition();

    // Simple AABB check
    float left = transform.position.x;
    float top = transform.position.y;
    float right = left + transform.size.x;
    float bottom = top + transform.size.y;

    return (mousePos.x >= left && mousePos.x <= right && mousePos.y >= top && mousePos.y <= bottom);
}

bool UISystem::IsButtonClicked()
{
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

} // namespace CHEngine
