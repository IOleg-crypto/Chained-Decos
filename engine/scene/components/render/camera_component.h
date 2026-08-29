#ifndef CH_CAMERA_COMPONENT_H
#define CH_CAMERA_COMPONENT_H

#include "engine/scene/camera.h"
#include "engine/reflection/reflection_rfl.h"
#include <glm/glm.hpp>
#include <string>

namespace Chained
{
	struct CameraComponent
	{
		Camera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		// Orbit camera settings
		bool IsOrbitCamera = false;
		std::string TargetEntityTag = "Player";
		float OrbitDistance = 10.0f;
		float OrbitYaw = 0.0f;
		float OrbitPitch = 20.0f;
		float LookSensitivity = 0.9f;

		// Runtime-only: smoothed pivot position for lag-free tracking (not serialized)
		glm::vec3 SmoothedPivot = glm::vec3(0.0f);
		bool PivotInitialized = false;

		static const char* GetStaticName()
		{
			return "CameraComponent";
		}

		struct UI
		{
			UIMeta TargetEntityTag = {.Tooltip = "Tag of the entity to follow (Orbit Camera)"};
			UIMeta OrbitDistance = {
				.Min = 1.0f, .Max = 500.0f, .Speed = 0.5f, .Tooltip = "Distance from the target when orbiting"};
			UIMeta OrbitYaw = {.Speed = 1.0f, .Tooltip = "Horizontal orbit angle (degrees)"};
			UIMeta OrbitPitch = {
				.Min = -89.0f, .Max = 89.0f, .Speed = 1.0f, .Tooltip = "Vertical orbit angle (degrees)"};
			UIMeta LookSensitivity = {
				.Min = 0.1f, .Max = 10.0f, .Speed = 0.05f, .Tooltip = "Mouse look sensitivity multiplier"};
			UIMeta SmoothedPivot = {.Transient = true};
			UIMeta PivotInitialized = {.Transient = true};
		};
	};

	CH_MARK_RFL(CameraComponent);

} // namespace Chained

#endif // CH_CAMERA_COMPONENT_H
