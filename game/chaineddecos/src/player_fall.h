#ifndef CH_PLAYER_FALL_H
#define CH_PLAYER_FALL_H

#include "engine/core/input.h"
#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include "engine/audio/audio.h"
#include "engine/audio/sound_asset.h"
#include "engine/graphics/asset_manager.h"
#include "engine/scene/project.h"
#include "raymath.h"
#include <cmath>

namespace CHEngine
{
    CH_SCRIPT(PlayerFall)
    {
    public:
        CH_UPDATE(deltaTime)
        {
            auto scene = GetScene();
            if (!scene) return;

            if (!HasComponent<RigidBodyComponent>() || !HasComponent<AudioComponent>())
                return;

            auto &rigidBody = GetComponent<RigidBodyComponent>();
            auto &audio = GetComponent<AudioComponent>();

            // Ensure sound asset is loaded (KISS: using AssetManager)
            if (!audio.Asset && !audio.SoundPath.empty())
            {
                if (Project::GetActive())
                {
                    audio.Asset = Project::GetActive()->GetAssetManager()->Get<SoundAsset>(audio.SoundPath);
                    if (audio.Asset) {
                        CH_CORE_INFO("PlayerFall: Loaded sound asset from path: {}", audio.SoundPath);
                    } else {
                        CH_CORE_ERROR("PlayerFall: FAILED to load sound asset from path: {}", audio.SoundPath);
                    }
                }
            }

            if (!audio.Asset) return;

            // Falling logic: play/adjust sound based on vertical velocity
            float fallSpeed = -rigidBody.Velocity.y;
            
            if (fallSpeed > 5.0f && !rigidBody.IsGrounded)
            {
                float targetVolume = Clamp((fallSpeed - 5.0f) / 25.0f, 0.0f, 1.0f);
                audio.Volume = targetVolume;
                audio.Loop = true; // Ensure looping is enabled

                if (!audio.IsPlaying)
                {
                    Audio::Play(audio.Asset, targetVolume, 1.0f, true);
                    audio.IsPlaying = true;
                    CH_CORE_INFO("Player started falling, wind sound ON. Speed: {:.2f}, Vol: {:.2f}", fallSpeed, targetVolume);
                }
            }
            else if (audio.IsPlaying)
            {
                Audio::Stop(audio.Asset);
                audio.IsPlaying = false;
                CH_CORE_INFO("Player stopped falling/landed, wind sound OFF.");
            }
        }

        CH_EVENT(e)
        {
        }
    private:
        // No need for local storage, AudioComponent handles state
    };
} // namespace CHEngine

#endif // CH_PLAYER_FALL_H
