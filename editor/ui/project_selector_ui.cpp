#include "project_selector_ui.h"
#include "editor/editor_layer.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome6.h"
#include <filesystem>
#include <string>

namespace CHEngine
{

ProjectSelectorUI::ProjectSelectorUI(EditorProjectManager& projectManager)
    : m_ProjectManager(projectManager)
{
}

void ProjectSelectorUI::OnImGuiRender()
{
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
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
    ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), false);
    
    // Banner / Logo Area
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
    ImGui::BeginChild("Banner", ImVec2(0, 60), false);
    ImGui::Dummy(ImVec2(0, 15));
    ImGui::SetCursorPosX(20.0f);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Large font if available
    ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), ICON_FA_CIRCLE_NODES);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), " Chained");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), " Engine");
    ImGui::PopFont();
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 20));
    ImGui::SetCursorPosX(20.0f);
    ImGui::TextDisabled("RECENT PROJECTS");
    ImGui::Dummy(ImVec2(0, 10));

    const auto& config = EditorLayer::Get().GetConfig();
    for (const auto& projectPath : config.RecentProjects)
    {
        std::filesystem::path path(projectPath);
        std::string projectName = path.stem().string();
        std::string projectDir = path.parent_path().string();

        ImGui::PushID(projectPath.c_str());
        ImGui::SetCursorPosX(10.0f);
        
        bool clicked = false;
        if (ImGui::BeginChild(projectPath.c_str(), ImVec2(sidebarWidth - 20, 55), false, ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.3f, 0.6f, 1.0f));
            if (ImGui::Selectable("##bg", false, ImGuiSelectableFlags_AllowOverlap, ImVec2(0, 55)))
            {
                clicked = true;
            }
            ImGui::PopStyleColor();
            
            ImGui::SetCursorPos(ImVec2(10, 8));
            ImGui::Text("%s", projectName.c_str());
            ImGui::SetCursorPos(ImVec2(10, 28));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::Text("%s", projectDir.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();
        ImGui::PopID();

        if (clicked)
        {
            m_ProjectManager.OpenProject(projectPath);
        }
        
        ImGui::Dummy(ImVec2(0, 5));
    }
    
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // Main Area
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
    ImGui::BeginChild("MainArea");
    
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    
    // Center the cards
    float cardsTotalWidth = 540.0f; // 250*2 + 40 gap
    float startX = (contentRegion.x - cardsTotalWidth) * 0.5f;
    float startY = (contentRegion.y - 300.0f) * 0.45f;
    
    ImGui::SetCursorPos(ImVec2(startX, startY));
    
    auto DrawCard = [&](const char* icon, const char* title, const char* desc, ImVec4 accentColor) {
        ImGui::BeginGroup();
        
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImVec2 size(250, 300);
        
        bool hovered = ImGui::ItemHoverable(ImRect(p, ImVec2(p.x + size.x, p.y + size.y)), ImGui::GetID(title), ImGuiItemFlags_None);
        bool pressed = hovered && ImGui::IsMouseClicked(0);
        
        // Background
        ImU32 bgColor = ImGui::GetColorU32(hovered ? ImVec4(0.18f, 0.18f, 0.20f, 1.0f) : ImVec4(0.14f, 0.14f, 0.16f, 1.0f));
        window->DrawList->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), bgColor, 12.0f);
        if (hovered)
            window->DrawList->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), ImGui::GetColorU32(accentColor), 12.0f, 0, 2.0f);
        
        // Icon (Centered)
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40);
        ImGui::SetWindowFontScale(4.0f);
        float iconWidth = ImGui::CalcTextSize(icon).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (size.x - iconWidth) * 0.5f);
        ImGui::TextColored(accentColor, "%s", icon);
        ImGui::SetWindowFontScale(1.0f);
        
        // Title
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40);
        float titleWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (size.x - titleWidth) * 0.5f);
        ImGui::Text("%s", title);
        
        // Subtitle/Desc
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + size.x - 20);
        float descWidth = std::min(size.x - 40.0f, ImGui::CalcTextSize(desc).x);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (size.x - descWidth) * 0.5f);
        ImGui::TextWrapped("%s", desc);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        
        ImGui::Dummy(size);
        ImGui::EndGroup();
        
        return pressed;
    };
    
    if (DrawCard(ICON_FA_PLUS, "New Project", "Start a fresh journey with a dedicated project folder and optimized settings.", ImVec4(0.3f, 0.5f, 1.0f, 1.0f)))
    {
        showCreateDialog = true;
    }
    
    ImGui::SameLine(0, 40);
    
    if (DrawCard(ICON_FA_FOLDER_OPEN, "Open Project", "Browse and load an existing Chained Engine project (.chproject) file.", ImVec4(0.8f, 0.8f, 0.8f, 1.0f)))
    {
        m_ProjectManager.OpenProject();
    }
    
    ImGui::EndChild();
    ImGui::PopStyleColor();

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

} // namespace CHEngine
