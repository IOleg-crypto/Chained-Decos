#ifndef SCRIPT_GLUE_AUDIO_H
#define SCRIPT_GLUE_AUDIO_H
#include "script_glue_internal.h"
#include "script_internal_call_registry.h"
#include "engine/scene/component_registry.h"
#include "engine/audio/audio.h"
#include "engine/core/service_locator.h"
#include "engine/project/project.h"

namespace Chained {

    void RegisterGlueAudio();

    // ── Audio ─────────────────────────────────────────────────────────────
    CH_SCRIPT_FUNC void Audio_Play(const char16_t* path, float volume, float pitch, bool loop);

    CH_SCRIPT_FUNC void Audio_Stop(const char16_t* path);

    CH_SCRIPT_FUNC void Audio_StopAll();

    CH_SCRIPT_FUNC void AudioComponent_SetVolume(uint64_t entityID, float volume);

    CH_SCRIPT_FUNC void AudioComponent_SetLoop(uint64_t entityID, bool loop);

    CH_SCRIPT_FUNC bool AudioComponent_IsPlaying(uint64_t entityID);

    CH_SCRIPT_FUNC const char16_t* AudioComponent_GetSoundPath(uint64_t entityID);

    // --- SpriteComponent ---
    CH_SCRIPT_FUNC const char16_t* SpriteComponent_GetTexturePath(uint64_t entityID);

    CH_SCRIPT_FUNC void SpriteComponent_SetTexturePath(uint64_t entityID, const char16_t* path);

    CH_SCRIPT_FUNC void SpriteComponent_GetTint(uint64_t entityID, glm::vec4* outTint);

    CH_SCRIPT_FUNC void SpriteComponent_SetTint(uint64_t entityID, glm::vec4 tint);

    CH_SCRIPT_FUNC bool SpriteComponent_GetFlipX(uint64_t entityID);

    CH_SCRIPT_FUNC void SpriteComponent_SetFlipX(uint64_t entityID, bool flip);

    CH_SCRIPT_FUNC bool SpriteComponent_GetFlipY(uint64_t entityID);

    CH_SCRIPT_FUNC void SpriteComponent_SetFlipY(uint64_t entityID, bool flip);

    CH_SCRIPT_FUNC int SpriteComponent_GetZOrder(uint64_t entityID);

    CH_SCRIPT_FUNC void SpriteComponent_SetZOrder(uint64_t entityID, int z);

} // namespace Chained
#endif
