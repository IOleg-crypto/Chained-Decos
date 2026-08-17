#ifndef CH_NAMETAG_SYSTEM_H
#define CH_NAMETAG_SYSTEM_H

#include "engine/graphics/camera_types.h"
#include <entt/entt.hpp>

namespace Chained
{
	class Scene;
	class Renderer;

	namespace NametagSystem
	{
		void DrawNametags(entt::registry& registry, Renderer* renderer, const Camera3D& camera);
		void DrawNametags(Scene* scene, Renderer* renderer, const Camera3D& camera);
		void Shutdown();
	} // namespace NametagSystem
} // namespace Chained
#endif
