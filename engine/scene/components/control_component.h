#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include "engine/reflection/reflection.h"
#include "engine/reflection/reflection_rfl.h"
#include "engine/graphics/ui/ui_style.h"
#include "engine/graphics/ui/ui_data_components.h"

namespace Chained
{


struct TextStyle
{
    std::string FontName = "Default";
    float FontSize = 18.0f;
    Color TextColor = { 255, 255, 255, 255 };
    bool Shadow = false;
    float ShadowOffset = 2.0f;
    Color ShadowColor = { 0, 0, 0, 255 };
    float LetterSpacing = { 1.0f };
    float LineHeight = 1.2f;
    HorizontalAlignment Horizontal = HorizontalAlignment::Center;
    VerticalAlignment Vertical = VerticalAlignment::Center;

    static const char* GetStaticName() { return "TextStyle"; }
};

CH_MARK_RFL(TextStyle);

struct UIStyle
{
    Color BackgroundColor = {40, 40, 40, 255};
    Color HoverColor = {60, 60, 60, 255};
    Color PressedColor = {30, 30, 30, 255};

    float Rounding = 4.0f;
    float BorderSize = 0.0f;
    Color BorderColor = { 255, 255, 255, 255 };

    bool UseGradient = false;
    Color GradientColor = {20, 20, 20, 255};

    float Padding = 4.0f;

    float HoverScale = 1.0f;
    float PressedScale = 1.0f;
    float TransitionSpeed = 0.1f;

    // Runtime state (not serialized)
    struct RuntimeState {
        float AnimationAlpha = 0.0f; // 0 = idle, 1 = hover/pressed
        float CurrentScale = 1.0f;
        Color CurrentColor = {255, 255, 255, 255};
    } State;
};

CH_MARK_RFL(UIStyle);


struct RectTransform
{
    glm::vec2 AnchorMin = {0.5f, 0.5f};
    glm::vec2 AnchorMax = {0.5f, 0.5f};
    glm::vec2 OffsetMin = {-50.0f, -20.0f};
    glm::vec2 OffsetMax = {50.0f, 20.0f};
    glm::vec2 Pivot = {0.5f, 0.5f};
    float Rotation = 0.0f;
    glm::vec2 Scale = {1.0f, 1.0f};

    static const char* GetStaticName() { return "RectTransform"; }
};

CH_MARK_RFL(RectTransform);

struct ControlComponent
{
    RectTransform Transform;
    int32_t ZOrder = 0;
    bool IsActive = true;
    bool HiddenInHierarchy = false;

    static const char* GetStaticName() { return "ControlComponent"; }
};

CH_MARK_RFL(ControlComponent);

struct WidgetComponent
{
    UIStyle BoxStyle;
    TextStyle TextStyle;
    WidgetData Data = std::monostate{};

    bool IsHovered = false;
    bool IsDown = false;
    bool PressedThisFrame = false;
    bool ValueChanged = false;

    static const char* GetStaticName() { return "WidgetComponent"; }
};

CH_MARK_RFL(WidgetComponent);

} // namespace Chained

#endif // CH_CONTROL_COMPONENT_H
