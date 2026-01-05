#include "project_browser_panel.h"
#include "nfd.h"
#include <filesystem>
#include <imgui.h>

namespace CH
{
ProjectBrowserPanel::ProjectBrowserPanel()
{
    std::string cwd = std::filesystem::current_path().string();
    strncpy(m_ProjectLocationBuffer, cwd.c_str(), sizeof(m_ProjectLocationBuffer));
}

void ProjectBrowserPanel::OnImGuiRender()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));

    ImGui::Begin("Project Browser", nullptr, windowFlags);

    DrawWelcomeScreen();

    if (m_ShowCreateDialog)
        DrawCreateProjectDialog();

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void ProjectBrowserPanel::DrawWelcomeScreen()
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 center = ImVec2(windowSize.x * 0.5f, windowSize.y * 0.5f);

    ImGui::SetCursorPos(ImVec2(center.x - 150, center.y - 100));
    ImGui::Text("Chained Engine");
    ImGui::SetCursorPos(ImVec2(center.x - 150, center.y - 70));
    ImGui::TextDisabled("Engine Editor");

    ImGui::SetCursorPos(ImVec2(center.x - 100, center.y));
    if (ImGui::Button("Create New Project", ImVec2(200, 40)))
    {
        m_ShowCreateDialog = true;
        ImGui::OpenPopup("Create New Project");
    }

    ImGui::SetCursorPos(ImVec2(center.x - 100, center.y + 50));
    if (ImGui::Button("Open Existing Project", ImVec2(200, 40)))
    {
        nfdfilteritem_t filterItem[1] = {{"Chained Engine Project", "chproject"}};
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

        if (result == NFD_OKAY)
        {
            if (m_OnProjectOpen)
                m_OnProjectOpen(outPath);
            NFD_FreePath(outPath);
        }
    }
}

void ProjectBrowserPanel::DrawCreateProjectDialog()
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 250));

    if (ImGui::BeginPopupModal("Create New Project", &m_ShowCreateDialog,
                               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Text("Project Name:");
        ImGui::InputText("##ProjectName", m_ProjectNameBuffer, sizeof(m_ProjectNameBuffer));

        ImGui::Spacing();
        ImGui::Text("Location:");
        ImGui::InputText("##ProjectLocation", m_ProjectLocationBuffer,
                         sizeof(m_ProjectLocationBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Browse..."))
        {
            nfdchar_t *outPath = nullptr;
            nfdresult_t result = NFD_PickFolder(&outPath, nullptr);

            if (result == NFD_OKAY)
            {
                strncpy(m_ProjectLocationBuffer, outPath, sizeof(m_ProjectLocationBuffer));
                NFD_FreePath(outPath);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            if (m_OnProjectCreate)
                m_OnProjectCreate(m_ProjectNameBuffer, m_ProjectLocationBuffer);
            m_ShowCreateDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            m_ShowCreateDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
} // namespace CH
