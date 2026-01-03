#include "core/log.h"
#include "ui_system.h"
#include "core/log.h"
#include "engine/scene/ecs/components/ui_components.h"
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>

namespace CHEngine
{

UISystem::UISystem()
{
    CD_CORE_INFO("[UISystem] Initialized with ImGui Backend");
}

void UISystem::Render(entt::registry &registry, int screenWidth, int screenHeight, Vector2 offset)
{
    // Render all entities with RectTransform
    auto view = registry.view<RectTransform>();

    for (auto entity : view)
    {
        RenderEntity(registry, entity, screenWidth, screenHeight, offset);
    }
}

void UISystem::RenderEntity(entt::registry &registry, entt::entity entity, int screenWidth,
                            int screenHeight, Vector2 offset)
{
    auto &transform = registry.get<RectTransform>(entity);
    if (!transform.active)
        return;

    Vector2 screenPos = CalculateScreenPosition(transform, screenWidth, screenHeight);
    ImVec2 imguiPos = {screenPos.x + offset.x, screenPos.y + offset.y};

    // Setup ImGui Window for this element
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;

    ImGui::SetNextWindowPos(imguiPos);
    ImGui::SetNextWindowSize({transform.size.x, transform.size.y});

    std::string winName = "##UI_Entity_" + std::to_string((uint32_t)entity);
    if (ImGui::Begin(winName.c_str(), nullptr, flags))
    {
        // 1. Button Rendering
        if (registry.all_of<UIButton>(entity))
        {
            auto &button = registry.get<UIButton>(entity);

            // In a real system, we'd also check for UIText on the same entity for the label
            std::string label = "Button";
            if (registry.all_of<UIText>(entity))
                label = registry.get<UIText>(entity).text;

            if (ImGui::Button(label.c_str(), {transform.size.x, transform.size.y}))
            {
                button.isPressed = true;

                // Trigger C++/Scripts callback
                if (!button.eventId.empty())
                {
                    auto it = m_ButtonHandlers.find(button.eventId);
                    if (it != m_ButtonHandlers.end())
                        it->second();
                }
            }
            else
            {
                button.isPressed = false;
            }

            button.isHovered = ImGui::IsItemHovered();
        }
        // 2. Text Rendering
        else if (registry.all_of<UIText>(entity))
        {
            auto &text = registry.get<UIText>(entity);
            ImGui::Text("%s", text.text.c_str());
        }

        ImGui::End();
    }
}

void UISystem::RegisterButtonHandler(const std::string &eventId, ButtonCallback callback)
{
    m_ButtonHandlers[eventId] = callback;
    CD_CORE_TRACE("[UISystem] Registered handler for: %s", eventId.c_str());
}

void UISystem::UnregisterButtonHandler(const std::string &eventId)
{
    m_ButtonHandlers.erase(eventId);
}

Vector2 UISystem::CalculateScreenPosition(const RectTransform &transform, int screenWidth,
                                          int screenHeight)
{
    Vector2 anchorPos = {0, 0};

    switch (transform.anchor)
    {
    case UIAnchor::TopLeft:
        anchorPos = {0.0f, 0.0f};
        break;
    case UIAnchor::TopCenter:
        anchorPos = {screenWidth * 0.5f, 0.0f};
        break;
    case UIAnchor::TopRight:
        anchorPos = {(float)screenWidth, 0.0f};
        break;
    case UIAnchor::MiddleLeft:
        anchorPos = {0.0f, screenHeight * 0.5f};
        break;
    case UIAnchor::MiddleCenter:
        anchorPos = {screenWidth * 0.5f, screenHeight * 0.5f};
        break;
    case UIAnchor::MiddleRight:
        anchorPos = {(float)screenWidth, screenHeight * 0.5f};
        break;
    case UIAnchor::BottomLeft:
        anchorPos = {0.0f, (float)screenHeight};
        break;
    case UIAnchor::BottomCenter:
        anchorPos = {screenWidth * 0.5f, (float)screenHeight};
        break;
    case UIAnchor::BottomRight:
        anchorPos = {(float)screenWidth, (float)screenHeight};
        break;
    }

    Vector2 finalPos = {anchorPos.x + transform.position.x, anchorPos.y + transform.position.y};

    // Apply pivot
    finalPos.x -= transform.size.x * transform.pivot.x;
    finalPos.y -= transform.size.y * transform.pivot.y;

    return finalPos;
}

} // namespace CHEngine
#include "core/log.h"

