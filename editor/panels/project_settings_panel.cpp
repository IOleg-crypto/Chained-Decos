#include "project_settings_panel.h"
#include "engine/scene/project.h"
#include "editor_layer.h"
#include "engine/scene/project_serializer.h"
#include "imgui.h"
#include "engine/core/platform.h"
#include "IconsFontAwesome6.h"

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
    {
        return;
    }

    auto project = Project::GetActive();
    if (!project)
    {
        m_IsOpen = false;
        return;
    }

    if (ImGui::Begin("Project Settings", &m_IsOpen))
    {
        auto& config = project->GetConfig();

        static int selectedCategory = 0;
        const char* categories[] = {
            ICON_FA_GEARS " General",
            ICON_FA_CODE " Scripting",
            ICON_FA_CUBES " Physics",
            ICON_FA_WINDOW_RESTORE " Window",
            ICON_FA_CAMERA " Editor",
            ICON_FA_MOUNTAIN_SUN " Rendering"
        };

        ImGui::Columns(2, "ProjectSettingsColumns", true);
        ImGui::SetColumnWidth(0, 180.0f);

        // Sidebar
        for (int i = 0; i < (int)IM_ARRAYSIZE(categories); i++)
        {
            if (ImGui::Selectable(categories[i], selectedCategory == i))
            {
                selectedCategory = i;
            }
        }

        ImGui::NextColumn();

        // Content
        if (selectedCategory == 0) // General
        {
            ImGui::TextDisabled("General Settings");
            char nameBuf[256];
            strncpy(nameBuf, config.Name.c_str(), 255);
            if (ImGui::InputText("Project Name", nameBuf, 255))
            {
                config.Name = nameBuf;
            }

            char iconBuf[512];
            strncpy(iconBuf, config.IconPath.c_str(), 511);
            iconBuf[511] = '\0';
            if (ImGui::InputText("Icon Path", iconBuf, 511))
            {
                config.IconPath = iconBuf;
            }
            ImGui::SameLine();
            if (ImGui::Button("...###IconBrowse"))
            {
                std::vector<FileDialogFilter> filters = {{"Image Files", "png,jpg,jpeg"}};
                auto result = CHEngine::Platform::OpenFile(filters);
                if (result)
                {
                    config.IconPath = Project::GetRelativePath(result->string());
                }
            }

            auto availableScenes = Project::GetAvailableScenes();
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
            ImGui::TextDisabled(ICON_FA_ROCKET " Execution Settings");

            if (config.LaunchProfiles.empty())
            {
                config.LaunchProfiles.push_back({"Default", "${BUILD}/ChainedRuntime.exe", "--project \"${PROJECT_FILE}\""});
            }

            auto& profile = config.LaunchProfiles[0];
            char pathBuf[512];
            strncpy(pathBuf, profile.BinaryPath.c_str(), 511);
            pathBuf[511] = '\0';
            if (ImGui::InputText("Runtime Path", pathBuf, 511))
            {
                profile.BinaryPath = pathBuf;
            }

            ImGui::SameLine();
            if (ImGui::Button("...###BinBrowse"))
            {
                std::vector<FileDialogFilter> filters = {{"Runtime Executable", "exe"}};
                auto result = CHEngine::Platform::OpenFile(filters);
                if (result)
                {
                    profile.BinaryPath = result->string();
                }
            }

            char argBuf[512];
            strncpy(argBuf, profile.Arguments.c_str(), 511);
            argBuf[511] = '\0';
            if (ImGui::InputText("Runtime Args", argBuf, 511))
            {
                profile.Arguments = argBuf;
            }
        }
        else if (selectedCategory == 1) // Scripting
        {
            ImGui::TextDisabled("Scripting Settings");
            char moduleNameBuf[256];
            strncpy(moduleNameBuf, config.Scripting.ModuleName.c_str(), 255);
            moduleNameBuf[255] = '\0';
            if (ImGui::InputText("Module Name", moduleNameBuf, 255))
            {
                config.Scripting.ModuleName = moduleNameBuf;
            }

            char moduleDirBuf[512];
            strncpy(moduleDirBuf, config.Scripting.ModuleDirectory.string().c_str(), 511);
            moduleDirBuf[511] = '\0';
            if (ImGui::InputText("Module Directory", moduleDirBuf, 511))
            {
                config.Scripting.ModuleDirectory = moduleDirBuf;
            }
            ImGui::SameLine();
            if (ImGui::Button("...###ModuleDirBrowse"))
            {
                auto result = CHEngine::Platform::PickFolder();
                if (result)
                {
                    config.Scripting.ModuleDirectory = Project::GetRelativePath(result->string());
                }
            }

            ImGui::Checkbox("Auto Load Module", &config.Scripting.AutoLoad);
        }
        else if (selectedCategory == 2) // Physics
        {
            ImGui::TextDisabled("Physics Settings");
            ImGui::DragFloat("World Gravity", &config.Physics.Gravity, 0.1f);
            ImGui::DragFloat("Fixed Timestep", &config.Physics.FixedTimestep, 0.001f, 0.001f, 0.1f, "%.4f");
        }
        else if (selectedCategory == 3) // Window
        {
            ImGui::TextDisabled("Window Settings");
            ImGui::DragInt("Width", &config.Window.Width, 1, 800, 3840);
            ImGui::DragInt("Height", &config.Window.Height, 1, 600, 2160);
            ImGui::Checkbox("VSync", &config.Window.VSync);
            ImGui::Checkbox("Resizable", &config.Window.Resizable);
        }
        else if (selectedCategory == 4) // Editor
        {
            ImGui::TextDisabled("Editor Settings");
            ImGui::DragFloat("Camera Speed", &config.Editor.CameraMoveSpeed, 0.1f, 0.1f, 100.0f);

            ImGui::Separator();
            ImGui::TextDisabled("Auto-Save Settings");
            auto& editorConfig = EditorLayer::Get().GetConfig();
            ImGui::Checkbox("Enable Auto-Save", &editorConfig.AutoSaveEnabled);
            ImGui::DragFloat("Auto-Save Interval (s)", &editorConfig.AutoSaveInterval, 1.0f, 10.0f, 3600.0f);
        }
        else if (selectedCategory == 5) // Rendering
        {
            ImGui::TextDisabled("Rendering Settings");
            ImGui::DragFloat("Ambient Intensity", &config.Render.AmbientIntensity, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Default Exposure", &config.Render.DefaultExposure, 0.01f, 0.0f, 10.0f);

            ImGui::Separator();
            ImGui::TextDisabled("Visual Quality");
            ImGui::Checkbox("Generate Mipmaps", &config.Texture.GenerateMipmaps);
            
            bool highQuality = (config.Texture.Filter >= TextureFilter::Anisotropic16x);
            if (ImGui::Checkbox("High Quality Textures", &highQuality))
            {
                config.Texture.Filter = highQuality ? TextureFilter::Anisotropic16x : TextureFilter::Bilinear;
            }
        }

        ImGui::Columns(1);
        ImGui::Separator();
        
        if (ImGui::Button("Save Project Settings"))
        {
            ProjectSerializer serializer(project);
            std::filesystem::path path = project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
            serializer.Serialize(path);
        }

    }
    ImGui::End();
}
} // namespace CHEngine
