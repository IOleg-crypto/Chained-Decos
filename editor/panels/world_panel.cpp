#include "world_panel.h"
#include "editor/editor_layer.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/environment.h"
#include "engine/graphics/importers/environment_importer.h"
#include "engine/scene/project.h"
#include "engine/core/dialogs.h"
#include "scene/scene.h"
#include <filesystem>

namespace CHEngine
{

WorldPanel::WorldPanel()
{
    m_Name = "World Settings";
}

void WorldPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

    ImGui::Begin(m_Name.c_str(), &m_IsOpen);

    if (!m_Context)
    {
        ImGui::Text("No active scene.");
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Scene Background", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        const char* bgModes[] = {"Solid Color", "Texture", "3D Environment"};
        int currentMode = (int)m_Context->GetSettings().Mode;
        if (ImGui::Combo("Background Mode", &currentMode, bgModes, 3))
        {
            m_Context->GetSettings().Mode = (BackgroundMode)currentMode;
        }

        if (m_Context->GetSettings().Mode == BackgroundMode::Color)
        {
            Color bgColor = m_Context->GetSettings().BackgroundColor;
            float c[4] = {bgColor.r/255.f, bgColor.g/255.f, bgColor.b/255.f, bgColor.a/255.f};
            if (ImGui::ColorEdit4("Background Color", c))
            {
                m_Context->GetSettings().BackgroundColor = {(uint8_t)(c[0]*255),(uint8_t)(c[1]*255),(uint8_t)(c[2]*255),(uint8_t)(c[3]*255)};
            }
        }
        else if (m_Context->GetSettings().Mode == BackgroundMode::Texture)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, m_Context->GetSettings().BackgroundTexturePath.c_str(), sizeof(buffer) - 1);
            if (ImGui::InputText("Texture Path", buffer, sizeof(buffer)))
            {
                m_Context->GetSettings().BackgroundTexturePath = buffer;
            }

