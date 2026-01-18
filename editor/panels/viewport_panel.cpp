#include "viewport_panel.h"
#include "engine/physics/physics.h"
#include "engine/renderer/render.h"
#include "engine/renderer/scene_render.h"
#include <imgui.h>
#include <rlImGui.h>

namespace CHEngine
{
ViewportPanel::ViewportPanel()
{
    m_ViewportTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
}

ViewportPanel::~ViewportPanel()
{
    UnloadRenderTexture(m_ViewportTexture);
}

Entity ViewportPanel::OnImGuiRender(Scene *scene, const Camera3D &camera, Entity selectedEntity,
                                    GizmoType &currentTool, EditorGizmo &gizmo,
                                    const DebugRenderFlags *debugFlags, bool allowTools)
{
    Entity pickedEntity = {};

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Viewport");

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
    BeginTextureMode(m_ViewportTexture);
    ClearBackground(DARKGRAY);

    if (scene && m_ViewportSize.x > 0)
    {
        SceneRender::BeginScene(scene, camera);
        if (debugFlags && debugFlags->DrawGrid)
            Render::DrawGrid((int)viewportPanelSize.x, 1.0f);
        SceneRender::SubmitScene(scene, debugFlags);

        SceneRender::EndScene();
    }

    // --- Picking Logic ---
    if (allowTools && m_Hovered && !gizmo.IsHovered() && ImGui::IsMouseClicked(0) && scene)
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
        RaycastResult res = Physics::Raycast(scene, ray);
        if (res.Hit)
        {
            pickedEntity = Entity{res.Entity, scene};
            if (m_EventCallback)
            {
                EntitySelectedEvent e(res.Entity, scene, res.MeshIndex);
                m_EventCallback(e);
            }
        }
        else
        {
            pickedEntity = {}; // Deselect if clicked empty space
            if (m_EventCallback)
            {
                EntitySelectedEvent e(entt::null, scene, -1);
                m_EventCallback(e);
            }
        }
    }

    EndTextureMode();

    // Draw texture in ImGui
    rlImGuiImageRenderTextureFit(&m_ViewportTexture, true);

    // Gizmos
    if (selectedEntity && allowTools)
    {
        gizmo.RenderAndHandle(scene, camera, selectedEntity, currentTool, viewportPos,
                              ImVec2{m_ViewportSize.x, m_ViewportSize.y});
    }

    // --- Gizmo Toolbar Overlay ---
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
            currentTool = target;

        ImGui::PopStyleColor();
    };

    drawToolButton("T", currentTool, GizmoType::TRANSLATE);
    ImGui::SameLine();
    drawToolButton("R", currentTool, GizmoType::ROTATE);
    ImGui::SameLine();
    drawToolButton("S", currentTool, GizmoType::SCALE);

    ImGui::SameLine();
    bool isLocal = gizmo.IsLocalSpace();
    if (ImGui::Button(isLocal ? "L" : "W", ImVec2(30, 30)))
        gizmo.SetLocalSpace(!isLocal);

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(isLocal ? "Local Space" : "World Space");

    ImGui::EndChild();

    // Reset cursor for overlay logic if needed, but EndChild handles it.

    // --- Snapping / Overlay Toolbar ---
    ImGui::SetCursorPos(ImVec2(10, 10));
    ImGui::BeginChild("ViewportToolbar", ImVec2(0, 0),
                      ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY |
                          ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    bool snapping = gizmo.IsSnappingEnabled();
    if (ImGui::Checkbox("Snap", &snapping))
        gizmo.SetSnapping(snapping);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(40);
    float grid = gizmo.GetGridSize();
    if (ImGui::DragFloat("##Grid", &grid, 0.1f, 0.1f, 10.0f, "Grid: %.1f"))
        gizmo.SetGridSize(grid);

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar();

    return pickedEntity;
}
bool ViewportPanel::IsFocused() const
{
    return m_Focused;
}
bool ViewportPanel::IsHovered() const
{
    return m_Hovered;
}
Vector2 ViewportPanel::GetSize() const
{
    return m_ViewportSize;
}
} // namespace CHEngine
