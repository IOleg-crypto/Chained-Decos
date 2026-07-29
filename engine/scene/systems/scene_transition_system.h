#ifndef CH_SCENE_TRANSITION_SYSTEM_H
#define CH_SCENE_TRANSITION_SYSTEM_H

#include <entt/entt.hpp>
#include <functional>

namespace Chained
{
class Event;

namespace SceneTransitionSystem
{
using EventCallbackFn = std::function<void(Event&)>;
void Update(entt::registry& reg, const EventCallbackFn& callback);
} // namespace SceneTransitionSystem
} // namespace Chained

#endif // CH_SCENE_TRANSITION_SYSTEM_H
