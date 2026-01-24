#include "scene_audio.h"
#include "components.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/profiler.h"
#include "scene.h"

namespace CHEngine
{
void SceneAudio::OnUpdate(float deltaTime)
{
    CH_PROFILE_FUNCTION();
    auto &registry = m_Scene->GetRegistry();

    registry.view<AudioComponent>().each(
        [](auto entity, auto &audio)
        {
            if (audio.Asset && audio.PlayOnStart)
            {
                Sound &sound = audio.Asset->GetSound();
                SetSoundVolume(sound, audio.Volume);
                SetSoundPitch(sound, audio.Pitch);
                ::PlaySound(sound);

                audio.PlayOnStart = false;
            }
        });
}
} // namespace CHEngine
