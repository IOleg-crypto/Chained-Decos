#ifndef CH_UI_LAYOUT_SYSTEM_H
#define CH_UI_LAYOUT_SYSTEM_H

#include "ui_types.h"
#include "engine/scene/entity.h"
#include "entt/entt.hpp"
#include "imgui.h"
#include <map>

namespace Chained
{
	class Scene;

	class UILayoutSystem
	{
	public:
		UILayoutSystem() = default;

		void Update(Scene* scene, const ImVec2& viewportSize, const ImVec2& viewportPos);

		UIRect GetEntityRect(entt::entity entity) const;
		float GetScaleFactor() const
		{
			return m_ScaleFactor;
		}
		const UIRect& GetVirtualCanvas() const
		{
			return m_VirtualCanvas;
		}

	private:
		UIRect CalculateRecursive(Entity entity, const UIRect& canvasRect);

		std::map<entt::entity, UIRect> m_RectCache;
		float m_ScaleFactor = 1.0f;
		UIRect m_VirtualCanvas;
	};

} // namespace Chained

#endif // CH_UI_LAYOUT_SYSTEM_H
