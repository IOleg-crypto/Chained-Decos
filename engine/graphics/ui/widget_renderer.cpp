// widget_renderer.cpp
// Chained Engine — ImGui-based UI rendering for in-game widgets and HUD.
// Draws WidgetComponent hierarchy, handles Z-order sorting, scissor clipping, and font management.

#include "widget_renderer.h"
#include "ui_font_registry.h"
#include "engine/core/service_locator.h"
#include <algorithm>

#include "engine/scene/components/core/hierarchy_component.h"
#include "engine/scene/components/core/tag_component.h"
#include "engine/scene/scene.h"
#include "ui_control_renderer.h"

namespace Chained
{

	void WidgetRenderer::Initialize()
	{
		CH_CORE_INFO("[UI] Initializing UI Renderer...");
	}

	void WidgetRenderer::Shutdown()
	{
	}

	UIFontRegistry& WidgetRenderer::GetFontRegistry()
	{
		return *ServiceLocator::TryGet<UIFontRegistry>();
	}

	void WidgetRenderer::LoadProjectFonts()
	{
		auto& fontRegistry = GetFontRegistry();
		fontRegistry.LoadProjectFonts();
		fontRegistry.EnsureDefaultProjectFont(18.0f, true);
	}

	UIRect WidgetRenderer::GetEntityRect(Entity entity)
	{
		return m_LayoutSystem.GetEntityRect((entt::entity)entity);
	}

	void WidgetRenderer::ResetButtonStates(Scene* scene)
	{
		if (!scene)
		{
			return;
		}

		auto& registry = scene->GetRegistry();
		auto view = registry.view<UIControlComponent>();
		for (entt::entity id : view)
		{
			view.get<UIControlComponent>(id).PressedThisFrame = false;
		}
	}

	std::vector<entt::entity> WidgetRenderer::SortUIEntities(entt::registry& registry)
	{
		auto view = registry.view<ControlComponent>();
		std::vector<entt::entity> sorted(view.begin(), view.end());

		// Sort by Z-order for correct back-to-front rendering sequence
		std::sort(sorted.begin(), sorted.end(), [&](entt::entity a, entt::entity b) {
			return view.get<ControlComponent>(a).ZOrder < view.get<ControlComponent>(b).ZOrder;
		});

		return sorted;
	}

	void WidgetRenderer::ProcessInput(Scene* scene, bool suppressInput)
	{
		if (!scene)
		{
			return;
		}

		auto& registry = scene->GetRegistry();

		if (!m_HasCanvasRect)
		{
			UpdateUIInput(registry, m_LayoutSystem, /*suppress=*/true);
			return;
		}

		ImVec2 refSize = (m_CanvasSize.x > 0) ? m_CanvasSize : ImGui::GetIO().DisplaySize;
		m_LayoutSystem.Update(scene, refSize, m_CanvasPos);
		UpdateUIInput(registry, m_LayoutSystem, suppressInput);
	}

	void WidgetRenderer::DrawCanvas(Scene* scene, const ImVec2& referencePosition, const ImVec2& referenceSize,
									bool editMode)
	{
		CH_CORE_ASSERT(scene, "Scene is null!");

		ImVec2 refSize = (referenceSize.x > 0) ? referenceSize : ImGui::GetIO().DisplaySize;
		if (refSize.x <= 0 || refSize.y <= 0)
		{
			return;
		}

		auto& registry = scene->GetRegistry();

		// Cache canvas geometry so next frame's ProcessInput can hit-test without
		// depending on this render path executing.
		m_CanvasPos = referencePosition;
		m_CanvasSize = refSize;
		m_HasCanvasRect = true;

		// 1. Update layout, then render (input is handled separately in ProcessInput).
		m_LayoutSystem.Update(scene, refSize, referencePosition);
		m_AnimationSystem.Update(registry, ImGui::GetIO().DeltaTime);

		// 2. Render UI elements
		auto uiEntities = SortUIEntities(registry);

		ImVec2 canvasClipMin = referencePosition;
		ImVec2 canvasClipMax = {referencePosition.x + refSize.x, referencePosition.y + refSize.y};
		ImGui::PushClipRect(canvasClipMin, canvasClipMax, true);

		auto& fontRegistry = GetFontRegistry();

		for (entt::entity id : uiEntities)
		{
			Entity entity(id, scene->GetRegistryPtr());

			auto& control = registry.get<ControlComponent>(id);
			if (!control.IsActive)
			{
				continue;
			}

			UIRect rect = m_LayoutSystem.GetEntityRect(id);
			ImVec2 screenPos = {rect.x, rect.y};
			ImVec2 size = {rect.width, rect.height};

			bool needsClipPop = false;

			// Safely retrieve parent hierarchy for scissor clipping
			if (entity.HasComponent<HierarchyComponent>())
			{
				auto& hierarchy = entity.GetComponent<HierarchyComponent>();
				auto parentID = hierarchy.Parent;

				if (parentID != entt::null && registry.valid(parentID))
				{
					UIRect parentRect = m_LayoutSystem.GetEntityRect(parentID);
					if (parentRect.width > 1.0f && parentRect.height > 1.0f)
					{
						ImGui::PushClipRect({parentRect.x, parentRect.y},
											{parentRect.x + parentRect.width, parentRect.y + parentRect.height}, true);
						needsClipPop = true;
					}
				}
			}

			if (size.x > 0.0f && size.y > 0.0f)
			{
				ImGui::SetCursorScreenPos(screenPos);
				ImGui::BeginGroup();
				ImGui::PushID((int)id);

				// Render the control component
				if (entity.HasComponent<UIControlComponent>())
				{
					auto& widget = entity.GetComponent<UIControlComponent>();
					if (!RenderControl(fontRegistry, widget, screenPos, size))
					{
						if (std::holds_alternative<std::monostate>(widget.Data))
						{
							CH_CORE_WARN("[UI] Entity '{}' has UIControlComponent with empty ControlData",
										 entity.GetComponent<TagComponent>().Tag);
						}
					}
				}

				// In edit mode, overlay with an invisible button to prevent game logic from firing
				if (editMode)
				{
					ImGui::SetCursorScreenPos(screenPos);
					ImGui::InvisibleButton("##DragZone", size);
				}

				ImGui::PopID();
				ImGui::EndGroup();
			}

			if (needsClipPop)
			{
				ImGui::PopClipRect();
			}
		}

		ImGui::PopClipRect();
	}
} // namespace Chained
