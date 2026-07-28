#include "audio_system.h"
#include "engine/audio/audio.h"
#include "engine/core/profiler.h"
#include "engine/core/service_locator.h"
#include "engine/scene/components/audio_component.h"
#include "engine/scene/components/camera_component.h"
#include "engine/scene/components/transform_component.h"

namespace Chained::AudioSystem
{

void Update(entt::registry& reg, Timestep ts)
{
    CH_PROFILE_FUNCTION();

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
            glm::vec3 up = rot * glm::vec3(1, 0, 0);

            if (auto* audioSvc = ServiceLocator::TryGet<Audio>())
                audioSvc->SetListenerPosition(pos, forward, up);
            break;
        }
    }

    auto audioView = reg.view<AudioComponent, TransformComponent>();
    auto* audioSvc = ServiceLocator::TryGet<Audio>();
    if (!audioSvc) return;
    for (auto entity : audioView)
    {
        auto& audio = audioView.get<AudioComponent>(entity);

        if (!audio.SoundPath.empty())
        {
            if (audio.SoundHandle == 0 || !audioSvc->IsSoundLoaded(audio.SoundHandle))
            {
                CH_CORE_INFO("AudioComponent: Loading sound: {}", audio.SoundPath);
                audio.SoundHandle = audioSvc->LoadSound(audio.SoundPath);
            }
        }

        if (audio.PlayOnStart && !audio.IsPlaying && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

            audioSvc->Play(audio.SoundHandle, audio.Volume, audio.Pitch, audio.Loop,
                           audio.Spatialized, worldPos);
            audio.IsPlaying = true;
        }
        else if (audio.IsPlaying && audio.Spatialized && audio.SoundHandle != 0)
        {
            auto& transform = audioView.get<TransformComponent>(entity);
            glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);
            audioSvc->SetInstancePosition(audio.SoundHandle, worldPos);
        }
    }
}

void OnRuntimeStart(entt::registry& reg)
{
    CH_PROFILE_FUNCTION();

    auto view = reg.view<AudioComponent>();
    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);
        audio.IsPlaying = false;
    }
}

void OnRuntimeStop(entt::registry& reg)
{
    CH_PROFILE_FUNCTION();

    if (auto* audioSvc = ServiceLocator::TryGet<Audio>())
    {
        audioSvc->StopAll();
    }

    auto view = reg.view<AudioComponent>();
    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);
        audio.IsPlaying = false;
    }
}

} // namespace Chained::AudioSystem
