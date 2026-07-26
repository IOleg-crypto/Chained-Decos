#include "world_panel.h"
#include "editor/layer.h"
#include "editor/project/project_serializer.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/environment_asset.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/ui/widget_renderer.h"
#include "engine/imgui/imgui_layer.h"
#include "engine/physics/physics.h"
#include "engine/platform/dialogs/dialogs.h"
#include "engine/project/project.h"
#include "scene/scene.h"
#include "thirdparty/IconsFontAwesome6.h"
#include <filesystem>
#include <fstream>

namespace Chained
{

static void ColorToFloat4(const Color& c, float out[4])
{
    out[0] = c.r / 255.f;
    out[1] = c.g / 255.f;
    out[2] = c.b / 255.f;
    out[3] = c.a / 255.f;
}

static Color Float4ToColor(const float c[4])
{
    return {(uint8_t)(c[0] * 255), (uint8_t)(c[1] * 255), (uint8_t)(c[2] * 255), (uint8_t)(c[3] * 255)};
}

static bool DrawDragFloat(const char* label, float* value, float speed = 0.1f, float min = 0.0f, float max = 0.0f,
                          const char* format = "%.3f")
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(-1);
    ImGui::PushID(label);
    bool changed = ImGui::DragFloat("##v", value, speed, min, max, format);
    ImGui::PopID();
    return changed;
}

static void SaveProjectConfig()
{
    if (auto project = Project::GetActive())
    {
        std::filesystem::path path =
            project->GetProjectDirectoryForProject() / (project->GetConfig().Name + ".chproject");
        EditorProjectSerializer::Serialize(project, path);
    }
}

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

    DrawSceneGeneral(readOnly);
    DrawSceneBackground(readOnly);

    if (m_Context->GetSettings().Mode == BackgroundMode::Environment3D)
    {
        DrawPhysicsSettings(readOnly);
        DrawEnvironmentSection(readOnly);
    }

    ImGui::End();
}

