#ifndef CH_CANVAS_RENDERER_H
#define CH_CANVAS_RENDERER_H

#include "engine/scene/components/widget_component.h"
#include "engine/scene/entity.h"
#include <imgui.h>

namespace CHEngine
{
class CanvasRenderer
{
public:
    static void DrawEntity(Entity entity, const ImVec2 &parentPos, const ImVec2 &parentSize,
                           bool editMode = false);

private:
    static void DrawStyledRect(const ImVec2 &p_min, const ImVec2 &p_max, const UIStyle &style,
                               bool isHovered, bool isPressed);
    static void DrawStyledText(const std::string &text, const ImVec2 &absPos, const ImVec2 &size,
                               const TextStyle &style);

    static void HandleButton(Entity entity, const ImVec2 &pos, const ImVec2 &size);
    static void HandlePanel(Entity entity, const ImVec2 &pos, const ImVec2 &size);
    static void HandleLabel(Entity entity, const ImVec2 &pos, const ImVec2 &size);
};
} // namespace CHEngine

#endif // CH_CANVAS_RENDERER_H
