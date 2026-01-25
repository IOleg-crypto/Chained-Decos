#include "project_browser_panel.h"
#include "editor/editor_utils.h"
#include "editor_settings.h"
#include "engine/renderer/asset_manager.h"
#include "nfd.h"
#include "raylib.h"
#include <extras/IconsFontAwesome6.h>
#include <filesystem>
#include <imgui.h>

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

void ProjectBrowserPanel::DrawWelcomeScreen()
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    ImVec2 pMin = ImGui::GetWindowPos();
    ImVec2 pMax = ImVec2(pMin.x + windowSize.x, pMin.y + windowSize.y);

    // 1. SIDEBAR (350px)
    float sidebarWidth = 350.0f;
    drawList->AddRectFilledMultiColor(pMin, ImVec2(pMin.x + sidebarWidth, pMax.y),
                                      ImColor(10, 15, 25), ImColor(5, 7, 12), ImColor(2, 3, 5),
                                      ImColor(8, 10, 18));

    // Sidebar Separator
    drawList->AddLine(ImVec2(pMin.x + sidebarWidth, pMin.y), ImVec2(pMin.x + sidebarWidth, pMax.y),
                      ImColor(255, 255, 255, 20));

    // Sidebar Content
    {
        ImGui::SetCursorPos(ImVec2(40, 60));

        // Brand Title
        const char *title = "Chained Engine";
        float oldScale = 1.0f;
        ImGui::SetWindowFontScale(3.0f);
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "%s", title);
        ImGui::SetWindowFontScale(1.0f);

        ImGui::SetCursorPosY(180);
        ImGui::SetCursorPosX(40);
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "RECENT PROJECTS");

        ImGui::SetCursorPosY(210);
        ImGui::SetCursorPosX(30);

        std::string lastPath = EditorSettings::Get().LastProjectPath;
        if (!lastPath.empty())
        {
            std::string fileName = std::filesystem::path(lastPath).filename().string();
            std::string dirName = std::filesystem::path(lastPath).parent_path().string();

            ImGui::BeginChild("##RecentList", ImVec2(sidebarWidth - 60, 200), false);

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
            if (ImGui::Button(fileName.c_str(), ImVec2(sidebarWidth - 60, 60)))
            {
                // Call EditorUtils directly - it will dispatch the event after loading
                ProjectUtils::OpenProject(lastPath);
            }
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

    // 2. MAIN AREA
    {
        // Background Grid
        float gridSize = 50.0f;
        for (float x = pMin.x + sidebarWidth + gridSize; x < pMax.x; x += gridSize)
            drawList->AddLine(ImVec2(x, pMin.y), ImVec2(x, pMax.y), ImColor(255, 255, 255, 10));
        for (float y = pMin.y + gridSize; y < pMax.y; y += gridSize)
            drawList->AddLine(ImVec2(pMin.x + sidebarWidth, y), ImVec2(pMax.x, y),
                              ImColor(255, 255, 255, 10));

        // Action Cards
        float cardWidth = 320.0f;
        float cardHeight = 400.0f;
        float gap = 60.0f;
        float totalWidth = (cardWidth * 2) + gap;
        float startX = sidebarWidth + (windowSize.x - sidebarWidth - totalWidth) * 0.5f;
        float centerY = windowSize.y * 0.5f;

        ImGui::SetCursorPos(ImVec2(startX, centerY - 200.0f));

        auto drawActionCard = [&](const char *label, const char *iconPath, const char *description,
                                  std::function<void()> onClick)
        {
            ImGui::BeginGroup();
            ImGui::PushID(label);

            ImVec2 cardPos = ImGui::GetCursorScreenPos();
            bool hovered = ImGui::IsMouseHoveringRect(
                cardPos, {cardPos.x + cardWidth, cardPos.y + cardHeight});

            if (hovered)
            {
                drawList->AddRect(ImVec2(cardPos.x - 4, cardPos.y - 4),
                                  ImVec2(cardPos.x + cardWidth + 4, cardPos.y + cardHeight + 4),
                                  ImColor(0, 150, 255, 80), 20.0f, 0, 6.0f);
            }

            ImGui::PushStyleColor(ImGuiCol_ChildBg, hovered ? ImVec4(0.15f, 0.18f, 0.25f, 0.9f)
                                                            : ImVec4(0.12f, 0.12f, 0.15f, 0.7f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 20.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 30));

            if (ImGui::BeginChild(label, {cardWidth, cardHeight}, true,
                                  ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoScrollWithMouse))
            {
                auto texAsset = Assets::Get<TextureAsset>(iconPath);
                Texture2D tex = texAsset ? texAsset->GetTexture() : Texture2D{0};
                ImVec2 imageSize = {160, 160};
                ImGui::SetCursorPosX((cardWidth - imageSize.x) * 0.5f);
                ImGui::SetCursorPosY(40.0f);

                ImGui::Image((void *)(intptr_t)tex.id, imageSize, {0, 0}, {1, 1},
                             hovered ? ImVec4(1, 1, 1, 1) : ImVec4(0.8f, 0.8f, 0.8f, 0.9f),
                             ImVec4(0, 0, 0, 0));

                ImGui::SetCursorPosY(230.0f);
                ImGui::SetWindowFontScale(1.4f);
                float nameWidth = ImGui::CalcTextSize(label).x;
                ImGui::SetCursorPosX((cardWidth - nameWidth) * 0.5f);
                ImGui::TextColored(hovered ? ImVec4(0.2f, 0.7f, 1.0f, 1.0f) : ImVec4(1, 1, 1, 1),
                                   label);
                ImGui::SetWindowFontScale(1.0f);

                ImGui::SetCursorPosY(280.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cardWidth - 60.0f);
                ImGui::TextWrapped("%s", description);
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();

                // Click handling
                if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
                    ImGui::IsMouseClicked(0))
                {
                    onClick();
                }
            }
            ImGui::EndChild();

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
            ImGui::PopID();
            ImGui::EndGroup();
        };

        drawActionCard(
            "New Project", "engine:icons/newproject.jpg",
            "Start a fresh journey with a dedicated project folder and optimized settings.",
            [&]()
            {
                CH_CORE_INFO("ProjectBrowser: New Project card clicked.");
                m_ShowCreateDialog = true;
                m_OpenCreatePopupRequest = true;
            });

        ImGui::SameLine(0, gap);

        drawActionCard("Open Project", "engine:icons/folder.png",
                       "Browse and load an existing Chained Engine project (.chproject) file.",
                       [&]()
                       {
                           CH_CORE_INFO("ProjectBrowser: Open Project card clicked.");
                           nfdu8filteritem_t filterItem[1] = {
                               {"Chained Engine Project", "chproject"}};
                           nfdchar_t *outPath = nullptr;
                           nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

                           if (result == NFD_OKAY)
                           {
                               // Call EditorUtils directly - it will dispatch the event after
                               // loading
                               ProjectUtils::OpenProject(outPath);
                               NFD_FreePath(outPath);
                           }
                       });
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