            ImGui::SameLine();
            if (ImGui::Button("...##BG"))
            {
                std::vector<FileDialogFilter> filters = {{"Textures", "png,jpg,tga,bmp"}};
                auto result = Dialogs::OpenFile(filters);
                if (result)
                {
                    std::filesystem::path p = *result;
                    if (Project::GetActive())
                    {
                        m_Context->GetSettings().BackgroundTexturePath =
                            std::filesystem::relative(p, Project::GetAssetDirectory()).string();
                    }
                    else
                    {
                        m_Context->GetSettings().BackgroundTexturePath = p.filename().string();
                    }
                }
            }
        }

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
    }

    if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        if (auto project = Project::GetActive())
        {
            auto& settings = project->GetConfig().Physics;
            
            ImGui::DragFloat("Gravity", &settings.Gravity, 0.1f);
            
            float fps = 1.0f / settings.FixedTimestep;
            if (ImGui::DragFloat("Fixed FPS", &fps, 1.0f, 10.0f, 240.0f))
            {
                settings.FixedTimestep = 1.0f / fps;
            }
        }
        else
        {
            ImGui::TextDisabled("No active project to store physics settings.");
        }

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
    }

    ImGui::Separator();

    auto env = m_Context->GetSettings().Environment;

    if (!readOnly)
    {
        if (ImGui::Button("Load Environment..."))
        {
            std::vector<FileDialogFilter> filters = {{"Environment", "chenv"}};
            auto result = Dialogs::OpenFile(filters);
            if (result)
            {
                if (auto project = Project::GetActive())
                {
                    m_Context->GetSettings().Environment = AssetManager::Get().Get<EnvironmentAsset>(result->string());
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("New Environment"))
        {
            std::vector<FileDialogFilter> filters = {{"Environment", "chenv"}};
            auto result = Dialogs::SaveFile(filters);
            if (result)
            {
                auto newEnv = std::make_shared<EnvironmentAsset>();
                newEnv->SetPath(result->string());
                m_Context->GetSettings().Environment = newEnv;
            }
        }
    }

    if (env)
    {
        if (!readOnly)
        {
            ImGui::Separator();
            ImGui::Text("Active: %s", env->GetPath().c_str());

            if (ImGui::Button("Save"))
            {
                EnvironmentImporter::SaveEnvironment(env, env->GetPath());
            }
        }

        DrawEnvironmentSettings(env, readOnly);
    }

    ImGui::End();
}

void WorldPanel::DrawEnvironmentSettings(std::shared_ptr<EnvironmentAsset> env, bool readOnly)
{
    auto& settings = env->GetSettings();

    if (ImGui::CollapsingHeader("Global Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        ImGui::DragFloat3("Direction", &settings.Lighting.Direction.x, 0.01f, -1.0f, 1.0f);

        float color[4] = {settings.Lighting.LightColor.r/255.f, settings.Lighting.LightColor.g/255.f,
                           settings.Lighting.LightColor.b/255.f, settings.Lighting.LightColor.a/255.f};
        if (ImGui::ColorEdit4("Light Color", color))
        {
            settings.Lighting.LightColor = {(uint8_t)(color[0]*255),(uint8_t)(color[1]*255),(uint8_t)(color[2]*255),(uint8_t)(color[3]*255)};
        }

        ImGui::DragFloat("Ambient", &settings.Lighting.Ambient, 0.005f, 0.0f, 2.0f);
        ImGui::DragFloat("Exposure", &settings.Lighting.Exposure, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Gamma", &settings.Lighting.Gamma, 0.01f, 1.0f, 4.0f);

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
    }

    if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, settings.Skybox.TexturePath.c_str(), sizeof(buffer) - 1);

        if (ImGui::InputText("Texture", buffer, sizeof(buffer)))
        {
            settings.Skybox.TexturePath = buffer;
        }

        ImGui::SameLine();
        if (ImGui::Button("...##Sky"))
        {
            std::vector<FileDialogFilter> filters = {{"Textures/HDR", "png,jpg,hdr"}};
            auto result = Dialogs::OpenFile(filters);
            if (result)
            {
                std::filesystem::path p = *result;
                if (Project::GetActive())
                {
                    settings.Skybox.TexturePath = std::filesystem::relative(p, Project::GetAssetDirectory()).string();
                }
                else
                {
                    settings.Skybox.TexturePath = p.filename().string();
                }
            }
        }

        ImGui::SliderInt("Mapping Mode", &settings.Skybox.Mode, 0, 2);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("0: Equirectangular (Sphere)\n1: Horizontal Cross (Cube)\n2: Cubemap (GPU Generated)");
        }
        ImGui::DragFloat("Exposure", &settings.Skybox.Exposure, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Brightness", &settings.Skybox.Brightness, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("Contrast", &settings.Skybox.Contrast, 0.01f, 0.0f, 5.0f);

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
    }

    if (ImGui::CollapsingHeader("Fog Visibility", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        auto& fog = settings.Fog;
        ImGui::Checkbox("Fog Enabled", &fog.Enabled);

        float fogColor[4] = {fog.FogColor.r/255.f, fog.FogColor.g/255.f, fog.FogColor.b/255.f, fog.FogColor.a/255.f};
        if (ImGui::ColorEdit4("Fog Color", fogColor))
        {
            fog.FogColor = {(uint8_t)(fogColor[0]*255),(uint8_t)(fogColor[1]*255),(uint8_t)(fogColor[2]*255),(uint8_t)(fogColor[3]*255)};
        }

        const char* fogModes[] = { "Linear", "Exponential", "Exponential Squared" };
        ImGui::Combo("Fog Mode", &fog.Mode, fogModes, 3);
        ImGui::DragFloat("Density", &fog.Density, 0.0001f, 0.0f, 0.1f, "%.4f");
        ImGui::DragFloat("Start", &fog.Start, 1.0f, 0.0f, 10000.0f);
        ImGui::DragFloat("End", &fog.End, 1.0f, 0.0f, 10000.0f);

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
    }
}

} // namespace CHEngine
