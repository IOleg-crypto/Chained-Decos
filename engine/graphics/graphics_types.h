#ifndef CH_GRAPHICS_TYPES_H
#define CH_GRAPHICS_TYPES_H

#include "material.h" // For MaterialInstance if needed
#include "raylib.h"
#include <string>
#include <vector>

namespace CHEngine
{

// --- 3D Rendering Types ---

enum class MaterialSlotTarget : uint8_t
{
    MaterialIndex = 0,
    MeshIndex = 1
};

struct MaterialSlot
{
    std::string Name;
    int Index = -1;
    MaterialSlotTarget Target = MaterialSlotTarget::MaterialIndex;
    MaterialInstance Material;

    MaterialSlot() = default;
    MaterialSlot(const std::string& name, int index)
        : Name(name),
          Index(index)
    {
    }
};

struct ShaderUniform
{
    std::string Name;
    int Type; // 0: Float, 1: Vec2, 2: Vec3, 3: Vec4, 4: Color
    float Value[4] = {0, 0, 0, 0};
};

struct RenderLight
{
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 16 bytes (vec4 in GLSL)
    Vector3 position = {0, 0, 0};              // 12 bytes (vec3 in GLSL)
    float intensity = 1.0f;                    // 4 bytes (fits in vec3 alignment hole)
    Vector3 direction = {0, -1, 0};            // 12 bytes
    float radius = 10.0f;                      // 4 bytes
    float innerCutoff = 15.0f;                 // 4 bytes
    float outerCutoff = 20.0f;                 // 4 bytes
    int type = 0;                              // 4 bytes
    int enabled = 0;                           // 4 bytes
};

struct GridSettings
{
    int Slices = 20;
    float Spacing = 1.0f;
};

// --- UI Rendering Types ---

enum class TextAlignment
{
    Left = 0,
    Center = 1,
    Right = 2,
    Top = 0,
    Bottom = 2
};

enum class CanvasScaleMode : uint8_t
{
    ConstantPixelSize,   // No scaling, pixel-perfect at any resolution
    ScaleWithScreenSize, // Scale proportionally based on reference resolution
};

struct CanvasSettings
{
    Vector2 ReferenceResolution = {0.0f, 0.0f};
    CanvasScaleMode ScaleMode = CanvasScaleMode::ConstantPixelSize;
    float MatchWidthOrHeight = 0.5f;
};

struct TextStyle
{
    std::string FontName = "Default";
    float FontSize = 18.0f;
    Color TextColor = WHITE;
    bool Shadow = false;
    float ShadowOffset = 2.0f;
    Color ShadowColor = BLACK;
    float LetterSpacing = 1.0f;
    float LineHeight = 1.2f;
    TextAlignment HorizontalAlignment = TextAlignment::Center;
    TextAlignment VerticalAlignment = TextAlignment::Center;
};

struct UIStyle
{
    Color BackgroundColor = {40, 40, 40, 255};
    Color HoverColor = {60, 60, 60, 255};
    Color PressedColor = {30, 30, 30, 255};

    float Rounding = 4.0f;
    float BorderSize = 0.0f;
    Color BorderColor = WHITE;

    bool UseGradient = false;
    Color GradientColor = {20, 20, 20, 255};

    float Padding = 4.0f;

    float HoverScale = 1.0f;
    float PressedScale = 1.0f;
    float TransitionSpeed = 0.1f;
};

} // namespace CHEngine

#endif // CH_GRAPHICS_TYPES_H
