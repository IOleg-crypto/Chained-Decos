#include "project_browser_panel.h"
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

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

    // Title Section
    {
        ImGui::SetCursorPosY(center.y - 250.0f);

        auto drawCenteredText = [](const char *text, ImVec4 color = {1, 1, 1, 1})
        {
            float width = ImGui::CalcTextSize(text).x;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - width) * 0.5f);
            ImGui::TextColored(color, "%s", text);
        };

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
        drawCenteredText("CHAINED ENGINE", {0.3f, 0.8f, 1.0f, 1.0f});
        ImGui::PopStyleColor();
    }

    // Actions Section
    float cardWidth = 250.0f;
    float cardHeight = 320.0f;
    float gap = 40.0f;
    float totalWidth = (cardWidth * 2) + gap;

    ImGui::SetCursorPos(ImVec2((windowSize.x - totalWidth) * 0.5f, center.y - 120.0f));

    auto drawActionCard = [&](const char *label, const char *iconPath, const char *description,
                              std::function<void()> onClick)
    {
        ImGui::BeginGroup();
        ImGui::PushID(label);

        ImVec4 cardBg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        cardBg.w = 0.5f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, cardBg);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));

        bool hovered = false;
        if (ImGui::BeginChild(label, {cardWidth, cardHeight}, true,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        {
            hovered = ImGui::IsWindowHovered();

            Texture2D tex = AssetManager::LoadTexture(iconPath);
            ImVec2 imageSize = {120, 120};
            ImGui::SetCursorPosX((cardWidth - imageSize.x) * 0.5f);
            ImGui::SetCursorPosY(30.0f);

            ImGui::Image((void *)(intptr_t)tex.id, imageSize, {0, 0}, {1, 1},
                         hovered ? ImVec4(1, 1, 1, 1) : ImVec4(0.8f, 0.8f, 0.8f, 0.8f),
                         ImVec4(0, 0, 0, 0));

            ImGui::SetCursorPosY(180.0f);
            float textWidth = ImGui::CalcTextSize(label).x;
            ImGui::SetCursorPosX((cardWidth - textWidth) * 0.5f);
            ImGui::Text(label);

            ImGui::SetCursorPosY(210.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cardWidth - 30.0f);
            ImGui::TextWrapped("%s", description);
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();

            ImGui::SetCursorPos(ImVec2(0, 0));
            if (ImGui::InvisibleButton("##CardClick", {cardWidth, cardHeight}))
            {
                onClick();
            }

            if (hovered)
            {
                ImGui::GetWindowDrawList()->AddRect(
                    ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                    ImColor(0.2f, 0.7f, 1.0f, 0.5f), 12.0f, 0, 2.0f);
            }
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::EndGroup();
    };

    drawActionCard("New Project", PROJECT_ROOT_DIR "/resources/map_editor/newproject.jpg",
                   "Start a fresh journey with a clean scene and default settings.",
                   [&]()
                   {
                       m_ShowCreateDialog = true;
                       ImGui::OpenPopup("Create New Project");
                   });

    ImGui::SameLine(0, gap);

    drawActionCard("Open Project", PROJECT_ROOT_DIR "/resources/map_editor/folder.png",
                   "Continue working on an existing Chained Engine project file.",
                   [&]()
                   {
                       nfdu8filteritem_t filterItem[1] = {{"Chained Engine Project", "chproject"}};
                       nfdchar_t *outPath = nullptr;
                       nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

                       if (result == NFD_OKAY)
                       {
                           if (m_EventCallback)
                           {
                               ProjectOpenedEvent e(outPath);
                               m_EventCallback(e);
                           }
                           NFD_FreePath(outPath);
                       }
                   });

    ImGui::PopFont();
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
            if (m_EventCallback)
            {
                ProjectCreatedEvent e(m_ProjectNameBuffer, m_ProjectLocationBuffer);
                m_EventCallback(e);
            }
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
} // namespace CHEngine
