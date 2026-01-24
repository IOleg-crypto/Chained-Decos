#ifndef CH_UI_EXAMPLE_H
#define CH_UI_EXAMPLE_H

#include "engine/core/application.h"
#include "engine/scene/scriptable_entity.h"
#include <imgui.h>


namespace CHEmple
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
        // Unity/Godot style simple GUI pass-through
        ImGui::Begin("Parkour HUD", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                         ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::SetWindowPos(ImVec2(20, 20));
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "CH_DECOS RUNTIME");
        ImGui::Separator();

        ImGui::Text("Session Time: %.2f s", m_Timer);

        if (ImGui::Button("Reset Timer"))
            m_Timer = 0.0f;

        if (ImGui::Button("Quit Game"))
            CHEngine::Application::Get().Close();

        ImGui::End();
    }
};
} // namespace CHEmple

#endif // CH_UI_EXAMPLE_H
