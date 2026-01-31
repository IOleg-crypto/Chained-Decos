#include "project_settings_panel.h"
#include "engine/scene/project.h"
#include "engine/scene/project_serializer.h"
#include "editor/utils/scene_registry.h"
#include "rlImGui/extras/iconsfontawesome6.h"
#include "imgui.h"

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
        ImGui::PushID(this);
        auto &config = project->GetConfig();

        if (ImGui::CollapsingHeader(ICON_FA_GEARS " General", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char nameBuf[256];
            strncpy(nameBuf, config.Name.c_str(), 255);
            if (ImGui::InputText("Project Name", nameBuf, 255))
            {
                config.Name = nameBuf;
            }

            // --- Declarative Scene Selection ---
            auto availableScenes = SceneRegistry::GetAvailableScenes();
            const char* currentScene = config.StartScene.c_str();
            
            if (ImGui::BeginCombo("Start Scene", currentScene))
            {
                for (const auto& scenePath : availableScenes)
                {
                    bool isSelected = (config.StartScene == scenePath);
                    if (ImGui::Selectable(scenePath.c_str(), isSelected))
                    {
                        config.StartScene = scenePath;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();
            ImGui::Text("Build & Run Settings");

            const char *configNames[] = {"Debug", "Release"};
            int currentConfig = (int)config.BuildConfig;
            if (ImGui::Combo("Build Configuration", &currentConfig, configNames, 2))
            {
                config.BuildConfig = (Configuration)currentConfig;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Select which runtime version to launch when clicking 'Build & Run Standalone'.");
            }
        }

        if (ImGui::CollapsingHeader(ICON_FA_CUBES " Physics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            config.Physics.DrawUI();
        }

        if (ImGui::CollapsingHeader(ICON_FA_WINDOW_RESTORE " Window", ImGuiTreeNodeFlags_DefaultOpen))
        {
            config.Window.DrawUI();
        }
        
        if (ImGui::CollapsingHeader(ICON_FA_PLAY " Runtime", ImGuiTreeNodeFlags_DefaultOpen))
        {
            config.Runtime.DrawUI();
        }

        if (ImGui::CollapsingHeader(ICON_FA_MOUNTAIN_SUN " Rendering", ImGuiTreeNodeFlags_DefaultOpen))
        {
            config.Render.DrawUI();
        }

        ImGui::Separator();
        if (ImGui::Button("Save Project Settings"))
        {
            ProjectSerializer serializer(project);
            std::filesystem::path path =
                project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
            serializer.Serialize(path);
        }
        ImGui::PopID();
    }
    ImGui::End();
}
} // namespace CHEngine
