#include "project_selector_ui.h"
#include "engine/core/service_locator.h"
#include "editor/editor_layer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome6.h"
#include <filesystem>
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/texture_asset.h"
#include <string>

namespace Chained
{

ProjectSelectorUI::ProjectSelectorUI(EditorProjectManager& projectManager)
    : m_ProjectManager(projectManager)
{
}

void ProjectSelectorUI::LoadEditorIcons()
{
    if (m_IconsLoaded) return;

    auto assetManager = ServiceLocator::Get<AssetManager>();
    if (assetManager)
    {
        // Імпортуємо іконки. Переконайся, що шляхи відносні до робочої папки запуску (CWD) бінарника рушія
        uint64_t newProjHandle = assetManager->ImportAsset("resources/icons/newproject.jpg");
        uint64_t openProjHandle = assetManager->ImportAsset("resources/icons/folder.png");

        m_NewProjectIcon = assetManager->GetAsset<TextureAsset>(newProjHandle);
        m_OpenProjectIcon = assetManager->GetAsset<TextureAsset>(openProjHandle);
        m_IconsLoaded = true;
    }
}

void ProjectSelectorUI::OnImGuiRender()
{
    LoadEditorIcons();

    static bool showCreateDialog = false;
    static char projectNameBuffer[128] = "NewProject";
    static char projectLocationBuffer[256] = "";
    static bool initialized = false;

    if (!initialized)
    {
        std::string cwd = std::filesystem::current_path().string();
        strncpy(projectLocationBuffer, cwd.c_str(), sizeof(projectLocationBuffer) - 1);
        projectLocationBuffer[sizeof(projectLocationBuffer) - 1] = '\0';
        initialized = true;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Project Selector", nullptr, windowFlags);

    float sidebarWidth = 320.0f;
    
    // Sidebar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.02f, 0.02f, 1.0f));
    ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), false);
    
    // Banner / Logo Area
    ImGui::Dummy(ImVec2(0, 15));
    ImGui::SetCursorPosX(20.0f);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), ICON_FA_LINK " Chained");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Engine");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    ImGui::Dummy(ImVec2(0, 15));
    ImGui::TextDisabled("   RECENT PROJECTS");
    ImGui::Dummy(ImVec2(0, 10));

    const auto& config = EditorLayer::Get().GetConfig();
    if (config.RecentProjects.empty())
    {
        ImGui::TextDisabled("   No recent projects.");
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.05f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

        for (const auto& projectPath : config.RecentProjects)
        {
            std::string fileName = std::filesystem::path(projectPath).filename().string();
            std::string dirName  = std::filesystem::path(projectPath).parent_path().filename().string();

            std::string label = ICON_FA_FOLDER_OPEN "  " + fileName + "\n      " + dirName;
            
            ImGui::SetCursorPosX(10.0f);
            if (ImGui::Button(label.c_str(), ImVec2(sidebarWidth - 20, 50)))
            {
                m_ProjectManager.OpenProject(projectPath);
                break;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", projectPath.c_str());
            }
            ImGui::Dummy(ImVec2(0, 5));
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
    
    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg Sidebar

    ImGui::SameLine();

    // Main Area
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
    ImGui::BeginChild("MainArea");
    
    float centerX = ImGui::GetContentRegionAvail().x * 0.5f;
    float centerY = ImGui::GetContentRegionAvail().y * 0.5f;

    ImGui::SetCursorPos(ImVec2(centerX - 350.0f, centerY - 150.0f));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.13f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.19f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

    ImGui::BeginGroup();
    {
        ImTextureID newProjTex = 0;
        if (m_NewProjectIcon) {
            newProjTex = (ImTextureID)(uintptr_t)m_NewProjectIcon->GetRendererID();
        }

        if (ImGui::ImageButton("##NewProject", newProjTex, {300, 300}, {0, 1}, {1, 0}))
        {
            showCreateDialog = true;
        }
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
    {
        ImTextureID openProjTex = 0;
        if (m_OpenProjectIcon) {
            openProjTex = (ImTextureID)(uintptr_t)m_OpenProjectIcon->GetRendererID();
        }

        if (ImGui::ImageButton("##OpenProject", openProjTex, {300, 300}, {0, 1}, {1, 0}))
        {
            m_ProjectManager.OpenProject();
        }
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
    ImGui::PopStyleColor(); // ChildBg MainArea

    if (showCreateDialog)
    {
        ImGui::OpenPopup("Create New Project");
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Create New Project", &showCreateDialog, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Project Name:");
            ImGui::InputText("##ProjectName", projectNameBuffer, sizeof(projectNameBuffer));

            ImGui::Text("Location:");
            ImGui::InputText("##ProjectLocation", projectLocationBuffer, sizeof(projectLocationBuffer));

            ImGui::Dummy(ImVec2(0, 10));

            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                std::filesystem::path fullPath = std::filesystem::path(projectLocationBuffer) / projectNameBuffer;
                m_ProjectManager.NewProject(projectNameBuffer, fullPath.string());
                showCreateDialog = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                showCreateDialog = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

} // namespace Chained