#include "world_panel.h"
#include "IconsFontAwesome6.h"
#include "editor/editor_layer.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/assets/environment.h"
#include "engine/core/platform.h"
#include <format>
#include "scene/scene.h"
#include "engine/scene/project.h"
#include <filesystem>
#include <fstream>


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

    if (ImGui::CollapsingHeader(ICON_FA_GLOBE "  Scene Background", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(10.0f);
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        const char* bgModes[] = {"Solid Color", "Texture", "3D Environment"};
        int currentMode = (int)m_Context->GetSettings().Mode;

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mode");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##BGMode", &currentMode, bgModes, 3))
        {
            m_Context->GetSettings().Mode = (BackgroundMode)currentMode;
        }

        if (m_Context->GetSettings().Mode == BackgroundMode::Color)
        {
            Color bgColor = m_Context->GetSettings().BackgroundColor;
            float c[4] = {bgColor.r / 255.f, bgColor.g / 255.f, bgColor.b / 255.f, bgColor.a / 255.f};
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Color");
            ImGui::SameLine(100);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::ColorEdit4("##BGColor", c))
            {
                m_Context->GetSettings().BackgroundColor = {(uint8_t)(c[0] * 255), (uint8_t)(c[1] * 255),
                                                            (uint8_t)(c[2] * 255), (uint8_t)(c[3] * 255)};
            }
        }
        else if (m_Context->GetSettings().Mode == BackgroundMode::Texture)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, m_Context->GetSettings().BackgroundTexturePath.c_str(), sizeof(buffer) - 1);

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Texture");
            ImGui::SameLine(100);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 35);
            if (ImGui::InputText("##BGPath", buffer, sizeof(buffer)))
            {
                m_Context->GetSettings().BackgroundTexturePath = buffer;
            }

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_FOLDER_OPEN "##BGSelect"))
            {
                std::vector<FileDialogFilter> filters = {{"Textures", "png,jpg,tga,bmp"}};
                auto result = CHEngine::Platform::OpenFile(filters);
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
        ImGui::Unindent(10.0f);
    }

    if (ImGui::CollapsingHeader(ICON_FA_MICROCHIP "  Physics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(10.0f);
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        if (auto project = Project::GetActive())
        {
            auto& settings = project->GetConfig().Physics;

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Gravity");
            ImGui::SameLine(100);
            ImGui::SetNextItemWidth(-1);
            ImGui::DragFloat("##Gravity", &settings.Gravity, 0.1f);

            float fps = 1.0f / settings.FixedTimestep;
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Fixed FPS");
            ImGui::SameLine(100);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::DragFloat("##FixedFPS", &fps, 1.0f, 10.0f, 240.0f))
            {
                settings.FixedTimestep = 1.0f / fps;
            }
        }
        else
        {
            ImGui::TextDisabled("No active project.");
        }

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
        ImGui::Unindent(10.0f);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto env = m_Context->GetSettings().Environment;

    if (!readOnly)
    {
        if (ImGui::Button(ICON_FA_FILE_IMPORT " Load Environment"))
        {
            std::vector<FileDialogFilter> filters = {{"Environment", "chenv"}};
            auto result = CHEngine::Platform::OpenFile(filters);
            if (result)
            {
                if (auto project = Project::GetActive())
                {
                    auto handle = ServiceLocator::Get<AssetManager>().ResolveToHandle(result->string(), EnvironmentAsset::GetStaticType());
                    m_Context->GetSettings().Environment = ServiceLocator::Get<AssetManager>().Get<EnvironmentAsset>(handle);
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FILE_CIRCLE_PLUS " New"))
        {
            std::vector<FileDialogFilter> filters = {{"Environment", "chenv"}};
            auto result = CHEngine::Platform::SaveFile(filters);
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
            ImGui::TextDisabled(ICON_FA_FILE_SIGNATURE " %s",
                                std::filesystem::path(env->GetPath()).filename().string().c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);

            if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save"))
            {
                const auto& settings = env->GetSettings();

                YAML::Emitter out;
                out << YAML::BeginMap;
                out << YAML::Key << "Environment" << YAML::BeginMap;
                out << YAML::Key << "Lighting" << YAML::BeginMap;
                out << YAML::Key << "Direction" << YAML::BeginMap;
                out << YAML::Key << "X" << YAML::Value << settings.Lighting.Direction.x;
                out << YAML::Key << "Y" << YAML::Value << settings.Lighting.Direction.y;
                out << YAML::Key << "Z" << YAML::Value << settings.Lighting.Direction.z;
                out << YAML::EndMap;

                out << YAML::Key << "LightColor" << YAML::BeginMap;
                out << YAML::Key << "R" << YAML::Value << (int)settings.Lighting.LightColor.r;
                out << YAML::Key << "G" << YAML::Value << (int)settings.Lighting.LightColor.g;
                out << YAML::Key << "B" << YAML::Value << (int)settings.Lighting.LightColor.b;
                out << YAML::Key << "A" << YAML::Value << (int)settings.Lighting.LightColor.a;
                out << YAML::EndMap;
                out << YAML::Key << "Ambient" << YAML::Value << settings.Lighting.Ambient;
                out << YAML::Key << "Exposure" << YAML::Value << settings.Lighting.Exposure;
                out << YAML::Key << "Gamma" << YAML::Value << settings.Lighting.Gamma;
                out << YAML::EndMap;

                out << YAML::Key << "Skybox" << YAML::BeginMap;
                out << YAML::Key << "TexturePath" << YAML::Value << settings.Skybox.TexturePath;
                out << YAML::Key << "Exposure" << YAML::Value << settings.Skybox.Exposure;
                out << YAML::Key << "Brightness" << YAML::Value << settings.Skybox.Brightness;
                out << YAML::Key << "Contrast" << YAML::Value << settings.Skybox.Contrast;
                out << YAML::EndMap;

                out << YAML::Key << "Fog" << YAML::BeginMap;
                out << YAML::Key << "Enabled" << YAML::Value << settings.Fog.Enabled;
                out << YAML::Key << "Color" << YAML::BeginMap;
                out << YAML::Key << "R" << YAML::Value << (int)settings.Fog.FogColor.r;
                out << YAML::Key << "G" << YAML::Value << (int)settings.Fog.FogColor.g;
                out << YAML::Key << "B" << YAML::Value << (int)settings.Fog.FogColor.b;
                out << YAML::Key << "A" << YAML::Value << (int)settings.Fog.FogColor.a;
                out << YAML::EndMap;
                out << YAML::Key << "Density" << YAML::Value << settings.Fog.Density;
                out << YAML::Key << "Start" << YAML::Value << settings.Fog.Start;
                out << YAML::Key << "End" << YAML::Value << settings.Fog.End;
                out << YAML::EndMap;

                out << YAML::EndMap;

                out << YAML::EndMap;
                out << YAML::EndMap;

                std::string path = env->GetPath();
                std::filesystem::path fullPath(path);
                std::filesystem::create_directories(fullPath.parent_path());
                std::ofstream fout(fullPath);
                if (fout.is_open())
                {
                    fout << out.c_str();
                }
            }
        }

        DrawEnvironmentSettings(env, readOnly);
    }

    ImGui::End();
}

void WorldPanel::DrawEnvironmentSettings(std::shared_ptr<EnvironmentAsset> env, bool readOnly)
{
    auto& settings = env->GetSettings();

    if (ImGui::CollapsingHeader(ICON_FA_SUN "  Global Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("GlobalLighting");
        ImGui::Indent(10.0f);
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        auto drawDragFloat = [&](const char* label, float* value, float speed, float min, float max,
                                 const char* format = "%.3f") {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", label);
            ImGui::SameLine(100);
            ImGui::SetNextItemWidth(-1);
            std::string id = "##";
            id += label;
            return ImGui::DragFloat(id.c_str(), value, speed, min, max, format);
        };

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Direction");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloat3("##Direction", &settings.Lighting.Direction.x, 0.01f, -1.0f, 1.0f);

        float color[4] = {settings.Lighting.LightColor.r / 255.f, settings.Lighting.LightColor.g / 255.f,
                          settings.Lighting.LightColor.b / 255.f, settings.Lighting.LightColor.a / 255.f};
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Color");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::ColorEdit4("##LightColor", color))
        {
            settings.Lighting.LightColor = {(uint8_t)(color[0] * 255), (uint8_t)(color[1] * 255),
                                            (uint8_t)(color[2] * 255), (uint8_t)(color[3] * 255)};
        }

        drawDragFloat("Ambient", &settings.Lighting.Ambient, 0.005f, 0.0f, 2.0f);
        drawDragFloat("Exposure", &settings.Lighting.Exposure, 0.01f, 0.0f, 10.0f);
        drawDragFloat("Gamma", &settings.Lighting.Gamma, 0.01f, 1.0f, 4.0f);

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
        ImGui::Unindent(10.0f);
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader(ICON_FA_CLOUD "  Skybox", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("Skybox");
        ImGui::Indent(10.0f);
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        auto drawDragFloat = [&](const char* label, float* value, float speed, float min, float max,
                                 const char* format = "%.3f") {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", label);
            ImGui::SameLine(100);
            ImGui::SetNextItemWidth(-1);
            std::string id = "##";
            id += label;
            return ImGui::DragFloat(id.c_str(), value, speed, min, max, format);
        };

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, settings.Skybox.TexturePath.c_str(), sizeof(buffer) - 1);

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Texture");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 35);
        if (ImGui::InputText("##SkyPath", buffer, sizeof(buffer)))
        {
            settings.Skybox.TexturePath = buffer;
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FOLDER_OPEN "##SkySelect"))
        {
            std::vector<FileDialogFilter> filters = {{"Textures/HDR", "png,jpg,hdr"}};
            auto result = CHEngine::Platform::OpenFile(filters);
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

        const char* mapModes[] = {"Sphere", "Cross", "Cubemap"};
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mapping");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##MapMode", &settings.Skybox.Mode, mapModes, 3);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Sphere: Equirectangular\nCross: Horizontal Cross\nCubemap: GPU Generated");
        }

        drawDragFloat("Exposure", &settings.Skybox.Exposure, 0.01f, 0.0f, 10.0f);
        drawDragFloat("Brightness", &settings.Skybox.Brightness, 0.01f, -2.0f, 2.0f);
        drawDragFloat("Contrast", &settings.Skybox.Contrast, 0.01f, 0.0f, 5.0f);

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
        ImGui::Unindent(10.0f);
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader(ICON_FA_SMOG "  Fog Visibility", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("FogVisibility");
        ImGui::Indent(10.0f);
        if (readOnly)
        {
            ImGui::BeginDisabled();
        }

        auto drawDragFloat = [&](const char* label, float* value, float speed, float min, float max,
                                 const char* format = "%.3f") {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", label);
            ImGui::SameLine(100);
            ImGui::SetNextItemWidth(-1);
            std::string id = "##";
            id += label;
            return ImGui::DragFloat(id.c_str(), value, speed, min, max, format);
        };

        auto& fog = settings.Fog;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Enabled");
        ImGui::SameLine(100);
        ImGui::Checkbox("##FogEnabled", &fog.Enabled);

        float fogColor[4] = {fog.FogColor.r / 255.f, fog.FogColor.g / 255.f, fog.FogColor.b / 255.f,
                             fog.FogColor.a / 255.f};
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Color");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::ColorEdit4("##FogColor", fogColor))
        {
            fog.FogColor = {(uint8_t)(fogColor[0] * 255), (uint8_t)(fogColor[1] * 255), (uint8_t)(fogColor[2] * 255),
                            (uint8_t)(fogColor[3] * 255)};
        }

        const char* fogModes[] = {"Linear", "Exponential", "Exponential Squared"};
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mode");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##FogMode", &fog.Mode, fogModes, 3);

        drawDragFloat("Density", &fog.Density, 0.0001f, 0.0f, 0.1f, "%.4f");
        drawDragFloat("Start", &fog.Start, 1.0f, 0.0f, 10000.0f);
        drawDragFloat("End", &fog.End, 1.0f, 0.0f, 10000.0f);

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
        ImGui::Unindent(10.0f);
        ImGui::PopID();
    }
}

} // namespace CHEngine
