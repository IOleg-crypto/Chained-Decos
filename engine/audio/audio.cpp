#include "audio.h"
#include "engine/audio/sound_asset.h"
#include "engine/core/application.h"
#include "engine/core/log.h"
#include "engine/scene/components.h"

#include "miniaudio.h"



namespace CHEngine
{
static Audio* s_Instance = nullptr;

Audio::Audio()
{
    CH_CORE_ASSERT(!s_Instance, "Audio system already exists!");
    s_Instance = this;
}

Audio::~Audio()
{
    InternalShutdown();
    s_Instance = nullptr;
}

Audio& Audio::Get()
{
    CH_CORE_ASSERT(s_Instance, "Audio system not initialized!");
    return *s_Instance;
}

void Audio::Init()
{
    if (!s_Instance)
        s_Instance = new Audio();
    s_Instance->InternalInit();
}

void Audio::InternalInit()
{
    m_Engine = new ma_engine();
    ma_result result = ma_engine_init(NULL, (ma_engine*)m_Engine);
    if (result != MA_SUCCESS)
    {
        CH_CORE_ERROR("Audio System: Failed to initialize miniaudio engine!");
        delete (ma_engine*)m_Engine;
        m_Engine = nullptr;
        return;
    }
    CH_CORE_INFO("Audio System Initialized (miniaudio).");
}

void Audio::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalShutdown();
        delete s_Instance;
        s_Instance = nullptr;
    }
}

void Audio::InternalShutdown()
{
    if (m_Engine)
    {
        ma_engine_uninit((ma_engine*)m_Engine);
        delete (ma_engine*)m_Engine;
        m_Engine = nullptr;
    }
    CH_CORE_INFO("Audio System Shutdown (miniaudio).");
}

void Audio::Update(Scene* scene, Timestep ts)
{
    if (!m_Engine || !scene)
        return;

    // 1. Update Listener (find active camera)
    // In a real engine we'd have a specific listener component or use the main camera
    // For now, let's assume the scene has a way to get the active camera transform
    // (This is a simplified implementation)

    auto view = scene->GetRegistry().view<AudioComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        glm::vec3 worldPos = glm::vec3(transform.WorldTransform[3]);

        if (audio.PlayOnStart && !audio.IsPlaying && audio.Asset && audio.Asset->GetState() == AssetState::Ready)
        {
            Play(audio.Asset, audio.Volume, audio.Pitch, audio.Loop, audio.Spatialized, worldPos);
            audio.IsPlaying = true;
        }

        if (audio.IsPlaying && audio.Spatialized && audio.Asset && audio.Asset->GetState() == AssetState::Ready)
        {
            ma_sound* sound = (ma_sound*)audio.Asset->GetSound().maSound;
            if (sound)
            {
                ma_sound_set_position(sound, worldPos.x, worldPos.y, worldPos.z);
            }
        }
    }
}

void Audio::SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
{
    if (!m_Engine) return;
    ma_engine_listener_set_position((ma_engine*)m_Engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction((ma_engine*)m_Engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up((ma_engine*)m_Engine, 0, up.x, up.y, up.z);
}

void Audio::Play(std::shared_ptr<SoundAsset> asset, float volume, float pitch, bool loop, bool spatial, const glm::vec3& pos)
{
    if (!m_Engine || !asset || asset->GetState() != AssetState::Ready)
        return;

    ma_sound* sound = (ma_sound*)asset->GetSound().maSound;
    if (sound)
    {
        ma_sound_set_volume(sound, volume);
        ma_sound_set_pitch(sound, pitch);
        ma_sound_set_looping(sound, loop ? MA_TRUE : MA_FALSE);
        
        if (spatial)
        {
            ma_sound_set_spatialization_enabled(sound, MA_TRUE);
            ma_sound_set_position(sound, pos.x, pos.y, pos.z);
        }
        else
        {
            ma_sound_set_spatialization_enabled(sound, MA_FALSE);
        }

        if (!ma_sound_is_playing(sound))
        {
            ma_sound_start(sound);
        }
    }
}

void Audio::Stop(std::shared_ptr<SoundAsset> asset)
{
    if (!m_Engine || !asset)
        return;

    ma_sound* sound = (ma_sound*)asset->GetSound().maSound;
    if (sound)
    {
        ma_sound_stop(sound);
        ma_sound_seek_to_pcm_frame(sound, 0); // Reset for next play
    }
}
} // namespace CHEngine

