#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include "engine/common/color.h"
#include "engine/graphics/ui/ui_data_components.h"
#include "engine/graphics/ui/ui_style.h"
#include "engine/reflection/reflection_rfl.h"


namespace Chained
{
struct RectTransform
{
    glm::vec2 AnchorMin = {0.5f, 0.5f};
    glm::vec2 AnchorMax = {0.5f, 0.5f};
    glm::vec2 OffsetMin = {-50.0f, -20.0f};
    glm::vec2 OffsetMax = {50.0f, 20.0f};
    glm::vec2 Pivot = {0.5f, 0.5f};
    float Rotation = 0.0f;
    glm::vec2 Scale = {1.0f, 1.0f};

    static const char* GetStaticName()
    {
        return "RectTransform";
    }

    // Declarative UI layout metadata for compile-time reflection
    struct UI
    {
        UIMeta AnchorMin = {.Speed = 0.01f,
                            .Tooltip = "Normalized minimum anchor point (0.0 to 1.0) for UI layout alignment"};
        UIMeta AnchorMax = {.Speed = 0.01f,
                            .Tooltip = "Normalized maximum anchor point (0.0 to 1.0) for UI layout alignment"};
        UIMeta OffsetMin = {.Speed = 1.0f, .Tooltip = "Pixel offset from the calculated minimum anchor positions"};
        UIMeta OffsetMax = {.Speed = 1.0f, .Tooltip = "Pixel offset from the calculated maximum anchor positions"};
        UIMeta Pivot = {.Speed = 0.01f,
                        .Tooltip = "The center pivot point around which the UI element rotates and scales"};
        UIMeta Rotation = {.Speed = 0.5f, .Tooltip = "Local rotation angle of the UI control specified in degrees"};
        UIMeta Scale = {.Speed = 0.05f, .Tooltip = "Local transform multipliers along local X and Y axes"};
    };
};

CH_MARK_RFL(RectTransform);

struct ControlComponent
{
    RectTransform Transform;
    int32_t ZOrder = 0;
    bool IsActive = true;
    bool HiddenInHierarchy = false;

    static const char* GetStaticName()
    {
        return "ControlComponent";
    }

    // Declarative UI layout metadata for compile-time reflection
    struct UI
    {
        UIMeta Transform = {.Tooltip = "Hierarchical rect transform configurations for bounding calculations"};
        UIMeta ZOrder = {.Speed = 1.0f,
                         .Tooltip = "Rendering layer priority; elements with higher values render on top"};
        UIMeta IsActive = {.Tooltip =
                               "Determines whether this UI control is updated, rendered, and processes user inputs"};
        UIMeta HiddenInHierarchy = {
            .Tooltip = "If enabled, skips showing this specific element inside the editor tree panels"};
    };
};

CH_MARK_RFL(ControlComponent);

struct UIControlComponent
{
    UIStyle BoxStyle;
    TextStyle TextStyle;
    ControlData Data = std::monostate{};

    // Runtime state flags
    bool IsHovered = false;
    bool IsDown = false;
    bool PrevIsDown = false;       // used for self-tracked edge detection
    bool PressedThisFrame = false;
    bool ValueChanged = false;

    static const char* GetStaticName()
    {
        return "UIControlComponent";
    }

    // Fixed the truncated struct declaration and fully localized attributes
    struct UI
    {
        UIMeta BoxStyle = {.Tooltip =
                               "Visual appearance, coloring, padding, and corner configurations for the canvas node"};
        UIMeta TextStyle = {.Tooltip = "Font, alignment, coloring, and styling configurations for label layers"};
        UIMeta Data = {.Tooltip = "Underlying variant data structure storage managed by this control instance"};

        // Interaction states - visible during runtime for live debugging, ignored by file saves
        UIMeta IsHovered = {.ReadOnly = true,
                            .Transient = true,
                            .Tooltip = "True if the mouse pointer is currently intersecting this canvas element"};
        UIMeta IsDown = {.ReadOnly = true,
                         .Transient = true,
                         .Tooltip = "True if the user interaction trigger is currently held down on this node"};
        UIMeta PressedThisFrame = {.ReadOnly = true,
                                   .Transient = true,
                                   .Tooltip = "Triggers true for a single frame immediately upon interaction click"};
        UIMeta ValueChanged = {.ReadOnly = true,
                               .Transient = true,
                               .Tooltip =
                                   "Internal dirty flag tracking if the variant state payload has been modified"};
    };
};

CH_MARK_RFL(UIControlComponent);

} // namespace Chained


#endif // CH_CONTROL_COMPONENT_H