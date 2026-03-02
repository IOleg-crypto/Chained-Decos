#ifndef CH_AUDIO_H
#define CH_AUDIO_H

#include "engine/core/base.h"
#include "engine/core/timestep.h"
#include "raylib.h"
#include <string>
#include <unordered_map>

namespace CHEngine
{
class Scene;

// Stateless action class for global audio management.
class Audio
{
public:
    Audio();
    ~Audio();

    // Initializes the audio backend.
    static void Init();

    // Shuts down the audio backend.
    static void Shutdown();

    // Updates all active audio sources in the scene.
    void Update(Scene* scene, Timestep ts);

    // Plays a sound by its path.
    void Play(const std::string& path, float volume = 1.0f, float pitch = 1.0f, bool loop = false);

    // Stops a sound by its path.
    void Stop(const std::string& path);

    static Audio& Get();

private:
    Sound GetOrLoadSound(const std::string& path);
    static Audio* s_Instance;
    std::unordered_map<std::string, Sound> m_Sounds;
};
} // namespace CHEngine

#endif // CH_AUDIO_H
