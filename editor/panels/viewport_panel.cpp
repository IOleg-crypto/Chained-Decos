#include "viewport_panel.h"
#include "editor/editor_colors.h"
#include "editor/layer.h"
#include "editor/scene_picking.h"
#include "editor/viewport/ui_manipulator.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/core/events/events.h"
#include "engine/core/input.h"
#include "engine/core/key_codes.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/api/framebuffer.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/pipeline/debug_renderer.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/ui/widget_renderer.h"
#include "engine/project/project.h"
#include "engine/scene/prefab_serializer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "events.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "scripting/scriptengine.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "editor/asset_types.h"
#include "undo/entity_commands.h"

namespace Chained
{

Camera3D ViewportPanel::GetActiveOrEditorCamera(Scene* scene) const
{
    if (!scene)
    {
        return {};
    }
    auto activeCameraOpt = SceneRenderer::GetActiveCamera(scene->GetRegistry());
    if (activeCameraOpt.has_value() && m_SceneManager.GetSceneState() == SceneState::Play)
    {
        return activeCameraOpt.value();
    }
    return m_CameraController->ToCamera3D();
}

void ViewportPanel::ClearSceneBackground(Scene* scene)
{
    auto mode = scene->GetSettings().Mode;
    if (mode == BackgroundMode::Color)
    {
        GraphicsDevice::Get().Clear(scene->GetSettings().BackgroundColor);
    }
    else if (mode == BackgroundMode::Texture)
    {
        auto& path = scene->GetSettings().BackgroundTexturePath;
        if (!path.empty())
        {
            // Fallback for now
            GraphicsDevice::Get().Clear(scene->GetSettings().BackgroundColor);
        }
    }
    else if (mode == BackgroundMode::Environment3D)
    {
        GraphicsDevice::Get().Clear({0, 0, 0, 255});
    }
}

static const GizmoBtn s_GizmoBtns[] = {
    {GizmoType::NONE, ICON_FA_ARROW_POINTER "##Select", "Select (Q)", Chained::KeyCode::Q},
    {GizmoType::TRANSLATE, ICON_FA_UP_DOWN_LEFT_RIGHT "##Translate", "Translate (W)", Chained::KeyCode::W},
    {GizmoType::ROTATE, ICON_FA_ARROWS_ROTATE "##Rotate", "Rotate (E)", Chained::KeyCode::E},
    {GizmoType::SCALE, ICON_FA_UP_RIGHT_FROM_SQUARE "##Scale", "Scale (R)", Chained::KeyCode::R}};

void ViewportPanel::DrawCameraSelector(Scene* scene)
{
    if (!scene)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Button, EditorColors::FloatingToolbarBg);
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
    ImGui::PushStyleColor(ImGuiCol_Button, EditorColors::TransparentButton); // Transparent buttons in toolbar

    for (const auto& btn : s_GizmoBtns)
    {
        bool selected = (m_CurrentTool == btn.type);
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, EditorColors::ActiveToolOrange);
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

// Project's RenderSettings::AntiAliasingSamples is 0/2/4/8 ("0 = off"); Framebuffer's
// Samples field uses "1 = off" (matches GL's own multisample vs. non-multisample distinction).
static uint32_t GetConfiguredMSAASamples()
{
    auto project = Project::GetActive();
    int samples = project ? project->GetConfig().Render.AntiAliasingSamples : 4;
    return samples > 1 ? (uint32_t)samples : 1u;
}

ViewportPanel::ViewportPanel(CommandHistory& cmd, EditorSceneManager& sceneMgr, EditorProjectManager& projMgr,
                             EditorState& state, ImVec2& editorViewportSize)
    : m_CommandHistory(cmd),
      m_SceneManager(sceneMgr),
      m_ProjectManager(projMgr),
      m_EditorState(state),
      m_EditorViewportSize(editorViewportSize)
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
    hdrSpec.Samples = GetConfiguredMSAASamples();
    m_HDRFramebufferSamples = hdrSpec.Samples;
    m_HDRFramebuffer = Framebuffer::Create(hdrSpec);

    m_SceneRenderer = new SceneRenderer();
    m_CameraController = std::make_unique<EditorCameraController>();
}

ViewportPanel::~ViewportPanel()
{
    delete m_SceneRenderer;
}

void ViewportPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

    auto activeScene = m_SceneManager.GetActiveScene();

    std::string sceneName = "None";
    if (activeScene && !activeScene->GetSettings().Name.empty())
    {
        sceneName = activeScene->GetSettings().Name;
    }
    std::string title = m_Name + " [" + sceneName + "]###" + m_Name;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGui::Begin(title.c_str(), &m_IsOpen);

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    ImVec2 viewportScreenPos = ImGui::GetCursorScreenPos();

    // 1. Initial State & Resizing
    HandleResize(viewportSize, activeScene.get());

    if (!activeScene || viewportSize.x <= 0 || viewportSize.y <= 0)
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    m_Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    m_Hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

    // 2. Rendering
    RenderViewportScene(activeScene.get());

    // 3. UI Image & Interaction
    if (!m_ViewportFramebuffer || !m_ViewportFramebuffer->IsValid())
    {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }
    uint32_t finalTextureID = m_ViewportFramebuffer->GetColorAttachmentRendererID();

    // Capture the EXACT screen position where the image starts to prevent gizmo offset
    viewportScreenPos = ImGui::GetCursorScreenPos();

    ImGui::Image((ImTextureID)(uintptr_t)finalTextureID, viewportSize, {0, 1}, {1, 0});

    // 4. Drag & Drop
    HandleDragDrop(activeScene.get());

    // 5. Overlays (Gizmos, UI, Highlights)
    RenderOverlays(activeScene.get(), viewportSize, viewportScreenPos);

    // 6. Picking
    HandlePicking(activeScene.get(), viewportSize, viewportScreenPos);

    // 7. Toolbars
    RenderToolbar(activeScene.get(), viewportSize, viewportScreenPos);

    // Shortcuts & Keyboard Input — must be before ImGui::End() so IsWindowFocused works
    if (m_Focused || m_Hovered)
    {
        HandleKeyboardShortcuts();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::OnUpdate(Timestep ts)
{
    // Update editor camera in Edit and Simulate modes (not during Play)
    SceneState state = m_SceneManager.GetSceneState();
    if (state == SceneState::Edit || state == SceneState::Simulate)
    {
        auto activeScene = m_SceneManager.GetActiveScene();
        // Use m_Hovered that was set in the PREVIOUS frame's ImGuiRender.
        // Also allow update if right mouse is held (user clicked into viewport from outside).
        bool mouseInViewport = m_Hovered || Chained::Core::Input::IsMouseButtonDown(Chained::MouseCode::ButtonRight);
        if (activeScene && mouseInViewport)
        {
            const auto& editorCfg = EditorLayer::Get().GetConfig();
            m_CameraController->SetMoveSpeed(editorCfg.CameraMoveSpeed);
            m_CameraController->SetBoostMultiplier(editorCfg.CameraBoostMultiplier);
            m_CameraController->SetDisableZoom(editorCfg.DisableCameraZoom);
            m_CameraController->SetRotationSpeed(editorCfg.CameraRotationSpeed);
            m_CameraController->SetZoomSpeedMultiplier(editorCfg.CameraZoomSpeedMultiplier);
            m_CameraController->SetFovDegrees(editorCfg.CameraFovDegrees);
            m_CameraController->SetNearClip(editorCfg.CameraNearClip);
            m_CameraController->SetFarClip(editorCfg.CameraFarClip);

            Entity primaryCamera =
                SceneRenderer::GetPrimaryCameraEntity(activeScene->GetRegistry(), activeScene->GetRegistryPtr());
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
            m_CameraController->SetFocalPoint(*reinterpret_cast<const glm::vec3*>(&transform.Translation));
            return true;
        }
        return false;
    });
}

void ViewportPanel::HandleKeyboardShortcuts()
{
    for (const auto& btn : s_GizmoBtns)
    {
        if (Chained::Core::Input::IsKeyPressed(btn.key))
        {
            m_CurrentTool = btn.type;
        }
    }

    if (Chained::Core::Input::IsKeyDown(Chained::KeyCode::LeftControl) &&
        Chained::Core::Input::IsKeyPressed(Chained::KeyCode::D))
    {
        Entity selected = m_EditorState.SelectedEntity;
        if (selected)
        {
            m_CommandHistory.PushCommand(std::make_unique<DuplicateEntityCommand>(selected));
        }
    }
}

