#include "scene_audio_system.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/audio/audio.h"
#include "engine/core/profiler.h"
#include <glm/gtx/norm.hpp>

namespace CHEngine
{
void SceneAudioSystem::Update(Scene* scene, Timestep ts)
{
    CH_PROFILE_FUNCTION();
    auto& reg = scene->GetRegistry();

    // 1. Sync Listener with Primary Camera
    auto cameraView = reg.view<CameraComponent, TransformComponent>();
    for (auto entity : cameraView)
    {
        auto& camera = cameraView.get<CameraComponent>(entity);
        if (camera.Primary)
        {
            auto& transform = cameraView.get<TransformComponent>(entity);
            glm::vec3 pos = glm::vec3(transform.WorldTransform[3]);
            glm::mat3 rot = glm::mat3(transform.WorldTransform);
            glm::vec3 forward = rot * glm::vec3(0, 0, -1);
            glm::vec3 up = rot * glm::vec3(0, 1, 0);

            Audio::Get().SetListenerPosition(pos, forward, up);
            break;
        }
    }

    // 2. Manage Audio Components
    auto audioView = reg.view<AudioComponent, TransformComponent>();
    for (auto entity : audioView)
    {
        auto& audio = audioView.get<AudioComponent>(entity);
        if (audio.PlayOnStart && !audio.IsPlaying && !audio.SoundPath.empty())
        {
            if (audio.SoundHandle == 0 || !Audio::Get().IsSoundLoaded(audio.SoundHandle))
            {
                audio.SoundHandle = Audio::Get().LoadSound(audio.SoundPath);
            }
            
            if (audio.SoundHandle != 0)
            {
                auto& transform = audioView.get<TransformComponent>(entity);
                glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

                Audio::Get().Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized,
                                  worldPos);
                audio.IsPlaying = true;
            }
        }
    }
}
} // namespace CHEngine
