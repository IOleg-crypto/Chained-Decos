#include "viewport_panel.h"
#include "editor/asset_types.h"
#include "editor/editor_colors.h"
#include "editor/layer.h"
#include "editor/scene_picking.h"
#include "editor/viewport/ui_manipulator.h"
#include "engine/app/application.h"
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
#include "engine/ui/widget_renderer.h"
#include "engine/project/project.h"
#include "engine/scene/prefab_serializer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "events.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "engine/scripting/scriptengine.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "undo/entity_commands.h"
#include <limits>

namespace Chained
{

	constexpr float kMinIconClickRadius = 14.0f;

	Camera3D ViewportPanel::GetActiveOrEditorCamera(Scene* scene) const
	{
		if (!scene)
		{
			return {};
		}
		auto activeCameraOpt = SceneRenderer::GetActiveCamera(scene->GetRegistry());
		if (activeCameraOpt.has_value() && EditorLayer::Get().GetSceneManager().GetSceneState() == SceneState::Play)
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

	static uint32_t GetIconHandle(const std::shared_ptr<TextureAsset>& icon)
	{
		if (icon && icon->IsReady())
		{
			auto tex = icon->GetTexture();
			if (tex)
			{
				return tex->GetNativeHandle();
			}
		}
		return 0;
	}

	static float ComputeIconSize(const glm::vec3& worldPos, const glm::vec3& cameraPos, float minSize, float maxSize,
								 float scale)
	{
		const float distance = glm::distance(worldPos, cameraPos);
		return std::clamp(distance * scale, minSize, maxSize);
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
		std::string currentLabel = primaryCam ? primaryCam.GetComponent<TagComponent>().Tag : "Editor Camera";

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

	ViewportPanel::ViewportPanel(ImVec2& editorViewportSize)
		: m_EditorViewportSize(editorViewportSize)
	{
		m_Name = "Viewport";

		FramebufferSpecification spec;
		spec.Width = 1280;
		spec.Height = 720;
		spec.ColorFormat = FramebufferColorFormat::RGBA8;

		if (Application::Get().GetWindow().GetNativeWindow())
		{
			spec.Width =
				Application::Get().GetWindow().GetWidth() > 0 ? Application::Get().GetWindow().GetWidth() : 1280;
			spec.Height =
				Application::Get().GetWindow().GetHeight() > 0 ? Application::Get().GetWindow().GetHeight() : 720;
		}

		m_ViewportFramebuffer = Framebuffer::Create(spec);

		FramebufferSpecification hdrSpec = spec;
		hdrSpec.ColorFormat = FramebufferColorFormat::RGBA16F;
		hdrSpec.Samples = GetConfiguredMSAASamples();
		m_MSAAFramebufferSamples = hdrSpec.Samples;
		m_HDRFramebuffer = Framebuffer::Create(hdrSpec);

		m_SceneRenderer = std::make_unique<SceneRenderer>();
		m_CameraController = std::make_unique<EditorCameraController>();
	}

	ViewportPanel::~ViewportPanel() = default;

	void ViewportPanel::OnImGuiRender(bool readOnly)
	{
		if (!m_IsOpen)
		{
			return;
		}

		auto activeScene = EditorLayer::Get().GetSceneManager().GetActiveScene();

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
		if (!activeScene->IsStartingUp())
		{
			RenderViewportScene(activeScene.get());
		}

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

		bool isTransitioning = EditorLayer::Get().GetSceneManager().IsTransitioning();
		if (activeScene->IsStartingUp() || isTransitioning)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 p0 = ImGui::GetItemRectMin();
			ImVec2 p1 = ImGui::GetItemRectMax();

			drawList->AddRectFilled(p0, p1, IM_COL32(15, 15, 20, 200));

			std::string status = EditorLayer::Get().GetSceneManager().GetLoadingStatus();
			if (status.empty())
			{
				status = "Loading Scene...";
			}
			const char* text = status.c_str();
			ImVec2 textSize = ImGui::CalcTextSize(text);
			ImVec2 textPos = ImVec2(p0.x + (p1.x - p0.x - textSize.x) * 0.5f, p0.y + (p1.y - p0.y - textSize.y) * 0.5f);
			drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), text);
		}

		// 4. Drag & Drop
		HandleDragDrop(activeScene.get());

