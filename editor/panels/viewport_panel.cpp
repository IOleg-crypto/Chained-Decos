#include "viewport_panel.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/scene/scene.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "raylib.h"
#include "rlImGui.h"
#include "engine/graphics/scene_renderer.h"
#include "engine/graphics/ui_renderer.h"
#include "editor_gui.h"
#include "editor_layout.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "engine/scene/project.h"
#include "engine/core/input.h"
#include "engine/core/events.h"
#include "engine/scene/scene_events.h"
#include "editor_events.h"
#include "engine/scene/prefab_serializer.h"
#include "engine/scene/components.h"
#include "engine/physics/physics.h"
#include "engine/physics/bvh/bvh.h"
#include "editor/actions/scene_actions.h"


namespace CHEngine
{
    static void ClearSceneBackground(Scene *scene, Vector2 size)
    {
        auto mode = scene->GetSettings().Mode;
        if (mode == BackgroundMode::Color)
        {
            ClearBackground(scene->GetSettings().BackgroundColor);
        }
        else if (mode == BackgroundMode::Texture)
        {
            auto &path = scene->GetSettings().BackgroundTexturePath;
            if (!path.empty())
            {
                // Fallback for now
                ClearBackground(scene->GetSettings().BackgroundColor);
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

    // --- 1. PREPARE SCENE RENDER ---
    // Get available content region dimensions (excluding window title/decorations)
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportScreenPos = ImGui::GetCursorScreenPos(); // Global top-left corner position

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

    SceneRenderer::RenderScene(activeScene.get(), camera, GetFrameTime(), &EditorLayer::Get().GetDebugRenderFlags());
    EndTextureMode();

    rlImGuiImageRenderTexture(&m_ViewportTexture);
    bool isViewportHovered = ImGui::IsItemHovered();

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
                 auto& modelcomp = entity.AddComponent<ModelComponent>();
                 // Use relative path if possible to satisfy portability
                 modelcomp.ModelPath = Project::GetRelativePath(filepath);
                 
                 // Select the new entity
                 EntitySelectedEvent e((entt::entity)entity, activeScene.get());
                 EditorLayer::Get().OnEvent(e);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // --- 2. UI OVERLAY & SELECTION ---
    ImGui::SetCursorScreenPos(viewportScreenPos);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    bool isUIChildHovered = false;
    bool isGizmoActive = false;
    bool isGizmoHovered = false;

    if (ImGui::BeginChild("##SceneUI", viewportSize, false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        // 1. Gizmo handling (inside child window for input priority)
        isGizmoActive = m_Gizmo.RenderAndHandle(!isUISelected ? m_CurrentTool : GizmoType::NONE, viewportScreenPos, viewportSize);
        isGizmoHovered = m_Gizmo.IsHovered();

        // 2. Game UI Overlay
        UIRenderer::DrawCanvas(activeScene.get(), {0, 0}, viewportSize, EditorLayer::Get().GetSceneState() == SceneState::Edit);
        isUIChildHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);

        // 3. Selection Highlight
        if (isUISelected && selectedEntity)
        {
            auto &cc = selectedEntity.GetComponent<ControlComponent>();
            auto rect = cc.Transform.CalculateRect({viewportSize.x, viewportSize.y}, {0, 0});
            
            ImVec2 p1 = {viewportScreenPos.x + rect.x, viewportScreenPos.y + rect.y};
            ImVec2 p2 = {p1.x + rect.width, p1.y + rect.height};

            ImGui::GetWindowDrawList()->AddRect(p1, p2, IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);

            // Simple Drag support
            if (ImGui::IsMouseHoveringRect(p1, p2) && ImGui::IsMouseClicked(0)) 
            {
                m_DraggingUI = true;
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // --- 3. OBJECT PICKING ---
    bool isHovered = isUIChildHovered; 
    bool isClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool isDragging = m_DraggingUI;
    SceneState sceneState = EditorLayer::Get().GetSceneState();

    if (isClicked) {
        CH_CORE_WARN("[Viewport] Click: Hovered={}, GizmoActive={}, GizmoHovered={}, Dragging={}, SceneState={}", 
            isHovered, isGizmoActive, isGizmoHovered, isDragging, (int)sceneState);
    }

    if (sceneState == SceneState::Edit && isHovered && isClicked && !isGizmoActive && !isGizmoHovered && !isDragging)
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 localMouseImGui = {mousePos.x - viewportScreenPos.x, mousePos.y - viewportScreenPos.y};
        Vector2 localMouse = {localMouseImGui.x, localMouseImGui.y};

        Ray ray = EditorGUI::GetMouseRay(camera, {localMouse.x, localMouse.y}, {viewportSize.x, viewportSize.y});
        
        bool isClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        if (isClicked)
        {
             static int clickCount = 0;
             CH_CORE_WARN("PICKING DEBUG #{}", ++clickCount);
             CH_CORE_INFO("Viewport: Pos({},{}), Size({},{})", viewportScreenPos.x, viewportScreenPos.y, viewportSize.x, viewportSize.y);
             CH_CORE_INFO("Mouse: ImGui({},{}), Local({},{})", mousePos.x, mousePos.y, localMouseImGui.x, localMouseImGui.y);
             CH_CORE_INFO("Ray: Origin({:.2f}, {:.2f}, {:.2f}), Dir({:.2f}, {:.2f}, {:.2f})", 
                 ray.position.x, ray.position.y, ray.position.z,
                 ray.direction.x, ray.direction.y, ray.direction.z);
        }
        
        Entity bestHit = {};
        float minDistance = FLT_MAX;

        // UI Picking
        auto uiView = activeScene->GetRegistry().view<ControlComponent>();
        for (auto entityID : uiView)
        {
            Entity entity(entityID, activeScene.get());
            auto &cc = uiView.get<ControlComponent>(entityID);
            if (!cc.IsActive) continue;

            auto rect = cc.Transform.CalculateRect(
                {viewportSize.x, viewportSize.y},
                {viewportScreenPos.x, viewportScreenPos.y});

            glm::vec2 mouse = {mousePos.x, mousePos.y};
            Vector2 mouseRaylib = {mouse.x, mouse.y};
            if (CheckCollisionPointRec(mouseRaylib, rect))
            {
                bestHit = entity;
                CH_CORE_INFO("HIT UI: {}", entity.GetComponent<TagComponent>().Tag);
            }
        }

        // 3D Picking
        if (!bestHit)
        {
            // 1. Physics Picking
            RaycastResult result = activeScene->GetPhysics().Raycast(ray);
            if (result.Hit)
            {
                bestHit = Entity(result.Entity, activeScene.get());
                minDistance = result.Distance;
                CH_CORE_INFO("HIT PHYSICS: {} at Dist {}", bestHit.GetComponent<TagComponent>().Tag, minDistance);
            }
            else
            {
                CH_CORE_TRACE("Physics Raycast missed.");
            }

            // 2. Visual Picking Fallback
            auto modelView = activeScene->GetRegistry().view<TransformComponent, ModelComponent>();
            int visualCandidates = 0;
            
            for (auto entityID : modelView)
            {
                visualCandidates++;
                if (bestHit && (entt::entity)bestHit == entityID) continue;

                Entity entity(entityID, activeScene.get());
                auto &modelComp = modelView.get<ModelComponent>(entityID);
                auto &tag = entity.GetComponent<TagComponent>().Tag;

                if (modelComp.ModelPath.empty()) {
                    CH_CORE_TRACE("Skip {}: No model path", tag);
                    continue;
                }

                auto modelAsset = Project::GetActive()->GetAssetManager()->Get<ModelAsset>(modelComp.ModelPath);
                if (!modelAsset || !modelAsset->IsReady()) {
                    CH_CORE_TRACE("Skip {}: Asset not ready", tag);
                    continue;
                }

                auto &tc = modelView.get<TransformComponent>(entityID);
                Matrix modelTransform = tc.GetTransform();
                Matrix invTransform = MatrixInvert(modelTransform);

                // Transform ray to local space
                Ray localRay;
                localRay.position = Vector3Transform(ray.position, invTransform);
                Vector3 localTarget = Vector3Transform(Vector3Add(ray.position, ray.direction), invTransform);
                localRay.direction = Vector3Normalize(Vector3Subtract(localTarget, localRay.position));

                float t_local = FLT_MAX;
                Vector3 localNormal = {0, 0, 0};
                int localMeshIndex = -1;

                bool hit = false;
                auto bvh = activeScene->GetPhysics().GetBVH(modelAsset.get());
                
                if (bvh)
                {
                    hit = bvh->Raycast(localRay, t_local, localNormal, localMeshIndex);
                }
                else
                {
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
                    Vector3 hitPosLocal = Vector3Add(localRay.position, Vector3Scale(localRay.direction, t_local));
                    Vector3 hitPosWorld = Vector3Transform(hitPosLocal, modelTransform);
                    float distWorld = Vector3Distance(ray.position, hitPosWorld);
                    
                    CH_CORE_INFO("VISUAL HIT Candidate: {} (Dist: {:.2f})", tag, distWorld);

                    if (distWorld < minDistance)
                    {
                        minDistance = distWorld;
                        bestHit = entity;
                    }
                }
            }
            CH_CORE_TRACE("Checked {} visual candidates", visualCandidates);
        }

        if (bestHit) {
            CH_CORE_WARN("FINAL SELECTION: {}", bestHit.GetComponent<TagComponent>().Tag);
            EntitySelectedEvent e((entt::entity)bestHit, activeScene.get());
            EditorLayer::Get().OnEvent(e);
        }
        else {
            CH_CORE_WARN("FINAL SELECTION: NONE");
            // Only deselect if we actually clicked and missed everything
            EntitySelectedEvent e(entt::null, activeScene.get());
            EditorLayer::Get().OnEvent(e);
        }
    }

    // --- 4. FLOATING HUD (Drawn last to be on top of SceneUI) ---
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
                CH_CORE_INFO("ViewportPanel: Stop Button Clicked");
                SceneStopEvent e; 
                EditorLayer::Get().OnEvent(e); 
            }
            else 
            { 
                CH_CORE_INFO("ViewportPanel: Play Button Clicked");
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

    // --- 5. ROCKET LAUNCH BUTTON (Top Right Overlay) ---
    // Wrapped in a child window to ensure input priority over full-screen overlays (like ##SceneUI)
    ImGui::SetCursorScreenPos({ viewportScreenPos.x + viewportSize.x - 110.0f, viewportScreenPos.y + 10.0f });
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.12f, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    
    if (ImGui::BeginChild("##LaunchHUD", ImVec2(100, 40), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::SetCursorPosY(6);
        ImGui::Indent(5);
        if (ImGui::Button(ICON_FA_ROCKET " Launch", ImVec2(90, 28)))
        {
            CH_CORE_INFO("Viewport: Launch button clicked");
            AppLaunchRuntimeEvent e;
            Application::Get().OnEvent(e);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Build & Run Standalone project (F5)");
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

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

    void ViewportPanel::OnUpdate(Timestep ts)
    {
        // Only update editor camera in Edit mode
        // In Play mode, the runtime scene's camera (Player/CameraController) should be used
        if (EditorLayer::Get().GetSceneState() == SceneState::Edit)
        {
            float deltaTime = ts;
            m_EditorCamera.OnUpdate(deltaTime);
        }
    }

} // namespace CHEngine
