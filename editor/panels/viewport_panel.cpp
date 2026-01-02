#include "viewport_panel.h"
#include "../viewport/viewport_picking.h"
#include "core/application/application.h"
#include "core/physics/physics.h"
#include "core/renderer/renderer.h"
#include "editor/logic/editor_scene_manager.h"
#include "scene/core/scene.h"
#include "scene/ecs/components/render_component.h"
#include "scene/ecs/components/transform_component.h"
#include "scene/ecs/systems/render_system.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <raymath.h>
#include <rlImGui.h>
#include <rlgl.h>

#include "editor/camera/editor_camera.h"
#include "editor/logic/editor_entity_factory.h"
#include "editor/logic/editor_project_actions.h"
#include "editor/logic/editor_scene_actions.h"
#include "editor/logic/scene_simulation_manager.h"
#include "editor/logic/selection_manager.h"
#include "editor/logic/undo/command_history.h"
#include "scene/ecs/systems/skybox_system.h"
#include "scene/ecs/systems/ui_render_system.h"

namespace CHEngine
{

ViewportPanel::ViewportPanel(EditorSceneActions *sceneActions, EditorProjectActions *projectActions,
                             SelectionManager *selection, SceneSimulationManager *simulation,
                             EditorCamera *camera, EditorEntityFactory *factory,
                             CommandHistory *history, Tool *activeTool)
    : m_SceneActions(sceneActions), m_ProjectActions(projectActions), m_SelectionManager(selection),
      m_SimulationManager(simulation), m_Camera(camera), m_EntityFactory(factory),
      m_CommandHistory(history), m_ActiveTool(activeTool)
{
}

ViewportPanel::~ViewportPanel()
{
    if (m_ViewportTexture.id != 0)
        UnloadRenderTexture(m_ViewportTexture);
}

bool ViewportPanel::IsFocused() const
{
    return m_Focused;
}

bool ViewportPanel::IsHovered() const
{
    return m_Hovered;
}

ImVec2 ViewportPanel::GetSize() const
{
    return {(float)m_Width, (float)m_Height};
}

void ViewportPanel::OnImGuiRender()
{
    SceneState sceneState = m_SimulationManager->GetSceneState();
    entt::entity selectedEntity = m_SelectionManager->GetSelectedEntity();

    std::shared_ptr<CHEngine::Scene> gameScene = m_SceneActions->GetActiveScene();

    Camera3D camera = m_Camera->GetCamera();
    Tool currentTool = m_ActiveTool ? *m_ActiveTool : Tool::MOVE;

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
            BeginTextureMode(m_ViewportTexture);
            ClearBackground(DARKGRAY);

            BeginMode3D(camera);
            rlEnableDepthTest();
            rlEnableDepthMask();

            SkyboxSystem::Render(gameScene->GetRegistry());

            if (gameScene)
            {
                RenderSystem::Render(gameScene->GetRegistry());
            }

            if (Renderer::IsCollisionDebugVisible() || Renderer::IsDebugInfoVisible() ||
                m_ShowPhysicsDebug)
            {
                Physics::Render();
            }

            if (!m_GridInitialized)
            {
                m_Grid.Init();
                m_GridInitialized = true;
            }

            if (sceneState == SceneState::Edit)
            {
                ViewportPicking picker;
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 viewportPos = ImGui::GetCursorScreenPos();
                ImVec2 viewportSize = {(float)m_Width, (float)m_Height};

                bool gizmoInteracting =
                    m_Gizmo.RenderAndHandle(gameScene, camera, selectedEntity, currentTool,
                                            viewportSize, m_Hovered, m_CommandHistory);

                if (m_Hovered && ImGui::IsMouseClicked(0) && !gizmoInteracting)
                {
                    if (gameScene)
                    {
                        entt::entity hitEntity = picker.PickEntity(
                            mousePos, viewportPos, viewportSize, camera, *gameScene);
                        if (hitEntity != entt::null)
                            m_SelectionManager->SetSelection(hitEntity);
                        else
                            m_SelectionManager->ClearSelection();
                    }
                }
            }

            if (sceneState == SceneState::Edit && !m_GridInitialized)
            {
                m_Grid.Init();
                m_GridInitialized = true;
            }
            m_Grid.Draw(camera, m_Width, m_Height);

            EndMode3D();

            if (m_ShowHUD && gameScene)
            {
                UIRenderSystem::RenderHUD(gameScene->GetRegistry(), m_Width, m_Height);
            }

            if (m_ShowFPS)
            {
                DrawText(TextFormat("FPS: %d", GetFPS()), 10, m_Height - 30, 20, GREEN);
            }

            EndTextureMode();
            rlImGuiImageRenderTextureFit(&m_ViewportTexture, true);

            // Snapping Toolbar
            if (sceneState == SceneState::Edit)
            {
                ImGui::SetCursorPos(ImVec2(10, 50));
                ImGui::BeginChild("SnappingToolbar", ImVec2(0, 0),
                                  ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysAutoResize |
                                      ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY,
                                  ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse);

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

                ImGui::SameLine();
                ImGui::Checkbox("HUD", &m_ShowHUD);

                ImGui::SameLine();
                ImGui::Checkbox("Physics", &m_ShowPhysicsDebug);

                ImGui::SameLine();
                ImGui::Checkbox("FPS", &m_ShowFPS);

                ImGui::EndChild();
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
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