		// 5. Overlays (Gizmos, UI, Highlights)
		RenderOverlays(activeScene.get(), viewportSize, viewportScreenPos);

		// 6. Picking
		HandlePicking(activeScene.get(), viewportSize, viewportScreenPos);

		// 7. Toolbars
		RenderToolbar(activeScene.get(), viewportScreenPos);

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
		// Unlock cursor if the window lost focus while locked
		if (m_CursorLocked && !Application::Get().GetWindow().IsFocused())
		{
			Application::Get().GetWindow().SetCursorMode(CursorMode::Normal);
			m_CursorLocked = false;
		}

		// Auto-switch camera 2D mode based on scene type
		auto activeScene = EditorLayer::Get().GetSceneManager().GetActiveScene();
		if (activeScene)
		{
			SceneType sceneType = activeScene->GetSettings().Type;
			if (sceneType != m_LastSceneType)
			{
				bool want2D = (sceneType == SceneType::UI);
				if (m_CameraController->Is2DMode() != want2D)
				{
					m_CameraController->Set2DMode(want2D);
					m_Gizmo.Set2DMode(want2D);
				}
				m_LastSceneType = sceneType;
			}
		}

		// Cursor lock/unlock for camera rotation
		if (m_Hovered || m_CursorLocked)
		{
			bool rightDown = Chained::Core::Input::IsMouseButtonDown(Chained::MouseCode::ButtonRight);
			if (rightDown && !m_CursorLocked)
			{
				Application::Get().GetWindow().SetCursorMode(CursorMode::Locked);
				m_CursorLocked = true;
			}
			else if (!rightDown && m_CursorLocked)
			{
				Application::Get().GetWindow().SetCursorMode(CursorMode::Normal);
				m_CursorLocked = false;
			}
		}

