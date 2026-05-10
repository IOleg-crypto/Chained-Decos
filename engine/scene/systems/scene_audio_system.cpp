#include "scene_audio_system.h"
#include "engine/scene/scene.h"
#include "engine/scene/components.h"
#include "engine/audio/audio.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include <glm/gtx/norm.hpp>

namespace CHEngine
{
void SceneAudioSystem::Update(Scene* scene, Timestep ts)
{
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

            ServiceLocator::Get<Audio>().SetListenerPosition(pos, forward, up);
            break;
        }
    }

    // 2. Manage Audio Components
    auto audioView = reg.view<AudioComponent, TransformComponent>();
    for (auto entity : audioView)
    {
        auto& audio = audioView.get<AudioComponent>(entity);
        
        // 1. Ensure the audio is loaded regardless of PlayOnStart so C# scripts can play it manually.
        if (!audio.SoundPath.empty() && (audio.SoundHandle == 0 || !ServiceLocator::Get<Audio>().IsSoundLoaded(audio.SoundHandle)))
        {
            audio.SoundHandle = ServiceLocator::Get<Audio>().LoadSound(audio.SoundPath);
        }

        // 2. Autoplay if requested
        if (audio.PlayOnStart && !audio.IsPlaying && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

            ServiceLocator::Get<Audio>().Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized,
                                worldPos);
            audio.IsPlaying = true;
        }
        // 2. Track Audio Positions constantly if playing and spatialized
        else if (audio.IsPlaying && audio.Spatialized && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);
            ServiceLocator::Get<Audio>().SetInstancePosition(audio.SoundHandle, worldPos);
        }
    }
}

void SceneAudioSystem::OnRuntimeStop(Scene* scene)
{
    auto& reg = scene->GetRegistry();

    // 1. Terminate all active sounds in the engine
    ServiceLocator::Get<Audio>().StopAll();

    // 2. Reset IsPlaying status for all components so they can re-trigger in next Play Mode
    auto view = reg.view<AudioComponent>();
    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);
        audio.IsPlaying = false;
    }
}
} // namespace CHEngine
