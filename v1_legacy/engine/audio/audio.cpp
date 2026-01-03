#include "audio.h"
#include "components/audio/core/audio_manager.h"
#include <memory>

namespace CHEngine
{
static std::unique_ptr<AudioManager> s_AudioManager = nullptr;

void Audio::Init()
{
    s_AudioManager = std::make_unique<AudioManager>();
    s_AudioManager->Initialize();
}

bool Audio::IsInitialized()
{
    return s_AudioManager != nullptr;
}

void Audio::Shutdown()
{
    s_AudioManager->Shutdown();
    s_AudioManager.reset();
}

void Audio::Update(float deltaTime)
{
    s_AudioManager->Update(deltaTime);
}

void Audio::PlaySoundEffect(const std::string &name, float volume, float pitch)
{
    s_AudioManager->PlaySoundEffect(name, volume, pitch);
}

void Audio::PlayLoopingSoundEffect(const std::string &name, float volume, float pitch)
{
    s_AudioManager->PlayLoopingSoundEffect(name, volume, pitch);
}

void Audio::StopLoopingSoundEffect(const std::string &name)
{
    s_AudioManager->StopLoopingSoundEffect(name);
}

void Audio::UpdateLoopingSounds()
{
    s_AudioManager->UpdateLoopingSounds();
}

void Audio::PlayMusic(const std::string &name, float volume)
{
    s_AudioManager->PlayMusic(name, volume);
}

void Audio::StopMusic()
{
    s_AudioManager->StopMusic();
}

void Audio::PauseMusic()
{
    s_AudioManager->PauseMusic();
}

void Audio::ResumeMusic()
{
    s_AudioManager->ResumeMusic();
}

void Audio::SetMasterVolume(float volume)
{
    s_AudioManager->SetMasterVolume(volume);
}

bool Audio::LoadSound(const std::string &name, const std::string &filePath)
{
    return s_AudioManager->LoadSound(name, filePath);
}

void Audio::SetMusicVolume(float volume)
{
    s_AudioManager->SetMusicVolume(volume);
}

void Audio::SetSoundVolume(float volume)
{
    s_AudioManager->SetSoundVolume(volume);
}
} // namespace CHEngine