		// Update editor camera in Edit and Simulate modes (not during Play)
		SceneState state = EditorLayer::Get().GetSceneManager().GetSceneState();
		if (state == SceneState::Edit || state == SceneState::Simulate)
		{
			auto activeScene = EditorLayer::Get().GetSceneManager().GetActiveScene();
			// Use m_Hovered that was set in the PREVIOUS frame's ImGuiRender.
			// Also allow update if right mouse is held (user clicked into viewport from outside).
			bool mouseInViewport =
				m_Hovered || Chained::Core::Input::IsMouseButtonDown(Chained::MouseCode::ButtonRight);
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
			Entity selected = EditorLayer::Get().GetEditorState().SelectedEntity;
			if (selected)
			{
				EditorLayer::Get().GetCommandHistory().PushCommand(std::make_unique<DuplicateEntityCommand>(selected));
			}
		}
	}

	Ray ViewportPanel::GetMouseRay(const glm::vec2& mousePosition)
	{
		auto activeScene = EditorLayer::Get().GetSceneManager().GetActiveScene();
		if (!activeScene)
		{
			return {};
		}
		Camera3D camera = GetActiveOrEditorCamera(activeScene.get());
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
				if (auto* renderer = ServiceLocator::TryGet<Renderer>())
				{
					renderer->SetViewportSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
				}

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
		if (m_HDRFramebuffer && configuredSamples != m_MSAAFramebufferSamples)
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
				m_MSAAFramebufferSamples = configuredSamples;
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

		auto camera = GetActiveOrEditorCamera(activeScene);

		if (glm::distance(glm::vec3(camera.Position), glm::vec3(camera.Target)) < 0.001f)
		{
			camera.Position.z += 1.0f;
			camera.ViewMatrix = glm::lookAt(glm::vec3(camera.Position), glm::vec3(camera.Target), glm::vec3(camera.Up));
		}

		SceneRenderOptions options;
		auto& currentDebugFlags = activeScene->GetSettings().DebugFlags;
		options.DrawGrid = currentDebugFlags.DrawGrid;
		options.ShowDebugColliders = currentDebugFlags.DrawColliders;
		options.ShowDebugSpawnZones = currentDebugFlags.DrawSpawnZones;
		options.SetCollisionWireframeMode = currentDebugFlags.SetCollisionWireframeMode;
		m_SceneRenderer->RenderScene(activeScene->GetRegistry(), activeScene->GetSettings(), camera, options);

		// Render proper editor icons (camera, light, spawn) with loaded textures
		if (EditorLayer::Get().GetSceneManager().GetSceneState() != SceneState::Play &&
			EditorLayer::Get().GetConfig().ShowEditorIcons)
		{
			RenderEditorIcons(activeScene->GetRegistry(), camera);
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

		if (auto* renderer = ServiceLocator::TryGet<Renderer>())
		{
			renderer->ApplyPostProcessing(m_HDRFramebuffer->GetColorAttachmentRendererID(),
										  m_HDRFramebuffer->GetDepthAttachmentRendererID(), camera, nullptr, {});
		}

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
					modelcomp.ModelPath = Project::GetActive()->GetRelativePath(filepath);

					SelectEntity(entity, activeScene);
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void ViewportPanel::RenderOverlays(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
	{
		auto selectedEntity = EditorLayer::Get().GetEditorState().SelectedEntity;
		if (selectedEntity)
		{
			if (!activeScene || selectedEntity.GetRegistryPtr() != &activeScene->GetRegistry() ||
				!selectedEntity.IsValid())
			{
				EditorLayer::Get().GetEditorState().SelectedEntity = {};
				selectedEntity = {};
			}
		}
		bool isUISelected = selectedEntity && selectedEntity.HasComponent<ControlComponent>();
		auto camera = GetActiveOrEditorCamera(activeScene);

		ImGui::SetCursorScreenPos(viewportScreenPos);

		// 1. Gizmo handling (using absolute screen coordinates)
		m_Gizmo.RenderAndHandle(!isUISelected ? m_CurrentTool : GizmoType::NONE, viewportScreenPos, viewportSize,
								camera);

		// 2. Game UI Overlay
		ImVec2 canvasOrigin = viewportScreenPos;
		if (auto* widgetRenderer = ServiceLocator::TryGet<WidgetRenderer>())
		{
			widgetRenderer->DrawCanvas(activeScene, canvasOrigin, viewportSize,
									   EditorLayer::Get().GetSceneManager().GetSceneState() == SceneState::Edit);
		}

		// 2b. Script UI Overlay (OnGUI)
		SceneState sceneState = EditorLayer::Get().GetSceneManager().GetSceneState();
		if (activeScene && (sceneState == SceneState::Play || sceneState == SceneState::Simulate))
		{
			ImGui::SetNextWindowPos(viewportScreenPos);
			ImGui::SetNextWindowSize(viewportSize);

			ImGuiWindowFlags scriptUIFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
											 ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings |
											 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

			if (ImGui::Begin("##ViewportScriptUIOverlay", nullptr, scriptUIFlags))
			{
				activeScene->OnRenderUI();
			}
			ImGui::End();
		}

		// 3. Selection Highlight
		if (isUISelected && selectedEntity && EditorLayer::Get().GetSceneManager().GetSceneState() == SceneState::Edit)
		{
			auto* widgetRenderer = ServiceLocator::TryGet<WidgetRenderer>();
			auto rect = widgetRenderer ? widgetRenderer->GetEntityRect(selectedEntity) : UIRect{0, 0, 0, 0};

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

	Entity ViewportPanel::HandleIconPicking(Scene* scene, const Camera3D& camera, const ImVec2& mousePos,
											const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
	{
		if (!EditorLayer::Get().GetConfig().ShowEditorIcons)
		{
			return {};
		}

		const auto& editorCfg = EditorLayer::Get().GetConfig();
		const float iconMin = editorCfg.IconSizeMin;
		const float iconMax = editorCfg.IconSizeMax;
		const float iconScale = editorCfg.IconSizeScale;

		const glm::mat4 vp = camera.ProjectionMatrix * camera.ViewMatrix;

		auto worldToScreen = [&](const glm::vec3& wp) -> glm::vec2 {
			glm::vec4 clip = vp * glm::vec4(wp, 1.0f);
			if (clip.w <= 0.0f)
			{
				return {-1.f, -1.f};
			}
			const glm::vec3 ndc = glm::vec3(clip) / clip.w;
			return {(ndc.x * 0.5f + 0.5f) * viewportSize.x + viewportScreenPos.x,
					(1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y + viewportScreenPos.y};
		};

		auto iconPixelRadius = [&](const glm::vec3& wp) -> float {
			const float dist = glm::distance(wp, camera.Position);
			const float worldSz = std::clamp(dist * iconScale, iconMin, iconMax);
			float ppu;
			if (camera.Projection == ProjectionType::Perspective && dist > 0.001f)
			{
				ppu = (viewportSize.y * 0.5f) / (std::tan(glm::radians(camera.FovDegrees) * 0.5f) * dist);
			}
			else
			{
				ppu = (viewportSize.y * 0.5f) / std::max(camera.OrthographicSize, 0.001f);
			}
			return std::max(worldSz * ppu * 0.5f, kMinIconClickRadius);
		};

		Entity bestHit = {};
		float bestIconDist = FLT_MAX;

		auto testIcon = [&](entt::entity id, const glm::vec3& wp) {
			const glm::vec2 sp = worldToScreen(wp);
			if (sp.x < 0.f)
			{
				return;
			}
			const float r = iconPixelRadius(wp);
			const float dx = mousePos.x - sp.x;
			const float dy = mousePos.y - sp.y;
			if (dx * dx + dy * dy <= r * r)
			{
				const float d = glm::distance(wp, camera.Position);
				if (d < bestIconDist)
				{
					bestIconDist = d;
					bestHit = Entity(id, &scene->GetRegistry());
				}
			}
		};

		auto& reg = scene->GetRegistry();

		reg.view<TransformComponent, CameraComponent>().each(
			[&](entt::entity id, TransformComponent& tc, CameraComponent&) {
				const glm::vec3 wp = glm::vec3(tc.WorldTransform[3]);
				if (glm::distance(wp, camera.Position) >= 0.25f)
				{
					testIcon(id, wp);
				}
			});

		reg.view<TransformComponent, LightComponent>().each(
			[&](entt::entity id, TransformComponent& tc, LightComponent&) {
				testIcon(id, glm::vec3(tc.WorldTransform[3]));
			});

		reg.view<TransformComponent, SpawnComponent>().each(
			[&](entt::entity id, TransformComponent& tc, SpawnComponent&) {
				testIcon(id, glm::vec3(tc.WorldTransform[3]));
			});

		reg.view<TransformComponent, AudioComponent>().each(
			[&](entt::entity id, TransformComponent& tc, AudioComponent&) {
				testIcon(id, glm::vec3(tc.WorldTransform[3]));
			});

		return bestHit;
	}

	void ViewportPanel::HandlePicking(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos)
	{
		if (EditorLayer::Get().GetSceneManager().IsTransitioning())
		{
			return;
		}

		// Object picking logic
		bool isClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
		bool isDragging = m_UIManipulator.IsActive();
		bool isGizmoDragging = m_Gizmo.IsDragging();
		bool isGizmoHovered = m_Gizmo.IsHovered();
		SceneState sceneState = EditorLayer::Get().GetSceneManager().GetSceneState();
		ImVec2 mousePos = ImGui::GetMousePos();
		bool mouseInViewport =
			(mousePos.x >= viewportScreenPos.x && mousePos.x <= viewportScreenPos.x + viewportSize.x &&
			 mousePos.y >= viewportScreenPos.y && mousePos.y <= viewportScreenPos.y + viewportSize.y);

		if ((sceneState == SceneState::Edit || sceneState == SceneState::Simulate) && mouseInViewport && isClicked &&
			!isGizmoDragging && !isGizmoHovered && !isDragging)
		{
			ImVec2 localMouseImGui = {mousePos.x - viewportScreenPos.x, mousePos.y - viewportScreenPos.y};

			Ray ray = GetMouseRay({localMouseImGui.x, localMouseImGui.y});

			Entity bestHit = {};

			// UI Picking
			auto uiView = activeScene->GetRegistry().view<ControlComponent>();
			int bestZOrder = std::numeric_limits<int>::min();
			for (auto entityID : uiView)
			{
				Entity entity(entityID, &activeScene->GetRegistry());
				auto& cc = uiView.get<ControlComponent>(entityID);
				if (!cc.IsActive || cc.HiddenInHierarchy)
				{
					continue;
				}

				auto* widgetRenderer = ServiceLocator::TryGet<WidgetRenderer>();
				auto rect = widgetRenderer ? widgetRenderer->GetEntityRect(entity) : UIRect{0, 0, 0, 0};
				if (mousePos.x >= rect.x && mousePos.x <= rect.x + rect.width && mousePos.y >= rect.y &&
					mousePos.y <= rect.y + rect.height)
				{
					if (cc.ZOrder >= bestZOrder)
					{
						bestZOrder = cc.ZOrder;
						bestHit = entity;
					}
				}
			}

			// Icon Picking — screen-space hit test against billboard icons
			if (!bestHit)
			{
				Camera3D cam = m_CameraController->ToCamera3D();
				bestHit = HandleIconPicking(activeScene, cam, mousePos, viewportSize, viewportScreenPos);
			}

			// 3D Picking
			if (!bestHit)
			{
				RaycastResult result = ScenePicker::Raycast(activeScene, ray);
				if (result.Hit)
				{
					bestHit = Entity(result.Entity, &activeScene->GetRegistry());
				}
			}

			if (bestHit)
			{
				SelectEntity(bestHit, activeScene);
			}
			else
			{
				if (mouseInViewport)
				{
					DeselectEntity(activeScene);
				}
			}
		}
	}

	void ViewportPanel::RenderToolbar(Scene* activeScene, const ImVec2& viewportScreenPos)
	{
		SceneState sceneState = EditorLayer::Get().GetSceneManager().GetSceneState();
		if (sceneState == SceneState::Play || sceneState == SceneState::Simulate)
		{
			// In Play/Simulate mode, Playback controls are in the Main Menu Bar at the top.
			// Viewport canvas stays completely clean for game rendering & HUD scripts.
			return;
		}

		ImVec2 toolbarPos = {viewportScreenPos.x + 10.0f, viewportScreenPos.y + 10.0f};
		ImGui::SetNextWindowPos(toolbarPos);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, EditorColors::ToolbarBg);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

		if (ImGui::BeginChild("##FloatingToolbar", ImVec2(750, 40), true,
							  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			ImGui::SetCursorPosY(6); // Center align vertically-ish
			ImGui::Indent(5);

			DrawGizmoButtons();

			ImGui::SameLine(0, 10);
			bool is2D = m_CameraController->Is2DMode();
			bool isUIScene = activeScene && activeScene->GetSettings().Type == SceneType::UI;
			if (is2D)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, EditorColors::ActiveToolOrange);
			}
			if (!isUIScene)
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::Button(is2D ? (ICON_FA_CAMERA " 2D") : (ICON_FA_CUBE " 3D"), {50, 28}))
			{
				m_CameraController->Set2DMode(!is2D);
				m_Gizmo.Set2DMode(!is2D);
			}
			if (!isUIScene)
			{
				ImGui::EndDisabled();
			}
			if (is2D)
			{
				ImGui::PopStyleColor();
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(isUIScene ? "Toggle 2D/3D Editor Mode" : "2D mode available for UI scenes only");
			}

			ImGui::SameLine(0, 10);
			DrawCameraSelector(activeScene);

			ImGui::SameLine(0, 10);
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine(0, 10);

			DrawSnapSection();
			DrawTransformSpaceToggle();

			ImGui::SameLine(0, 15);
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine(0, 15);

			DrawScriptReloadButton();
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
		auto scene = EditorLayer::Get().GetActiveScene();
		float gridSize = scene->GetSettings().Grid.Spacing;
		ImGui::SetNextItemWidth(60);
		if (ImGui::DragFloat("##SnapValue", &gridSize, 0.1f, 0.1f, 50.0f, "%.1f"))
		{
			scene->GetSettings().Grid.Spacing = gridSize;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Grid Snap Size (synced with visual grid)");
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
				if (auto* scriptEngine = ServiceLocator::TryGet<ScriptEngine>())
				{
					scriptEngine->RequestAssemblyReload(assemblyPath.string(), "ViewportPanel");
				}
			}
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Reload Scripts (Ctrl+R)");
		}
	}

	void ViewportPanel::RenderLightIcons(entt::registry& registry, const Camera3D& camera, float iconMin, float iconMax,
										 float iconScale)
	{
		const glm::vec3 activeCameraPos = camera.Position;

		auto lightView = registry.view<TransformComponent, LightComponent>();
		for (auto entity : lightView)
		{
			auto [transform, light] = lightView.get<TransformComponent, LightComponent>(entity);
			const glm::vec3 iconPos = glm::vec3(transform.WorldTransform[3]);
			const float iconSize = ComputeIconSize(iconPos, activeCameraPos, iconMin, iconMax, iconScale);

			glm::vec4 lightTint = {light.LightColor.r / 255.0f, light.LightColor.g / 255.0f,
								   light.LightColor.b / 255.0f, 0.95f};

			uint32_t iconTextureId = GetIconHandle(m_EditorIcons.LightIcon);
			DrawBillboardIcon(camera, iconTextureId, iconPos, iconSize, lightTint);

			if (iconTextureId != 0 && light.Type == LightType::Directional)
			{
				glm::vec3 dir = glm::normalize(glm::vec3(transform.WorldTransform[2])) * 0.45f;
				if (auto* debugRenderer = ServiceLocator::TryGet<DebugRenderer>())
				{
					debugRenderer->DrawLine(iconPos, iconPos + dir, lightTint);
				}
			}
			else if (iconTextureId == 0 && light.Type == LightType::Directional)
			{
				glm::vec3 dir = glm::normalize(glm::vec3(transform.WorldTransform[2])) * 0.5f;
				if (auto* debugRenderer = ServiceLocator::TryGet<DebugRenderer>())
				{
					debugRenderer->DrawLine(iconPos, iconPos + dir, lightTint);
				}
			}
		}
	}

	void ViewportPanel::DrawBillboardIcon(const Camera3D& camera, uint32_t textureId, const glm::vec3& worldPos,
										  float iconSize, const glm::vec4& tint)
	{
		if (textureId != 0)
		{
			if (auto* renderer = ServiceLocator::TryGet<Renderer>())
			{
				renderer->DrawBillboard(camera, textureId, worldPos, iconSize, tint);
			}
		}
	}

	void ViewportPanel::RenderEditorIcons(entt::registry& registry, const Camera3D& camera)
	{
		const glm::vec3 activeCameraPos = camera.Position;

		auto tryLoadIcon = [&](const char* path, std::shared_ptr<TextureAsset>& cachedIcon) {
			if (cachedIcon)
			{
				return;
			}

			if (auto* assetManager = ServiceLocator::TryGet<AssetManager>())
			{
				cachedIcon = assetManager->Load<TextureAsset>(path);
			}
		};

		tryLoadIcon("engine/resources/icons/camera_icon.png", m_EditorIcons.CameraIcon);
		tryLoadIcon("engine/resources/icons/light_bulb.png", m_EditorIcons.LightIcon);
		tryLoadIcon("engine/resources/icons/leaf_icon.png", m_EditorIcons.SpawnIcon);
		tryLoadIcon("engine/resources/icons/audio.png", m_EditorIcons.AudioIcon);

		// Gizmo icon sizing comes from the global editor settings (Editor Settings > Appearance).
		const auto& editorCfg = EditorLayer::Get().GetConfig();
		const float iconMin = editorCfg.IconSizeMin;
		const float iconMax = editorCfg.IconSizeMax;
		const float iconScale = editorCfg.IconSizeScale;

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

			const float iconSize = ComputeIconSize(iconPos, activeCameraPos, iconMin, iconMax, iconScale);
			const glm::vec4 cameraTint = glm::vec4(0.65f, 0.95f, 1.0f, 0.95f);
			DrawBillboardIcon(camera, GetIconHandle(m_EditorIcons.CameraIcon), iconPos, iconSize, cameraTint);
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
				const float iconSize = ComputeIconSize(iconPos, activeCameraPos, iconMin, iconMax, iconScale);
				const glm::vec4 spawnTint = {1.0f, 1.0f, 1.0f, 0.95f};
				DrawBillboardIcon(camera, GetIconHandle(m_EditorIcons.SpawnIcon), iconPos, iconSize, spawnTint);
			}
		}
		{
			auto audioView = registry.view<TransformComponent, AudioComponent>();
			for (auto entity : audioView)
			{
				auto [transform, audio] = audioView.get<TransformComponent, AudioComponent>(entity);
				const glm::vec3 iconPos = glm::vec3(transform.WorldTransform[3]);
				const float iconSize = ComputeIconSize(iconPos, activeCameraPos, iconMin, iconMax, iconScale);
				const glm::vec4 audioTint = {1.0f, 1.0f, 1.0f, 0.95f};
				DrawBillboardIcon(camera, GetIconHandle(m_EditorIcons.AudioIcon), iconPos, iconSize, audioTint);
			}
		}
	}
} // namespace Chained
