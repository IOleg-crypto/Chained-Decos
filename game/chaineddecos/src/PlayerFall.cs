//========= Copyright Chained Decos, All rights reserved. ============//
using System;
using CHEngine;

namespace ChainedDecos.Scripts
{
public class PlayerFall : Script
{
    private Entity? m_Camera;
    private float m_Intensity = 0.0f;
    private float m_LogTimer = 0.0f;

    // Constants for the "natural" feel
    private const float LIGHT_FALL_THRESHOLD = 5.0f;  // Wind feeling starts here
    private const float HEAVY_FALL_THRESHOLD = 15.0f; // Turbulence starts here
    private const float MAX_SHAKE_SPEED = 40.0f;      // Max intensity reached here

    public override void OnCreate()
    {
        Log.Info("[PlayerFall] OnCreate: Initializing natural fall effect.");
        m_Intensity = 0.0f;

        // Reset shaders state
        ulong[] shaderEntities = Entity.FindAllWithComponent<ShaderComponent>();
        foreach (ulong id in shaderEntities)
        {
            Entity e = new Entity(id);
            var shader = e.GetComponent<ShaderComponent>();
            if (shader != null)
            {
                shader.Enabled = false;
                shader.SetFloat("uIntensity", 0.0f);
            }
        }

        m_Camera = Scene.GetMainCamera();
    }

    public override void OnUpdate(float deltaTime)
    {
        RigidBodyComponent? rb = Entity.GetComponent<RigidBodyComponent>();
        if (rb == null) return;

        float fallSpeed = -rb.Velocity.Y;
        bool isGrounded = rb.IsGrounded;

        float targetIntensity = 0.0f;
        
        if (!isGrounded && fallSpeed > 2.0f) // Threshold for at least some descent
        {
            // 1. Light wind shake (0.0 to 0.15 range)
            float lightWind = Mathf.Clamp((fallSpeed - 2.0f) / 10.0f, 0.0f, 0.15f);
            
            // 2. Heavy turbulence (only above heavy threshold)
            float heavyTurbulence = 0.0f;
            if (fallSpeed > HEAVY_FALL_THRESHOLD)
            {
                heavyTurbulence = Mathf.Clamp((fallSpeed - HEAVY_FALL_THRESHOLD) / (MAX_SHAKE_SPEED - HEAVY_FALL_THRESHOLD), 0.0f, 0.85f);
            }
            
            targetIntensity = lightWind + heavyTurbulence;
        }

        // Smooth transition
        // Ground landing removes shake faster (lerp speed 15)
        // airborne entry/exit is smoother (lerp speed 4)
        float lerpSpeed = isGrounded ? 15.0f : 4.0f;
        m_Intensity = Mathf.Lerp(m_Intensity, targetIntensity, deltaTime * lerpSpeed);
        
        // Snap to zero when very low and grounded
        if (isGrounded && m_Intensity < 0.005f) m_Intensity = 0.0f;

        if (m_Camera == null) m_Camera = Scene.GetMainCamera();
        if (m_Camera != null)
        {
            var shader = m_Camera.GetComponent<ShaderComponent>();
            if (shader != null)
            {
                bool shouldEnable = m_Intensity > 0.001f;
                if (shader.Enabled != shouldEnable)
                {
                    shader.Enabled = shouldEnable;
                }

                if (shouldEnable)
                {
                    shader.SetFloat("uIntensity", m_Intensity);
                    
                    m_LogTimer += deltaTime;
                    if (m_LogTimer > 1.0f)
                    {
                        Log.Info($"[PlayerFall] State: {(isGrounded ? "Grounded" : "Airborne")}, Speed={fallSpeed:F1}, Intensity={m_Intensity:F2}");
                        m_LogTimer = 0.0f;
                    }
                }
            }
        }
    }
}
}
