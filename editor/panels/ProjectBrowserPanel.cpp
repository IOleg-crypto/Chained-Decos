#include "ProjectBrowserPanel.h"
#include "core/Log.h"
#include "editor/logic/EditorProjectActions.h"
#include "nfd.h"
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>


namespace CHEngine
{

ProjectBrowserPanel::ProjectBrowserPanel()
{
    // Set default location to current working directory
    std::string cwd = std::filesystem::current_path().string();
    strncpy(m_ProjectLocationBuffer, cwd.c_str(), sizeof(m_ProjectLocationBuffer));
}

ProjectBrowserPanel::ProjectBrowserPanel(EditorProjectActions *projectActions)
    : m_ProjectActions(projectActions)
{
    std::string cwd = std::filesystem::current_path().string();
    strncpy(m_ProjectLocationBuffer, cwd.c_str(), sizeof(m_ProjectLocationBuffer));
}

void ProjectBrowserPanel::OpenCreateDialog()
{
    m_TriggerOpenCreateDialog = true;
}

const std::vector<std::string> &ProjectBrowserPanel::GetRecentProjects() const
{
    return m_RecentProjects;
}

void ProjectBrowserPanel::OnImGuiRender()
{
    // Fullscreen welcome window
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("##ProjectBrowser", nullptr, windowFlags);

    if (m_TriggerOpenCreateDialog)
    {
        m_ShowCreateDialog = true;
        ImGui::OpenPopup("Create New Project");
        m_TriggerOpenCreateDialog = false;
    }

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

    // Title
    ImGui::SetCursorPos(ImVec2(center.x - 150, center.y - 200));
    ImGui::Text("CHEngine");

    ImGui::SetCursorPos(ImVec2(center.x - 100, center.y - 160));
    ImGui::TextDisabled("Game Engine Project Manager");

    // Buttons
    ImGui::SetCursorPos(ImVec2(center.x - 100, center.y - 80));
    if (ImGui::Button("Create New Project", ImVec2(200, 40)))
    {
        m_ShowCreateDialog = true;
        ImGui::OpenPopup("Create New Project");
    }

    ImGui::SetCursorPos(ImVec2(center.x - 100, center.y - 30));
    if (ImGui::Button("Open Existing Project", ImVec2(200, 40)))
    {
        nfdfilteritem_t filterItem[1] = {{"CHEngine Project", "chproject"}};
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

        if (result == NFD_OKAY)
        {
            if (m_ProjectActions)
                m_ProjectActions->OpenProject(outPath);
            NFD_FreePath(outPath);
        }
    }

    // Recent Projects
    if (!m_RecentProjects.empty())
    {
        ImGui::SetCursorPos(ImVec2(center.x - 200, center.y + 40));
        ImGui::Separator();
        ImGui::SetCursorPos(ImVec2(center.x - 200, center.y + 50));
        ImGui::Text("Recent Projects:");

        DrawRecentProjects();
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

        if (strlen(m_ProjectNameBuffer) == 0 || strlen(m_ProjectLocationBuffer) == 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::Text("Project name and location must be specified!");
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            if (strlen(m_ProjectNameBuffer) > 0 && strlen(m_ProjectLocationBuffer) > 0)
            {
                if (m_ProjectActions)
                    m_ProjectActions->NewProject(m_ProjectNameBuffer, m_ProjectLocationBuffer);
                m_ShowCreateDialog = false;
                ImGui::CloseCurrentPopup();
            }
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

void ProjectBrowserPanel::DrawRecentProjects()
{
    ImVec2 center = ImVec2(ImGui::GetWindowSize().x * 0.5f, ImGui::GetWindowSize().y * 0.5f);

    for (size_t i = 0; i < m_RecentProjects.size() && i < 5; ++i)
    {
        std::filesystem::path projectPath(m_RecentProjects[i]);
        std::string projectName = projectPath.stem().string();

        ImGui::SetCursorPos(ImVec2(center.x - 200, center.y + 70 + i * 30));
        if (ImGui::Selectable(projectName.c_str(), false, 0, ImVec2(400, 25)))
        {
            if (m_ProjectActions)
                m_ProjectActions->OpenProject(m_RecentProjects[i]);
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", m_RecentProjects[i].c_str());
        }
    }
}

void ProjectBrowserPanel::AddRecentProject(const std::string &path)
{
    // Remove if already exists
    auto it = std::find(m_RecentProjects.begin(), m_RecentProjects.end(), path);
    if (it != m_RecentProjects.end())
        m_RecentProjects.erase(it);

    // Add to front
    m_RecentProjects.insert(m_RecentProjects.begin(), path);

    // Keep max 10
    if (m_RecentProjects.size() > 10)
        m_RecentProjects.resize(10);
}

} // namespace CHEngine
