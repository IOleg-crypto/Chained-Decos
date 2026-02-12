#ifndef CH_VIEWPORT_PANEL_H
#define CH_VIEWPORT_PANEL_H

// Removed redundant include: engine/graphics/render.h
#include "panel.h"
#include "raylib.h"
#include "viewport/editor_camera.h"
#include "viewport/editor_gizmo.h"
#include "viewport/ui_manipulator.h"
#include "engine/core/timestep.h"

namespace CHEngine
{
struct GizmoBtn { GizmoType type; const char* icon; const char* tooltip; int key; };

class ViewportPanel : public Panel
{
public:
    ViewportPanel();
    ~ViewportPanel();

public:
    virtual void OnImGuiRender(bool readOnly = false) override;
    virtual void OnUpdate(Timestep ts) override;

public:
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
public: 
    void DrawGizmoButtons();

private:
    RenderTexture2D m_ViewportTexture;
    Vector2 m_ViewportSize = {0, 0};
    bool m_Focused = false;
    bool m_Hovered = false;

    EditorCamera m_EditorCamera;
    EditorGizmo m_Gizmo;
    EditorUIManipulator m_UIManipulator;
    GizmoType m_CurrentTool = GizmoType::TRANSLATE;
    Entity m_SelectedEntity;
    std::unique_ptr<class SceneRenderer> m_SceneRenderer;
    
    // UI Interaction state
    ImVec2 m_UIDragOffset = {0, 0};
};

} // namespace CHEngine

#endif // CH_VIEWPORT_PANEL_H
