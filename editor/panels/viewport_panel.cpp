#include "viewport_panel.h"
#include "editor/actions/scene_actions.h"
#include "editor/viewport/ui_manipulator.h"
#include "editor_events.h"
#include "editor_gui.h"
#include "editor_layer.h"
#include "editor_layout.h"
#include "engine/core/application.h"
#include "engine/core/events.h"
#include "engine/core/input.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/scene/components.h"
#include "engine/scene/prefab_serializer.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scene_picking.h"
#include "scripting/scriptengine.h"
#include "undo/entity_commands.h"
#include "imgui/IconsFontAwesome6.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "engine/graphics/pipeline/render_command.h"


namespace CHEngine
{
static void ClearSceneBackground(Scene* scene, Vector2 size)
{
    auto mode = scene->GetSettings().Mode;
    if (mode == BackgroundMode::Color)
        RenderCommand::Clear(scene->GetSettings().BackgroundColor);
    else if (mode == BackgroundMode::Texture)
    {
        auto& path = scene->GetSettings().BackgroundTexturePath;
        if (!path.empty())
        {
            // Fallback for now
            RenderCommand::Clear(scene->GetSettings().BackgroundColor);
        }
    }
    else if (mode == BackgroundMode::Environment3D)
    {
        RenderCommand::Clear({ 0, 0, 0, 255 });
    }
}


static const GizmoBtn s_GizmoBtns[] = {{GizmoType::NONE, ICON_FA_ARROW_POINTER "##Select", "Select (Q)", Key::Q},
                                       {GizmoType::TRANSLATE, ICON_FA_UP_DOWN_LEFT_RIGHT "##Translate", "Translate (W)", Key::W},
                                       {GizmoType::ROTATE, ICON_FA_ARROWS_ROTATE "##Rotate", "Rotate (E)", Key::E},
                                       {GizmoType::SCALE, ICON_FA_UP_RIGHT_FROM_SQUARE "##Scale", "Scale (R)", Key::R}};


void ViewportPanel::DrawCameraSelector(Scene* scene)
{
    if (!scene)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, {0.1f, 0.1f, 0.12f, 0.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    auto view = scene->GetRegistry().view<CameraComponent>();
    Entity primaryCam = scene->GetPrimaryCameraEntity();
    std::string currentLabel = primaryCam ? primaryCam.GetComponent<TagComponent>().Tag : "No Camera";

    ImGui::SetNextItemWidth(150);
    if (ImGui::BeginCombo("##CameraSelector", (ICON_FA_VIDEO "  " + currentLabel).c_str(), ImGuiComboFlags_None))
    {
        for (auto entityHandle : view)
        {
            Entity entity(entityHandle, &scene->GetRegistry());
            bool isSelected = (entity == primaryCam);
            std::string tag = entity.GetComponent<TagComponent>().Tag;

            if (ImGui::Selectable(tag.c_str(), isSelected))
            {
                // Unset all and set this one as primary
                for (auto otherHandle : view)
                {
                    scene->GetRegistry().get<CameraComponent>(otherHandle).Primary = false;
                }
                entity.GetComponent<CameraComponent>().Primary = true;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void ViewportPanel::DrawGizmoButtons()
{
    ImGui::PushStyleColor(ImGuiCol_Button, {0.1f, 0.1f, 0.1f, 0.0f}); // Transparent buttons in toolbar

    for (const auto& btn : s_GizmoBtns)
    {
        bool selected = (m_CurrentTool == btn.type);
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, {0.9f, 0.45f, 0.0f, 1.0f});
        }

        if (ImGui::Button(btn.icon, {28, 28}))
        {
            m_CurrentTool = btn.type;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", btn.tooltip);
        }

        if (selected)
        {
            ImGui::PopStyleColor();
        }
        ImGui::SameLine(0, 5);
    }

    ImGui::PopStyleColor();
}

ViewportPanel::ViewportPanel()
{
    m_Name = "Viewport";

    FramebufferSpecification spec;
    spec.Width = 1280;
    spec.Height = 720;

    if (Application::Get().GetWindow().GetNativeWindow())
    {
        spec.Width = Application::Get().GetWindow().GetWidth() > 0 ? Application::Get().GetWindow().GetWidth() : 1280;
        spec.Height = Application::Get().GetWindow().GetHeight() > 0 ? Application::Get().GetWindow().GetHeight() : 720;
    }

    m_ViewportFramebuffer = Framebuffer::Create(spec);
    m_HDRFramebuffer = Framebuffer::Create(spec);

    m_SceneRenderer = std::make_unique<SceneRenderer>();
    m_CameraController = std::make_unique<EditorCameraController>();
}

ViewportPanel::~ViewportPanel()
{
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

    // --- 1. PREPARE SCENE RENDER ---
    // Get available content region dimensions (excluding window title/decorations)
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportScreenPos = ImGui::GetCursorScreenPos(); // Global top-left corner position

    // --- 1. PREPARE SCENE RENDER ---
    auto selectedEntity = EditorLayer::Get().GetSelectedEntity();
    bool isUISelected = selectedEntity && selectedEntity.HasComponent<ControlComponent>();

    // Disable grid when editing UI
    auto& debugFlags = EditorLayer::Get().GetDebugRenderFlags();
    bool oldGrid = debugFlags.DrawGrid;
    if (isUISelected)
    {
        debugFlags.DrawGrid = false;
    }

    // Framebuffer management
    auto activeScene = EditorLayer::Get().GetActiveScene();
    if (viewportSize.x != m_ViewportFramebuffer->GetSpecification().Width ||
        viewportSize.y != m_ViewportFramebuffer->GetSpecification().Height)
    {
        if (viewportSize.x > 0 && viewportSize.y > 0)
        {
            m_ViewportFramebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            m_HDRFramebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

            EditorLayer::Get().SetViewportSize(viewportSize);
            if (activeScene)
            {
                activeScene->OnViewportResize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            }
        }
    }

    if (!activeScene || viewportSize.x <= 0 || viewportSize.y <= 0)
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    m_HDRFramebuffer->Bind();
    auto activeScene_raw = activeScene.get();
    ClearSceneBackground(activeScene_raw, {viewportSize.x, viewportSize.y});

    auto activeCameraOpt = activeScene_raw->GetActiveCamera();
    bool cameraFound = activeCameraOpt.has_value();
    CHEngine::Camera3D camera;
    float nearClip = 0.01f;
    float farClip = 10000.0f;

    // Default to Editor Camera
    auto& edCam = m_CameraController->GetCamera();
    Vector3 pos = edCam.CalculatePosition();
    camera.Position = {pos.x, pos.y, pos.z};
    
    Vector3 fp = edCam.GetFocalPoint();
    camera.Target = {fp.x, fp.y, fp.z};
    
    Vector3 up = edCam.GetUpDirection();
    camera.Up = {up.x, up.y, up.z};
    
    camera.Fovy = glm::degrees(edCam.GetPerspectiveVerticalFOV()); // Fovy in degrees
    camera.Projection = 0; // Perspective
    
    nearClip = edCam.GetPerspectiveNearClip();
    farClip = edCam.GetPerspectiveFarClip();

    // If an entity camera is active (usually during Play mode), override it
    if (cameraFound)
    {
        camera = activeCameraOpt.value();
        Entity primaryCam = activeScene_raw->GetPrimaryCameraEntity();
        if (primaryCam && primaryCam.HasComponent<CameraComponent>())
        {
            auto& cameraComp = primaryCam.GetComponent<CameraComponent>().Camera;
            nearClip = cameraComp.GetPerspectiveNearClip();
            farClip = cameraComp.GetPerspectiveFarClip();
        }
    }

    SceneRenderOptions options;
    options.DrawGrid = EditorLayer::Get().GetDebugRenderFlags().DrawGrid;
    options.ShowDebugColliders = EditorLayer::Get().GetDebugRenderFlags().DrawColliders;
    options.ShowDebugCollisionModelBox = EditorLayer::Get().GetDebugRenderFlags().DrawCollisionModelBox;
    options.ShowDebugSpawnZones = EditorLayer::Get().GetDebugRenderFlags().DrawSpawnZones;
    options.ShowEditorIcons = true;

    if (auto project = Project::GetActive())
    {
        options.TargetFPS = project->GetConfig().Animation.TargetFPS;
    }

    m_SceneRenderer->RenderScene(activeScene.get(), camera, nearClip, farClip, Application::Get().GetFrameTime(),
                                    options);
    m_HDRFramebuffer->Unbind();

    // --- 2. APPLY POST-PROCESSING ---
    m_ViewportFramebuffer->Bind();
    RenderCommand::Clear({ 0, 0, 0, 255 }); // Clear viewport buffer
    Renderer::Get().ApplyPostProcessing(
        m_HDRFramebuffer->GetColorAttachmentRendererID(),
        m_HDRFramebuffer->GetDepthAttachmentRendererID(),
        camera
    );
    m_ViewportFramebuffer->Unbind();

    uint32_t finalTextureID = m_ViewportFramebuffer->GetColorAttachmentRendererID();
    ImGui::Image((ImTextureID)(uintptr_t)finalTextureID, viewportSize, { 0, 1 }, { 1, 0 });

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

    if (ImGui::BeginChild("##SceneUI", viewportSize, false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse))
    {
        // 1. Gizmo handling (inside child window for input priority)
        isGizmoActive = m_Gizmo.RenderAndHandle(!isUISelected ? m_CurrentTool : GizmoType::NONE, viewportScreenPos,
                                                viewportSize, camera);
        isGizmoHovered = m_Gizmo.IsHovered();

        // 2. Game UI Overlay
        // Use child window's actual cursor position rather than the external viewportScreenPos
        // to ensure UI elements are positioned relative to this child window's origin,
        // matching the runtime behavior.
        ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
        UIRenderer::Get().DrawCanvas(activeScene.get(), canvasOrigin, viewportSize,
                                     EditorLayer::Get().GetSceneState() == SceneState::Edit);
        isUIChildHovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);

        // 3. Selection Highlight (FIX for play mode)
        if (isUISelected && selectedEntity && EditorLayer::Get().GetSceneState() == SceneState::Edit)
        {
            auto rect = UIRenderer::Get().GetEntityRect(selectedEntity, viewportSize, viewportScreenPos);

            ImVec2 p1 = ImVec2(rect.x, rect.y);
            ImVec2 p2 = ImVec2(p1.x + rect.width, p1.y + rect.height);

            ImGui::GetWindowDrawList()->AddRect(p1, p2, IM_COL32(255, 255, 0, 255), 0, 0, 2.0f);

            // Use the new UI Manipulator
            m_UIManipulator.OnImGuiRender(selectedEntity, viewportScreenPos, viewportSize);

            // Debug info
            if (ImGui::IsMouseHoveringRect(p1, p2))
            {
                ImGui::GetWindowDrawList()->AddRect(p1, p2, IM_COL32(0, 255, 0, 255), 0, 0, 1.0f);
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // --- 3. OBJECT PICKING ---
    bool isHovered = isUIChildHovered;
    bool isClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool isDragging = m_UIManipulator.IsActive();
    SceneState sceneState = EditorLayer::Get().GetSceneState();

    if (isClicked)
    {
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
            CH_CORE_INFO("Viewport: Pos({},{}), Size({},{})", viewportScreenPos.x, viewportScreenPos.y, viewportSize.x,
                         viewportSize.y);
            CH_CORE_INFO("Mouse: ImGui({},{}), Local({},{})", mousePos.x, mousePos.y, localMouseImGui.x,
                         localMouseImGui.y);
            CH_CORE_INFO("Ray: Origin({:.2f}, {:.2f}, {:.2f}), Dir({:.2f}, {:.2f}, {:.2f})", ray.position.x,
                         ray.position.y, ray.position.z, ray.direction.x, ray.direction.y, ray.direction.z);
        }

        Entity bestHit = {};
        float minDistance = FLT_MAX;

        // UI Picking
        auto uiView = activeScene->GetRegistry().view<ControlComponent>();
        for (auto entityID : uiView)
        {
            Entity entity(entityID, &activeScene->GetRegistry());
            auto& cc = uiView.get<ControlComponent>(entityID);
            if (!cc.IsActive)
            {
                continue;
            }

            auto rect = UIRenderer::Get().GetEntityRect(entity, viewportSize, viewportScreenPos);

            Vector2 mouse = {mousePos.x, mousePos.y};
            if (mouse.x >= rect.x && mouse.x <= rect.x + rect.width && mouse.y >= rect.y &&
                mouse.y <= rect.y + rect.height)
            {
                bestHit = entity;
                CH_CORE_INFO("HIT UI: {}", entity.GetComponent<TagComponent>().Tag);
            }
        }

        // 3D Picking (only when camera is present)
        if (!bestHit && cameraFound)
        {
            SceneRaycastResult result = ScenePicker::Raycast(activeScene.get(), ray);
            if (result.Hit)
            {
                bestHit = result.HitEntity;
                minDistance = result.Distance;
                CH_CORE_INFO("HIT: {} at Dist {:.2f}", bestHit.GetComponent<TagComponent>().Tag, minDistance);
            }
        }

        if (bestHit)
        {
            CH_CORE_WARN("FINAL SELECTION: {}", bestHit.GetComponent<TagComponent>().Tag);
            EntitySelectedEvent e((entt::entity)bestHit, activeScene.get());
            EditorLayer::Get().OnEvent(e);
        }
        else
        {
            CH_CORE_WARN("FINAL SELECTION: NONE");
            // Only deselect if we actually clicked and missed everything
            EntitySelectedEvent e(entt::null, activeScene.get());
            EditorLayer::Get().OnEvent(e);
        }
    }

    // --- 4. FLOATING HUD (Drawn last to be on top of SceneUI) ---
    // Floating style for cleaner viewport
    ImVec2 toolbarPos = {viewportScreenPos.x + 10.0f, viewportScreenPos.y + 10.0f};
    ImGui::SetNextWindowPos(toolbarPos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.12f, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

    if (ImGui::BeginChild("##FloatingToolbar", ImVec2(850, 40), true,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::SetCursorPosY(6); // Center align vertically-ish
        ImGui::Indent(5);

        DrawGizmoButtons();

        DrawCameraSelector(activeScene.get());
        
        ImGui::SameLine(0, 10);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 10);

        // Snapping toggle
        bool snapping = m_Gizmo.IsSnappingEnabled();
        if (snapping) ImGui::PushStyleColor(ImGuiCol_Text, {0.3f, 0.8f, 1.0f, 1.0f});
        if (ImGui::Button(ICON_FA_MAGNET "##SnapToggle", {28, 28}))
        {
            m_Gizmo.SetSnapping(!snapping);
        }
        if (snapping) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enable Grid Snapping");

        ImGui::SameLine(0, 5);
        float gridSize = m_Gizmo.GetGridSize();
        ImGui::SetNextItemWidth(45);
        if (ImGui::DragFloat("##SnapValue", &gridSize, 0.1f, 0.1f, 10.0f, "%.1f"))
        {
            m_Gizmo.SetGridSize(gridSize);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Grid Snap Size");

        ImGui::SameLine(0, 10);
        
        // Local/World toggle
        bool isLocal = m_Gizmo.IsLocalSpace();
        if (ImGui::Button(isLocal ? (ICON_FA_CUBE " Local") : (ICON_FA_EARTH_AMERICAS " World"), {70, 28}))
        {
            m_Gizmo.SetLocalSpace(!isLocal);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Local/World Space");

        ImGui::SameLine(0, 15);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 15);

        // Playback Tools
        SceneState sceneState = EditorLayer::Get().GetSceneState();
        bool isPlaying = (sceneState == SceneState::Play);
        ImGui::SameLine(0, 10);

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

        ImGui::SameLine(0, 5);
        if (ImGui::Button(ICON_FA_FILE_CODE "##ReloadToolbar", ImVec2(28, 28)))
        {
            ScriptEngine::Get().ReloadAssembly();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Reload Scripts (Ctrl+R)");
        }

        ImGui::SameLine(0, 15);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 15);

        // Camera Info (Read-only status)
        Entity primaryCam = activeScene->GetPrimaryCameraEntity();
        if (primaryCam)
        {
            ImGui::TextDisabled(ICON_FA_CAMERA " %s", primaryCam.GetComponent<TagComponent>().Tag.c_str());
        }
        else
        {
            ImGui::TextColored({1, 0, 0, 1}, ICON_FA_CIRCLE_EXCLAMATION " No Primary Camera");
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    // --- 5. ROCKET LAUNCH BUTTON (Top Right Overlay) ---
    // Wrapped in a child window to ensure input priority over full-screen overlays (like ##SceneUI)
    ImGui::SetCursorScreenPos({viewportScreenPos.x + viewportSize.x - 110.0f, viewportScreenPos.y + 10.0f});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.12f, 0.8f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

    if (ImGui::BeginChild("##LaunchHUD", ImVec2(100, 40), true,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
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

    ImGui::End();
    ImGui::PopStyleVar();
    debugFlags.DrawGrid = oldGrid;

    // Shortcuts
    if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered())
    {
        for (const auto& btn : s_GizmoBtns)
        {
            if (CHEngine::Input::IsKeyPressed(btn.key))
            {
                m_CurrentTool = btn.type;
            }
        }

        if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed(Key::D))

        {
            Entity selected = EditorLayer::Get().GetSelectedEntity();
            if (selected)
            {
                EditorLayer::GetCommandHistory().PushCommand(std::make_unique<DuplicateEntityCommand>(selected));
            }
        }
    }
}

void ViewportPanel::OnUpdate(Timestep ts)
{
    // Only update editor camera in Edit mode
    if (EditorLayer::Get().GetSceneState() == SceneState::Edit)
    {
        auto activeScene = EditorLayer::Get().GetActiveScene();
        if (activeScene)
        {
            Entity primaryCamera = activeScene->GetPrimaryCameraEntity();
            if (primaryCamera)
            {
                m_CameraController->OnUpdate(primaryCamera, ts);
            }
        }
    }
}

void ViewportPanel::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<ViewportFocusEntityEvent>([this](ViewportFocusEntityEvent& ev) {
        Entity entity = ev.GetEntity();
        if (entity && entity.HasComponent<TransformComponent>())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            m_CameraController->GetCamera().SetFocalPoint(*reinterpret_cast<const Vector3*>(&transform.Translation));
            return true;
        }
        return false;
    });
}

} // namespace CHEngine
