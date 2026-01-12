#include "project_selector.h"
#include <imgui.h>
#include <rlImGui.h>

namespace CHEngine
{
namespace UI
{

void DrawProjectSelector(bool active, Texture2D icon, std::function<void()> onNew,
                         std::function<void()> onOpen, std::function<void()> onExit)
{
    if (active)
        return;

    ImGui::OpenPopup("Project Selector");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 300));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    if (ImGui::BeginPopupModal("Project Selector", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoTitleBar))
    {
        // Centralized Icon
        if (icon.id > 0)
        {
            float iconSize = 80.0f;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - iconSize) * 0.5f);
            rlImGuiImageSize(&icon, (int)iconSize, (int)iconSize);
            ImGui::Spacing();
        }

        ImGui::SetCursorPosX(
            (ImGui::GetWindowSize().x - ImGui::CalcTextSize("Chained Decos Editor").x) * 0.5f);
        ImGui::Text("Chained Decos Editor");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Open Project", ImVec2(360, 45)))
        {
            if (onOpen)
                onOpen();
        }
        ImGui::Spacing();
        if (ImGui::Button("New Project", ImVec2(360, 45)))
        {
            if (onNew)
                onNew();
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Exit", ImVec2(360, 40)))
        {
            if (onExit)
                onExit();
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

} // namespace UI
} // namespace CHEngine
