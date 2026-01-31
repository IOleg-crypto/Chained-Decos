#include "viewport_panel.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/graphics/draw_command.h"
#include "engine/scene/scene.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "rlImGui.h"
#include "ui/editor_gui.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/core/input.h"
#include "engine/core/events.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/ui_math.h"


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
        m_ViewportTexture = { 0 }; // Initialize to empty
        
        if (IsWindowReady())
        {
            int w = GetScreenWidth();
            int h = GetScreenHeight();
            m_ViewportTexture = LoadRenderTexture(w > 0 ? w : 1280, h > 0 ? h : 720);
        }
    }

    ViewportPanel::~ViewportPanel()
    {
        if (IsWindowReady() && m_ViewportTexture.id > 0)
        {
            UnloadRenderTexture(m_ViewportTexture);
        }
    }

    void ViewportPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

    // Remove window padding to let the image fill the entire window area
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin(m_Name.c_str(), &m_IsOpen);
    ImGui::PushID(this);

    // Important: ImGuizmo needs to begin frame at the start of the window
    ImGuizmo::BeginFrame();

    // Get available content region dimensions (excluding window title/decorations)
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportScreenPos = ImGui::GetCursorScreenPos(); // Global top-left corner position

    // --- 0. FLOATING TOOLBAR ---
    // Floating style for cleaner viewport
    ImVec2 toolbarPos = { viewportScreenPos.x + 10.0f, viewportScreenPos.y + 10.0f };
    ImGui::SetNextWindowPos(toolbarPos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.12f, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    
    if (ImGui::BeginChild("##FloatingToolbar", ImVec2(320, 38), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        // Gizmo Buttons (No labels, icons only)
        auto& activeTool = m_CurrentTool;
        
        // Select Tool - capture state BEFORE button to keep Push/Pop balanced
        bool wasSelect = (activeTool == GizmoType::NONE);
        if (wasSelect) 
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.45f, 0.0f, 1.0f));
        }
        if (ImGui::Button(ICON_FA_ARROW_POINTER, ImVec2(28, 28))) 
        {
            activeTool = GizmoType::NONE;
        }
        if (wasSelect) 
        {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) 
        {
            ImGui::SetTooltip("Select (Q)");
        }
        ImGui::SameLine();

        // Translate Tool
        bool wasTranslate = (activeTool == GizmoType::TRANSLATE);
        if (wasTranslate) 
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.45f, 0.0f, 1.0f));
        }
        if (ImGui::Button(ICON_FA_UP_DOWN_LEFT_RIGHT, ImVec2(28, 28))) 
        {
            activeTool = GizmoType::TRANSLATE;
        }
        if (wasTranslate) 
        {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) 
        {
            ImGui::SetTooltip("Translate (W)");
        }
        ImGui::SameLine();

        // Rotate Tool
        bool wasRotate = (activeTool == GizmoType::ROTATE);
        if (wasRotate) 
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.45f, 0.0f, 1.0f));
        }
        if (ImGui::Button(ICON_FA_ARROWS_ROTATE, ImVec2(28, 28))) 
        {
            activeTool = GizmoType::ROTATE;
        }
        if (wasRotate) 
        {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) 
        {
            ImGui::SetTooltip("Rotate (E)");
        }
        ImGui::SameLine();

        // Scale Tool
        bool wasScale = (activeTool == GizmoType::SCALE);
        if (wasScale) 
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.45f, 0.0f, 1.0f));
        }
        if (ImGui::Button(ICON_FA_UP_RIGHT_FROM_SQUARE, ImVec2(28, 28))) 
        {
            activeTool = GizmoType::SCALE;
        }
        if (wasScale) 
        {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) 
        {
            ImGui::SetTooltip("Scale (R)");
        }
        ImGui::SameLine();

        ImGui::SameLine(0, 15);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 15);

        // Playback Tools
        SceneState sceneState = EditorLayer::Get().GetSceneState();
        bool isPlaying = (sceneState == SceneState::Play);

        if (isPlaying) 
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
        }
        if (ImGui::Button(ICON_FA_PLAY, ImVec2(28, 28))) 
        { 
            ScenePlayEvent e; 
            EditorLayer::Get().OnEvent(e); 
        }
        if (isPlaying) 
        {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();

        if (ImGui::Button(isPlaying ? ICON_FA_STOP : ICON_FA_PAUSE, ImVec2(28, 28))) 
        {
            if (isPlaying) 
            { 
                SceneStopEvent e; 
                EditorLayer::Get().OnEvent(e); 
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    // Rocket Launch button (Top Right)
    ImGui::SetCursorScreenPos({ viewportScreenPos.x + viewportSize.x - 100.0f, viewportScreenPos.y + 10.0f });
    if (ImGui::Button(ICON_FA_ROCKET " Launch", ImVec2(90, 28)))
    {
        AppLaunchRuntimeEvent e;
        Application::OnEvent(e);
    }

    // --- 1. PREPARE SCENE RENDER ---
    auto selectedEntity = EditorLayer::Get().GetSelectedEntity();
    bool isUISelected = selectedEntity && selectedEntity.HasComponent<ControlComponent>();
    
    // Disable grid when editing UI
    auto &debugFlags = EditorLayer::Get().GetDebugRenderFlags();
    bool oldGrid = debugFlags.DrawGrid;
    if (isUISelected)
    {
        debugFlags.DrawGrid = false;
    }

    // Gizmo rendering
    m_Gizmo.RenderAndHandle(!isUISelected ? m_CurrentTool : GizmoType::NONE);

    // Framebuffer management
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
    if (!activeScene || viewportSize.x <= 0 || viewportSize.y <= 0)
    {
        ImGui::PopID();
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    // Scene Rendering
    BeginTextureMode(m_ViewportTexture);
    ClearSceneBackground(activeScene.get(), {viewportSize.x, viewportSize.y});
    Camera3D camera = EditorUI::GUI::GetActiveCamera(EditorLayer::Get().GetSceneState());
    activeScene->OnRender(camera, &EditorLayer::Get().GetDebugRenderFlags());
    EndTextureMode();

    rlImGuiImageRenderTexture(&m_ViewportTexture);

    // --- 2. UI OVERLAY & SELECTION ---
    ImGui::SetCursorScreenPos(viewportScreenPos);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::BeginChild("##SceneUI", viewportSize, false, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs))
    {
        // Render Game UI Overlay
        activeScene->OnImGuiRender(viewportScreenPos, viewportSize, 0, EditorLayer::Get().GetSceneState() == SceneState::Edit);

        // Selection Highlight
        if (isUISelected)
        {
            auto &cc = selectedEntity.GetComponent<ControlComponent>();
            auto& canvas = activeScene->GetCanvasSettings();
            
            // Use UIMath helper for simple anchoring
            auto rect = UIMath::CalculateRect(cc.Transform, 
                {viewportSize.x, viewportSize.y}, 
                {viewportScreenPos.x, viewportScreenPos.y});
            
            ImVec2 p1 = {rect.Min.x, rect.Min.y};
            ImVec2 p2 = {rect.Max.x, rect.Max.y};

            // Selection Frame
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRect(p1, p2, IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);

            // Simple Drag support
            if (ImGui::IsMouseHoveringRect(p1, p2) && ImGui::IsMouseClicked(0)) 
            {
                m_DraggingUI = true;
            }
            
            if (m_DraggingUI)
            {
                if (ImGui::IsMouseDown(0)) 
                {
                    ImVec2 delta = ImGui::GetIO().MouseDelta;
                    // No scale used now, pure pixel movement
                    cc.Transform.OffsetMin.x += delta.x;
                    cc.Transform.OffsetMax.x += delta.x;
                    cc.Transform.OffsetMin.y += delta.y;
                    cc.Transform.OffsetMax.y += delta.y;
                }
                else 
                {
                    m_DraggingUI = false;
                }
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // --- 3. OBJECT PICKING ---
    if (EditorLayer::Get().GetSceneState() == SceneState::Edit && ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !m_DraggingUI && !ImGui::IsAnyItemActive())
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 localMouseImGui = {mousePos.x - viewportScreenPos.x, mousePos.y - viewportScreenPos.y};
        Vector2 localMouse = {localMouseImGui.x, localMouseImGui.y};

        Ray ray = GetMouseRay(localMouse, camera);
        Entity bestHit = {};
        float minDistance = FLT_MAX;

        // UI Picking
        auto uiView = activeScene->GetRegistry().view<ControlComponent>();
        for (auto entityID : uiView)
        {
            Entity entity{entityID, activeScene.get()};
            auto &cc = uiView.get<ControlComponent>(entityID);
            if (!cc.IsActive) 
            {
                continue;
            }

            auto rect = UIMath::CalculateRect(cc.Transform,
                {viewportSize.x, viewportSize.y},
                {viewportScreenPos.x, viewportScreenPos.y});

            glm::vec2 mouse = {mousePos.x, mousePos.y};
            if (rect.Contains(mouse))
            {
                bestHit = entity;
            }
        }

        // 3D Picking (if no UI hit)
        if (!bestHit)
        {
            auto view = activeScene->GetRegistry().view<TransformComponent, ModelComponent>();
            for (auto entityID : view)
            {
                Entity entity{entityID, activeScene.get()};
                auto &modelComp = view.get<ModelComponent>(entityID);
                if (modelComp.ModelPath.empty()) continue;

                auto modelAsset = AssetManager::Get<ModelAsset>(modelComp.ModelPath);
                if (!modelAsset || !modelAsset->IsReady()) continue;

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
        }

        if (bestHit) {
            EntitySelectedEvent e((entt::entity)bestHit, activeScene.get());
            EditorLayer::Get().OnEvent(e);
        }
        else {
            EntitySelectedEvent e(entt::null, activeScene.get());
            EditorLayer::Get().OnEvent(e);
        }
    }

    ImGui::PopID();
    ImGui::End();
    ImGui::PopStyleVar();
    debugFlags.DrawGrid = oldGrid;

    // Shortcuts
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
