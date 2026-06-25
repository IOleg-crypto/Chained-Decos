#ifndef CH_UI_CONTROL_RENDERER_H
#define CH_UI_CONTROL_RENDERER_H

#include "engine/scene/components.h"
#include "engine/scene/entity.h"
#include "imgui.h"

namespace Chained
{
class UIFontRegistry;

// Free function to render any UI control based on its variant type.
// Eliminates the boilerplate single-function Dispatcher struct.
bool RenderControl(const UIFontRegistry& fontRegistry,
                   Entity entity, UIControlComponent& control, const ImVec2& screenPos, const ImVec2& size);

} // namespace Chained
#endif // CH_UI_CONTROL_RENDERER_H
