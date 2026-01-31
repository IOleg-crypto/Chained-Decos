#include "project_browser_panel.h"
#include "engine/core/application.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/project.h"
#include "editor_layer.h"

// Removed redundant include: engine/graphics/asset_manager.h
#include "extras/iconsfontawesome6.h"
#include "filesystem"
#include "imgui.h"
#include "nfd.h"
#include "rlImGui.h"

namespace CHEngine
{
    ProjectBrowserPanel::ProjectBrowserPanel()
    {
        m_Name = "Project Browser";
        std::string cwd = std::filesystem::current_path().string();
        memset(m_ProjectLocationBuffer, 0, sizeof(m_ProjectLocationBuffer));
        cwd.copy(m_ProjectLocationBuffer, sizeof(m_ProjectLocationBuffer) - 1);

        // Load icons
        const char *newProjectIconPath = PROJECT_ROOT_DIR "/engine/resources/icons/newproject.jpg";
        const char *openProjectIconPath = PROJECT_ROOT_DIR "/engine/resources/icons/folder.png";

        if (std::filesystem::exists(newProjectIconPath))
        {
            Texture2D *tex = new Texture2D(LoadTexture(newProjectIconPath));
            m_NewProjectIcon = tex;
        }

        if (std::filesystem::exists(openProjectIconPath))
        {
            Texture2D *tex = new Texture2D(LoadTexture(openProjectIconPath));
            m_OpenProjectIcon = tex;
        }

        m_IconsLoaded = (m_NewProjectIcon != nullptr && m_OpenProjectIcon != nullptr);
    }

    ProjectBrowserPanel::~ProjectBrowserPanel()
    {
        if (m_NewProjectIcon)
        {
            UnloadTexture(*(Texture2D *)m_NewProjectIcon);
            delete (Texture2D *)m_NewProjectIcon;
        }
        if (m_OpenProjectIcon)
        {
            UnloadTexture(*(Texture2D *)m_OpenProjectIcon);
            delete (Texture2D *)m_OpenProjectIcon;
        }
    }

    void ProjectBrowserPanel::OnImGuiRender(bool readOnly)
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Project Browser", nullptr, windowFlags);
        ImGui::PushID(this);

        if (m_OpenCreatePopupRequest)
        {
            ImGui::OpenPopup("Create New Project");
            m_OpenCreatePopupRequest = false;
        }

        DrawWelcomeScreen();

        if (m_ShowCreateDialog)
            DrawCreateProjectDialog();

        ImGui::PopID();
        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    // Helper: Draw sidebar with recent projects
    static void DrawSidebar(ImDrawList *drawList, ImVec2 pMin, ImVec2 pMax, float sidebarWidth)
    {
        // Gradient background
        drawList->AddRectFilledMultiColor(pMin, ImVec2(pMin.x + sidebarWidth, pMax.y), ImColor(10, 15, 25),
                                          ImColor(5, 7, 12), ImColor(2, 3, 5), ImColor(8, 10, 18));
        drawList->AddLine(ImVec2(pMin.x + sidebarWidth, pMin.y), ImVec2(pMin.x + sidebarWidth, pMax.y),
                          ImColor(255, 255, 255, 20));

        ImGui::SetCursorPos(ImVec2(40, 60));
        ImGui::SetWindowFontScale(3.0f);
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "Chained Engine");
        ImGui::SetWindowFontScale(1.0f);

