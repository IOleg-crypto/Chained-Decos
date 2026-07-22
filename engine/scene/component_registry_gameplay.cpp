// Gameplay & Audio component registrations (Audio, Animation, SceneTransition)
// Split into its own TU to reduce obj file size in MinGW Clang Debug builds.
#include "component_registry.h"
#include "components/animation_component.h"
#include "components/audio_component.h"
#include "components/scene_transition_component.h"
#include "thirdparty/IconsFontAwesome6.h"

namespace Chained
{
void RegisterGameplayComponents()
{
    ComponentRegistry::RegisterReflective<AudioComponent>("Audio", ICON_FA_VOLUME_HIGH, "Audio");
    ComponentRegistry::RegisterReflective<AnimationComponent>("Animation", ICON_FA_FILM, "Animation");
    ComponentRegistry::RegisterReflective<SceneTransitionComponent>("Scene Transition", ICON_FA_DOOR_OPEN, "Gameplay");
}
} // namespace Chained
