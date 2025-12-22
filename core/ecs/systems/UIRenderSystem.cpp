#include "UIRenderSystem.h"
#include "core/Engine.h"
#include "core/events/UIEventRegistry.h"
#include "scene/SceneManager.h"
#include "scene/resources/font/FontService.h"
#include <imgui.h>
#include <raymath.h>

namespace ChainedDecos
{
void UIRenderSystem::Render(int screenWidth, int screenHeight)
{
    auto &registry = ECSRegistry::Get();

    // Render all UI elements with RectTransform
    auto view = registry.view<RectTransform>();

    for (auto entity : view)
    {
        auto &transform = view.get<RectTransform>(entity);
        Vector2 screenPos = CalculateScreenPosition(transform, screenWidth, screenHeight);

        // Handle Button Input
        if (registry.all_of<UIButton>(entity))
        {
            auto &button = registry.get<UIButton>(entity);

            // Define button rectangle
            Rectangle btnRect = {screenPos.x, screenPos.y, transform.size.x, transform.size.y};

            // Check interaction
            Vector2 mousePos = GetMousePosition();
            bool isHovered = CheckCollisionPointRec(mousePos, btnRect);

            button.isHovered = isHovered;

            if (isHovered)
            {
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    button.isPressed = true;
                }
                else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                {
                    if (button.isPressed)
                    {
                        // Trigger Event
                        if (!button.eventId.empty())
                        {
                            UIEventRegistry::Get().Trigger(button.eventId);
                        }

                        // Execute Action
                        if (button.actionType == "LoadScene" && !button.actionTarget.empty())
                        {
                            SceneManager::Get().LoadScene(button.actionTarget);
                        }
                        else if (button.actionType == "Quit")
                        {
                            ChainedEngine::Engine::Instance().RequestExit();
                        }
                        else if (button.actionType == "OpenURL" && !button.actionTarget.empty())
                        {
                            OpenURL(button.actionTarget.c_str());
                        }
                    }
                    button.isPressed = false;
                }
            }
            else
            {
                button.isPressed = false;
            }

            // Render Button
            Color color = button.normalColor;
            if (button.isPressed)
                color = button.pressedColor;
            else if (button.isHovered)
                color = button.hoverColor;

            if (button.borderRadius > 0.0f)
            {
                float roundness =
                    button.borderRadius / (std::min(transform.size.x, transform.size.y) * 0.5f);
                DrawRectangleRounded(btnRect, roundness, 16, color);
                if (button.borderWidth > 0.0f)
                {
                    DrawRectangleRoundedLinesEx(btnRect, roundness, 16, button.borderWidth,
                                                button.borderColor);
                }
            }
            else
            {
                DrawRectangle(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y),
                              static_cast<int>(transform.size.x),
                              static_cast<int>(transform.size.y), color);
                if (button.borderWidth > 0.0f)
                {
                    DrawRectangleLinesEx(btnRect, button.borderWidth, button.borderColor);
                }
            }
        }

        // Render UIImage if present
        if (registry.all_of<UIImage>(entity))
        {
            auto &image = registry.get<UIImage>(entity);
            Rectangle rect = {screenPos.x, screenPos.y, transform.size.x, transform.size.y};

            if (image.borderRadius > 0.0f)
            {
                float roundness =
                    image.borderRadius / (std::min(transform.size.x, transform.size.y) * 0.5f);
                DrawRectangleRounded(rect, roundness, 16, image.tint);
                if (image.borderWidth > 0.0f)
                {
                    DrawRectangleRoundedLinesEx(rect, roundness, 16, image.borderWidth,
                                                image.borderColor);
                }
            }
            else
            {
                DrawRectangle(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y),
                              static_cast<int>(transform.size.x),
                              static_cast<int>(transform.size.y), image.tint);
                if (image.borderWidth > 0.0f)
                {
                    DrawRectangleLinesEx(rect, image.borderWidth, image.borderColor);
                }
            }
        }

        // Render UIText if present
        if (registry.all_of<UIText>(entity))
        {
            auto &text = registry.get<UIText>(entity);
            Font font = FontService::Get().GetFont(text.fontName);
            DrawTextEx(font, text.text.c_str(), screenPos, text.fontSize, text.spacing, text.color);
        }

        // Render ImGui components if present
        if (registry.all_of<ImGuiComponent>(entity))
        {
            auto &imgui = registry.get<ImGuiComponent>(entity);
            if (imgui.isButton)
            {
                // Position ImGui button according to RectTransform
                ImGui::SetNextWindowPos({screenPos.x, screenPos.y});
                ImGui::SetNextWindowSize({transform.size.x, transform.size.y});

                // Create a transparent window to hold the button
                std::string winName = "##imgui_win_" + std::to_string((uint32_t)entity);
                ImGui::Begin(winName.c_str(), nullptr,
                             ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoInputs);

                if (ImGui::Button(imgui.label.c_str(), {transform.size.x, transform.size.y}))
                {
                    if (!imgui.eventId.empty())
                    {
                        UIEventRegistry::Get().Trigger(imgui.eventId);
                    }
                }

                ImGui::End();
            }
        }
    }
}

