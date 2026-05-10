#include "viewport_panel.h"


#include "IconsFontAwesome6.h"
#include "editor/editor_layer.h"
#include "editor/viewport/ui_manipulator.h"
#include "editor_events.h"
#include "editor_gui.h"
#include "editor_layer.h"
#include "editor/editor_context.h"
#include "engine/core/application.h"
#include "engine/core/events.h"
#include "engine/core/input.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/scene/components.h"
#include "engine/scene/prefab_serializer.h"
#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scene_picking.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "engine/core/service_locator.h"
#include "scripting/scriptengine.h"
#include "undo/entity_commands.h"

namespace CHEngine
{
void ViewportPanel::ClearSceneBackground(Scene* scene)
{
    auto mode = scene->GetSettings().Mode;
    if (mode == BackgroundMode::Color)
    {
        RenderCommand::Clear(scene->GetSettings().BackgroundColor);
    }
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
        RenderCommand::Clear({0, 0, 0, 255});
    }
}

static const GizmoBtn s_GizmoBtns[] = {
    {GizmoType::NONE, ICON_FA_ARROW_POINTER "##Select", "Select (Q)", Key::Q},
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
    Entity primaryCam = SceneRenderer::GetPrimaryCameraEntity(scene->GetRegistry(), scene->GetRegistryPtr());
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
    spec.ColorFormat = FramebufferColorFormat::RGBA8;

    if (Application::Get().GetWindow().GetNativeWindow())
    {
        spec.Width = Application::Get().GetWindow().GetWidth() > 0 ? Application::Get().GetWindow().GetWidth() : 1280;
        spec.Height = Application::Get().GetWindow().GetHeight() > 0 ? Application::Get().GetWindow().GetHeight() : 720;
    }

    m_ViewportFramebuffer = Framebuffer::Create(spec);

    FramebufferSpecification hdrSpec = spec;
    hdrSpec.ColorFormat = FramebufferColorFormat::RGBA16F;
    m_HDRFramebuffer = Framebuffer::Create(hdrSpec);

    m_SceneRenderer = std::make_unique<SceneRenderer>();
    m_CameraController = std::make_unique<EditorCameraController>();
}

ViewportPanel::~ViewportPanel()
{
}

void ViewportPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin(m_Name.c_str(), &m_IsOpen);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportScreenPos = ImGui::GetCursorScreenPos();

    auto activeScene = ServiceLocator::Get<EditorLayer>().GetActiveScene();

    // 1. Initial State & Resizing
    HandleResize(viewportSize, activeScene.get());

    if (!activeScene || viewportSize.x <= 0 || viewportSize.y <= 0)
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    m_Focused = ImGui::IsWindowFocused();
    m_Hovered = ImGui::IsWindowHovered();

    // 2. Rendering
    RenderViewportScene(activeScene.get());

    // 3. UI Image & Interaction
    uint32_t finalTextureID = m_ViewportFramebuffer->GetColorAttachmentRendererID();
    ImGui::Image((ImTextureID)(uintptr_t)finalTextureID, viewportSize, {0, 1}, {1, 0});

    // 4. Drag & Drop
    HandleDragDrop(activeScene.get());

    // 5. Overlays (Gizmos, UI, Highlights)
    RenderOverlays(activeScene.get(), viewportSize, viewportScreenPos);

    // 6. Picking
    HandlePicking(activeScene.get(), viewportSize, viewportScreenPos);

    // 7. Toolbars
    RenderToolbar(activeScene.get(), viewportSize, viewportScreenPos);
    RenderLaunchHUD(viewportSize, viewportScreenPos);

    ImGui::End();
    ImGui::PopStyleVar();

    // Shortcuts & Keyboard Input
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
            Entity selected = ServiceLocator::Get<EditorLayer>().GetSelectedEntity();
            if (selected){
                ServiceLocator::Get<EditorLayer>().GetCommandHistory().PushCommand(std::make_unique<DuplicateEntityCommand>(selected));
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
        // Use m_Focused/m_Hovered that were set in the PREVIOUS frame's ImGuiRender.
        // Also allow update if right mouse is held (user clicked into viewport from outside).
        bool mouseInViewport = m_Hovered || m_Focused || Input::IsMouseButtonDown(Mouse::ButtonRight);
        if (activeScene && mouseInViewport)
        {
            Entity primaryCamera = SceneRenderer::GetPrimaryCameraEntity(activeScene->GetRegistry(), activeScene->GetRegistryPtr());
            m_CameraController->OnUpdate(primaryCamera, ts, m_ViewportSize);
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
            m_CameraController->GetCamera().SetFocalPoint(*reinterpret_cast<const glm::vec3*>(&transform.Translation));
            return true;
        }
        return false;
    });
}

