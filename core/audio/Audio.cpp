#include "Audio.h"
#include "components/audio/interfaces/IAudioManager.h"
#include "core/Engine.h"

namespace CHEngine
{

void Audio::PlaySoundEffect(const std::string &name, float volume, float pitch)
{
    Engine::Instance().GetAudioManager().PlaySoundEffect(name, volume, pitch);
}

void Audio::PlayLoopingSoundEffect(const std::string &name, float volume, float pitch)
{
    Engine::Instance().GetAudioManager().PlayLoopingSoundEffect(name, volume, pitch);
}

void Audio::StopLoopingSoundEffect(const std::string &name)
{
    Engine::Instance().GetAudioManager().StopLoopingSoundEffect(name);
}

void Audio::UpdateLoopingSounds()
{
    Engine::Instance().GetAudioManager().UpdateLoopingSounds();
}

void Audio::PlayMusic(const std::string &name, float volume)
{
    Engine::Instance().GetAudioManager().PlayMusic(name, volume);
}

void Audio::StopMusic()
{
    Engine::Instance().GetAudioManager().StopMusic();
}

void Audio::PauseMusic()
{
    Engine::Instance().GetAudioManager().PauseMusic();
}

void Audio::ResumeMusic()
{
    Engine::Instance().GetAudioManager().ResumeMusic();
}

void Audio::SetMasterVolume(float volume)
{
    Engine::Instance().GetAudioManager().SetMasterVolume(volume);
}

bool Audio::LoadSound(const std::string &name, const std::string &filePath)
{
    return Engine::Instance().GetAudioManager().LoadSound(name, filePath);
}

void Audio::SetMusicVolume(float volume)
{
    Engine::Instance().GetAudioManager().SetMusicVolume(volume);
}

void Audio::SetSoundVolume(float volume)
{
    Engine::Instance().GetAudioManager().SetSoundVolume(volume);
}

} // namespace CHEngine