Ray ViewportPanel::GetMouseRay(const glm::vec2& mousePosition)
{
    auto activeScene = m_SceneManager.GetActiveScene();
    auto activeCameraOpt = SceneRenderer::GetActiveCamera(activeScene->GetRegistry());

    Camera3D camera;
    if (activeCameraOpt.has_value() && m_SceneManager.GetSceneState() == SceneState::Play)
    {
        camera = activeCameraOpt.value();
    }
    else
    {
        camera = m_CameraController->ToCamera3D();
    }

    return ScenePicker::CreateRayFromViewport(camera, mousePosition, m_ViewportSize);
}

void ViewportPanel::HandleResize(const ImVec2& viewportSize, Scene* activeScene)
{
    if (viewportSize.x != m_ViewportSize.x || viewportSize.y != m_ViewportSize.y)
    {
        m_ViewportSize = {viewportSize.x, viewportSize.y};
        if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
        {
            if (m_ViewportFramebuffer)
            {
                m_ViewportFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            }
            if (m_HDRFramebuffer)
            {
                m_HDRFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            }

            // Keep Renderer in sync so frustum & projection use correct aspect ratio
            ServiceLocator::Get<Renderer>()->SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

            m_EditorViewportSize = {m_ViewportSize.x, m_ViewportSize.y};
            m_CameraController->SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

            if (activeScene)
            {
                activeScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            }
        }
    }

    // Recreate FBOs if they became invalid (e.g. after context loss or bad resize),
    // or if the project's AntiAliasingSamples setting changed since we last (re)created them -
    // the sample count is baked into the framebuffer at creation and can't change in place.
    uint32_t configuredSamples = GetConfiguredMSAASamples();
    if (m_HDRFramebuffer && configuredSamples != m_HDRFramebufferSamples)
    {
        m_HDRFramebuffer.reset();
    }

    if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
    {
        if (!m_ViewportFramebuffer || !m_ViewportFramebuffer->IsValid())
        {
            FramebufferSpecification spec;
            spec.Width = (uint32_t)m_ViewportSize.x;
            spec.Height = (uint32_t)m_ViewportSize.y;
            spec.ColorFormat = FramebufferColorFormat::RGBA8;
            m_ViewportFramebuffer = Framebuffer::Create(spec);
        }
        if (!m_HDRFramebuffer || !m_HDRFramebuffer->IsValid())
        {
            FramebufferSpecification hdrSpec;
            hdrSpec.Width = (uint32_t)m_ViewportSize.x;
            hdrSpec.Height = (uint32_t)m_ViewportSize.y;
            hdrSpec.ColorFormat = FramebufferColorFormat::RGBA16F;
            hdrSpec.Samples = configuredSamples;
            m_HDRFramebufferSamples = configuredSamples;
            m_HDRFramebuffer = Framebuffer::Create(hdrSpec);
        }
    }
}

