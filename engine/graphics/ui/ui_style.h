#ifndef CH_UI_STYLE_H
#define CH_UI_STYLE_H

#include <string>
#include <cstdint>
#include <glm/glm.hpp>
#include "engine/reflection/reflection_rfl.h"

namespace Chained
{
// Typography & Visual Styles
enum class HorizontalAlignment
{
    Left = 0,
    Center = 1,
    Right = 2
};

enum class VerticalAlignment
{
    Top = 0,
    Center = 1,
    Bottom = 2
};

enum class CanvasScaleMode : uint8_t
{
    ConstantPixelSize,
    ScaleWithScreenSize,
};

struct CanvasSettings
{
    glm::vec2 ReferenceResolution = {1920.0f, 1080.0f};
    CanvasScaleMode ScaleMode = CanvasScaleMode::ConstantPixelSize;
    float MatchWidthOrHeight = 0.5f;
};

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

} // namespace Chained

#endif // CH_UI_STYLE_H