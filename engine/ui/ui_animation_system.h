#ifndef CH_UI_ANIMATION_SYSTEM_H
#define CH_UI_ANIMATION_SYSTEM_H

#include "engine/scene/components/ui/control_component.h"
#include "entt/entt.hpp"

namespace Chained
{

	class UIAnimationSystem
	{
	public:
		void Update(entt::registry& registry, float dt);

	private:
		void UpdateStyle(UIStyle& style, bool isHovered, bool isDown, float dt);
	};

} // namespace Chained

#endif // CH_UI_ANIMATION_SYSTEM_H
