#include "engine/platform/utils/file_dialogs.h"
#include "project_settings_panel.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "layer.h"
#include "engine/core/platform.h"
#include "engine/project/project.h"
#include "project_manager.h"
#include "project/project_serializer.h"
#include "imgui.h"

namespace Chained
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
        const char* categories[] = {ICON_FA_GEARS " General",    ICON_FA_CODE " Scripting",
                                    ICON_FA_CUBES " Physics",    ICON_FA_WINDOW_RESTORE " Window",
                                    ICON_FA_CAMERA " Editor",    ICON_FA_MOUNTAIN_SUN " Rendering",
                                    ICON_FA_VOLUME_HIGH " Audio"};

        ImGui::Columns(2, "ProjectSettingsColumns", true);
        ImGui::SetColumnWidth(0, 180.0f);

        // Sidebar
        for (int i = 0; i < IM_ARRAYSIZE(categories); i++)
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
                auto result = Chained::FileDialogs::OpenFile(filters);
                if (result)
                {
                    config.IconPath = project->GetRelativePathForProject(result->string());
                }
            }

            auto availableScenes = project->GetAvailableScenes();
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

            // (Launch profiles removed from engine config)
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
                auto result = Chained::FileDialogs::PickFolder();
                if (result)
                {
                    config.Scripting.ModuleDirectory = project->GetRelativePathForProject(result->string());
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
            auto& editorSettings = EditorLayer::Get().GetProjectManager().GetEditorSettings();
            
            ImGui::DragFloat("Camera Speed", &editorSettings.CameraMoveSpeed, 0.1f, 0.1f, 100.0f);

            ImGui::Separator();
            ImGui::TextDisabled("Visual Feedback");
            ImGui::Checkbox("Show Grid", &editorSettings.ShowGrid);
            ImGui::Checkbox("Show Gizmos", &editorSettings.ShowGizmos);
            ImGui::Checkbox("Show Selected Wireframe", &editorSettings.ShowSelectedWireframe);

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

            const char* shadowResNames[] = {"512", "1024", "2048", "4096"};
            int currentShadowResIdx = 2; // Default 2048
            if (config.Render.ShadowResolution == 512)
            {
                currentShadowResIdx = 0;
            }
            else if (config.Render.ShadowResolution == 1024)
            {
                currentShadowResIdx = 1;
            }
            else if (config.Render.ShadowResolution == 2048)
            {
                currentShadowResIdx = 2;
            }
            else if (config.Render.ShadowResolution == 4096)
            {
                currentShadowResIdx = 3;
            }

            if (ImGui::Combo("Shadow Resolution", &currentShadowResIdx, shadowResNames, 4))
            {
                if (currentShadowResIdx == 0)
                {
                    config.Render.ShadowResolution = 512;
                }
                else if (currentShadowResIdx == 1)
                {
                    config.Render.ShadowResolution = 1024;
                }
                else if (currentShadowResIdx == 2)
                {
                    config.Render.ShadowResolution = 2048;
                }
                else if (currentShadowResIdx == 3)
                {
                    config.Render.ShadowResolution = 4096;
                }
            }

            const char* aaNames[] = {"None", "2x MSAA", "4x MSAA", "8x MSAA"};
            int currentAAIdx = 0;
            if (config.Render.AntiAliasingSamples == 0)
            {
                currentAAIdx = 0;
            }
            else if (config.Render.AntiAliasingSamples == 2)
            {
                currentAAIdx = 1;
            }
            else if (config.Render.AntiAliasingSamples == 4)
            {
                currentAAIdx = 2;
            }
            else if (config.Render.AntiAliasingSamples == 8)
            {
                currentAAIdx = 3;
            }

            if (ImGui::Combo("Anti-Aliasing", &currentAAIdx, aaNames, 4))
            {
                if (currentAAIdx == 0)
                {
                    config.Render.AntiAliasingSamples = 0;
                }
                else if (currentAAIdx == 1)
                {
                    config.Render.AntiAliasingSamples = 2;
                }
                else if (currentAAIdx == 2)
                {
                    config.Render.AntiAliasingSamples = 4;
                }
                else if (currentAAIdx == 3)
                {
                    config.Render.AntiAliasingSamples = 8;
                }
            }

            ImGui::Checkbox("Enable SSAO", &config.Render.EnableSSAO);
            ImGui::Checkbox("Enable Bloom", &config.Render.EnableBloom);

            ImGui::Separator();
            ImGui::TextDisabled("Textures");
            ImGui::Checkbox("Generate Mipmaps", &config.Texture.GenerateMipmaps);

            const char* filterNames[] = {"None", "Bilinear", "Trilinear", "Aniso 4x", "Aniso 8x", "Aniso 16x"};
            int currentFilter = (int)config.Texture.Filter;
            if (ImGui::Combo("Texture Filtering", &currentFilter, filterNames, 6))
            {
                config.Texture.Filter = (TextureFilter)currentFilter;
            }
        }
        else if (selectedCategory == 6) // Audio (Added case)
        {
            ImGui::TextDisabled("Audio Settings");
            ImGui::SliderFloat("Master Volume", &config.Audio.MasterVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("Music Volume", &config.Audio.MusicVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("SFX Volume", &config.Audio.SFXVolume, 0.0f, 1.0f);
        }

        ImGui::Columns(1);
        ImGui::Separator();

        if (ImGui::Button("Save Project Settings"))
        {
            std::filesystem::path path =
                project->GetProjectDirectoryForProject() / (project->GetConfig().Name + ".chproject");
            auto& editorSettings = EditorLayer::Get().GetProjectManager().GetEditorSettings();
            EditorProjectSerializer::Serialize(project, editorSettings, path);
        }
    }
    ImGui::End();
}
} // namespace Chained
