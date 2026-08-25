#ifndef CH_VIEWPORT_PANEL_H
#define CH_VIEWPORT_PANEL_H

#include "panel.h"
#include "engine/common/timestep.h"
#include "engine/physics/raycast_result.h"
#include "engine/scene/scene_settings.h"
#include "viewport/camera.h"
#include "viewport/gizmo.h"
#include "viewport/ui_manipulator.h"
#include "icons.h"

#include <memory>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include "engine/core/key_codes.h"

struct GLFWwindow;

namespace Chained
{

	class Framebuffer;
	class Scene;
	class SceneRenderer;
	class Renderer;
	struct SceneSettings;
	struct Camera3D;
	class Event;

	struct GizmoBtn
	{
		GizmoType type;
		const char* icon;
		const char* tooltip;
		KeyCode key;
	};

	class ViewportPanel : public Panel
	{
	public:
		ViewportPanel(ImVec2& editorViewportSize);
		~ViewportPanel() override;

		void OnImGuiRender(bool readOnly = false) override;
		void OnUpdate(Timestep ts) override;
		void OnEvent(Event& e) override;

		bool IsFocused() const
		{
			return m_Focused;
		}
		bool IsHovered() const
		{
			return m_Hovered;
		}
		glm::vec2 GetSize() const
		{
			return m_ViewportSize;
		}
		GizmoType& GetCurrentTool()
		{
			return m_CurrentTool;
		}

		void DrawGizmoButtons();
		void DrawCameraSelector(Scene* scene);
		Ray GetMouseRay(const glm::vec2& mousePosition);

		std::shared_ptr<Framebuffer> GetViewportFramebuffer() const
		{
			return m_ViewportFramebuffer;
		}

	private:
		glm::vec2 m_ViewportSize = {0, 0};
		bool m_Focused = false;
		bool m_Hovered = false;
		bool m_CursorLocked = false;
		GLFWwindow* m_PlatformWindow = nullptr;
		GLFWwindow* m_LockedWindow = nullptr;
		GizmoType m_CurrentTool = GizmoType::TRANSLATE;

		std::unique_ptr<EditorCameraController> m_CameraController;
		EditorGizmo m_Gizmo;
		EditorUIManipulator m_UIManipulator;
		EditorIcons m_EditorIcons;

		SceneType m_LastSceneType = SceneType::Default;

		std::shared_ptr<Framebuffer> m_ViewportFramebuffer;
		std::shared_ptr<Framebuffer> m_HDRFramebuffer;
		uint32_t m_MSAAFramebufferSamples = 1; // MSAA sample count m_HDRFramebuffer was last (re)created with

		// Engine subsystem pointers are now accessed via static APIs (Renderer::Get(), etc.)
		std::unique_ptr<SceneRenderer> m_SceneRenderer;

		ImVec2& m_EditorViewportSize;

	private:
		void HandleResize(const ImVec2& viewportSize, Scene* activeScene);
		void RenderViewportScene(Scene* activeScene);
		void HandleDragDrop(Scene* activeScene);
		void RenderOverlays(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos);
		void HandlePicking(Scene* activeScene, const ImVec2& viewportSize, const ImVec2& viewportScreenPos);
		void RenderToolbar(Scene* activeScene, const ImVec2& viewportScreenPos);
		void HandleKeyboardShortcuts();

		// Toolbar sub-sections
		void DrawSnapSection();
		void DrawTransformSpaceToggle();
		void DrawScriptReloadButton();

		// Picking helpers
		Entity HandleIconPicking(Scene* scene, const Camera3D& camera, const ImVec2& mousePos,
								 const ImVec2& viewportSize, const ImVec2& viewportScreenPos);

		// Icon rendering helpers
		void DrawBillboardIcon(const Camera3D& camera, uint32_t textureId, const glm::vec3& worldPos, float iconSize,
							   const glm::vec4& tint);
		void RenderEditorIcons(entt::registry& registry, const Camera3D& camera);
		void RenderLightIcons(entt::registry& registry, const Camera3D& camera, float iconMin, float iconMax,
							  float iconScale);
		void ClearSceneBackground(Scene* scene);

		Camera3D GetActiveOrEditorCamera(Scene* scene) const;
	};

} // namespace Chained

#endif // CH_VIEWPORT_PANEL_H