Ray ViewportPanel::GetMouseRay(const glm::vec2& mousePosition)
{
    auto activeScene = ServiceLocator::Get<EditorLayer>().GetActiveScene();
    auto activeCameraOpt = SceneRenderer::GetActiveCamera(activeScene->GetRegistry());
    CHEngine::Camera3D camera;

    if (activeCameraOpt.has_value() && EditorLayer::Get().GetSceneState() == SceneState::Play)
    {
        camera = activeCameraOpt.value();
    }
    else
    {
        // Fallback to editor camera
        auto& edCam = m_CameraController->GetCamera();
        glm::vec3 pos = edCam.CalculatePosition();
        camera.Position = {pos.x, pos.y, pos.z};
        glm::vec3 fp = edCam.GetFocalPoint();
        camera.Target = {fp.x, fp.y, fp.z};
        glm::vec3 up = edCam.GetUpDirection();
        camera.Up = {up.x, up.y, up.z};
        camera.Fovy = glm::degrees(edCam.GetPerspectiveVerticalFOV());
        camera.Projection = 0; // Perspective
    }

    return ScenePicker::CreateRayFromViewport(camera, mousePosition, m_ViewportSize);
}

void ViewportPanel::HandleResize(const ImVec2& viewportSize, Scene* activeScene)
{
    if (viewportSize.x != m_ViewportSize.x || viewportSize.y != m_ViewportSize.y)
    {
        m_ViewportSize = { viewportSize.x, viewportSize.y };
        if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
        {
            m_ViewportFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_HDRFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

            // Keep Renderer in sync so frustum & projection use correct aspect ratio
            if (ServiceLocator::Has<Renderer>())
            {
                ServiceLocator::Get<Renderer>().SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            }

            EditorLayer::Get().OnViewportResized({ m_ViewportSize.x, m_ViewportSize.y });
            m_CameraController->GetCamera().SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

            if (activeScene)
            {
                activeScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            }
        }
    }
}

void ViewportPanel::RenderViewportScene(Scene* activeScene)
{
    m_HDRFramebuffer->Bind();
    ClearSceneBackground(activeScene);

    if (!activeScene)
    {
        return;
    }

    auto activeCameraOpt = SceneRenderer::GetActiveCamera(activeScene->GetRegistry());
    bool cameraFound = activeCameraOpt.has_value();
    CHEngine::Camera3D camera;
    float nearClip = 0.01f;
    float farClip = 10000.0f;

    // Default to Editor Camera
    auto& edCam = m_CameraController->GetCamera();
    glm::vec3 pos = edCam.CalculatePosition();
    camera.Position = {pos.x, pos.y, pos.z};

    glm::vec3 fp = edCam.GetFocalPoint();
    camera.Target = {fp.x, fp.y, fp.z};

    glm::vec3 up = edCam.GetUpDirection();
    camera.Up = {up.x, up.y, up.z};

    camera.Fovy = glm::degrees(edCam.GetPerspectiveVerticalFOV()); // Fovy in degrees
    camera.Projection = 0;                                         // Perspective

    nearClip = edCam.GetPerspectiveNearClip();
    farClip = edCam.GetPerspectiveFarClip();

    // If an entity camera is active during Play mode, override the viewport perspective
    if (cameraFound && EditorLayer::Get().GetSceneState() == SceneState::Play)
    {
        camera = activeCameraOpt.value();
        Entity primaryCam = SceneRenderer::GetPrimaryCameraEntity(activeScene->GetRegistry(), activeScene->GetRegistryPtr());
        if (primaryCam && primaryCam.HasComponent<CameraComponent>())
        {
            auto& cameraComp = primaryCam.GetComponent<CameraComponent>().Camera;
            nearClip = cameraComp.GetPerspectiveNearClip();
            farClip = cameraComp.GetPerspectiveFarClip();
        }
    }

    SceneRenderOptions options;
    auto& currentDebugFlags = activeScene->GetSettings().DebugFlags;
    options.DrawGrid = currentDebugFlags.DrawGrid;
    options.ShowDebugColliders = currentDebugFlags.DrawColliders;
    options.ShowDebugCollisionModelBox = currentDebugFlags.DrawCollisionModelBox;
    options.ShowDebugSpawnZones = currentDebugFlags.DrawSpawnZones;
    options.SetCollisionWireframeMode = currentDebugFlags.SetCollisionWireframeMode;
    options.ShowEditorIcons = true;

    m_SceneRenderer->RenderScene(activeScene->GetRegistry(), activeScene->GetSettings(), camera, nearClip, farClip, options);
    m_HDRFramebuffer->Unbind();
 
    // 3. Application of Post-processing
    m_ViewportFramebuffer->Bind();
    RenderCommand::Clear({0, 0, 0, 255}); 

    ServiceLocator::Get<Renderer>().ApplyPostProcessing(
        m_HDRFramebuffer->GetColorAttachmentRendererID(),
        m_HDRFramebuffer->GetDepthAttachmentRendererID(), camera,
        nullptr, {});

 
    m_ViewportFramebuffer->Unbind();
}

