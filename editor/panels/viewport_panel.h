#ifndef CH_VIEWPORT_PANEL_H
#define CH_VIEWPORT_PANEL_H

#include "engine/core/timestep.h"
#include "panel.h"
#include "imgui.h"
#include "imgui/IconsFontAwesome6.h"
#include <memory>
#include "engine/graphics/api/framebuffer.h"
#include "viewport/editor_camera.h"
#include "viewport/editor_gizmo.h"
#include "viewport/ui_manipulator.h"

namespace CHEngine
{
struct GizmoBtn
{
    GizmoType type;
    const char* icon;
    const char* tooltip;
    int key;
};

class ViewportPanel : public Panel
{
public:
    ViewportPanel();
    ~ViewportPanel();

public:
    virtual void OnImGuiRender(bool readOnly = false) override;
    virtual void OnUpdate(Timestep ts) override;
    virtual void OnEvent(Event& e) override;

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

    GizmoType& GetCurrentTool()
    {
        return m_CurrentTool;
    }

public:
    void DrawGizmoButtons();
    void DrawCameraSelector(class Scene* scene);

private:
    std::shared_ptr<Framebuffer> m_ViewportFramebuffer;
    std::shared_ptr<Framebuffer> m_HDRFramebuffer;
    Vector2 m_ViewportSize = {0, 0};
    bool m_Focused = false;
    bool m_Hovered = false;

    std::unique_ptr<EditorCameraController> m_CameraController;
    EditorGizmo m_Gizmo;
    EditorUIManipulator m_UIManipulator;
    GizmoType m_CurrentTool = GizmoType::TRANSLATE;
    Entity m_SelectedEntity;
    std::unique_ptr<class SceneRenderer> m_SceneRenderer;

    // UI Interaction state
    ImVec2 m_UIDragOffset = {0, 0};

    // Viewport Camera State
    uint64_t m_ViewportCameraEntityUUID = 0; // 0 = Editor Camera
};

} // namespace CHEngine

#endif // CH_VIEWPORT_PANEL_H
