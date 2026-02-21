#ifndef CH_UI_EXAMPLE_H
#define CH_UI_EXAMPLE_H

#include "engine/core/application.h"
#include "engine/scene/scriptable_entity.h"
#include "imgui.h"

namespace CHEngine
{
CH_SCRIPT(GameHUD)
{
    float m_Timer = 0.0f;

    CH_START()
    {
        CH_CORE_INFO("GameHUD Initialized!");
    }

    CH_UPDATE(dt)
    {
        m_Timer += dt;
    }

    CH_GUI()
    {
        // 1. Calculate Player Altitude
        float altitude = 0.0f;
        auto* scene = GetScene();
        if (scene)
        {
            Entity player = scene->FindEntityByTag("Player");
            if (player && player.HasComponent<TransformComponent>())
            {
                altitude = player.GetComponent<TransformComponent>().Translation.y;
            }
        }

        // 2. Format Time (Hh Mm Ss)
        int hours = (int)(m_Timer / 3600.0f);
        int minutes = (int)((m_Timer - hours * 3600.0f) / 60.0f);
        int seconds = (int)(m_Timer) % 60;

        // 3. Premium Overlay Styling
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoBackground;

        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("ParkourHUD", nullptr, window_flags);

        // Absolute position
        ImGui::SetWindowPos(ImVec2(30, 30));

        // Display Altitude
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.9f), "%.0fm", altitude);
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Display Time
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.9f), "%dh %dm %ds", hours, minutes, seconds);

        // Shortcut hint (R to Reset)
        if (ImGui::IsKeyPressed(ImGuiKey_R))
        {
            m_Timer = 0.0f;
        }

        ImGui::End();
    }
};
} // namespace CHEngine

#endif // CH_UI_EXAMPLE_H
