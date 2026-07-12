#include "project_settings_panel.h"
#include "engine/core/platform.h"
#include "engine/platform/dialogs/file_dialogs.h"
#include "engine/project/project.h"
#include "imgui.h"
#include "layer.h"
#include "project/project_serializer.h"
#include "project_manager.h"
#include "thirdparty/IconsFontAwesome6.h"
#include <filesystem>
#include <misc/cpp/imgui_stdlib.h>

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
        auto& editorSettings = EditorLayer::Get().GetProjectManager().GetEditorSettings();
        auto& editorConfig = EditorLayer::Get().GetConfig();

        static int selectedCategory = 0;
        const char* categories[] = {ICON_FA_GEARS " General",     ICON_FA_CODE " Scripting",
                                    ICON_FA_CUBES " Physics",     ICON_FA_WINDOW_RESTORE " Window",
                                    ICON_FA_CAMERA " Editor",     ICON_FA_MOUNTAIN_SUN " Rendering",
                                    ICON_FA_VOLUME_HIGH " Audio", ICON_FA_CAMERA " Camera (Edit Mode)",
                                    ICON_FA_CUBE " Mesh",         ICON_FA_PLAY " Runtime"};

        // Two-column layout: sidebar left, content right
        ImGui::Columns(2, "ProjectSettingsColumns", true);

        static bool widthSet = false;
        if (!widthSet)
        {
            ImGui::SetColumnWidth(0, 200.0f);
            widthSet = true;
        }

        // --- Left sidebar ---
        ImGui::BeginChild("SettingsSidebar", ImVec2(0, 0), ImGuiChildFlags_NavFlattened);
        for (int i = 0; i < IM_ARRAYSIZE(categories); i++)
        {
            if (ImGui::Selectable(categories[i], selectedCategory == i, ImGuiSelectableFlags_DontClosePopups))
            {
                selectedCategory = i;
            }
        }
        ImGui::EndChild();

        ImGui::NextColumn();

        // --- Right content panel ---
        ImGui::BeginChild("SettingsContent", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                          ImGuiChildFlags_NavFlattened);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

        if (selectedCategory == 0) // General
        {
            ImGui::TextDisabled("General Settings");
            ImGui::Separator();
            ImGui::Spacing();

            char nameBuf[256];
            snprintf(nameBuf, sizeof(nameBuf), "%s", config.Name.c_str());
            if (ImGui::InputText("Project Name", nameBuf, sizeof(nameBuf)))
            {
                config.Name = nameBuf;
            }

            char iconBuf[512];
            snprintf(iconBuf, sizeof(iconBuf), "%s", config.IconPath.c_str());
            if (ImGui::InputText("Icon Path", iconBuf, sizeof(iconBuf)))
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
        }
        else if (selectedCategory == 1) // Scripting
        {
            ImGui::TextDisabled("Scripting Settings");
            ImGui::Separator();
            ImGui::Spacing();

            char moduleNameBuf[256];
            snprintf(moduleNameBuf, sizeof(moduleNameBuf), "%s", config.Scripting.ModuleName.c_str());
            if (ImGui::InputText("Module Name", moduleNameBuf, sizeof(moduleNameBuf)))
            {
                config.Scripting.ModuleName = moduleNameBuf;
            }

            char moduleDirBuf[512];
            snprintf(moduleDirBuf, sizeof(moduleDirBuf), "%s", config.Scripting.ModuleDirectory.string().c_str());
            if (ImGui::InputText("Module Directory", moduleDirBuf, sizeof(moduleDirBuf)))
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
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::DragFloat("World Gravity", &config.Physics.Gravity, 0.1f);
            ImGui::DragFloat("Fixed Timestep", &config.Physics.FixedTimestep, 0.001f, 0.001f, 0.1f, "%.4f");
        }
        else if (selectedCategory == 3) // Window
        {
            ImGui::TextDisabled("Window Settings");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::DragInt("Width", &config.Window.Width, 1, 800, 3840);
            ImGui::DragInt("Height", &config.Window.Height, 1, 600, 2160);
            ImGui::Checkbox("VSync", &config.Window.VSync);
            ImGui::Checkbox("Resizable", &config.Window.Resizable);
        }
        else if (selectedCategory == 4) // Editor
        {
            ImGui::TextDisabled("Auto-Save Settings");
            ImGui::Checkbox("Enable Auto-Save", &editorConfig.AutoSaveEnabled);
            ImGui::DragFloat("Auto-Save Interval (s)", &editorConfig.AutoSaveInterval, 1.0f, 10.0f, 3600.0f);
        }
        else if (selectedCategory == 5) // Rendering
        {
            ImGui::TextDisabled("Rendering Settings");
            ImGui::Separator();
            ImGui::Spacing();

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
        else if (selectedCategory == 6) // Audio
        {
            ImGui::TextDisabled("Audio Settings");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SliderFloat("Master Volume", &config.Audio.MasterVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("Music Volume", &config.Audio.MusicVolume, 0.0f, 1.0f);
            ImGui::SliderFloat("SFX Volume", &config.Audio.SFXVolume, 0.0f, 1.0f);
        }
        else if (selectedCategory == 7) // Camera Settings (Edit Mode)
        {
            ImGui::TextDisabled("Camera Settings");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SliderFloat("Move Speed", &editorSettings.CameraMoveSpeed, 0.1f, 100.0f, "%.1f");
            ImGui::SliderFloat("Boost Multiplier", &editorSettings.CameraBoostMultiplier, 1.0f, 10.0f, "%.1f");

            ImGui::Separator();
            ImGui::Checkbox("Disable Camera Zoom", &editorSettings.DisableCameraZoom);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Prevent mouse wheel from zooming the editor camera");
            }
        }
        else if (selectedCategory == 8) // Mesh
        {
            ImGui::TextDisabled("Mesh Import Settings");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("Import Materials", &config.Mesh.ImportMaterials);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Automatically import materials when loading meshes");
            }

            ImGui::Checkbox("Calculate Tangents", &config.Mesh.CalculateTangents);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Calculate tangent vectors for normal mapping");
            }

            ImGui::Checkbox("Flip UVs", &config.Mesh.FlipUVs);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Flip texture UV coordinates vertically on import");
            }
        }
        else if (selectedCategory == 9) // Runtime
        {
            ImGui::TextDisabled("Runtime Settings");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("Fullscreen", &config.Runtime.Fullscreen);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Start the game in fullscreen mode");
            }

            ImGui::Checkbox("Show Stats Overlay", &config.Runtime.ShowStats);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Display FPS and performance stats in the runtime window");
            }

            ImGui::Checkbox("Enable Console", &config.Runtime.EnableConsole);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Enable the in-game developer console");
            }

            ImGui::DragInt("Target FPS", &config.Runtime.TargetFPS, 1, 0, 240);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("0 = Uncapped framerate");
            }
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();

        ImGui::Columns(1);
        ImGui::Separator();

        float buttonWidth = ImGui::CalcTextSize("Save Project Settings").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth);

        if (ImGui::Button("Save Project Settings"))
        {
            std::filesystem::path path =
                project->GetProjectDirectoryForProject() / (project->GetConfig().Name + ".chproject");
            EditorProjectSerializer::Serialize(project, editorSettings, path);
        }
    }
    ImGui::End();
}
} // namespace Chained