void WorldPanel::DrawSceneGeneral(bool readOnly)
{
    if (!ImGui::CollapsingHeader(ICON_FA_SLIDERS "  Scene General", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Indent(10.0f);
    if (readOnly) ImGui::BeginDisabled();

    auto& settings = m_Context->GetSettings();
    const char* typeModes[] = {"Default (3D)", "UI"};
    int currentType = (int)settings.Type;

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Type");
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##SceneType", &currentType, typeModes, 2))
    {
        settings.Type = (SceneType)currentType;
    }

    if (readOnly) ImGui::EndDisabled();
    ImGui::Unindent(10.0f);
}

void WorldPanel::DrawSceneBackground(bool readOnly)
{
    if (!ImGui::CollapsingHeader(ICON_FA_GLOBE "  Scene Background", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Indent(10.0f);
    if (readOnly) ImGui::BeginDisabled();

    auto& settings = m_Context->GetSettings();
    const char* bgModes[] = {"Solid Color", "Texture", "3D Environment"};
    int currentMode = (int)settings.Mode;

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Mode");
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##BGMode", &currentMode, bgModes, 3))
    {
        settings.Mode = (BackgroundMode)currentMode;
    }

    if (settings.Mode == BackgroundMode::Color)
    {
        float c[4];
        ColorToFloat4(settings.BackgroundColor, c);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Color");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::ColorEdit4("##BGColor", c))
        {
            settings.BackgroundColor = Float4ToColor(c);
        }
    }
    else if (settings.Mode == BackgroundMode::Texture)
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s", settings.BackgroundTexturePath.c_str());

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Texture");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 35);
        if (ImGui::InputText("##BGPath", buffer, sizeof(buffer)))
        {
            settings.BackgroundTexturePath = buffer;
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FOLDER_OPEN "##BGSelect"))
        {
            std::vector<DialogFilter> filters = {{"Textures", "png,jpg,tga,bmp"}};
            auto result = Chained::Dialogs::OpenFile(filters);
            if (result)
            {
                std::filesystem::path p = *result;
                if (Project::GetActive())
                {
                    settings.BackgroundTexturePath =
                        std::filesystem::relative(p, Project::GetAssetDirectory()).string();
                }
                else
                {
                    settings.BackgroundTexturePath = p.filename().string();
                }
            }
        }
    }

    if (readOnly) ImGui::EndDisabled();
    ImGui::Unindent(10.0f);
}

void WorldPanel::DrawPhysicsSettings(bool readOnly)
{
    if (!ImGui::CollapsingHeader(ICON_FA_MICROCHIP "  Physics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Indent(10.0f);
    if (readOnly) ImGui::BeginDisabled();

    if (auto project = Project::GetActive())
    {
        auto& settings = project->GetConfig().Physics;

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Gravity");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloat("##Gravity", &settings.Gravity, 0.1f);

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            SceneState state = EditorLayer::Get().GetSceneState();
            if (state == SceneState::Play || state == SceneState::Simulate)
            {
                if (auto* physics = ServiceLocator::TryGet<Physics>())
                {
                    if (auto* world = physics->GetWorld())
                    {
                        world->SetGravity(settings.Gravity);
                    }
                }
            }
            SaveProjectConfig();
        }

        float fps = 1.0f / settings.FixedTimestep;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Fixed FPS");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::DragFloat("##FixedFPS", &fps, 1.0f, 10.0f, 240.0f))
        {
            settings.FixedTimestep = 1.0f / fps;
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            SaveProjectConfig();
        }
    }
    else
    {
        ImGui::TextDisabled("No active project.");
    }

    if (readOnly) ImGui::EndDisabled();
    ImGui::Unindent(10.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

void WorldPanel::DrawEnvironmentSection(bool readOnly)
{
    auto env = m_Context->GetSettings().Environment;

    if (!readOnly)
    {
        if (ImGui::Button(ICON_FA_FILE_IMPORT " Load Environment"))
        {
            std::vector<DialogFilter> filters = {{"Environment", "chenv"}};
            auto result = Chained::Dialogs::OpenFile(filters);
            if (result)
            {
                if (auto project = Project::GetActive())
                {
                    auto handle = ServiceLocator::Get<AssetManager>()->ResolveToHandle(result->string());
                    m_Context->GetSettings().Environment =
                        ServiceLocator::Get<AssetManager>()->Get<EnvironmentAsset>(result->string());
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FILE_CIRCLE_PLUS " New"))
        {
            std::vector<DialogFilter> filters = {{"Environment", "chenv"}};
            auto result = Chained::Dialogs::SaveFile(filters);
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
                const auto& s = env->GetSettings();

                YAML::Emitter out;
                out << YAML::BeginMap;
                out << YAML::Key << "Environment" << YAML::BeginMap;

                out << YAML::Key << "Lighting" << YAML::BeginMap;
                out << YAML::Key << "Direction" << YAML::BeginMap;
                out << YAML::Key << "X" << YAML::Value << s.Lighting.Direction.x;
                out << YAML::Key << "Y" << YAML::Value << s.Lighting.Direction.y;
                out << YAML::Key << "Z" << YAML::Value << s.Lighting.Direction.z;
                out << YAML::EndMap;
                out << YAML::Key << "LightColor" << YAML::BeginMap;
                out << YAML::Key << "R" << YAML::Value << (int)s.Lighting.LightColor.r;
                out << YAML::Key << "G" << YAML::Value << (int)s.Lighting.LightColor.g;
                out << YAML::Key << "B" << YAML::Value << (int)s.Lighting.LightColor.b;
                out << YAML::Key << "A" << YAML::Value << (int)s.Lighting.LightColor.a;
                out << YAML::EndMap;
                out << YAML::Key << "Ambient" << YAML::Value << s.Lighting.Ambient;
                out << YAML::Key << "Exposure" << YAML::Value << s.Lighting.Exposure;
                out << YAML::Key << "Gamma" << YAML::Value << s.Lighting.Gamma;
                out << YAML::EndMap;

                out << YAML::Key << "Skybox" << YAML::BeginMap;
                out << YAML::Key << "TexturePath" << YAML::Value << s.Skybox.TexturePath;
                out << YAML::Key << "Exposure" << YAML::Value << s.Skybox.Exposure;
                out << YAML::Key << "Brightness" << YAML::Value << s.Skybox.Brightness;
                out << YAML::Key << "Contrast" << YAML::Value << s.Skybox.Contrast;
                out << YAML::EndMap;

                out << YAML::Key << "Fog" << YAML::BeginMap;
                out << YAML::Key << "Enabled" << YAML::Value << s.Fog.Enabled;
                out << YAML::Key << "Color" << YAML::BeginMap;
                out << YAML::Key << "R" << YAML::Value << (int)s.Fog.FogColor.r;
                out << YAML::Key << "G" << YAML::Value << (int)s.Fog.FogColor.g;
                out << YAML::Key << "B" << YAML::Value << (int)s.Fog.FogColor.b;
                out << YAML::Key << "A" << YAML::Value << (int)s.Fog.FogColor.a;
                out << YAML::EndMap;
                out << YAML::Key << "Density" << YAML::Value << s.Fog.Density;
                out << YAML::Key << "Start" << YAML::Value << s.Fog.Start;
                out << YAML::Key << "End" << YAML::Value << s.Fog.End;
                out << YAML::Key << "HeightFalloff" << YAML::Value << s.Fog.HeightFalloff;
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
}

void WorldPanel::DrawEnvironmentSettings(std::shared_ptr<EnvironmentAsset> env, bool readOnly)
{
    auto& settings = env->GetSettings();

    if (ImGui::CollapsingHeader(ICON_FA_SUN "  Global Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("GlobalLighting");
        ImGui::Indent(10.0f);
        if (readOnly) ImGui::BeginDisabled();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Direction");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloat3("##Direction", &settings.Lighting.Direction.x, 0.01f, -1.0f, 1.0f);

        float color[4];
        ColorToFloat4(settings.Lighting.LightColor, color);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Color");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::ColorEdit4("##LightColor", color))
        {
            settings.Lighting.LightColor = Float4ToColor(color);
        }

        DrawDragFloat("Ambient", &settings.Lighting.Ambient, 0.005f, 0.0f, 2.0f);
        DrawDragFloat("Exposure", &settings.Lighting.Exposure, 0.01f, 0.0f, 10.0f);
        DrawDragFloat("Gamma", &settings.Lighting.Gamma, 0.01f, 1.0f, 4.0f);

        if (readOnly) ImGui::EndDisabled();
        ImGui::Unindent(10.0f);
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader(ICON_FA_CLOUD "  Skybox", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("Skybox");
        ImGui::Indent(10.0f);
        if (readOnly) ImGui::BeginDisabled();

        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s", settings.Skybox.TexturePath.c_str());

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
            std::vector<DialogFilter> filters = {{"Textures/HDR", "png,jpg,hdr"}};
            auto result = Chained::Dialogs::OpenFile(filters);
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

        DrawDragFloat("Exposure", &settings.Skybox.Exposure, 0.01f, 0.0f, 10.0f);
        DrawDragFloat("Brightness", &settings.Skybox.Brightness, 0.01f, -2.0f, 2.0f);
        DrawDragFloat("Contrast", &settings.Skybox.Contrast, 0.01f, 0.0f, 5.0f);

        if (readOnly) ImGui::EndDisabled();
        ImGui::Unindent(10.0f);
        ImGui::PopID();
    }

    if (ImGui::CollapsingHeader(ICON_FA_SMOG "  Fog Visibility", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("FogVisibility");
        ImGui::Indent(10.0f);
        if (readOnly) ImGui::BeginDisabled();

        auto& fog = settings.Fog;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Enabled");
        ImGui::SameLine(100);
        ImGui::Checkbox("##FogEnabled", &fog.Enabled);

        float fogColor[4];
        ColorToFloat4(fog.FogColor, fogColor);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Color");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::ColorEdit4("##FogColor", fogColor))
        {
            fog.FogColor = Float4ToColor(fogColor);
        }

        const char* fogModes[] = {"Linear", "Exponential", "Exponential Squared"};
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mode");
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##FogMode", &fog.Mode, fogModes, 3);

        DrawDragFloat("Density", &fog.Density, 0.0001f, 0.0f, 10.f, "%.4f");
        DrawDragFloat("Start", &fog.Start, 1.0f, 0.0f, 10000.0f);
        DrawDragFloat("End", &fog.End, 1.0f, 0.0f, 10000.0f);
        DrawDragFloat("Height Falloff", &fog.HeightFalloff, 0.01f, 0.0f, 1.0f, "%.2f");

        if (readOnly) ImGui::EndDisabled();
        ImGui::Unindent(10.0f);
        ImGui::PopID();
    }
}

} // namespace Chained
