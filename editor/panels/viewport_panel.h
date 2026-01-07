#ifndef CH_VIEWPORT_PANEL_H
#define CH_VIEWPORT_PANEL_H

#include "engine/scene/scene.h"
#include "viewport/editor_gizmo.h"
#include <raylib.h>

namespace CH
{
class ViewportPanel
{
public:
    ViewportPanel();
    ~ViewportPanel();

public:
    Entity OnImGuiRender(Scene *scene, const Camera3D &camera, Entity selectedEntity,
                         GizmoType &currentTool, EditorGizmo &gizmo, bool allowTools = true);

public:
    bool IsFocused() const;
    bool IsHovered() const;
    Vector2 GetSize() const;

private:
    RenderTexture2D m_ViewportTexture;
    Vector2 m_ViewportSize = {0, 0};
    bool m_Focused = false;
    bool m_Hovered = false;
};
} // namespace CH

#endif // CH_VIEWPORT_PANEL_H
