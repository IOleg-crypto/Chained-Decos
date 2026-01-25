#ifndef CH_WIDGET_COMPONENT_H
#define CH_WIDGET_COMPONENT_H

#include "engine/core/base.h"
#include <memory>
#include <raylib.h>
#include <string>

namespace CHEngine
{
// Base Widget Component (Common for all UI)
struct WidgetComponent
{
    struct RectTransform
    {
        Vector2 AnchorMin = {0.5f, 0.5f};
        Vector2 AnchorMax = {0.5f, 0.5f};
        Vector2 OffsetMin = {-50.0f, -20.0f};
        Vector2 OffsetMax = {50.0f, 20.0f};
        Vector2 Pivot = {0.5f, 0.5f};
    };

    RectTransform Transform;
    bool IsActive = true;
    bool HiddenInHierarchy = false;

    WidgetComponent() = default;
};

// Specialized Visual/Data components
struct ImageWidget
{
    enum class ImageScaleMode : uint8_t
    {
        Stretch,
        Fit,
        Fill
    };

    ImageScaleMode ScaleMode = ImageScaleMode::Stretch;
    Color BackgroundColor = {255, 255, 255, 255};
    float Rounding = 0.0f;
    Vector2 Padding = {0, 0};
    bool UseBackground = true;

    std::string TexturePath = "";
    std::shared_ptr<class TextureAsset> Texture;
};

struct TextWidget
{
    std::string Text = "Text";
    std::string FontPath = "";
    float FontSize = 18.0f;
    std::shared_ptr<class FontAsset> Font;
    Color Color = {255, 255, 255, 255};
};

struct ButtonWidget
{
    bool Interactable = true; // Can the button be clicked?
    bool Pressed = false;
    void (*OnPressed)() = nullptr;
};

struct SliderWidget
{
    float Value = 0.0f;
    float Min = 0.0f;
    float Max = 1.0f;
    bool Changed = false;
};

struct CheckboxWidget
{
    bool Checked = false;
    bool Changed = false;
};

} // namespace CHEngine

#endif // CH_WIDGET_COMPONENT_H
