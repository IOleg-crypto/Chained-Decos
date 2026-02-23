#include "project_settings_panel.h"
#include "engine/scene/project.h"
#include "engine/scene/project_serializer.h"
#include "imgui.h"
#include "nfd.h"
#include "rlImGui/extras/IconsFontAwesome6.h"
#include <format>

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
        ImGui::PushID(this);
        auto& config = project->GetConfig();

        static int selectedCategory = 0;
        const char* categories[] = {
            ICON_FA_GEARS " General",
            ICON_FA_CODE " Scripting",
            ICON_FA_CUBES " Physics",
            ICON_FA_WINDOW_RESTORE " Window",
            ICON_FA_PLAY " Runtime",
            ICON_FA_CAMERA " Editor",
            ICON_FA_MOUNTAIN_SUN " Rendering"
        };

        ImGui::Columns(2, "ProjectSettingsColumns", true);
        ImGui::SetColumnWidth(0, 200.0f);

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
                nfdu8char_t* outPath = NULL;
                nfdu8filteritem_t filterItem[1] = {{"Image Files", "png,jpg,jpeg"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                if (result == NFD_OKAY)
                {
                    config.IconPath = Project::GetRelativePath(outPath);
                    NFD_FreePath(outPath);
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
            ImGui::Text(ICON_FA_ROCKET " Launch Profiles");

            if (config.LaunchProfiles.empty())
            {
                if (ImGui::Button("Add Default Profile"))
                {
                    LaunchProfile debug;
                    debug.Name = "Debug Runtime";
                    debug.BinaryPath = "${BUILD}/ChainedRuntime.exe";
                    debug.Arguments = "--project \"${PROJECT_FILE}\"";
                    config.LaunchProfiles.push_back(debug);
                }
            }

            for (int i = 0; i < (int)config.LaunchProfiles.size(); i++)
            {
                auto& profile = config.LaunchProfiles[i];
                ImGui::PushID(i);

                bool isActive = (config.ActiveLaunchProfileIndex == i);
                if (ImGui::RadioButton("Active", isActive))
                {
                    config.ActiveLaunchProfileIndex = i;
                }
                ImGui::SameLine();

                if (ImGui::CollapsingHeader(std::format("{}###Header", profile.Name).c_str()))
                {
                    char profileNameBuf[128];
                    strncpy(profileNameBuf, profile.Name.c_str(), 127);
                    profileNameBuf[127] = '\0';
                    if (ImGui::InputText("Profile Name", profileNameBuf, 127))
                    {
                        profile.Name = profileNameBuf;
                    }

                    char pathBuf[512];
                    strncpy(pathBuf, profile.BinaryPath.c_str(), 511);
                    pathBuf[511] = '\0';
                    if (ImGui::InputText("Binary Path", pathBuf, 511))
                    {
                        profile.BinaryPath = pathBuf;
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("..."))
                    {
                        nfdu8char_t* outPath = NULL;
                        nfdu8filteritem_t filterItem[1] = {{"Runtime Executable", "exe"}};
                        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                        if (result == NFD_OKAY)
                        {
                            profile.BinaryPath = outPath;
                            NFD_FreePath(outPath);
                        }
                    }

                    char argBuf[512];
                    strncpy(argBuf, profile.Arguments.c_str(), 511);
                    argBuf[511] = '\0';
                    if (ImGui::InputText("Arguments", argBuf, 511))
                    {
                        profile.Arguments = argBuf;
                    }

                    ImGui::Checkbox("Use Default Project Args", &profile.UseDefaultArgs);

                    if (ImGui::Button("Remove Profile"))
                    {
                        config.LaunchProfiles.erase(config.LaunchProfiles.begin() + i);
                        if (config.ActiveLaunchProfileIndex >= (int)config.LaunchProfiles.size())
                        {
                            config.ActiveLaunchProfileIndex = std::max(0, (int)config.LaunchProfiles.size() - 1);
                        }
                        ImGui::PopID();
                        break;
                    }
                }
                ImGui::PopID();
            }

            if (ImGui::Button(ICON_FA_PLUS " Add New Profile"))
            {
                config.LaunchProfiles.push_back({"New Profile", "", ""});
                if (config.LaunchProfiles.size() == 1)
                {
                    config.ActiveLaunchProfileIndex = 0;
                }
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
                nfdu8char_t* outPath = NULL;
                nfdresult_t result = NFD_PickFolder(&outPath, NULL);
                if (result == NFD_OKAY)
                {
                    config.Scripting.ModuleDirectory = Project::GetRelativePath(outPath);
                    NFD_FreePath(outPath);
                }
            }

            ImGui::Checkbox("Auto Load Module", &config.Scripting.AutoLoad);
        }
        else if (selectedCategory == 2) // Physics
        {
            ImGui::TextDisabled("Physics Settings");
            ImGui::DragFloat("World Gravity", &config.Physics.Gravity, 0.1f, 0.0f, 100.0f);
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
        else if (selectedCategory == 4) // Runtime
        {
            ImGui::TextDisabled("Runtime Settings");
            ImGui::Checkbox("Fullscreen", &config.Runtime.Fullscreen);
            ImGui::Checkbox("Show Stats", &config.Runtime.ShowStats);
            ImGui::Checkbox("Enable Console", &config.Runtime.EnableConsole);
        }
        else if (selectedCategory == 5) // Editor
        {
            ImGui::TextDisabled("Editor Settings");
            ImGui::DragFloat("Camera Speed", &config.Editor.CameraMoveSpeed, 0.1f, 0.1f, 100.0f);
            ImGui::DragFloat("Rotation Speed", &config.Editor.CameraRotationSpeed, 0.01f, 0.01f, 1.0f);
            ImGui::DragFloat("Boost Multiplier", &config.Editor.CameraBoostMultiplier, 0.1f, 1.0f, 20.0f);
        }
        else if (selectedCategory == 6) // Rendering
        {
            ImGui::TextDisabled("Rendering Settings");
            ImGui::DragFloat("Ambient Intensity", &config.Render.AmbientIntensity, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Default Exposure", &config.Render.DefaultExposure, 0.01f, 0.0f, 10.0f);

            ImGui::Separator();
            ImGui::Text("Texture Quality");
            ImGui::Checkbox("Generate Mipmaps", &config.Texture.GenerateMipmaps);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Generates mip chain on texture upload.\nReduces aliasing and bandwidth.\nApplied to newly loaded textures.");
            }

            const char* filterNames[] = {"Point (No Filter)", "Bilinear", "Trilinear", "Anisotropic 4x",
                                          "Anisotropic 8x", "Anisotropic 16x"};
            int currentFilter = (int)config.Texture.Filter;
            if (ImGui::Combo("Texture Filter", &currentFilter, filterNames, 6))
            {
                config.Texture.Filter = (TextureFilter)currentFilter;
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

        ImGui::PopID();
    }
    ImGui::End();
}
} // namespace CHEngine
