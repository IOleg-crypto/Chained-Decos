// Gameplay & Audio component registrations (Audio, Animation, SceneTransition)
// Split into its own TU to reduce obj file size in MinGW Clang Debug builds.
#include "component_registry.h"
#include "components/animation_component.h"
#include "components/player_component.h"
#include "components/audio_component.h"
#include "components/scene_transition_component.h"
#include "components/network_identity_component.h"
#include "thirdparty/IconsFontAwesome6.h"

namespace Chained
{
	void RegisterGameplayComponents()
	{
		ComponentRegistry::RegisterReflective<AudioComponent>("Audio", ICON_FA_VOLUME_HIGH, "Audio");
		ComponentRegistry::RegisterReflective<PlayerComponent>("Player", ICON_FA_USER, "Gameplay");
		ComponentRegistry::RegisterReflective<AnimationComponent>("Animation", ICON_FA_FILM, "Animation");
		ComponentRegistry::RegisterReflective<SceneTransitionComponent>("Scene Transition", ICON_FA_DOOR_OPEN,
																		"Gameplay");
		ComponentRegistry::RegisterReflective<NetworkIdentityComponent>("Network Identity", ICON_FA_LINK, "Networking");
	}
} // namespace Chained