void ViewportPanel::RenderViewportScene(Scene* activeScene)
{
    if (!m_HDRFramebuffer || !m_HDRFramebuffer->IsValid())
    {
        return;
    }

    m_HDRFramebuffer->Bind();
    ClearSceneBackground(activeScene);

    if (!activeScene)
    {
        m_HDRFramebuffer->Unbind();
        return;
    }

    auto activeCameraOpt = SceneRenderer::GetActiveCamera(activeScene->GetRegistry());
    bool cameraFound = activeCameraOpt.has_value();
    auto camera = m_CameraController->ToCamera3D();

    if (cameraFound && m_SceneManager.GetSceneState() == SceneState::Play)
    {
        camera = activeCameraOpt.value();
    }

    if (glm::distance(glm::vec3(camera.Position), glm::vec3(camera.Target)) < 0.001f)
    {
        camera.Position.z += 1.0f;
    }

    camera.ViewMatrix = glm::lookAt(glm::vec3(camera.Position), glm::vec3(camera.Target), glm::vec3(camera.Up));

    float aspect = (float)m_ViewportSize.x / std::max((float)m_ViewportSize.y, 1.0f);
    if (camera.Projection == ProjectionType::Perspective)
    {
        camera.ProjectionMatrix =
            glm::perspective(glm::radians(camera.FovDegrees), aspect, camera.NearClip, camera.FarClip);
    }
    else // Orthographic
    {
        float orthoSize = camera.OrthographicSize;
        camera.ProjectionMatrix =
            glm::ortho(-aspect * orthoSize, aspect * orthoSize, -orthoSize, orthoSize, camera.NearClip, camera.FarClip);
    }

    SceneRenderOptions options;
    auto& currentDebugFlags = activeScene->GetSettings().DebugFlags;
    options.DrawGrid = currentDebugFlags.DrawGrid;
    options.ShowDebugColliders = currentDebugFlags.DrawColliders;
    options.ShowDebugSpawnZones = currentDebugFlags.DrawSpawnZones;
    options.SetCollisionWireframeMode = currentDebugFlags.SetCollisionWireframeMode;
    m_SceneRenderer->RenderScene(activeScene->GetRegistry(), activeScene->GetSettings(), camera, camera.NearClip,
                                 camera.FarClip, options);

    // Render proper editor icons (camera, light, spawn) with loaded textures
    if (m_SceneManager.GetSceneState() != SceneState::Play && EditorLayer::Get().GetConfig().ShowEditorIcons)
    {
        RenderEditorIcons(activeScene->GetRegistry(), activeScene->GetSettings(), camera);
    }

    m_HDRFramebuffer->Unbind();
    // Multisample attachments aren't directly sampleable - resolve into the single-sample
    // texture that ApplyPostProcessing()/GetColorAttachmentRendererID() below reads from.
    m_HDRFramebuffer->Resolve();

    if (!m_ViewportFramebuffer || !m_ViewportFramebuffer->IsValid())
    {
        return;
    }

    m_ViewportFramebuffer->Bind();
    GraphicsDevice::Get().Clear({0, 0, 0, 255});

    ServiceLocator::Get<Renderer>()->ApplyPostProcessing(m_HDRFramebuffer->GetColorAttachmentRendererID(),
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
                m_SceneManager.OpenScene(filepath);
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

                // Select the new entity. Dispatch through the app so Inspector/Material
                // panels (which subscribe to EntitySelectedEvent) also refresh — the event
                // handler in EditorLayer::OnEvent updates m_EditorState.SelectedEntity for us.
                EntitySelectedEvent e((entt::entity)entity, activeScene);
                Application::Get().OnEvent(e);
            }

        }
        ImGui::EndDragDropTarget();
    }
}

