#include "project_browser_panel.h"
#include "actions/project_actions.h"
#include "editor.h"
#include "editor_layer.h"


// Removed redundant include: engine/graphics/asset_manager.h
#include "extras/iconsfontawesome6.h"
#include "filesystem"
#include "imgui.h"
#include "nfd.h"
#include "raylib.h"

namespace CHEngine
{
ProjectBrowserPanel::ProjectBrowserPanel()
{
    m_Name = "Project Browser";
    std::string cwd = std::filesystem::current_path().string();
    strncpy(m_ProjectLocationBuffer, cwd.c_str(), sizeof(m_ProjectLocationBuffer));
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

    if (m_OpenCreatePopupRequest)
    {
        ImGui::OpenPopup("Create New Project");
        m_OpenCreatePopupRequest = false;
    }

    DrawWelcomeScreen();

    if (m_ShowCreateDialog)
        DrawCreateProjectDialog();

    ImGui::End();
    ImGui::PopStyleVar(3);
}

// Helper: Draw sidebar with recent projects
static void DrawSidebar(ImDrawList *drawList, ImVec2 pMin, ImVec2 pMax, float sidebarWidth)
{
    // Gradient background
    drawList->AddRectFilledMultiColor(pMin, ImVec2(pMin.x + sidebarWidth, pMax.y),
                                      ImColor(10, 15, 25), ImColor(5, 7, 12), ImColor(2, 3, 5),
                                      ImColor(8, 10, 18));
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
                           const char *title, const char *desc1, const char *desc2,
                           CardIconType iconType, const char *buttonId)
{
    ImVec2 cardMax = ImVec2(cardMin.x + cardWidth, cardMin.y + cardHeight);
    bool hovered = ImGui::IsMouseHoveringRect(cardMin, cardMax);
    ImU32 cardColor = hovered ? ImColor(55, 55, 60, 255) : ImColor(40, 40, 45, 255);
    ImU32 borderColor = hovered ? ImColor(100, 180, 255, 200) : ImColor(70, 70, 75, 255);

    drawList->AddRectFilled(cardMin, cardMax, cardColor, 15.0f);
    drawList->AddRect(cardMin, cardMax, borderColor, 15.0f, 0, 2.0f);

    // Icon
    float iconSize = 60.0f;
    ImVec2 iconPos = ImVec2(cardMin.x + (cardWidth - iconSize) * 0.5f, cardMin.y + 30.0f);
    drawList->AddRectFilled(iconPos, ImVec2(iconPos.x + iconSize, iconPos.y + iconSize * 0.8f),
                            ImColor(80, 80, 90), 8.0f);
    drawList->AddRect(iconPos, ImVec2(iconPos.x + iconSize, iconPos.y + iconSize * 0.8f),
                      ImColor(120, 120, 130), 8.0f, 0, 2.0f);

    float cx = iconPos.x + iconSize * 0.5f;
    float cy = iconPos.y + iconSize * 0.4f;
    if (iconType == CardIconType::NewProject)
    {
        drawList->AddLine(ImVec2(cx - 10, cy), ImVec2(cx + 10, cy), ImColor(200, 200, 210), 3.0f);
        drawList->AddLine(ImVec2(cx, cy - 10), ImVec2(cx, cy + 10), ImColor(200, 200, 210), 3.0f);
    }
    else
    {
        drawList->AddTriangleFilled(ImVec2(cx - 12, cy + 5), ImVec2(cx + 12, cy + 5),
                                    ImVec2(cx, cy - 10), ImColor(200, 200, 210));
    }

    // Title
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    drawList->AddText(ImVec2(cardMin.x + (cardWidth - titleSize.x) * 0.5f, cardMin.y + 105.0f),
                      ImColor(230, 230, 235), title);

    // Description
    ImVec2 d1Size = ImGui::CalcTextSize(desc1);
    ImVec2 d2Size = ImGui::CalcTextSize(desc2);
    drawList->AddText(ImVec2(cardMin.x + (cardWidth - d1Size.x) * 0.5f, cardMin.y + 130.0f),
                      ImColor(140, 140, 145), desc1);
    drawList->AddText(ImVec2(cardMin.x + (cardWidth - d2Size.x) * 0.5f, cardMin.y + 145.0f),
                      ImColor(140, 140, 145), desc2);

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
        drawList->AddLine(ImVec2(pMin.x + sidebarWidth, y), ImVec2(pMax.x, y),
                          ImColor(255, 255, 255, 10));

    // Action Cards Layout
    float cardWidth = 180.0f, cardHeight = 180.0f, gap = 40.0f;
    float totalWidth = (cardWidth * 2) + gap;
    float mainAreaWidth = windowSize.x - sidebarWidth;
    float startX = pMin.x + sidebarWidth + (mainAreaWidth - totalWidth) * 0.5f;
    float centerY = pMin.y + windowSize.y * 0.45f;

    ImVec2 card1Min = ImVec2(startX, centerY - cardHeight * 0.5f);
    if (DrawActionCard(drawList, card1Min, cardWidth, cardHeight, "New Project",
                       "Start a fresh journey with a dedicated",
                       "project folder and optimized settings.", CardIconType::NewProject,
                       "##NewProjectCard"))
        m_ShowCreateDialog = true;

    ImVec2 card2Min = ImVec2(startX + cardWidth + gap, centerY - cardHeight * 0.5f);
    if (DrawActionCard(drawList, card2Min, cardWidth, cardHeight, "Open Project",
                       "Browse and load an existing Chained", "Engine project (.chproject) file.",
                       CardIconType::OpenProject, "##OpenProjectCard"))
        ProjectActions::Open();
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

        // Path Preview
        std::filesystem::path root(m_ProjectLocationBuffer);
        std::filesystem::path finalPath;
        if (root.filename().string() == m_ProjectNameBuffer)
            finalPath = root / (std::string(m_ProjectNameBuffer) + ".chproject");
        else
            finalPath =
                root / m_ProjectNameBuffer / (std::string(m_ProjectNameBuffer) + ".chproject");

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
                CH_CORE_INFO(
                    "ProjectBrowser: Dispatching ProjectCreatedEvent - Name: {0}, Location: {1}",
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
