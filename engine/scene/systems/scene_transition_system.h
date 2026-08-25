#ifndef CH_SCENE_TRANSITION_SYSTEM_H
#define CH_SCENE_TRANSITION_SYSTEM_H

#include <entt/entt.hpp>
#include <optional>
#include <string>

namespace Chained
{
	namespace SceneTransitionSystem
	{
		std::optional<std::string> Update(entt::registry& reg);
	} // namespace SceneTransitionSystem
} // namespace Chained

#endif // CH_SCENE_TRANSITION_SYSTEM_H
