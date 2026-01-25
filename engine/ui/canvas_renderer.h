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
    static void DrawImage(struct ImageWidget &img, const ImVec2 &absPos, const ImVec2 &size,
                          const Color &overrideColor);
    static void DrawText(struct TextWidget &txt, const ImVec2 &absPos, const ImVec2 &size);
};
} // namespace CHEngine

#endif // CH_CANVAS_RENDERER_H
