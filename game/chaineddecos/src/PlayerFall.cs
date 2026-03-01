using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class PlayerFall : Script
{
    public override void OnUpdate(float deltaTime)
    {
        if (!Entity.HasComponent<RigidBodyComponent>() || !Entity.HasComponent<AudioComponent>())
            return;

        RigidBodyComponent ?rb = Entity.GetComponent<RigidBodyComponent>();
        AudioComponent ?audio = Entity.GetComponent<AudioComponent>();

        Vector3 velocity = rb.Velocity;
        float fallSpeed = -velocity.Y;

        string soundPath = audio.SoundPath;
        if (string.IsNullOrEmpty(soundPath))
            return;

        if (fallSpeed > 5.0f && !rb.IsGrounded)
        {
            float targetVolume = (fallSpeed - 5.0f) / 25.0f;
            if (targetVolume > 1.01f)
                targetVolume = 1.0f;
            if (targetVolume < 0.0f)
                targetVolume = 0.0f;

            audio.Volume = targetVolume;
            audio.Loop = true;

            if (!audio.IsPlaying)
            {
                Audio.Play(soundPath, targetVolume, 1.0f, true);
                Log.Info("playerfall: Wind sound ON");
            }
        }
        else
        {
            if (audio.IsPlaying)
            {
                Audio.Stop(soundPath);
                Log.Info("playerfall: Wind sound OFF");
                
            }
        }
    }
}
}
