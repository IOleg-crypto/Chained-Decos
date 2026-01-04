#ifndef CH_AUDIO_MANAGER_H
#define CH_AUDIO_MANAGER_H

#include <raylib.h>
#include <string>
#include <unordered_map>


namespace CH
{
class AudioManager
{
public:
    static void Init();
    static void Shutdown();

    static void LoadSound(const std::string &name, const std::string &path);
    static void PlaySound(const std::string &name, float volume = 1.0f, float pitch = 1.0f);
    static void StopSound(const std::string &name);

    static void SetMasterVolume(float volume);

private:
    static std::unordered_map<std::string, Sound> s_Sounds;
};
} // namespace CH

#endif // CH_AUDIO_MANAGER_H
