#include "ui_render_system.h"
#include "core/application/application.h"
#include "events/ui_event_registry.h"
#include "scene/ecs/components/player_component.h"
#include "scene/ecs/components/transform_component.h"
#include "scene/ecs/components/utility_components.h"
#include "scene/resources/font/font_service.h"
#include "scene/resources/texture/texture_service.h"
#include <algorithm>
#include <imgui.h>
#include <raymath.h>
#include <string>

namespace CHEngine
{
void UIRenderSystem::Render(entt::registry &registry, int screenWidth, int screenHeight)
{
    // 0. Render Background
    auto bgView = registry.view<UIBackground>();
    for (auto entity : bgView)
    {
        auto &bg = bgView.get<UIBackground>(entity);

        // Draw Solid Color if alpha > 0
        if (bg.color.a > 0)
        {
            DrawRectangle(0, 0, screenWidth, screenHeight, bg.color);
        }

        // Draw Texture if path is provided
        if (!bg.texturePath.empty())
        {
            Texture2D tex = TextureService::GetTexture(bg.texturePath);
            if (tex.id == 0)
            {
                TextureService::LoadTexture(bg.texturePath, bg.texturePath);
                tex = TextureService::GetTexture(bg.texturePath);
            }

            if (tex.id != 0)
            {
                Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
                Rectangle dest = {0, 0, (float)screenWidth, (float)screenHeight};
                DrawTexturePro(tex, source, dest, {0, 0}, 0.0f, WHITE);
            }
        }
    }

    // Render all UI elements with RectTransform
    auto view = registry.view<RectTransform>();

    for (auto entity : view)
    {
        auto &transform = view.get<RectTransform>(entity);
        if (!transform.active)
            continue;

        Vector2 screenPos = CalculateScreenPosition(transform, screenWidth, screenHeight);

        // Skip entities that are handled by ImGui during standard Raylib render pass
        if (registry.all_of<ImGuiComponent>(entity))
        {
            continue;
        }

        // 2. STANDARD RAYLIB RENDERING
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
                            UIEventRegistry::Trigger(button.eventId);
                        }

                        // Execute Action
                        if (button.actionType == "Quit")
                        {
                            Application::Get().Close();
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

            Texture2D tex = {0};
            if (!image.texturePath.empty())
            {
                tex = TextureService::GetTexture(image.texturePath);
                if (tex.id == 0)
                {
                    TextureService::LoadTexture(image.texturePath, image.texturePath);
                    tex = TextureService::GetTexture(image.texturePath);
                }
            }

            if (tex.id != 0)
            {
                Rectangle source = {0, 0, (float)tex.width, (float)tex.height};
                DrawTexturePro(tex, source, rect, {0, 0}, 0.0f, image.tint);
            }
            else
            {
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
        }

        // Render UIText if present
        if (registry.all_of<UIText>(entity))
        {
            auto &text = registry.get<UIText>(entity);
            Font font = FontService::GetFont(text.fontName);

            Vector2 textPos = screenPos;

            if (registry.all_of<UIButton>(entity) || registry.all_of<UIImage>(entity))
            {
                Vector2 textSize =
                    MeasureTextEx(font, text.text.c_str(), text.fontSize, text.spacing);
                textPos.x += (transform.size.x - textSize.x) * 0.5f;
                textPos.y += (transform.size.y - textSize.y) * 0.5f;
            }

            DrawTextEx(font, text.text.c_str(), textPos, text.fontSize, text.spacing, text.color);
        }
    }
}

void UIRenderSystem::RenderHUD(entt::registry &registry, int screenWidth, int screenHeight)
{
    auto view = registry.view<PlayerComponent, TransformComponent>();

    for (auto entity : view)
    {
        auto &playerComp = registry.get<PlayerComponent>(entity);

        int hours = (int)playerComp.runTimer / 3600;
        int minutes = ((int)playerComp.runTimer % 3600) / 60;
        int seconds = (int)playerComp.runTimer % 60;

        const float margin = 30.0f;
        const float fontSize = 24.0f;
        const float spacing = 1.0f;
        const Color accentColor = WHITE;
        const Color shadowColor = ColorAlpha(BLACK, 0.4f);
        const Vector2 shadowOffset = {1.5f, 1.5f};

        // Use a generic name or consistent path from FontService
        Font hudFont = GetFontDefault(); // Fallback
        // Actually HUD usually expects a specific font like Orbitron
        // For now, let's use default or what's loaded.

        std::string heightStr = std::to_string((int)playerComp.maxHeight) + "m";
        const char *heightText = heightStr.c_str();
        Vector2 heightTextSize = {(float)MeasureText(heightText, (int)fontSize), fontSize};

        // Height
        DrawText(heightText, (int)margin + 2, (int)margin + 2, (int)fontSize, BLACK);
        DrawText(heightText, (int)margin, (int)margin, (int)fontSize, WHITE);

        // Timer
        float timerX = margin + heightTextSize.x + 25.0f;
        DrawCircleLines((int)timerX + 8, (int)margin + 12, 7, WHITE);
        const char *timerText = (hours > 0) ? TextFormat("%dh %dm %ds", hours, minutes, seconds)
                                            : TextFormat("%dm %ds", minutes, seconds);
        DrawText(timerText, (int)timerX + 24, (int)margin + 2, (int)fontSize, BLACK);
        DrawText(timerText, (int)timerX + 22, (int)margin, (int)fontSize, WHITE);

        // Meter
        float meterX = margin + 5.0f;
        float meterY = margin + 45.0f;
        float meterHeight = 120.0f;
        DrawRectangle((int)meterX, (int)meterY, 2, (int)meterHeight, ColorAlpha(WHITE, 0.5f));

        // Tips
        const char *tipText = "[F] Respawn";
        DrawText(tipText, (int)margin, (int)(screenHeight - margin - 20), 20,
                 ColorAlpha(WHITE, 0.6f));
    }
}

Vector2 UIRenderSystem::CalculateScreenPosition(const RectTransform &transform, int screenWidth,
                                                int screenHeight)
{
    Vector2 anchorPos = GetAnchorPosition(transform.anchor, screenWidth, screenHeight);
    Vector2 finalPos = Vector2Add(anchorPos, transform.position);
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

entt::entity UIRenderSystem::PickUIEntity(entt::registry &registry, Vector2 mousePos,
                                          int screenWidth, int screenHeight)
{
    auto view = registry.view<RectTransform>();
    entt::entity picked = entt::null;
    for (auto entity : view)
    {
        auto &transform = view.get<RectTransform>(entity);
        if (!transform.active)
            continue;
        Vector2 screenPos = CalculateScreenPosition(transform, screenWidth, screenHeight);
        Rectangle btnRect = {screenPos.x, screenPos.y, transform.size.x, transform.size.y};
        if (CheckCollisionPointRec(mousePos, btnRect))
            picked = entity;
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

void UIRenderSystem::RenderImGui(entt::registry &registry, int screenWidth, int screenHeight,
                                 Vector2 offset)
{
    auto view = registry.view<RectTransform, ImGuiComponent>();
    for (auto entity : view)
    {
        auto &transform = view.get<RectTransform>(entity);
        if (!transform.active)
            continue;
        auto &imgui = view.get<ImGuiComponent>(entity);
        Vector2 localPos = CalculateScreenPosition(transform, screenWidth, screenHeight);
        Vector2 screenPos = Vector2Add(localPos, offset);
        if (imgui.isButton)
        {
            ImGui::SetNextWindowPos({screenPos.x, screenPos.y});
            ImGui::SetNextWindowSize({transform.size.x, transform.size.y});
            std::string winName = "##imgui_win_" + std::to_string((uint32_t)entity);
            ImGui::Begin(winName.c_str(), nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoInputs);
            if (ImGui::Button(imgui.label.c_str(), {transform.size.x, transform.size.y}))
                if (!imgui.eventId.empty())
                    UIEventRegistry::Trigger(imgui.eventId);
            ImGui::End();
        }
        else
        {
            ImGui::SetNextWindowPos({screenPos.x, screenPos.y});
            std::string winName = "##imgui_text_win_" + std::to_string((uint32_t)entity);
            ImGui::Begin(winName.c_str(), nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoInputs);
            ImGui::Text("%s", imgui.label.c_str());
            ImGui::End();
        }
    }
}
} // namespace CHEngine
