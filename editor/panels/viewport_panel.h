#ifndef CH_VIEWPORT_PANEL_H
#define CH_VIEWPORT_PANEL_H

#include "engine/core/events.h"
#include "engine/renderer/render.h"
#include "engine/renderer/scene_render.h"
#include "engine/scene/scene.h"
#include "viewport/editor_gizmo.h"
#include <raylib.h>

namespace CHEngine
{
class ViewportPanel
{
public:
    ViewportPanel();
    ~ViewportPanel();

public:
    Entity OnImGuiRender(Scene *scene, const Camera3D &camera, Entity selectedEntity,
                         GizmoType &currentTool, EditorGizmo &gizmo,
                         const DebugRenderFlags *debugFlags, bool allowTools = true);

public:
    void SetEventCallback(const EventCallbackFn &callback)
    {
        m_EventCallback = callback;
    }

    bool IsFocused() const;
    bool IsHovered() const;
    Vector2 GetSize() const;

private:
    RenderTexture2D m_ViewportTexture;
    Vector2 m_ViewportSize = {0, 0};
    bool m_Focused = false;
    bool m_Hovered = false;
    EventCallbackFn m_EventCallback;
};

} // namespace CHEngine

#endif // CH_VIEWPORT_PANEL_H
