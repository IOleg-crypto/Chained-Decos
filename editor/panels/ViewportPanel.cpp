#include "ViewportPanel.h"
#include "../viewport/ViewportPicking.h"
#include "core/application/Application.h"
#include "core/physics/Physics.h"
#include "scene/core/Scene.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/RenderComponent.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/resources/map/MapRenderer.h"
#include <imgui.h>

using namespace CHEngine;
#include <imgui_internal.h>
#include <raymath.h>
#include <rlImGui.h>
#include <rlgl.h>

namespace CHEngine
{

// =========================================================================
// State & Configuration
// =========================================================================

bool ViewportPanel::IsFocused() const
{
    return m_Focused;
}

bool ViewportPanel::IsHovered() const
{
    return m_Hovered;
}

bool ViewportPanel::IsVisible() const
{
    return m_isVisible;
}

void ViewportPanel::SetVisible(bool visible)
{
    m_isVisible = visible;
}

ImVec2 ViewportPanel::GetSize() const
{
    return {(float)m_Width, (float)m_Height};
}

// =========================================================================
// Panel Lifecycle
// =========================================================================

ViewportPanel::~ViewportPanel()
{
    if (m_ViewportTexture.id != 0)
        UnloadRenderTexture(m_ViewportTexture);
}

void ViewportPanel::OnImGuiRender(
    SceneState sceneState, SelectionType selectionType,
    const std::shared_ptr<GameScene> &legacyScene, const std::shared_ptr<Scene> &modernScene,
    const Camera3D &camera, int selectedObjectIndex, Tool currentTool,
    const std::function<void(int)> &onSelect, CommandHistory *history,
    const std::function<void(const std::string &, const Vector3 &)> &onAssetDropped)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("Viewport", nullptr, windowFlags);

