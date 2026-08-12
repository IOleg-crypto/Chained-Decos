#include "ui_layout_system.h"
#include "engine/scene/scene.h"
#include "engine/scene/components/core/hierarchy_component.h"
#include "engine/scene/components/core/component_utils.h"
#include "imgui.h"

namespace Chained
{

	void UILayoutSystem::Update(Scene* scene, const ImVec2& viewportSize, const ImVec2& viewportPos)
	{
		m_RectCache.clear();
		if (!scene)
		{
			return;
		}

		const CanvasSettings& canvas = scene->GetSettings().Canvas;
		m_ScaleFactor = 1.0f;

		if (canvas.ScaleMode == CanvasScaleMode::ScaleWithScreenSize && canvas.ReferenceResolution.x > 0 &&
			canvas.ReferenceResolution.y > 0)
		{
			float scaleX = viewportSize.x / canvas.ReferenceResolution.x;
			float scaleY = viewportSize.y / canvas.ReferenceResolution.y;
			m_ScaleFactor = scaleX * (1.0f - canvas.MatchWidthOrHeight) + scaleY * canvas.MatchWidthOrHeight;
		}

		m_VirtualCanvas = {
			0,
			0,
			viewportSize.x / m_ScaleFactor,
			viewportSize.y / m_ScaleFactor,
		};

		auto& registry = scene->GetRegistry();
		auto view = registry.view<ControlComponent>();

		for (auto entityID : view)
		{
			Entity entity(entityID, &registry);
			CalculateRecursive(entity, m_VirtualCanvas);
		}

		// Final pass to apply scale and viewport position
		for (auto& [id, rect] : m_RectCache)
		{
			rect.x = viewportPos.x + rect.x * m_ScaleFactor;
			rect.y = viewportPos.y + rect.y * m_ScaleFactor;
			rect.width *= m_ScaleFactor;
			rect.height *= m_ScaleFactor;
		}
	}

	UIRect UILayoutSystem::GetEntityRect(entt::entity entity) const
	{
		auto it = m_RectCache.find(entity);
		if (it != m_RectCache.end())
		{
			return it->second;
		}
		return {0, 0, 0, 0};
	}

	UIRect UILayoutSystem::CalculateRecursive(Entity entity, const UIRect& canvasRect)
	{
		entt::entity id = (entt::entity)entity;
		if (m_RectCache.count(id))
		{
			return m_RectCache[id];
		}

		// Insert sentinel before recursing to break any parent↔child cycles.
		m_RectCache[id] = {0, 0, canvasRect.width, canvasRect.height};

		auto& control = entity.GetComponent<ControlComponent>();
		UIRect parentRect = {0, 0, canvasRect.width, canvasRect.height};

		if (entity.HasComponent<HierarchyComponent>())
		{
			auto parentID = entity.GetComponent<HierarchyComponent>().Parent;
			if (parentID != entt::null && entity.GetRegistryPtr()->valid(parentID))
			{
				Entity parent(parentID, entity.GetRegistryPtr());
				if (parent.HasComponent<ControlComponent>())
				{
					parentRect = CalculateRecursive(parent, canvasRect);
				}
			}
		}

		// Auto-size logic for labels (moved from WidgetRenderer)
		if (entity.HasComponent<UIControlComponent>())
		{
			auto& widget = entity.GetComponent<UIControlComponent>();
			if (std::holds_alternative<LabelData>(widget.Data))
			{
				auto& label = std::get<LabelData>(widget.Data);
				if (label.AutoSize)
				{
					ImVec2 ts = ImGui::CalcTextSize(label.Text.c_str());
					control.Transform.OffsetMax = {control.Transform.OffsetMin.x + ts.x + 10.0f,
												   control.Transform.OffsetMin.y + ts.y + 4.0f};
				}
			}
		}

		UIRect r = ComponentUtils::CalculateRect(control.Transform, {parentRect.width, parentRect.height},
												 {parentRect.x, parentRect.y});

		m_RectCache[id] = r;
		return r;
	}

} // namespace Chained