void ViewportPanel::RenderOverlays(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
{
    auto selectedEntity = m_EditorState.SelectedEntity;
    bool isUISelected = selectedEntity && selectedEntity.HasComponent<ControlComponent>();
    auto camera = GetActiveOrEditorCamera(activeScene);

    ImGui::SetCursorScreenPos(viewportScreenPos);

    // 1. Gizmo handling (using absolute screen coordinates)
    m_Gizmo.RenderAndHandle(!isUISelected ? m_CurrentTool : GizmoType::NONE, viewportScreenPos, viewportSize, camera);

    // 2. Game UI Overlay
    ImVec2 canvasOrigin = viewportScreenPos;
    ServiceLocator::Get<WidgetRenderer>()->DrawCanvas(activeScene, canvasOrigin, viewportSize,
                                                  m_SceneManager.GetSceneState() == SceneState::Edit);

    // 3. Selection Highlight
    if (isUISelected && selectedEntity && m_SceneManager.GetSceneState() == SceneState::Edit)
    {
        auto rect = ServiceLocator::Get<WidgetRenderer>()->GetEntityRect(activeScene, selectedEntity, viewportSize,
                                                                     viewportScreenPos);

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

void ViewportPanel::HandlePicking(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
{
    // Object picking logic
    auto activeCameraOpt = SceneRenderer::GetActiveCamera(activeScene->GetRegistry());
    bool isUIChildHovered =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    bool isClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool isDragging = m_UIManipulator.IsActive();
    bool isGizmoDragging = m_Gizmo.IsDragging();
    bool isGizmoHovered = m_Gizmo.IsHovered();
    SceneState sceneState = m_SceneManager.GetSceneState();

    if ((sceneState == SceneState::Edit || sceneState == SceneState::Simulate) && isUIChildHovered && isClicked &&
        !isGizmoDragging && !isGizmoHovered && !isDragging)
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
            if (!cc.IsActive)
            {
                continue;
            }

            auto rect =
                ServiceLocator::Get<WidgetRenderer>()->GetEntityRect(activeScene, entity, viewportSize, viewportScreenPos);
            if (mousePos.x >= rect.x && mousePos.x <= rect.x + rect.width && mousePos.y >= rect.y &&
                mousePos.y <= rect.y + rect.height)
            {
                bestHit = entity;
            }
        }

        // 3D Picking
        if (!bestHit)
        {
            SceneRaycastResult result = ScenePicker::Raycast(activeScene, ray);
            if (result.Hit)
            {
                bestHit = result.HitEntity;
            }
        }

        if (bestHit)
        {
            // Dispatch via Application so Inspector / MaterialPanel (which listen for
            // EntitySelectedEvent in their OnEvent handlers) refresh too. EditorLayer's
            // own handler is what writes m_EditorState.SelectedEntity — don't do it twice.
            EntitySelectedEvent e((entt::entity)bestHit, activeScene);
            Application::Get().OnEvent(e);
        }
        else
        {
            // Only deselect if the mouse is genuinely inside the viewport area
            ImVec2 mousePos = ImGui::GetMousePos();
            bool mouseInViewport =
                (mousePos.x >= viewportScreenPos.x && mousePos.x <= viewportScreenPos.x + viewportSize.x &&
                 mousePos.y >= viewportScreenPos.y && mousePos.y <= viewportScreenPos.y + viewportSize.y);
            if (mouseInViewport)
            {
                EntitySelectedEvent e(entt::null, activeScene);
                Application::Get().OnEvent(e);
            }
        }
    }
}

void ViewportPanel::RenderToolbar(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
{
    ImVec2 toolbarPos = {viewportScreenPos.x + 10.0f, viewportScreenPos.y + 10.0f};
    ImGui::SetNextWindowPos(toolbarPos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, EditorColors::ToolbarBg);
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

        DrawSnapSection();
        DrawTransformSpaceToggle();

        ImGui::SameLine(0, 15);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 15);

        DrawPlaybackControls();
        DrawScriptReloadButton();

        ImGui::SameLine(0, 15);
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine(0, 15);

        DrawRunInNewWindowButton();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void ViewportPanel::DrawSnapSection()
{
    bool snapping = m_Gizmo.IsSnappingEnabled();
    if (snapping)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::ActiveSnapBlue);
    }
    if (ImGui::Button(ICON_FA_MAGNET "##SnapToggle", {28, 28}))
    {
        m_Gizmo.SetSnapping(!snapping);
    }
    if (snapping)
    {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Enable Grid Snapping");
    }

    ImGui::SameLine(0, 5);
    float gridSize = m_Gizmo.GetGridSize();
    ImGui::SetNextItemWidth(45);
    if (ImGui::DragFloat("##SnapValue", &gridSize, 0.1f, 0.1f, 10.0f, "%.1f"))
    {
        m_Gizmo.SetGridSize(gridSize);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Grid Snap Size");
    }
}

void ViewportPanel::DrawTransformSpaceToggle()
{
    ImGui::SameLine(0, 10);

    bool isLocal = m_Gizmo.IsLocalSpace();
    if (ImGui::Button(isLocal ? (ICON_FA_CUBE " Local") : (ICON_FA_EARTH_AMERICAS " World"), {70, 28}))
    {
        m_Gizmo.SetLocalSpace(!isLocal);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Toggle Local/World Space");
    }
}

void ViewportPanel::DrawPlaybackControls()
{
    SceneState sceneState = m_SceneManager.GetSceneState();
    bool isPlaying = (sceneState == SceneState::Play);
    bool isSimulating = (sceneState == SceneState::Simulate);
    ImGui::SameLine(0, 10);

    if (isPlaying)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::PlayGreen);
    }
    if (ImGui::Button(isPlaying ? ICON_FA_STOP : ICON_FA_PLAY, ImVec2(28, 28)))
    {
        if (isPlaying)
        {
            m_SceneManager.SetSceneState(SceneState::Edit);
        }
        else
        {
            m_SceneManager.SetSceneState(SceneState::Play);
        }
    }
    if (isPlaying)
    {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(isPlaying ? "Stop" : "Play (Run Physics & Scripts)");
    }

    ImGui::SameLine(0, 5);

    if (isSimulating)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::SimulateOrange);
    }
    if (ImGui::Button(isSimulating ? ICON_FA_STOP : ICON_FA_GEARS, ImVec2(28, 28)))
    {
        if (isSimulating)
        {
            m_SceneManager.SetSceneState(SceneState::Edit);
        }
        else
        {
            m_SceneManager.SetSceneState(SceneState::Simulate);
        }
    }
    if (isSimulating)
    {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(isSimulating ? "Stop Simulation" : "Simulate (Run Physics Only)");
    }
}

