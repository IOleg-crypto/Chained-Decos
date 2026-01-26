#ifndef CH_WIDGET_COMPONENT_H
#define CH_WIDGET_COMPONENT_H

#include "engine/core/base.h"
#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

namespace CHEngine
{

// Typography & Visual Styles
struct TextStyle
{
    std::string FontPath = "";
    float FontSize = 18.0f;
    Color TextColor = WHITE;
    bool bShadow = false;
    float LetterSpacing = 1.0f;
    // Future: Alignment
};

struct UIStyle
{
    Color BackgroundColor = {40, 40, 40, 255};
    Color HoverColor = {60, 60, 60, 255};
    Color PressedColor = {30, 30, 30, 255};

    float Rounding = 4.0f;
    float BorderSize = 0.0f;
    Color BorderColor = WHITE;

    bool bUseGradient = false;
    Color GradientColor = {20, 20, 20, 255};
};

struct RectTransform
{
    Vector2 AnchorMin = {0.5f, 0.5f};
    Vector2 AnchorMax = {0.5f, 0.5f};
    Vector2 OffsetMin = {-50.0f, -20.0f};
    Vector2 OffsetMax = {50.0f, 20.0f};
    Vector2 Pivot = {0.5f, 0.5f};
    Vector2 RectCoordinates = {0.0f, 0.0f};
};

// Base Component
struct WidgetComponent
{
    RectTransform Transform;
    int32_t ZOrder = 0;
    bool IsActive = true;
    bool HiddenInHierarchy = false;

    WidgetComponent() = default;
};

// --- Unified Specialized Widgets ---

struct ButtonWidget
{
    std::string Label = "Button";
    TextStyle Text;
    UIStyle Style;

    bool IsInteractable = true;
    bool PressedThisFrame = false;

    // Internal state
    bool IsHovered = false;
    bool IsDown = false;

    ButtonWidget() = default;
};

struct PanelWidget
{
    UIStyle Style;
    std::string TexturePath = ""; // For 2D backgrounds
    std::shared_ptr<class TextureAsset> Texture = nullptr;

    bool FullScreen = false;
};

struct LabelWidget
{
    std::string Text = "Text Label";
    TextStyle Style;
};

struct SliderWidget
{
    float Value = 0.5f;
    float Min = 0.0f;
    float Max = 1.0f;
    bool Changed = false;

    UIStyle Style;
};

struct CheckboxWidget
{
    bool Checked = false;
    bool Changed = false;

    UIStyle Style;
};

// Layouts
struct VerticalLayoutGroup
{
    float Spacing = 10.0f;
    Vector2 Padding = {10, 10};
    // Future: Alignment
};

} // namespace CHEngine

#endif // CH_WIDGET_COMPONENT_H
