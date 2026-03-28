#include "project_browser_panel.h"
#include "actions/project_actions.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_events.h"
#include "imgui/IconsFontAwesome6.h"
#include "engine/graphics/importers/texture_importer.h"

#include "imgui.h"
#include "engine/core/dialogs.h"
#include <filesystem>

namespace CHEngine
{
ProjectBrowserPanel::ProjectBrowserPanel()
{
    m_Name = "Project Browser";
    std::string cwd = std::filesystem::current_path().string();
    memset(m_ProjectLocationBuffer, 0, sizeof(m_ProjectLocationBuffer));
    cwd.copy(m_ProjectLocationBuffer, sizeof(m_ProjectLocationBuffer) - 1);

    std::string root = PROJECT_ROOT_DIR; // Use the macro defined in CMake
    m_NewProjectIconAsset = TextureImporter::ImportTexture(root + "/resources/icons/newproject.jpg");
    if (m_NewProjectIconAsset) m_NewProjectIcon = m_NewProjectIconAsset->GetTexture();
    
    m_OpenProjectIconAsset = TextureImporter::ImportTexture(root + "/resources/icons/folder.png");
    if (m_OpenProjectIconAsset) m_OpenProjectIcon = m_OpenProjectIconAsset->GetTexture();
}

ProjectBrowserPanel::~ProjectBrowserPanel()
{
    // Textures are handled by asset manager or manually if needed
}

void ProjectBrowserPanel::OnImGuiRender(bool readOnly)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus |
                                   ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Project Browser", nullptr, windowFlags);

    if (m_OpenCreatePopupRequest)
    {
        ImGui::OpenPopup("Create New Project");
        m_OpenCreatePopupRequest = false;
    }

    DrawWelcomeScreen();

    if (m_ShowCreateDialog)
    {
        DrawCreateProjectDialog();
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void ProjectBrowserPanel::DrawWelcomeScreen()
{
    // 1. Sidebar (Recent Projects)
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
    ImGui::BeginChild("Sidebar", ImVec2(300, 0), true);

    ImGui::Spacing();
    ImGui::SetCursorPosX(20);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), ICON_FA_LINK " Chained");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Engine");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    ImGui::Spacing();
    ImGui::TextDisabled("   RECENT PROJECTS");
    ImGui::Spacing();

    std::string lastPath = EditorLayer::Get().GetConfig().LastProjectPath;
    if (!lastPath.empty())
    {
        std::string fileName = std::filesystem::path(lastPath).filename().string();
        std::string dirName = std::filesystem::path(lastPath).parent_path().string();

        // Custom Button Style for Recent Project
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

        // Draw a big button that looks like the card in the screenshot
        if (ImGui::Button(fileName.c_str(), ImVec2(-1, 50)))
        {
            ProjectActions::Open(lastPath);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", lastPath.c_str());
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::TextDisabled("   %s", dirName.c_str());
    }
    else
    {
        ImGui::TextDisabled("   No recent projects.");
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // 2. Main Area (Actions)
    // Darker background for main area to match screenshot
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
    ImGui::BeginChild("MainArea", ImVec2(0, 0), false);

    float centerX = ImGui::GetContentRegionAvail().x * 0.5f;
    float centerY = ImGui::GetContentRegionAvail().y * 0.5f;

    ImGui::SetCursorPos(ImVec2(centerX - 350, centerY - 150));

    // Large Card Style
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.13f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

    ImGui::BeginGroup();
    if (ImGui::ImageButton("##NewProject", (ImTextureID)(uintptr_t)m_NewProjectIcon.id, {300, 300}, {0, 1}, {1, 0}))
    {
        m_OpenCreatePopupRequest = true;
        m_ShowCreateDialog = true;
    }
    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("New Project");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 280);
    ImGui::TextDisabled("Start a fresh journey with a dedicated");
    ImGui::TextDisabled("project folder and optimized settings.");
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();

    ImGui::SameLine(0, 40);

    ImGui::BeginGroup();
    if (ImGui::ImageButton("##OpenProject", (ImTextureID)(uintptr_t)m_OpenProjectIcon.id, {300, 300}, {0, 1}, {1, 0}))
    {
        ProjectActions::Open();
    }
    ImGui::SetWindowFontScale(1.3f);
    ImGui::Text("Open Project");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 280);
    ImGui::TextDisabled("Browse and load an existing Chained");
    ImGui::TextDisabled("Engine project (.chproject) file.");
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);

    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg
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
        ImGui::InputText("##ProjectLocation", m_ProjectLocationBuffer, sizeof(m_ProjectLocationBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Browse..."))
        {
            auto result = Dialogs::PickFolder();
            if (result)
            {
                memset(m_ProjectLocationBuffer, 0, sizeof(m_ProjectLocationBuffer));
                std::string path = result->string();
                path.copy(m_ProjectLocationBuffer, sizeof(m_ProjectLocationBuffer) - 1);
            }
        }

        ImGui::Spacing();

        // Path Preview
        std::filesystem::path root(m_ProjectLocationBuffer);
        std::filesystem::path finalPath;
        if (root.filename().string() == m_ProjectNameBuffer)
        {
            finalPath = root / (std::string(m_ProjectNameBuffer) + ".chproject");
        }
        else
        {
            finalPath = root / m_ProjectNameBuffer / (std::string(m_ProjectNameBuffer) + ".chproject");
        }

        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Resulting Project File:");
        ImGui::SetWindowFontScale(0.9f);
        ImGui::TextWrapped("%s", finalPath.string().c_str());
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            if (strlen(m_ProjectNameBuffer) == 0)
            {
                CH_CORE_ERROR("Project Name cannot be empty!");
            }
            else if (m_EventCallback)
            {
                CH_CORE_INFO("ProjectBrowser: Dispatching ProjectCreatedEvent - Name: {0}, Location: {1}",
                             m_ProjectNameBuffer, m_ProjectLocationBuffer);
                ProjectCreatedEvent e(m_ProjectNameBuffer, m_ProjectLocationBuffer);
                m_EventCallback(e);
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
} // namespace CHEngine
