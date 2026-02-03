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
#include "editor_gui.h"
#include "editor_layout.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/core/input.h"
#include "engine/core/events.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/prefab_serializer.h"
#include "engine/scene/components.h"
#include "engine/physics/physics.h"
#include "engine/physics/bvh/bvh.h"
#include "editor/actions/scene_actions.h"


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

    static const GizmoBtn s_GizmoBtns[] = {
        { GizmoType::NONE,      ICON_FA_ARROW_POINTER,         "Select (Q)",    KEY_Q },
        { GizmoType::TRANSLATE, ICON_FA_UP_DOWN_LEFT_RIGHT,    "Translate (W)", KEY_W },
        { GizmoType::ROTATE,    ICON_FA_ARROWS_ROTATE,         "Rotate (E)",    KEY_E },
        { GizmoType::SCALE,     ICON_FA_UP_RIGHT_FROM_SQUARE,  "Scale (R)",     KEY_R }
    };

    void ViewportPanel::DrawGizmoButtons()
    {
        ImGui::PushStyleColor(ImGuiCol_Button, {0.1f, 0.1f, 0.1f, 0.0f}); // Transparent buttons in toolbar
        
        for (const auto& btn : s_GizmoBtns)
        {
            bool selected = (m_CurrentTool == btn.type);
            if (selected) ImGui::PushStyleColor(ImGuiCol_Button, {0.9f, 0.45f, 0.0f, 1.0f});

            if (ImGui::Button(btn.icon, {28, 28})) m_CurrentTool = btn.type;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", btn.tooltip);
            
            if (selected) ImGui::PopStyleColor();
            ImGui::SameLine(0, 5);
        }

        ImGui::PopStyleColor();
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
    
    if (ImGui::BeginChild("##FloatingToolbar", ImVec2(280, 40), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::SetCursorPosY(6); // Center align vertically-ish
        ImGui::Indent(5);
        
        DrawGizmoButtons();
        
        ImGui::SameLine(0, 10);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 10);

        // Playback Tools
        SceneState sceneState = EditorLayer::Get().GetSceneState();
        bool isPlaying = (sceneState == SceneState::Play);

        if (isPlaying) 
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
        }
        if (ImGui::Button(isPlaying ? ICON_FA_STOP : ICON_FA_PLAY, ImVec2(28, 28))) 
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
        if (isPlaying) 
        {
            ImGui::PopStyleColor();
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

    // Framebuffer management
    if (viewportSize.x != m_ViewportTexture.texture.width || viewportSize.y != m_ViewportTexture.texture.height)
    {
        if (viewportSize.x > 0 && viewportSize.y > 0)
        {
            UnloadRenderTexture(m_ViewportTexture);
            m_ViewportTexture = LoadRenderTexture((int)viewportSize.x, (int)viewportSize.y);
            EditorLayer::Get().SetViewportSize(viewportSize);
        }
    }

    auto activeScene = EditorLayer::Get().GetActiveScene();
    if (!activeScene || viewportSize.x <= 0 || viewportSize.y <= 0)
    {
        ImGui::PopID();
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    BeginTextureMode(m_ViewportTexture);
    ClearSceneBackground(activeScene.get(), {viewportSize.x, viewportSize.y});
    Camera3D camera = EditorGUI::GetActiveCamera(EditorLayer::Get().GetSceneState());
    activeScene->OnRender(camera, &EditorLayer::Get().GetDebugRenderFlags());
    EndTextureMode();

    rlImGuiImageRenderTexture(&m_ViewportTexture);

    // Drag & Drop Target
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char* path = (const char*)payload->Data;
            std::filesystem::path filepath = std::filesystem::path(path); // Ensure cross-platform path handling
            std::string ext = filepath.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".chscene")
            {
                SceneActions::Open(filepath);
            }
            else if (ext == ".chprefab")
            {
                 PrefabSerializer::Deserialize(activeScene.get(), filepath.string());
            }
            else if (ext == ".gltf" || ext == ".glb" || ext == ".obj")
            {
                 std::string filename = filepath.stem().string();
                 Entity entity = activeScene->CreateEntity(filename);
                 auto& mc = entity.AddComponent<ModelComponent>();
                 // Use generic string for consistent paths
                 mc.ModelPath = filepath.generic_string();
                 
                 // Select the new entity
                 EntitySelectedEvent e((entt::entity)entity, activeScene.get());
                 EditorLayer::Get().OnEvent(e);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Gizmo rendering (after scene image so it overlays properly)
    m_Gizmo.RenderAndHandle(!isUISelected ? m_CurrentTool : GizmoType::NONE, viewportScreenPos, viewportSize);

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
            
            // Use member function for simple anchoring
            auto rect = cc.Transform.CalculateRect( 
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

        Ray ray = EditorGUI::GetMouseRay(camera, {viewportScreenPos.x, viewportScreenPos.y}, {viewportSize.x, viewportSize.y});
        Entity bestHit = {};
        float minDistance = FLT_MAX;

        // UI Picking
        auto uiView = activeScene->GetRegistry().view<ControlComponent>();
        for (auto entityID : uiView)
        {
            Entity entity(entityID, activeScene.get());
            auto &cc = uiView.get<ControlComponent>(entityID);
            if (!cc.IsActive) 
            {
                continue;
            }

            auto rect = cc.Transform.CalculateRect(
                {viewportSize.x, viewportSize.y},
                {viewportScreenPos.x, viewportScreenPos.y});

            glm::vec2 mouse = {mousePos.x, mousePos.y};
            if (rect.Contains(mouse))
            {
                bestHit = entity;
            }
        }

        // 3D Picking
        if (!bestHit)
        {
            // 1. Physics Picking (Colliders: Box, Mesh BVH)
            RaycastResult result = Physics::Raycast(activeScene.get(), ray);
            if (result.Hit)
            {
                bestHit = Entity(result.Entity, activeScene.get());
                minDistance = result.Distance;
            }

            // 2. Visual Picking Fallback (Meshes without colliders)
            // Only run if we don't have a very close physics hit, or to find things without colliders
            auto modelView = activeScene->GetRegistry().view<TransformComponent, ModelComponent>();
            for (auto entityID : modelView)
            {
                // Skip if already hit by physics (unless we want to prioritize visuals?)
                // Usually physics is the intended interaction volume.
                if (bestHit && (entt::entity)bestHit == entityID) continue;

                Entity entity(entityID, activeScene.get());
                auto &modelComp = modelView.get<ModelComponent>(entityID);
                if (modelComp.ModelPath.empty()) continue;

                auto modelAsset = AssetManager::Get<ModelAsset>(modelComp.ModelPath);
                if (!modelAsset || !modelAsset->IsReady()) continue;

                auto &tc = modelView.get<TransformComponent>(entityID);
                Matrix modelTransform = tc.GetTransform();
                Matrix invTransform = MatrixInvert(modelTransform);

                // Transform ray to local space for BVH/Mesh check
                Ray localRay;
                localRay.position = Vector3Transform(ray.position, invTransform);
                Vector3 localTarget = Vector3Transform(Vector3Add(ray.position, ray.direction), invTransform);
                localRay.direction = Vector3Normalize(Vector3Subtract(localTarget, localRay.position));

                float t_local = FLT_MAX;
                Vector3 localNormal = {0, 0, 0};
                int localMeshIndex = -1;

                bool hit = false;
                auto bvh = Physics::GetBVH(modelAsset.get());
                if (bvh)
                {
                    hit = bvh->Raycast(localRay, t_local, localNormal, localMeshIndex);
                }
                else
                {
                    // Fallback to per-mesh ray testing if no BVH (slow but accurate)
                    Model &model = modelAsset->GetModel();
                    for (int m = 0; m < model.meshCount; m++)
                    {
                        RayCollision collision = GetRayCollisionMesh(localRay, model.meshes[m], MatrixIdentity());
                        if (collision.hit && collision.distance < t_local)
                        {
                            t_local = collision.distance;
                            hit = true;
                        }
                    }
                }

                if (hit)
                {
                    // Convert hit distance back to world space
                    Vector3 hitPosLocal = Vector3Add(localRay.position, Vector3Scale(localRay.direction, t_local));
                    Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelTransform);
                    float distWorld = Vector3Distance(ray.position, hitPosWorld);

                    if (distWorld < minDistance)
                    {
                        minDistance = distWorld;
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
        for (const auto& btn : s_GizmoBtns)
        {
            if (CHEngine::Input::IsKeyPressed(btn.key)) 
                m_CurrentTool = btn.type;
        }
    }
}

    void ViewportPanel::OnUpdate(float deltaTime)
    {
        m_EditorCamera.OnUpdate(deltaTime);
    }

} // namespace CHEngine