Vector2 UIRenderSystem::CalculateScreenPosition(const RectTransform &transform, int screenWidth,
                                                int screenHeight)
{
    Vector2 anchorPos = GetAnchorPosition(transform.anchor, screenWidth, screenHeight);
    Vector2 finalPos = Vector2Add(anchorPos, transform.position);

    // Apply pivot offset
    finalPos.x -= transform.size.x * transform.pivot.x;
    finalPos.y -= transform.size.y * transform.pivot.y;

    return finalPos;
}

Vector2 UIRenderSystem::GetAnchorPosition(UIAnchor anchor, int screenWidth, int screenHeight)
{
    switch (anchor)
    {
    case UIAnchor::TopLeft:
        return {0, 0};
    case UIAnchor::TopCenter:
        return {screenWidth / 2.0f, 0};
    case UIAnchor::TopRight:
        return {static_cast<float>(screenWidth), 0};
    case UIAnchor::MiddleLeft:
        return {0, screenHeight / 2.0f};
    case UIAnchor::MiddleCenter:
        return {screenWidth / 2.0f, screenHeight / 2.0f};
    case UIAnchor::MiddleRight:
        return {static_cast<float>(screenWidth), screenHeight / 2.0f};
    case UIAnchor::BottomLeft:
        return {0, static_cast<float>(screenHeight)};
    case UIAnchor::BottomCenter:
        return {screenWidth / 2.0f, static_cast<float>(screenHeight)};
    case UIAnchor::BottomRight:
        return {static_cast<float>(screenWidth), static_cast<float>(screenHeight)};
    default:
        return {0, 0};
    }
}
entt::entity UIRenderSystem::PickUIEntity(Vector2 mousePos, int screenWidth, int screenHeight)
{
    auto &registry = ECSRegistry::Get();
    auto view = registry.view<RectTransform>();

    entt::entity picked = entt::null;

    for (auto entity : view)
    {
        auto &transform = view.get<RectTransform>(entity);
        Vector2 screenPos = CalculateScreenPosition(transform, screenWidth, screenHeight);

        Rectangle btnRect = {screenPos.x, screenPos.y, transform.size.x, transform.size.y};

        if (CheckCollisionPointRec(mousePos, btnRect))
        {
            picked = entity;
            // No break here to pick the topmost element (last rendered)
        }
    }

    return picked;
}

void UIRenderSystem::DrawSelectionHighlight(entt::entity entity, int screenWidth, int screenHeight)
{
    if (entity == entt::null)
        return;

    auto &registry = ECSRegistry::Get();
    if (!registry.valid(entity) || !registry.all_of<RectTransform>(entity))
        return;

    auto &transform = registry.get<RectTransform>(entity);
    Vector2 screenPos = CalculateScreenPosition(transform, screenWidth, screenHeight);

    Rectangle rect = {screenPos.x, screenPos.y, transform.size.x, transform.size.y};
    DrawRectangleLinesEx(rect, 2.0f, ORANGE);

    // Draw small handles at corners
    float handleSize = 6.0f;
    DrawRectangleV({rect.x - handleSize / 2, rect.y - handleSize / 2}, {handleSize, handleSize},
                   WHITE);
    DrawRectangleV({rect.x + rect.width - handleSize / 2, rect.y - handleSize / 2},
                   {handleSize, handleSize}, WHITE);
    DrawRectangleV({rect.x - handleSize / 2, rect.y + rect.height - handleSize / 2},
                   {handleSize, handleSize}, WHITE);
    DrawRectangleV({rect.x + rect.width - handleSize / 2, rect.y + rect.height - handleSize / 2},
                   {handleSize, handleSize}, WHITE);
}

} // namespace ChainedDecos
