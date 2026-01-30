#include "viewport_panel.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/graphics/draw_command.h"
#include "engine/scene/scene.h"
#include "extras/iconsfontawesome6.h"
#include "imgui.h"
#include "raylib.h"
#include "rlimgui.h"
#include "ui/editor_gui.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/core/input.h"
#include "engine/core/events.h"


namespace CHEngine
{
    static void ClearSceneBackground(Scene *scene, Vector2 size)
    {
        auto mode = scene->GetBackgroundMode();
        if (mode == BackgroundMode::Color)
        {
            ClearBackground(scene->GetBackgroundColor());
        }
        else if (mode == BackgroundMode::Texture)
        {
            auto &path = scene->GetBackgroundTexturePath();
            if (!path.empty())
            {
                // Fallback for now
                ClearBackground(scene->GetBackgroundColor());
            }
        }
        else if (mode == BackgroundMode::Environment3D)
        {
            ClearBackground(BLACK);
        }
    }

    ViewportPanel::ViewportPanel()
    {
        m_Name = "Viewport";
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        m_ViewportTexture = LoadRenderTexture(w > 0 ? w : 1280, h > 0 ? h : 720);
    }

    ViewportPanel::~ViewportPanel()
    {
        UnloadRenderTexture(m_ViewportTexture);
    }

    void ViewportPanel::OnImGuiRender(bool readOnly)
    {
        if (!m_IsOpen)
            return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin(m_Name.c_str(), &m_IsOpen);
        ImGui::PushID(this);

        // Declarative visibility: Hide 3D tools when UI is selected
        auto selectedEntity = EditorLayer::Get().GetSelectedEntity();
        bool isUISelected = selectedEntity && selectedEntity.HasComponent<ControlComponent>();
        auto &debugFlags = EditorLayer::Get().GetDebugRenderFlags();
        
        bool oldGrid = debugFlags.DrawGrid;
        bool oldAxes = debugFlags.DrawAxes;
        
        if (isUISelected)
        {
            debugFlags.DrawGrid = false;
            debugFlags.DrawAxes = false;
        }

        m_Gizmo.RenderAndHandle(!isUISelected ? m_CurrentTool : GizmoType::NONE);

        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        if (viewportSize.x != m_ViewportTexture.texture.width || viewportSize.y != m_ViewportTexture.texture.height)
        {
            if (viewportSize.x > 0 && viewportSize.y > 0)
            {
                UnloadRenderTexture(m_ViewportTexture);
                m_ViewportTexture = LoadRenderTexture((int)viewportSize.x, (int)viewportSize.y);
                EditorLayer::Get().s_ViewportSize = viewportSize;
            }
        }

        auto activeScene = Application::Get().GetActiveScene();
        if (!activeScene)
        {
            ImGui::PopID();
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        // Render to texture
        BeginTextureMode(m_ViewportTexture);
        ClearSceneBackground(activeScene.get(), {viewportSize.x, viewportSize.y});

        Camera3D camera = EditorUI::GUI::GetActiveCamera(EditorLayer::Get().GetSceneState());
        activeScene->OnRender(camera, &EditorLayer::Get().GetDebugRenderFlags());

        EndTextureMode();

        // Draw the viewport texture
        rlImGuiImageRenderTextureFit(&m_ViewportTexture, true);

        // --- Render Scene UI (ImGui Overlay) ---
        ImGui::SetCursorPos(ImVec2(0, 0));
        bool sceneUIVisible = ImGui::BeginChild("##SceneUI", viewportSize, false,
                              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                                  ImGuiWindowFlags_NoInputs);
        if (sceneUIVisible)
        {
            activeScene->OnImGuiRender(ImGui::GetCursorScreenPos(), viewportSize, 0,
                                       EditorLayer::Get().GetSceneState() == SceneState::Edit);
        }
        ImGui::EndChild();

        // --- Entity Picking (Mouse Interaction) ---
        if (EditorLayer::Get().GetSceneState() == SceneState::Edit && ImGui::IsWindowHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGui::IsAnyItemActive())
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 viewportPos = ImGui::GetWindowPos();
            Vector2 localMouse = {mousePos.x - viewportPos.x, mousePos.y - viewportPos.y};

            Ray ray = GetMouseRay(localMouse, camera);

            Entity bestHit = {};
            float minDistance = FLT_MAX;

            auto view = activeScene->GetRegistry().view<TransformComponent, ModelComponent>();
            for (auto entityID : view)
            {
                Entity entity{entityID, activeScene.get()};
                auto &modelComp = view.get<ModelComponent>(entityID);

                if (modelComp.ModelPath.empty())
                    continue;

                auto modelAsset = AssetManager::Get<ModelAsset>(modelComp.ModelPath);
                if (!modelAsset || !modelAsset->IsReady())
                    continue;

                // Mesh-based picking (more robust if GetRayCollisionModel is missing or weird)
                Model &model = modelAsset->GetModel();
                for (int m = 0; m < model.meshCount; m++)
                {
                    RayCollision collision = GetRayCollisionMesh(ray, model.meshes[m], model.transform);
                    if (collision.hit && collision.distance < minDistance)
                    {
                        minDistance = collision.distance;
                        bestHit = entity;
                    }
                }
            }

            if (bestHit)
            {
                EntitySelectedEvent e((entt::entity)bestHit, activeScene.get());
                EditorLayer::Get().OnEvent(e);
            }
            else
            {
                // Deselect if clicked in air
                EntitySelectedEvent e(entt::null, activeScene.get());
                EditorLayer::Get().OnEvent(e);
            }
        }

        // --- Toolbar Overlay (Floating) ---
        ImGui::SetCursorPos(ImVec2(10, 10)); // Fixed offset from top-left of viewport content

        float toolbarWidth = 40.0f;
        float toolbarHeight = 40.0f;

        // Center horizontally
        ImGui::SetCursorPos(ImVec2((viewportSize.x - toolbarWidth) * 0.5f, 10.0f));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.11f, 0.8f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::BeginChild("##ViewportToolbar", ImVec2(toolbarWidth, toolbarHeight), false,
                              ImGuiWindowFlags_NoDecoration))
        {
            SceneState sceneState = EditorLayer::Get().GetSceneState();
            bool isPlaying = sceneState == SceneState::Play;
            const char *icon = isPlaying ? ICON_FA_CIRCLE_STOP : ICON_FA_PLAY;
            ImVec4 iconColor = isPlaying ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);

