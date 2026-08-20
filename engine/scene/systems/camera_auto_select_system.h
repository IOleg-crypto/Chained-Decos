#ifndef CH_CAMERA_AUTO_SELECT_SYSTEM_H
#define CH_CAMERA_AUTO_SELECT_SYSTEM_H

#include <entt/entt.hpp>

namespace Chained
{
	namespace CameraAutoSelectSystem
	{
		void OnRuntimeStart(entt::registry& reg);
		void OnSceneUpdate(entt::registry& reg);

		bool IsActiveCamera2D(entt::registry& reg);
	} // namespace CameraAutoSelectSystem
} // namespace Chained

#endif // CH_CAMERA_AUTO_SELECT_SYSTEM_H