        ImGui::SetCursorPosY(180);
        ImGui::SetCursorPosX(40);
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "RECENT PROJECTS");

        ImGui::SetCursorPosY(210);
        ImGui::SetCursorPosX(30);

        std::string lastPath = Editor::Get().GetEditorConfig().LastProjectPath;
        if (!lastPath.empty())
        {
            std::string fileName = std::filesystem::path(lastPath).filename().string();
            std::string dirName = std::filesystem::path(lastPath).parent_path().string();

            ImGui::BeginChild("##RecentList", ImVec2(sidebarWidth - 60, 200), false);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            if (ImGui::Button(fileName.c_str(), ImVec2(sidebarWidth - 60, 60)))
                ProjectActions::Open(lastPath);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", lastPath.c_str());
                ImGui::EndTooltip();
            }
            ImGui::PopStyleVar();
            ImGui::SetCursorPosX(10);
            ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), "%s", dirName.c_str());
            ImGui::EndChild();
        }
        else
        {
            ImGui::SetCursorPosX(40);
            ImGui::TextDisabled("No recent projects found.");
        }
    }

    static bool DrawActionCard(ImDrawList *drawList, ImVec2 cardMin, float cardWidth, float cardHeight,
                               const char *title, const char *desc1, const char *desc2, Texture2D *iconTexture,
                               const char *buttonId)
    {
        ImVec2 cardMax = ImVec2(cardMin.x + cardWidth, cardMin.y + cardHeight);
        bool hovered = ImGui::IsMouseHoveringRect(cardMin, cardMax);
        ImU32 cardColor = hovered ? ImColor(55, 55, 60, 255) : ImColor(40, 40, 45, 255);
        ImU32 borderColor = hovered ? ImColor(100, 180, 255, 200) : ImColor(70, 70, 75, 255);

        drawList->AddRectFilled(cardMin, cardMax, cardColor, 15.0f);
        drawList->AddRect(cardMin, cardMax, borderColor, 15.0f, 0, 2.0f);

        // Icon Rendering - Using Image Textures instead of ImDrawList drawings
        float textureSize = 160.0f; // Increased for better visibility as per design
        ImVec2 texturePos = ImVec2(cardMin.x + (cardWidth - textureSize) * 0.5f, cardMin.y + 15.0f);

        if (iconTexture && iconTexture->id > 0)
        {
            ImGui::SetCursorScreenPos(texturePos);
            rlImGuiImageSize(iconTexture, (int)textureSize, (int)textureSize);
        }
        else
        {
            // Fallback or empty space
            drawList->AddRectFilled(texturePos, ImVec2(texturePos.x + textureSize, texturePos.y + textureSize),
                                    ImColor(50, 50, 55, 255), 12.0f);
        }

        // Title
        ImGui::SetWindowFontScale(1.4f);
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        drawList->AddText(ImVec2(cardMin.x + (cardWidth - titleSize.x) * 0.5f, cardMin.y + 175.0f),
                          ImColor(240, 240, 245), title);
        ImGui::SetWindowFontScale(1.0f);

        // Description
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.62f, 1.0f));
        float descStartY = cardMin.y + 195.0f;

        ImVec2 d1Size = ImGui::CalcTextSize(desc1);
        drawList->AddText(ImVec2(cardMin.x + (cardWidth - d1Size.x) * 0.5f, descStartY), ImColor(100, 100, 105), desc1);

        ImVec2 d2Size = ImGui::CalcTextSize(desc2);
        drawList->AddText(ImVec2(cardMin.x + (cardWidth - d2Size.x) * 0.5f, descStartY + 26.0f), ImColor(100, 100, 105),
                          desc2);
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos(cardMin);
        return ImGui::InvisibleButton(buttonId, ImVec2(cardWidth, cardHeight));
    }

    void ProjectBrowserPanel::DrawWelcomeScreen()
    {
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 pMin = ImGui::GetWindowPos();
        ImVec2 pMax = ImVec2(pMin.x + windowSize.x, pMin.y + windowSize.y);
        float sidebarWidth = 350.0f;

        DrawSidebar(drawList, pMin, pMax, sidebarWidth);

        // Main Area - Background Grid
        float gridSize = 50.0f;
        for (float x = pMin.x + sidebarWidth + gridSize; x < pMax.x; x += gridSize)
            drawList->AddLine(ImVec2(x, pMin.y), ImVec2(x, pMax.y), ImColor(255, 255, 255, 10));
        for (float y = pMin.y + gridSize; y < pMax.y; y += gridSize)
            drawList->AddLine(ImVec2(pMin.x + sidebarWidth, y), ImVec2(pMax.x, y), ImColor(255, 255, 255, 10));

        // Action Cards Layout
        float cardWidth = 260.0f, cardHeight = 260.0f, gap = 150.0f;
        float totalWidth = (cardWidth * 2) + gap;
        float mainAreaWidth = windowSize.x - sidebarWidth;
        float startX = pMin.x + sidebarWidth + (mainAreaWidth - totalWidth) * 0.5f;
        float centerY = pMin.y + windowSize.y * 0.45f;

        ImVec2 card1Min = ImVec2(startX, centerY - cardHeight * 0.5f);
        if (DrawActionCard(drawList, card1Min, cardWidth, cardHeight, "New Project",
                           "Start a fresh journey with a dedicated", "project folder and optimized settings.",
                           (Texture2D *)m_NewProjectIcon, "##NewProjectCard")){
            m_OpenCreatePopupRequest = true;
            m_ShowCreateDialog = true;
        }

        ImVec2 card2Min = ImVec2(startX + cardWidth + gap, centerY - cardHeight * 0.5f);
        if (DrawActionCard(drawList, card2Min, cardWidth, cardHeight, "Open Project",
                           "Browse and load an existing Chained", "Engine project (.chproject) file.",
                           (Texture2D *)m_OpenProjectIcon, "##OpenProjectCard")){
            ProjectActions::Open();
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
            ImGui::InputText("##ProjectLocation", m_ProjectLocationBuffer, sizeof(m_ProjectLocationBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Browse..."))
            {
                nfdchar_t *outPath = nullptr;
                nfdresult_t result = NFD_PickFolder(&outPath, nullptr);

                if (result == NFD_OKAY)
                {
                    memset(m_ProjectLocationBuffer, 0, sizeof(m_ProjectLocationBuffer));
                    std::string path(outPath);
                    path.copy(m_ProjectLocationBuffer, sizeof(m_ProjectLocationBuffer) - 1);
                    NFD_FreePath(outPath);
                }
            }

            ImGui::Spacing();

            // Path Preview
            std::filesystem::path root(m_ProjectLocationBuffer);
            std::filesystem::path finalPath;
            if (root.filename().string() == m_ProjectNameBuffer)
                finalPath = root / (std::string(m_ProjectNameBuffer) + ".chproject");
            else
                finalPath = root / m_ProjectNameBuffer / (std::string(m_ProjectNameBuffer) + ".chproject");

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
