#ifndef CH_UI_STYLE_H
#define CH_UI_STYLE_H

namespace CHEngine
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

struct Rectangle
{
    float x, y, width, height;
};
struct CanvasSettings
{
    glm::vec2 ReferenceResolution = {1920.0f, 1080.0f};
    CanvasScaleMode ScaleMode = CanvasScaleMode::ConstantPixelSize;
    float MatchWidthOrHeight = 0.5f;
};
} // namespace CHEngine

#endif // CH_UI_STYLE_H