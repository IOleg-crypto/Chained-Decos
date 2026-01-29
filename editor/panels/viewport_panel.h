#ifndef CH_VIEWPORT_PANEL_H
#define CH_VIEWPORT_PANEL_H

// Removed redundant include: engine/graphics/render.h
#include "panel.h"
#include "raylib.h"
#include "viewport/editor_camera.h"
#include "viewport/editor_gizmo.h"


namespace CHEngine
{
class ViewportPanel : public Panel
{
public:
    ViewportPanel();
    ~ViewportPanel();

    virtual void OnImGuiRender(bool readOnly = false) override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnEvent(Event &e) override;

    bool IsFocused() const
    {
        return m_Focused;
    }
    bool IsHovered() const
    {
        return m_Hovered;
    }
    Vector2 GetSize() const
    {
        return m_ViewportSize;
    }

    EditorCamera &GetCamera()
    {
        return m_EditorCamera;
    }
    GizmoType &GetCurrentTool()
    {
        return m_CurrentTool;
    }

private:
    RenderTexture2D m_ViewportTexture;
    Vector2 m_ViewportSize = {0, 0};
    bool m_Focused = false;
    bool m_Hovered = false;

    EditorCamera m_EditorCamera;
    EditorGizmo m_Gizmo;
    GizmoType m_CurrentTool = GizmoType::TRANSLATE;
    Entity m_SelectedEntity;
};

} // namespace CHEngine

#endif // CH_VIEWPORT_PANEL_H
