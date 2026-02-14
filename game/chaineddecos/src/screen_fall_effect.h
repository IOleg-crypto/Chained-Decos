#ifndef CH_SCREEN_FALL_EFFECT_H
#define CH_SCREEN_FALL_EFFECT_H

#include "engine/scene/components.h"
#include "engine/scene/scriptable_entity.h"
#include "engine/graphics/shader_asset.h"
#include "engine/scene/project.h"
#include "raylib.h"
#include "raymath.h"

namespace CHEngine
{
    CH_SCRIPT(ScreenFallEffect)
    {
    public:
        std::string TargetEntityTag = "Player";
        float SpeedThreshold = 5.0f;
        float MaxSpeed = 30.0f;
        Color EffectColor = { 200, 220, 255, 255 };

        CH_START()
        {
            CH_CORE_INFO("ScreenFallEffect: Started monitoring {}", TargetEntityTag);
        }

        CH_UPDATE(deltaTime)
        {
            if (!m_Shader && Project::GetActive())
            {
                m_Shader = Project::GetActive()->GetAssetManager()->Get<ShaderAsset>("shaders/screen_falling.chshader");
                if (m_Shader)
                {
                    CH_CORE_INFO("ScreenFallEffect: Shader loaded successfully.");
                }
            }

            Entity target = GetScene()->FindEntityByTag(TargetEntityTag);
            if (target && target.HasComponent<RigidBodyComponent>())
            {
                auto& rb = target.GetComponent<RigidBodyComponent>();
                float fallSpeed = -rb.Velocity.y;
                
                if (!rb.IsGrounded && fallSpeed > SpeedThreshold)
                {
                    float targetIntensity = Clamp((fallSpeed - SpeedThreshold) / (MaxSpeed - SpeedThreshold), 0.0f, 1.0f);
                    m_Intensity = Lerp(m_Intensity, targetIntensity, deltaTime * 2.0f);
                }
                else
                {
                    m_Intensity = Lerp(m_Intensity, 0.0f, deltaTime * 5.0f);
                }
            }
            else
            {
                m_Intensity = Lerp(m_Intensity, 0.0f, deltaTime * 5.0f);
            }
        }

        CH_GUI()
        {
            if (m_Intensity > 0.01f && m_Shader && m_Shader->IsReady())
            {
                BeginShaderMode(m_Shader->GetShader());
                
                // Set uniforms
                int intensityLoc = GetShaderLocation(m_Shader->GetShader(), "intensity");
                SetShaderValue(m_Shader->GetShader(), intensityLoc, &m_Intensity, SHADER_UNIFORM_FLOAT);
                
                int timeLoc = GetShaderLocation(m_Shader->GetShader(), "time");
                float time = (float)GetTime();
                SetShaderValue(m_Shader->GetShader(), timeLoc, &time, SHADER_UNIFORM_FLOAT);
                
                int colorLoc = GetShaderLocation(m_Shader->GetShader(), "color");
                Vector3 col = { EffectColor.r / 255.0f, EffectColor.g / 255.0f, EffectColor.b / 255.0f };
                SetShaderValue(m_Shader->GetShader(), colorLoc, &col, SHADER_UNIFORM_VEC3);

                // Draw full screen quad
                // Using DrawRectangleRec to cover the whole screen area
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
                
                EndShaderMode();
            }
        }

    private:
        std::shared_ptr<ShaderAsset> m_Shader;
        float m_Intensity = 0.0f;
    };
}

#endif // CH_SCREEN_FALL_EFFECT_H