void ViewportPanel::DrawScriptReloadButton()
{
    ImGui::SameLine(0, 5);
    if (ImGui::Button(ICON_FA_FILE_CODE "##ReloadToolbar", ImVec2(28, 28)))
    {
        auto project = Project::GetActive();
        if (project)
        {
            auto assemblyPath = ScriptEngine::ResolveAssemblyPath(project->GetConfig().Scripting,
                                                                   project->GetConfig().ProjectDirectory);
            auto& scriptEngine = *ServiceLocator::Get<ScriptEngine>();
            scriptEngine.RequestAssemblyReload(assemblyPath.string(), "ViewportPanel");
        }
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Reload Scripts (Ctrl+R)");
    }
}

void ViewportPanel::DrawRunInNewWindowButton()
{
    if (ImGui::Button(ICON_FA_WINDOW_MAXIMIZE "##RunSceneInNewWindow", ImVec2(28, 28)))
    {
        AppLaunchRuntimeEvent e;
        Application::Get().OnEvent(e);
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Run Scene in New Window (Shift+F5)");
    }
}

void ViewportPanel::RenderLightIcons(entt::registry& registry, const Camera3D& camera, float iconMin, float iconMax, float iconScale)
{
    const glm::vec3 activeCameraPos = camera.Position;

    auto iconSizeFromDistance = [&](const glm::vec3& worldPos) {
        const float distanceToCamera = glm::distance(worldPos, activeCameraPos);
        return std::clamp(distanceToCamera * iconScale, iconMin, iconMax);
    };

    auto getIconHandle = [](const std::shared_ptr<TextureAsset>& icon) -> uint32_t {
        if (icon && icon->GetTexture())
        {
            return icon->GetTexture()->GetNativeHandle();
        }
        return 0;
    };

    auto lightView = registry.view<TransformComponent, LightComponent>();
    for (auto entity : lightView)
    {
        auto [transform, light] = lightView.get<TransformComponent, LightComponent>(entity);
        const glm::vec3 iconPos = glm::vec3(transform.WorldTransform[3]);
        const float iconSize = iconSizeFromDistance(iconPos);

        glm::vec4 lightTint = {light.LightColor.r / 255.0f, light.LightColor.g / 255.0f,
                               light.LightColor.b / 255.0f, 0.95f};

        uint32_t handle = getIconHandle(m_EditorIcons.LightIcon);
        if (handle != 0)
        {
            ServiceLocator::Get<Renderer>()->DrawBillboard(camera, handle, iconPos, iconSize, lightTint);
            if (light.Type == LightType::Directional)
            {
                glm::vec3 dir = glm::normalize(glm::vec3(transform.WorldTransform[2])) * 0.45f;
                ServiceLocator::Get<DebugRenderer>()->DrawLine(iconPos, iconPos + dir, lightTint);
            }
        }
        else if (light.Type == LightType::Directional)
        {
            glm::vec3 dir = glm::normalize(glm::vec3(transform.WorldTransform[2])) * 0.5f;
            ServiceLocator::Get<DebugRenderer>()->DrawLine(iconPos, iconPos + dir, lightTint);
        }
        else if (light.Type == LightType::Point)
        {
            ServiceLocator::Get<DebugRenderer>()->DrawSphereWires(transform.WorldTransform, light.Radius * 0.1f,
                                                                   lightTint, *ServiceLocator::Get<Renderer>());
        }
        else if (light.Type == LightType::Spot)
        {
            ServiceLocator::Get<DebugRenderer>()->DrawSphereWires(transform.WorldTransform, light.Radius * 0.05f,
                                                                   lightTint, *ServiceLocator::Get<Renderer>());
        }
    }
}

