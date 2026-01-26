#include "project_settings_panel.h"
#include "engine/scene/project.h"
#include "engine/scene/project_serializer.h"
#include <imgui.h>

namespace CHEngine
{
ProjectSettingsPanel::ProjectSettingsPanel()
{
    m_Name = "Project Settings";
    m_IsOpen = false;
}

void ProjectSettingsPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
        return;

    auto project = Project::GetActive();
    if (!project)
    {
        m_IsOpen = false;
        return;
    }

    if (ImGui::Begin("Project Settings", &m_IsOpen))
    {
        auto &config = const_cast<ProjectConfig &>(project->GetConfig());

        if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char nameBuf[256];
            strncpy(nameBuf, config.Name.c_str(), 255);
            if (ImGui::InputText("Project Name", nameBuf, 255))
                config.Name = nameBuf;

            char sceneBuf[256];
            strncpy(sceneBuf, config.StartScene.c_str(), 255);
            if (ImGui::InputText("Start Scene", sceneBuf, 255))
                config.StartScene = sceneBuf;

            ImGui::Separator();
            ImGui::Text("Build & Run Settings");

            const char *configNames[] = {"Debug", "Release"};
            int currentConfig = (int)config.BuildConfig;
            if (ImGui::Combo("Build Configuration", &currentConfig, configNames, 2))
            {
                config.BuildConfig = (Configuration)currentConfig;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Select which runtime version to launch when clicking 'Build & "
                                  "Run Standalone'.");
        }

        if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("World Gravity", &config.Physics.Gravity, 0.1f, 0.0f, 100.0f);
        }

        if (ImGui::CollapsingHeader("Window", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragInt("Width", &config.Window.Width, 1, 800, 3840);
            ImGui::DragInt("Height", &config.Window.Height, 1, 600, 2160);
            ImGui::Checkbox("VSync", &config.Window.VSync);
            ImGui::Checkbox("Resizable", &config.Window.Resizable);
        }

        ImGui::Separator();
        if (ImGui::Button("Save Project Settings"))
        {
            ProjectSerializer serializer(project);
            std::filesystem::path path =
                project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
            serializer.Serialize(path);
        }
    }
    ImGui::End();
}
} // namespace CHEngine
