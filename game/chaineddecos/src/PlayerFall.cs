//========= Copyright Chained Decos, All rights reserved. ============//
//
// Purpose: Handles player falling logic and environmental audio hints.
//
//=============================================================================//
using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class PlayerFall : Script
{
    public override void OnCreate()
    {
        // Reset fall effect at the start of every play session
        m_FallTime = 0.0f;
        m_CurrentIntensity = 0.0f;
        m_LoggedNoShader = false;
        m_LoggedNoCamera = false;

        Entity? camera = Scene.GetMainCamera();
        if (camera == null) camera = Scene.FindEntityByTag("MainCamera");
        if (camera != null)
        {
            ShaderComponent? shader = camera.GetComponent<ShaderComponent>();
            if (shader != null)
                shader.SetFloat("intensity", 0.0f);
        }
    }

    public override void OnUpdate(float deltaTime)
    {
        if (!Entity.HasComponent<RigidBodyComponent>() || !Entity.HasComponent<AudioComponent>())
            return;

        RigidBodyComponent? rb = Entity.GetComponent<RigidBodyComponent>();
        AudioComponent? audio = Entity.GetComponent<AudioComponent>();

        if (rb == null || audio == null)
            return;

        Vector3 velocity = rb.Velocity;
        float fallSpeed = -velocity.Y;

        string soundPath = audio.SoundPath;
        if (string.IsNullOrEmpty(soundPath))
            return;

        // Update screen-space wind shader effect
        Entity? camera = Scene.GetMainCamera();
        if (camera == null) camera = Scene.FindEntityByTag("MainCamera");

        if (camera != null)
        {
            ShaderComponent? shader = camera.GetComponent<ShaderComponent>();
            
            if (shader == null && !m_LoggedNoShader)
            {
                Log.Warn($"[PlayerFall] Main Camera '{camera}' found but has NO ShaderComponent!");
                m_LoggedNoShader = true;
            }
            else if (shader != null)
            {
                // Logic: Only show effect when falling fast, and not grounded
                float speedThreshold = 15.0f; // Even higher threshold
                float maxSpeed = 45.0f;
                float targetIntensity = 0.0f;

                // Accummulate fall time to prevent "immediate" appearance
                if (!rb.IsGrounded && fallSpeed > 10.0f)
                {
                    m_FallTime += deltaTime;
                }
                else
                {
                    m_FallTime = 0.0f;
                }

                // Only start showing after a substantial delay (e.g. 1.0 second of falling)
                float delayThreshold = 1.0f;
                if (m_FallTime > delayThreshold)
                {
                    float factor = Mathf.Clamp((fallSpeed - speedThreshold) / (maxSpeed - speedThreshold), 0.0f, 1.0f);
                    // Fade in over 1 second after hurdle
                    float timeFade = Mathf.Clamp((m_FallTime - delayThreshold), 0.0f, 1.0f);
                    targetIntensity = factor * timeFade;
                }

                // Smoothly interpolate current intensity
                m_CurrentIntensity = Mathf.Lerp(m_CurrentIntensity, targetIntensity, deltaTime * 2.0f);

                shader.SetFloat("intensity", m_CurrentIntensity);
                shader.SetFloat("fallSpeed", fallSpeed);
                m_LoggedNoShader = false;
            }
        }
        else if (!m_LoggedNoCamera)
        {
            Log.Error("[PlayerFall] NO Main Camera found!");
            m_LoggedNoCamera = true;
        }

        bool isWindNeeded = (fallSpeed > 5.0f && !rb.IsGrounded);

        if (isWindNeeded)
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
                if (!m_WindSoundPlaying)
                {
                    Log.Info("playerfall: Wind sound ON");
                    m_WindSoundPlaying = true;
                }
            }
        }
        else
        {
            if (audio.IsPlaying)
            {
                Audio.Stop(soundPath);
            }
            
            if (m_WindSoundPlaying)
            {
                Log.Info("playerfall: Wind sound OFF");
                m_WindSoundPlaying = false;
            }
        }
    }

    private bool m_WindSoundPlaying = false;
    private bool m_LoggedNoShader = false;
    private bool m_LoggedNoCamera = false;
    private float m_FallTime = 0.0f;
    private float m_CurrentIntensity = 0.0f;
}
}
