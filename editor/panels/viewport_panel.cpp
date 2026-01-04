#include "viewport_panel.h"
#include "engine/renderer/renderer.h"
#include <imgui.h>
#include <rlImGui.h>

namespace CH
{
ViewportPanel::ViewportPanel()
{
    m_ViewportTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
}

ViewportPanel::~ViewportPanel()
{
    UnloadRenderTexture(m_ViewportTexture);
}

void ViewportPanel::OnImGuiRender(Scene *scene, const Camera3D &camera, Entity selectedEntity,
                                  GizmoType currentTool, EditorGizmo &gizmo)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin("Viewport");

    m_Focused = ImGui::IsWindowFocused();
    m_Hovered = ImGui::IsWindowHovered();

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
        Renderer::BeginScene(camera);
        Renderer::DrawGrid(20, 1.0f);
        Renderer::DrawScene(scene);

        // Gizmos
        if (selectedEntity)
        {
            gizmo.RenderAndHandle(scene, camera, selectedEntity, currentTool,
                                  {m_ViewportSize.x, m_ViewportSize.y}, m_Hovered);
        }

        Renderer::EndScene();
    }

    EndTextureMode();

    // Draw texture in ImGui
    rlImGuiImageRenderTextureFit(&m_ViewportTexture, true);

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
    ImGui::SetNextItemWidth(80);
    float grid = gizmo.GetGridSize();
    if (ImGui::DragFloat("##Grid", &grid, 0.1f, 0.1f, 10.0f, "Grid: %.1f"))
        gizmo.SetGridSize(grid);

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar();
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
} // namespace CH
