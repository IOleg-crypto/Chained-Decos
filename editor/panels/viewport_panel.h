#ifndef CH_VIEWPORT_PANEL_H
#define CH_VIEWPORT_PANEL_H

#include "engine/core/timestep.h"
#include "extras/IconsFontAwesome6.h"
#include "panel.h"
#include "raylib.h"
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

static const GizmoBtn s_GizmoBtns[] = {{GizmoType::NONE, ICON_FA_ARROW_POINTER, "Select (Q)", KEY_Q},
                                       {GizmoType::TRANSLATE, ICON_FA_UP_DOWN_LEFT_RIGHT, "Translate (W)", KEY_W},
                                       {GizmoType::ROTATE, ICON_FA_ARROWS_ROTATE, "Rotate (E)", KEY_E},
                                       {GizmoType::SCALE, ICON_FA_UP_RIGHT_FROM_SQUARE, "Scale (R)", KEY_R}};

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

    GizmoType& GetCurrentTool()
    {
        return m_CurrentTool;
    }

public:
    void DrawGizmoButtons();
    void DrawCameraSelector(class Scene* scene);

private:
    RenderTexture2D m_ViewportTexture;
    RenderTexture2D m_HDRTexture;
    Vector2 m_ViewportSize = {0, 0};
    bool m_Focused = false;
    bool m_Hovered = false;

    std::unique_ptr<EditorCameraController> m_CameraController;
    EditorGizmo m_Gizmo;
    EditorUIManipulator m_UIManipulator;
    GizmoType m_CurrentTool = GizmoType::TRANSLATE;
    Entity m_SelectedEntity;

    // UI Interaction state
    ImVec2 m_UIDragOffset = {0, 0};

    // Viewport Camera State
    uint64_t m_ViewportCameraEntityUUID = 0; // 0 = Editor Camera
};

} // namespace CHEngine

#endif // CH_VIEWPORT_PANEL_H