            // Center button in child
            ImGui::SetCursorPos(ImVec2((toolbarWidth - 28.0f) * 0.5f, (toolbarHeight - 28.0f) * 0.5f));

            ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
            if (ImGui::Button(icon, ImVec2(28, 28)))
            {
                if (isPlaying)
                {
                    SceneStopEvent e;
                    EditorLayer::Get().OnEvent(e);
                }
                else
                {
                    ScenePlayEvent e;
                    EditorLayer::Get().OnEvent(e);
                }
            }
            ImGui::PopStyleColor(3);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(isPlaying ? "Stop (ESC)" : "Play");
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::PopID();
        ImGui::End();
        ImGui::PopStyleVar();

        // Restore state
        debugFlags.DrawGrid = oldGrid;
        debugFlags.DrawAxes = oldAxes;

        // Tool Switching (Declarative shortcuts) - Moved from OnUpdate to avoid cur_window assertion
        if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered())
        {
            if (CHEngine::Input::IsKeyPressed(KEY_W)) m_CurrentTool = GizmoType::TRANSLATE;
            if (CHEngine::Input::IsKeyPressed(KEY_E)) m_CurrentTool = GizmoType::ROTATE;
            if (CHEngine::Input::IsKeyPressed(KEY_R)) m_CurrentTool = GizmoType::SCALE;
            if (CHEngine::Input::IsKeyPressed(KEY_Q)) m_CurrentTool = GizmoType::NONE;
        }
    }

    void ViewportPanel::OnUpdate(float deltaTime)
    {
        m_EditorCamera.OnUpdate(deltaTime);
    }

    void ViewportPanel::OnEvent(Event &e)
    {
        // m_EditorCamera doesn't have OnEvent yet, so we just let it be for now
    }

} // namespace CHEngine
