#include "camera_auto_select_system.h"
#include "engine/core/profiler.h"
#include "engine/scene/components/render/camera_component.h"
#include "engine/scene/components/core/tag_component.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_settings.h"

namespace Chained::CameraAutoSelectSystem
{
	static constexpr const char* kCamera2DTag = "MenuCamera2D";
	static constexpr const char* kCamera3DTag = "MenuCamera3D";

	void OnRuntimeStart(entt::registry& reg)
	{
		CH_PROFILE_FUNCTION();

		auto* scenePtr = reg.ctx().find<Scene*>();
		if (!scenePtr || !*scenePtr)
		{
			return;
		}

		Scene* scene = *scenePtr;
		auto& settings = scene->GetSettings();
		bool want2D = (settings.Mode == BackgroundMode::Color || settings.Mode == BackgroundMode::Texture);

		Entity cam2D = scene->FindEntityByTag(kCamera2DTag);
		Entity cam3D = scene->FindEntityByTag(kCamera3DTag);

		if (cam2D && cam2D.HasComponent<CameraComponent>())
		{
			auto& comp = cam2D.GetComponent<CameraComponent>();
			comp.Primary = want2D;
			comp.Camera.SetOrthographic(10.0f, -1.0f, 1.0f);
		}

		if (cam3D && cam3D.HasComponent<CameraComponent>())
		{
			auto& comp = cam3D.GetComponent<CameraComponent>();
			comp.Primary = !want2D;
			comp.Camera.SetPerspective(glm::radians(60.0f), 0.1f, 1000.0f);
		}
	}
} // namespace Chained::CameraAutoSelectSystem
