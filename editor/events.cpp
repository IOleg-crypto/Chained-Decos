#include "events.h"
#include "engine/app/application.h"
#include "engine/scene/scene_events.h"

namespace Chained
{

	void SelectEntity(Entity entity, Scene* scene)
	{
		EntitySelectedEvent e((entt::entity)entity, scene);
		Application::Get().OnEvent(e);
	}

	void DeselectEntity(Scene* scene)
	{
		EntitySelectedEvent e(entt::null, scene);
		Application::Get().OnEvent(e);
	}

} // namespace Chained