    m_Focused = ImGui::IsWindowFocused();
    m_Hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
                                       ImGuiHoveredFlags_ChildWindows);

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    if (m_Width != (uint32_t)viewportPanelSize.x || m_Height != (uint32_t)viewportPanelSize.y)
    {
        Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
    }

    if (m_ViewportTexture.id != 0)
    {
        // 1. Render Scene to Texture
        BeginTextureMode(m_ViewportTexture);

        if (legacyScene)
        {
            const auto &meta = legacyScene->GetMapMetaData();
            if (meta.sceneType == SceneType::UI_MENU)
            {
                m_Renderer.RenderUIBackground(meta, m_Width, m_Height);
            }
            else
            {
                ClearBackground(DARKGRAY);
            }

            MapRenderer renderer;
            // Enter 3D Mode for ALL editor 3D drawing
            BeginMode3D(camera);

            // Ensure depth test is ON
            rlEnableDepthTest();
            rlEnableDepthMask();

            // 1. Draw Map Content
            bool hideSpawnZones = (sceneState != SceneState::Edit);
            bool isUIScene = (legacyScene->GetMapMetaData().sceneType == SceneType::UI_MENU);

            if (!isUIScene)
            {
                renderer.DrawMapContent(*legacyScene, camera, hideSpawnZones);
            }

            // 1.6 Draw New Scene Entities (New system)
            if (modernScene)
            {
                auto &registry = modernScene->GetRegistry();
                auto sceneView = registry.view<TransformComponent, RenderComponent>();
                for (auto entity : sceneView)
                {
                    auto &transform = sceneView.get<TransformComponent>(entity);
                    auto &render = sceneView.get<RenderComponent>(entity);

                    if (!render.visible || !render.model)
                        continue;

                    Matrix translation = MatrixTranslate(transform.position.x + render.offset.x,
                                                         transform.position.y + render.offset.y,
                                                         transform.position.z + render.offset.z);
                    Matrix rotation = MatrixRotateXYZ(transform.rotation);
                    Matrix scale =
                        MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z);

                    Matrix modelTransform =
                        MatrixMultiply(MatrixMultiply(scale, rotation), translation);
                    render.model->transform = modelTransform;

                    DrawModel(*render.model, {0, 0, 0}, 1.0f, render.tint);
                }
            }

            // 1.7 Draw Physics Debug Visualization
            if (Application::Get().IsCollisionDebugVisible() ||
                Application::Get().IsDebugInfoVisible())
            {
                Physics::Render();
            }

            // 2. Editor Helpers
            if (!m_GridInitialized)
            {
                m_Grid.Init();
                m_GridInitialized = true;
            }

            // 3. Picking & Gizmo Logic (Decomposed)
            if (sceneState == SceneState::Edit)
            {
                ViewportPicking picker;
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 viewportPos = ImGui::GetCursorScreenPos();
                ImVec2 viewportSize = {(float)m_Width, (float)m_Height};

                // Handle Gizmo interaction and rendering
                bool gizmoInteracting =
                    m_Gizmo.RenderAndHandle(legacyScene, camera, selectedObjectIndex, currentTool,
                                            viewportSize, m_Hovered, history);

                // Object Picking (If hovered, clicked, and NOT interacting with gizmo)
                if (m_Hovered && ImGui::IsMouseClicked(0) && !gizmoInteracting)
                {
                    int hitIndex =
                        picker.PickObject(mousePos, viewportPos, viewportSize, camera, legacyScene);
                    if (hitIndex != -1)
                        onSelect(hitIndex);
                }
            }

            // Draw selection highlight via extracted renderer (ONLY in Edit mode)
            if (sceneState == SceneState::Edit && selectedObjectIndex >= 0 &&
                selectedObjectIndex < (int)legacyScene->GetMapObjects().size())
            {
                m_Renderer.RenderSelectionHighlight(
                    legacyScene->GetMapObjects()[selectedObjectIndex], legacyScene->GetMapModels(),
                    camera);
            }

            // Draw Grid
            if (sceneState == SceneState::Edit && !isUIScene)
                m_Grid.Draw(camera, m_Width, m_Height);

            EndMode3D();

            // 4. Render UI Elements and Labels
            if (isUIScene)
            {
                int selectedUIIndex =
                    (selectionType == SelectionType::UI_ELEMENT) ? selectedObjectIndex : -1;
                m_Renderer.RenderUIElements(legacyScene->GetUIElements(), selectedUIIndex);
            }

            // Draw 2D Labels AFTER EndMode3D (ONLY in Edit mode)
            if (sceneState == SceneState::Edit && selectedObjectIndex >= 0 &&
                selectedObjectIndex < (int)legacyScene->GetMapObjects().size())
            {
                m_Renderer.RenderAxisLabels(legacyScene->GetMapObjects()[selectedObjectIndex],
                                            camera, currentTool);
            }
        }

        EndTextureMode();
        rlImGuiImageRenderTextureFit(&m_ViewportTexture, true);

        // --- Drag and Drop Target ---
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string assetPath = (const char *)payload->Data;

                // Create Raycast to find placement point
                ViewportPicking picker;
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 viewportPos = ImGui::GetCursorScreenPos(); // Still valid here?
                // Actually need the image position, ImGui::GetItemRectMin()
                ImVec2 itemPos = ImGui::GetItemRectMin();
                ImVec2 itemSize = ImGui::GetItemRectSize();

                Ray ray = picker.GetMouseRay(mousePos, itemPos, itemSize, camera);

                // Default placement on ground plane (y=0)
                float t = -ray.position.y / ray.direction.y;
                Vector3 worldPos = Vector3Add(ray.position, Vector3Scale(ray.direction, t));

                if (onAssetDropped)
                    onAssetDropped(assetPath, worldPos);
            }
            ImGui::EndDragDropTarget();
        }

        // --- Stats Overlay ---
        {
            ImGui::SetCursorPos(ImVec2(10, 10));
            ImGui::BeginChild("StatsOverlay", ImVec2(0, 0),
                              ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX |
                                  ImGuiChildFlags_AutoResizeY,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoBackground);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %d", GetFPS());
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Time: %.3f ms",
                               GetFrameTime() * 1000.0f);
            ImGui::EndChild();
        }

        // --- Snapping Toolbar Overlay ---
        if (sceneState == SceneState::Edit)
        {
            ImGui::SetCursorPos(ImVec2(10, 50));
            ImGui::BeginChild("SnappingToolbar", ImVec2(0, 0),
                              ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysAutoResize |
                                  ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            bool snapping = m_Gizmo.IsSnappingEnabled();
            if (ImGui::Checkbox("Snap", &snapping))
                m_Gizmo.SetSnapping(snapping);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            float grid = m_Gizmo.GetGridSize();
            if (ImGui::DragFloat("##Grid", &grid, 0.1f, 0.1f, 10.0f, "Grid: %.1f"))
                m_Gizmo.SetGridSize(grid);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            float rot = m_Gizmo.GetRotationStep();
            if (ImGui::DragFloat("##Rot", &rot, 1.0f, 1.0f, 180.0f, "Rot: %.0f"))
                m_Gizmo.SetRotationStep(rot);

            ImGui::EndChild();
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;
    if (m_ViewportTexture.id != 0)
        UnloadRenderTexture(m_ViewportTexture);
    m_Width = width;
    m_Height = height;
    m_ViewportTexture = LoadRenderTexture(m_Width, m_Height);
}

Vector2 ViewportPanel::GetViewportMousePosition() const
{
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentPos = ImGui::GetWindowContentRegionMin();

    return {mousePos.x - (windowPos.x + contentPos.x), mousePos.y - (windowPos.y + contentPos.y)};
}

} // namespace CHEngine