void ViewportPanel::HandleDragDrop(Scene* activeScene)
{
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
                EditorLayer::Get().GetSceneManager().OpenScene(filepath);
            }
            else if (ext == ".chprefab")
            {
                PrefabSerializer::Deserialize(activeScene, filepath.string());
            }
            else if (ext == ".gltf" || ext == ".glb" || ext == ".obj")
            {
                std::string filename = filepath.stem().string();
                Entity entity = activeScene->CreateEntity(filename);
                auto& modelcomp = entity.AddComponent<ModelComponent>();
                // Use relative path if possible to satisfy portability
                modelcomp.ModelPath = Project::GetRelativePath(filepath);

                // Select the new entity
                EntitySelectedEvent e((entt::entity)entity, activeScene);
                EditorLayer::Get().OnEvent(e);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void ViewportPanel::RenderOverlays(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
{
    auto selectedEntity = EditorLayer::Get().GetSelectedEntity();
    bool isUISelected = selectedEntity && selectedEntity.HasComponent<ControlComponent>();
    auto activeCameraOpt = SceneRenderer::GetActiveCamera(activeScene->GetRegistry());
    CHEngine::Camera3D camera;
    if (activeCameraOpt.has_value())
    {
        camera = activeCameraOpt.value();
    }
    else
    {
        // Fallback to editor camera for gizmos even if no scene camera
        auto& edCam = m_CameraController->GetCamera();
        glm::vec3 pos = edCam.CalculatePosition();
        camera.Position = {pos.x, pos.y, pos.z};
        glm::vec3 fp = edCam.GetFocalPoint();
        camera.Target = {fp.x, fp.y, fp.z};
        glm::vec3 up = edCam.GetUpDirection();
        camera.Up = {up.x, up.y, up.z};
        camera.Fovy = glm::degrees(edCam.GetPerspectiveVerticalFOV());
        camera.Projection = 0;
    }

    ImGui::SetCursorScreenPos(viewportScreenPos);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::BeginChild("##SceneUI", viewportSize, false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse))
    {
        // 1. Gizmo handling (inside child window for input priority)
        m_Gizmo.RenderAndHandle(!isUISelected ? m_CurrentTool : GizmoType::NONE, viewportScreenPos,
                                viewportSize, camera);

        // 2. Game UI Overlay
        ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
        ServiceLocator::Get<UIRenderer>().DrawCanvas(activeScene, canvasOrigin, viewportSize,
                                     EditorLayer::Get().GetSceneState() == SceneState::Edit);

        // 3. Selection Highlight
        if (isUISelected && selectedEntity && EditorLayer::Get().GetSceneState() == SceneState::Edit)
        {
            auto rect = ServiceLocator::Get<UIRenderer>().GetEntityRect(activeScene, selectedEntity, viewportSize, viewportScreenPos);

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
}

void ViewportPanel::HandlePicking(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
{
    // Object picking logic
    auto activeCameraOpt = SceneRenderer::GetActiveCamera(activeScene->GetRegistry());
    ImGuiContext& g = *GImGui;
    bool isUIChildHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    bool isClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool isDragging = m_UIManipulator.IsActive();
    bool isGizmoDragging = m_Gizmo.IsDragging();
    bool isGizmoHovered = m_Gizmo.IsHovered();
    SceneState sceneState = EditorLayer::Get().GetSceneState();

    if (sceneState == SceneState::Edit && isUIChildHovered && isClicked && !isGizmoDragging && !isGizmoHovered && !isDragging)
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 localMouseImGui = {mousePos.x - viewportScreenPos.x, mousePos.y - viewportScreenPos.y};

        Ray ray = GetMouseRay({localMouseImGui.x, localMouseImGui.y});

        Entity bestHit = {};

        // UI Picking
        auto uiView = activeScene->GetRegistry().view<ControlComponent>();
        for (auto entityID : uiView)
        {
            Entity entity(entityID, &activeScene->GetRegistry());
            auto& cc = uiView.get<ControlComponent>(entityID);
            if (!cc.IsActive) continue;

            auto rect = ServiceLocator::Get<UIRenderer>().GetEntityRect(activeScene, entity, viewportSize, viewportScreenPos);
            if (mousePos.x >= rect.x && mousePos.x <= rect.x + rect.width && mousePos.y >= rect.y && mousePos.y <= rect.y + rect.height)
            {
                bestHit = entity;
            }
        }

        // 3D Picking
        if (!bestHit && activeCameraOpt.has_value())
        {
            SceneRaycastResult result = ScenePicker::Raycast(activeScene, ray);
            if (result.Hit)
            {
                bestHit = result.HitEntity;
            }
        }

        if (bestHit)
        {
            EntitySelectedEvent e((entt::entity)bestHit, activeScene);
            EditorLayer::Get().OnEvent(e);
        }
        else
        {
            // Only deselect if the mouse is genuinely inside the viewport area
            ImVec2 mousePos = ImGui::GetMousePos();
            bool mouseInViewport = (mousePos.x >= viewportScreenPos.x &&
                                    mousePos.x <= viewportScreenPos.x + viewportSize.x &&
                                    mousePos.y >= viewportScreenPos.y &&
                                    mousePos.y <= viewportScreenPos.y + viewportSize.y);
            if (mouseInViewport)
            {
                EntitySelectedEvent e(entt::null, activeScene);
                EditorLayer::Get().OnEvent(e);
            }
        }
    }
}

void ViewportPanel::RenderToolbar(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
{
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
        DrawCameraSelector(activeScene);

        ImGui::SameLine(0, 10);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 10);

        // Snapping toggle
        bool snapping = m_Gizmo.IsSnappingEnabled();
        if (snapping) ImGui::PushStyleColor(ImGuiCol_Text, {0.3f, 0.8f, 1.0f, 1.0f});
        if (ImGui::Button(ICON_FA_MAGNET "##SnapToggle", {28, 28})) m_Gizmo.SetSnapping(!snapping);
        if (snapping) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enable Grid Snapping");

        ImGui::SameLine(0, 5);
        float gridSize = m_Gizmo.GetGridSize();
        ImGui::SetNextItemWidth(45);
        if (ImGui::DragFloat("##SnapValue", &gridSize, 0.1f, 0.1f, 10.0f, "%.1f")) m_Gizmo.SetGridSize(gridSize);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Grid Snap Size");

        ImGui::SameLine(0, 10);

        // Local/World toggle
        bool isLocal = m_Gizmo.IsLocalSpace();
        if (ImGui::Button(isLocal ? (ICON_FA_CUBE " Local") : (ICON_FA_EARTH_AMERICAS " World"), {70, 28})) m_Gizmo.SetLocalSpace(!isLocal);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Local/World Space");

        ImGui::SameLine(0, 15);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 15);

        // Playback Tools
        SceneState sceneState = EditorLayer::Get().GetSceneState();
        bool isPlaying = (sceneState == SceneState::Play);
        ImGui::SameLine(0, 10);

        if (isPlaying) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
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
        if (isPlaying) ImGui::PopStyleColor();

        ImGui::SameLine(0, 5);
        // Reload Scripts (Ctrl+R)
        if (ImGui::Button(ICON_FA_FILE_CODE "##ReloadToolbar", ImVec2(28, 28)))
        {
            auto project = Project::GetActive();
            if (project)
            {
                std::string moduleName = project->GetConfig().Scripting.ModuleName;
                if (moduleName.find(".dll") == std::string::npos)
                    moduleName += ".dll";
                    
                std::filesystem::path assemblyPath = Project::GetAssetDirectory() / "bin" / moduleName;
                auto& scriptEngine = ServiceLocator::Get<ScriptEngine>();
                scriptEngine.RequestAssemblyReload(assemblyPath.string(), "ViewportPanel");
            }
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reload Scripts (Ctrl+R)");

        ImGui::SameLine(0, 15);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 15);

        // Run scene in new window (for play mode testing)
        if (ImGui::Button(ICON_FA_WINDOW_MAXIMIZE "##RunSceneInNewWindow", ImVec2(28, 28)))
        {
            AppLaunchRuntimeEvent e;
            Application::Get().OnEvent(e);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Run Scene in New Window (Shift+F5)");

        // Camera Info
        Entity primaryCam = SceneRenderer::GetPrimaryCameraEntity(activeScene->GetRegistry(), activeScene->GetRegistryPtr());
        if (primaryCam) ImGui::TextDisabled(ICON_FA_CAMERA " %s", primaryCam.GetComponent<TagComponent>().Tag.c_str());
        else ImGui::TextColored({1, 0, 0, 1}, ICON_FA_CIRCLE_EXCLAMATION " No Primary Camera");
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void ViewportPanel::RenderLaunchHUD(const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
{
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
            AppLaunchRuntimeEvent e;
            Application::Get().OnEvent(e);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Build & Run Standalone project (F5)");
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

} // namespace CHEngine