void ViewportPanel::RenderEditorIcons(entt::registry& registry, const SceneSettings& settings, const Camera3D& camera)
{
    const glm::vec3 activeCameraPos = camera.Position;

    auto tryLoadIcon = [&](const char* path, std::shared_ptr<TextureAsset>& cachedIcon) {
        if (cachedIcon)
        {
            return;
        }

        cachedIcon = ServiceLocator::Get<AssetManager>()->Load<TextureAsset>(path);
    };

    tryLoadIcon("engine/resources/icons/camera_icon.png", m_EditorIcons.CameraIcon);
    tryLoadIcon("engine/resources/icons/light_bulb.png", m_EditorIcons.LightIcon);
    tryLoadIcon("engine/resources/icons/leaf_icon.png", m_EditorIcons.SpawnIcon);
    tryLoadIcon("engine/resources/icons/audio.png", m_EditorIcons.AudioIcon);

    auto iconSizeFromDistance = [&](const glm::vec3& worldPos, float minSize, float maxSize, float scale) {
        const float distanceToCamera = glm::distance(worldPos, activeCameraPos);
        return std::clamp(distanceToCamera * scale, minSize, maxSize);
    };

    // Gizmo icon sizing comes from the global editor settings (Editor Settings > Appearance).
    const auto& editorCfg = EditorLayer::Get().GetConfig();
    const float iconMin = editorCfg.IconSizeMin;
    const float iconMax = editorCfg.IconSizeMax;
    const float iconScale = editorCfg.IconSizeScale;

    auto getIconHandle = [](const std::shared_ptr<TextureAsset>& icon) -> uint32_t {
        if (icon && icon->GetTexture())
        {
            return icon->GetTexture()->GetNativeHandle();
        }
        return 0;
    };

    // Camera icons
    auto cameraView = registry.view<TransformComponent, CameraComponent>();
    for (auto entity : cameraView)
    {
        auto [transform, cameraComp] = cameraView.get<TransformComponent, CameraComponent>(entity);
        const glm::vec3 iconPos = glm::vec3(transform.WorldTransform[3]);
        if (glm::distance(iconPos, activeCameraPos) < 0.25f)
        {
            continue;
        }

        const float iconSize = iconSizeFromDistance(iconPos, iconMin, iconMax, iconScale);
        const glm::vec4 cameraTint = glm::vec4(0.65f, 0.95f, 1.0f, 0.95f);
        uint32_t handle = getIconHandle(m_EditorIcons.CameraIcon);
        if (handle != 0)
        {
            ServiceLocator::Get<Renderer>()->DrawBillboard(camera, handle, iconPos, iconSize, cameraTint);
        }
    }

    // Light icons
    RenderLightIcons(registry, camera, iconMin, iconMax, iconScale);

    // Spawn icons
    {
        auto spawnView = registry.view<TransformComponent, SpawnComponent>();
        for (auto entity : spawnView)
        {
            auto [transform, spawn] = spawnView.get<TransformComponent, SpawnComponent>(entity);
            const glm::vec3 iconPos = glm::vec3(transform.WorldTransform[3]);
            const float iconSize = iconSizeFromDistance(iconPos, iconMin, iconMax, iconScale);
            glm::vec4 spawnTint = {1.0f, 1.0f, 1.0f, 0.95f};
            uint32_t handle = getIconHandle(m_EditorIcons.SpawnIcon);
            if (handle != 0)
            {
                ServiceLocator::Get<Renderer>()->DrawBillboard(camera, handle, iconPos, iconSize, spawnTint);
            }
        }
    }
    {
        // Audio Component
        auto audioView = registry.view<TransformComponent, AudioComponent>();
        for (auto entity : audioView)
        {
            auto [transform, audio] = audioView.get<TransformComponent, AudioComponent>(entity);
            const glm::vec3 iconPos = glm::vec3(transform.WorldTransform[3]);
            const float iconSize = iconSizeFromDistance(iconPos, iconMin, iconMax, iconScale);
            glm::vec4 audioTint = {1.0f, 1.0f, 1.0f, 0.95f};
            uint32_t handle = getIconHandle(m_EditorIcons.AudioIcon);
            if (handle != 0)
            {
                ServiceLocator::Get<Renderer>()->DrawBillboard(camera, handle, iconPos, iconSize, audioTint);
            }
        }
    }
}
} // namespace Chained
