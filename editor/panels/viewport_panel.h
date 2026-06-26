#ifndef CH_VIEWPORT_PANEL_H
#define CH_VIEWPORT_PANEL_H

#include "panel.h"
#include "engine/foundation/timestep.h"
#include "viewport/editor_camera.h"
#include "viewport/editor_gizmo.h"
#include "viewport/ui_manipulator.h"
#include "physics/collision_core.h" 
#include "editor_icons.h"           

#include <memory>
#include <glm/glm.hpp>
#include <entt/entt.hpp> 
#include "engine/core/key_codes.h"


namespace Chained
{

class Framebuffer;
class Scene;
class SceneRenderer;
class Renderer;
class TextureManager;
struct SceneSettings;
struct Camera3D;
class Event;

struct GizmoBtn
{
    GizmoType type;
    const char* icon;
    const char* tooltip;
    KeyCode key;
};

class ViewportPanel : public Panel
{
public:
    ViewportPanel();
    ~ViewportPanel() override;

    void OnImGuiRender(bool readOnly = false) override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

    bool IsFocused() const { return m_Focused; }
    bool IsHovered() const { return m_Hovered; }
    glm::vec2 GetSize() const { return m_ViewportSize; }
    GizmoType& GetCurrentTool() { return m_CurrentTool; }

    void DrawGizmoButtons();
    void DrawCameraSelector(Scene* scene);
    Ray GetMouseRay(const glm::vec2& mousePosition);

private:
    // UI та Стан в'юпорту
    glm::vec2 m_ViewportSize = {0, 0};
    bool m_Focused = false;
    bool m_Hovered = false;
    GizmoType m_CurrentTool = GizmoType::TRANSLATE;

    // Системи керування камерою та маніпуляціями
    std::unique_ptr<EditorCameraController> m_CameraController;
    EditorGizmo m_Gizmo;
    EditorUIManipulator m_UIManipulator;
    EditorIcons m_EditorIcons;

    // Буфери для рендерингу (залишаємо share_ptr, але класи задекларовані вище)
    std::shared_ptr<Framebuffer> m_ViewportFramebuffer;
    std::shared_ptr<Framebuffer> m_HDRFramebuffer;

    // Engine subsystem pointers are now accessed via static APIs (Renderer::Get(), etc.)
    SceneRenderer* m_SceneRenderer = nullptr;
    TextureManager* m_TextureManager = nullptr;

private:
    void HandleResize(const ImVec2& viewportSize, Scene* activeScene);
    void RenderViewportScene(Scene* activeScene);
    void HandleDragDrop(Scene* activeScene);
    void RenderOverlays(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos);
    void HandlePicking(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos);
    void RenderToolbar(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos);
    void RenderLaunchHUD(const ImVec2& viewportSize, const ImVec2& viewportScreenPos);

    // Метод малювання іконок, який ми успішно забрали з SceneRenderer
    void RenderEditorIcons(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera);
    void ClearSceneBackground(Scene* scene);
};

} // namespace Chained

#endif // CH_VIEWPORT_PANEL_H