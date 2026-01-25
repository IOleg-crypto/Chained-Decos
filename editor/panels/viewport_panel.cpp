#include "viewport_panel.h"
#include "editor/editor_layer.h"
#include "engine/core/application.h"
#include "engine/core/input.h"
#include "engine/physics/physics.h"
#include "engine/renderer/render.h"
#include "engine/ui/imgui_raylib_ui.h"
#include <imgui.h>

namespace CHEngine
{
ViewportPanel::ViewportPanel()
{
    m_Name = "Viewport";
    m_ViewportTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

    m_EditorCamera.SetPosition({10.0f, 10.0f, 10.0f});
    m_EditorCamera.SetTarget({0.0f, 0.0f, 0.0f});
    m_EditorCamera.SetFOV(90.0f);
}

ViewportPanel::~ViewportPanel()
{
    UnloadRenderTexture(m_ViewportTexture);
}

void ViewportPanel::OnImGuiRender(bool readOnly)
{

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

    if (EditorLayer::Get().IsFullscreenGame())
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::Begin("Viewport", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove);
    }
    else
    {
        ImGui::Begin("Viewport");
    }

    m_Focused = ImGui::IsWindowFocused();
    m_Hovered = ImGui::IsWindowHovered();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos(); // Absolute position of viewport content

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    if (m_ViewportSize.x != viewportPanelSize.x || m_ViewportSize.y != viewportPanelSize.y)
    {
        if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
        {
            // Resize RenderTexture
            UnloadRenderTexture(m_ViewportTexture);
            m_ViewportTexture =
                LoadRenderTexture((int)viewportPanelSize.x, (int)viewportPanelSize.y);
            m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};
        }
    }

    // Render scene to texture
    Render::BeginToTexture(m_ViewportTexture);

    // Use white background for UI scenes, dark gray for 3D
    Color bgColor = (m_Context && m_Context->GetType() == SceneType::SceneUI)
                        ? Color{245, 245, 245, 255} // Light gray (UI canvas)
                        : DARKGRAY;                 // Dark gray (3D viewport)
    ClearBackground(bgColor);

    auto camera = m_EditorCamera.GetRaylibCamera();

    if (m_Context && m_ViewportSize.x > 0)
    {
        if (EditorLayer::GetSceneState() == SceneState::Play)
        {
            auto view = m_Context->GetRegistry().view<PlayerComponent, TransformComponent>();
            if (view.begin() != view.end())
            {
                auto entity = *view.begin();
                auto &transform = view.get<TransformComponent>(entity);
                auto &player = view.get<PlayerComponent>(entity);

                Vector3 target = transform.Translation;
                target.y += 1.0f;

                float yawRad = player.CameraYaw * DEG2RAD;
                float pitchRad = player.CameraPitch * DEG2RAD;
                Vector3 offset = {player.CameraDistance * cosf(pitchRad) * sinf(yawRad),
                                  player.CameraDistance * sinf(pitchRad),
                                  player.CameraDistance * cosf(pitchRad) * cosf(yawRad)};

                camera.position = Vector3Add(target, offset);
                camera.target = target;
                camera.up = {0.0f, 1.0f, 0.0f};
                camera.fovy = 60.0f;
                camera.projection = CAMERA_PERSPECTIVE;
            }
        }
        else if (m_Context->GetType() == SceneType::SceneUI)
        {
            // Pixel-perfect mapping for UI editing
            camera.target = {m_ViewportSize.x * 0.5f, m_ViewportSize.y * 0.5f, 0.0f};
            camera.position = {m_ViewportSize.x * 0.5f, m_ViewportSize.y * 0.5f, 10.0f};
            camera.up = {0.0f, -1.0f, 0.0f}; // Invert Y for screen space (0,0 is top-left)
            camera.fovy = m_ViewportSize.y;
            camera.projection = CAMERA_ORTHOGRAPHIC;
        }

        Render::BeginScene(camera);

        bool is3D = m_Context->GetType() == SceneType::Scene3D;

        if (m_DebugFlags && m_DebugFlags->DrawGrid && is3D)
            ::DrawGrid(10, 1.0f);

        // BVH/Collision wires
        if (m_DebugFlags && m_DebugFlags->DrawColliders && is3D)
        {
            // m_Context->OnDebugRender(m_DebugFlags);
        }
        m_Context->OnRender(camera, m_DebugFlags);
        Render::EndScene();
    }

    Render::EndToTexture();

    // --- Picking Logic ---
    bool allowTools = !readOnly;
    if (allowTools && m_Hovered && !m_Gizmo.IsHovered() && ImGui::IsMouseClicked(0) && m_Context)
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        Vector2 relativeMousePos = {mousePos.x - viewportPos.x, mousePos.y - viewportPos.y};

        // Get Ray from Camera
        // Note: Raylib's GetMouseRay uses GetWindowSize, which is wrong for ImGui Viewport.
        // We must calculate it manually or pretend window size is viewport size.
        // Manual Ray Calculation:
        Ray ray = {0};
        ray.position = camera.position;

        // Normalized Device Coordinates
        float x = (2.0f * relativeMousePos.x) / m_ViewportSize.x - 1.0f;
        float y = 1.0f - (2.0f * relativeMousePos.y) / m_ViewportSize.y;
        float z = 1.0f;

        // Inverse Projection and View Matrix
        Matrix matView = GetCameraMatrix(camera);
        Matrix matProj = MatrixPerspective(camera.fovy * DEG2RAD,
                                           m_ViewportSize.x / m_ViewportSize.y, 0.01f, 1000.0f);
        Matrix matViewProj = MatrixMultiply(matView, matProj);
        Matrix matInvViewProj = MatrixInvert(matViewProj);

        // Unproject
        Vector4 clipCoords = {x, y, z, 1.0f};
        // Transform to World
        // Note: Raylib matrix math might be column/row major specific.
        // Simplest fallback: usage of CameraUnproject if we can set window size context? No.

        // Alternative: Use Raylib's GetMouseRay but temporary set window dimensions? Dangerous.
        // Let's rely on GetMouseRay by converting relative pos to "Window" pos if the viewport
        // WAS the window. Actually, let's implement the Ray calculation directly using Raylib
        // math.

        Vector3 deviceCoords = {x, y, 1.0f};
        Vector3 farPoint = Vector3Transform(deviceCoords, matInvViewProj);
        // Perspective divide
        // Wait, Vector3Transform assumes w=1. For projection we need Vector4 mul.
        // Let's use Raylib GetMouseRay logic adapted:

        // Proper way using Raylib API:
        // We can't easily change global window state.
        // But we can reproduce `GetMouseRay`:
        // It uses `GetCameraMatrix` (View) and `MatrixPerspective` (Proj).
        // Let's use the exact same logic.

        // Calculate direction
        Vector3 target =
            Vector3Unproject({relativeMousePos.x, relativeMousePos.y, 1.0f}, matProj, matView);
        // Wait, Vector3Unproject doesn't take viewport size? In Raylib it assumes screen size?
        // Let's check Raylib source mentally:
        // Vector3Unproject(source, proj, view) assumes viewport is 0,0,width,height?
        // Raylib 5.0 Vector3Unproject:
        //   Calculate MatrixInv(View * Proj)
        //   Transform vector.
        //   Divide by w.
        //   However, input X,Y are expected in Screen Coordinates?
        //   NO, it expects viewport relative coordinates if we constructs the matrix correctly?
        //   Raylib internal implementation uses GetScreenWidth/Height for viewport mapping if
        //   not provided? Actually Raylib's Vector3Unproject doesn't take viewport rect. It
        //   takes source x,y which it maps from Viewport to NDC? No, Vector3Unproject inputs
        //   ARE coordinates. Wait, Raylib's Unproject assumes (0,0,w,h) viewport?

        // Let's use simpler approach:
        // Since we have Physics::Raycast, let's use a "Center Screen" ray for testing if
        // needed, but for Picking we need exact mouse ray.

        // Let's trust Raylib's picking ONLY IF we can offset picking?
        // No.

        // Let's Try:
        ray = GetMouseRay({relativeMousePos.x, relativeMousePos.y}, camera);
        // But Update Raylib's concept of screen size?
        // No, `GetMouseRay` internally calls `GetMousePosition()`? No, we pass mousePosition.
        // But it uses `GetScreenWidth()` for aspect ratio and normalization.

        // If we want correct ray, we should manually construct it.
        // Ray direction = Normalize(Unproject(MouseX, MouseY, 1.0f) - CameraPos)

        // Custom Unproject:
        // 1. NDC
        float ndc_x = (2.0f * relativeMousePos.x) / m_ViewportSize.x - 1.0f;
        float ndc_y = 1.0f - (2.0f * relativeMousePos.y) / m_ViewportSize.y; // Invert Y

        // 2. Clip Space
        Vector4 clip = {ndc_x, ndc_y, 1.0f, 1.0f}; // Forward

        // 3. World Space
        // We need MatrixMultiply(View, Proj) -> Invert.
        // Raylib Match:
        Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, m_ViewportSize.x / m_ViewportSize.y,
                                        0.01f, 1000.0f);
        Matrix view = GetCameraMatrix(camera);
        Matrix invVP = MatrixInvert(MatrixMultiply(view, proj));

        // Transform
        Quaternion q;        // Unused
        Vector3 scale;       // Unused
        Vector3 translation; // Unused
        // Just raw matrix mul
        // Manually:
        Vector4 worldPos;
        worldPos.x = clip.x * invVP.m0 + clip.y * invVP.m4 + clip.z * invVP.m8 + clip.w * invVP.m12;
        worldPos.y = clip.x * invVP.m1 + clip.y * invVP.m5 + clip.z * invVP.m9 + clip.w * invVP.m13;
        worldPos.z =
            clip.x * invVP.m2 + clip.y * invVP.m6 + clip.z * invVP.m10 + clip.w * invVP.m14;
        worldPos.w =
            clip.x * invVP.m3 + clip.y * invVP.m7 + clip.z * invVP.m11 + clip.w * invVP.m15;

        // Perspective Divide
        if (worldPos.w != 0.0f)
        {
            worldPos.x /= worldPos.w;
            worldPos.y /= worldPos.w;
            worldPos.z /= worldPos.w;
        }

        ray.position = camera.position;
        ray.direction = Vector3Normalize(
            Vector3Subtract({worldPos.x, worldPos.y, worldPos.z}, camera.position));

        // Perform Raycast
        RaycastResult res = Physics::Raycast(m_Context.get(), ray);
        if (res.Hit)
        {
            m_SelectedEntity = Entity{res.Entity, m_Context.get()};
            EntitySelectedEvent e(res.Entity, m_Context.get(), res.MeshIndex);
            // We should probably dispatch this through a global event system
            // For now, if we have a way to reach the layer...
            // better yet, panels should just dispatch events that the layer can catch.
        }
        else
        {
            m_SelectedEntity = {};
            EntitySelectedEvent e(entt::null, m_Context.get(), -1);
        }
    }

    EndTextureMode();

    // Draw texture in ImGui
    rlImGuiImageRenderTextureFit(&m_ViewportTexture, true);

    // Gizmos
    if (m_SelectedEntity && m_SelectedEntity.GetScene() != m_Context.get())
        m_SelectedEntity = {};

    bool isUIEntity = m_SelectedEntity && m_SelectedEntity.HasComponent<WidgetComponent>();
    if (m_SelectedEntity && allowTools && !isUIEntity && m_Context &&
        m_Context->GetType() != SceneType::SceneUI)
    {
        m_Gizmo.RenderAndHandle(m_Context.get(), camera, m_SelectedEntity, m_CurrentTool,
                                viewportPos, ImVec2{m_ViewportSize.x, m_ViewportSize.y});
    }

    // --- Gizmo Toolbar Overlay ---
    if (m_Context && m_Context->GetType() == SceneType::Scene3D)
    {
        ImGui::SetCursorPos(ImVec2(10, 30));                          // Slightly below top-left
        ImGui::BeginChild("GizmoToolbar", ImVec2(160, 40), false, 0); // Transparent background

        auto drawToolButton = [&](const char *label, GizmoType type, GizmoType target)
        {
            bool active = (type == target);
            if (active)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
            else
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0.5f));

            if (ImGui::Button(label, ImVec2(30, 30)))
                m_CurrentTool = target;

            ImGui::PopStyleColor();
        };

        drawToolButton("T", m_CurrentTool, GizmoType::TRANSLATE);
        ImGui::SameLine();
        drawToolButton("R", m_CurrentTool, GizmoType::ROTATE);
        ImGui::SameLine();
        drawToolButton("S", m_CurrentTool, GizmoType::SCALE);

        ImGui::SameLine();
        bool isLocal = m_Gizmo.IsLocalSpace();
        if (ImGui::Button(isLocal ? "L" : "W", ImVec2(30, 30)))
            m_Gizmo.SetLocalSpace(!isLocal);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(isLocal ? "Local Space" : "World Space");

        ImGui::EndChild();

        // --- Snapping / Overlay Toolbar ---
        ImGui::SetCursorPos(ImVec2(10, 10));
        ImGui::BeginChild("ViewportToolbar", ImVec2(0, 0),
                          ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY |
                              ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        bool snapping = m_Gizmo.IsSnappingEnabled();
        if (ImGui::Checkbox("Snap", &snapping))
            m_Gizmo.SetSnapping(snapping);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        float grid = m_Gizmo.GetGridSize();
        if (ImGui::DragFloat("##Grid", &grid, 0.1f, 0.1f, 10.0f, "Grid: %.1f"))
            m_Gizmo.SetGridSize(grid);

        ImGui::EndChild();
    }

    if (m_Context)
    {
        if (EditorLayer::GetSceneState() == SceneState::Play ||
            m_Context->GetType() == SceneType::SceneUI)
        {
            bool editMode = EditorLayer::GetSceneState() == SceneState::Edit;
            m_Context->OnImGuiRender(viewportPos, viewportPanelSize, ImGui::GetWindowViewport()->ID,
                                     editMode);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::OnUpdate(float deltaTime)
{
    if (m_Focused || Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        m_EditorCamera.OnUpdate(deltaTime);
    }
}

void ViewportPanel::OnEvent(Event &e)
{
    // m_EditorCamera.OnEvent(e); // Removed as EditorCamera handles update in OnUpdate

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<EntitySelectedEvent>(
        [this](EntitySelectedEvent &ev)
        {
            m_SelectedEntity = Entity{ev.GetEntity(), ev.GetScene()};
            return false;
        });

    dispatcher.Dispatch<KeyPressedEvent>(
        [this](KeyPressedEvent &ev)
        {
            if (m_Focused && !Input::IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            {
                switch (ev.GetKeyCode())
                {
                case KEY_Q:
                    m_CurrentTool = GizmoType::NONE;
                    return true;
                case KEY_W:
                    m_CurrentTool = GizmoType::TRANSLATE;
                    return true;
                case KEY_E:
                    m_CurrentTool = GizmoType::ROTATE;
                    return true;
                case KEY_R:
                    m_CurrentTool = GizmoType::SCALE;
                    return true;
                }
            }
            return false;
        });
}
} // namespace CHEngine
