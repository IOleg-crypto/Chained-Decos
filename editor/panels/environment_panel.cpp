#include "environment_panel.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/render.h"
#include "engine/scene/project.h"
#include <filesystem>
#include <imgui.h>
#include <nfd.h>

namespace CHEngine
{

EnvironmentPanel::EnvironmentPanel()
{
    m_Name = "Environment";
}

void EnvironmentPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
        return;

    ImGui::Begin(m_Name.c_str(), &m_IsOpen);

    if (!m_Context)
    {
        ImGui::Text("No active scene.");
        ImGui::End();
        return;
    }

    if (m_Context->GetType() == SceneType::SceneUI)
    {
        ImGui::TextDisabled("UI Scene Context");
        ImGui::Separator();
        ImGui::TextWrapped("Global 3D lighting and skybox settings are disabled for UI Scenes.");

        if (ImGui::Button("Convert to 3D Scene"))
        {
            m_Context->SetType(SceneType::Scene3D);
        }

        ImGui::End();
        return;
    }

    auto env = m_Context->GetEnvironment();

    if (!readOnly)
    {
        if (ImGui::Button("Load Environment..."))
        {
            nfdu8char_t *outPath = NULL;
            nfdu8filteritem_t filterList[1] = {{"Environment", "chenv"}};
            nfdresult_t result = NFD_OpenDialog(&outPath, filterList, 1, NULL);
            if (result == NFD_OKAY)
            {
                m_Context->SetEnvironment(Assets::LoadEnvironment(outPath));
                NFD_FreePath(outPath);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("New Environment"))
        {
            auto newEnv = std::make_shared<EnvironmentAsset>();
            newEnv->SetPath("environments/new_environment.chenv");
            m_Context->SetEnvironment(newEnv);
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
                env->Save(env->GetPath());
            }
        }

        DrawEnvironmentSettings(env, readOnly);
    }
    else
    {
        ImGui::Text("No Environment Asset assigned.");
        if (!readOnly)
        {
            if (ImGui::Button("Create Default"))
            {
                auto defaultEnv = std::make_shared<EnvironmentAsset>();
                defaultEnv->SetPath("assets/environments/default.chenv");
                m_Context->SetEnvironment(defaultEnv);
            }
        }
    }

    if (m_DebugFlags && m_Context->GetType() == SceneType::Scene3D)
    {
        ImGui::Separator();
        ImGui::Text("Debug Rendering");
        ImGui::Checkbox("Colliders", &m_DebugFlags->DrawColliders);
        ImGui::Checkbox("Lights", &m_DebugFlags->DrawLights);
        ImGui::Checkbox("Spawn Zones", &m_DebugFlags->DrawSpawnZones);
        ImGui::Checkbox("Draw Grid", &m_DebugFlags->DrawGrid);
    }

    ImGui::End();
}

void EnvironmentPanel::DrawEnvironmentSettings(std::shared_ptr<EnvironmentAsset> env, bool readOnly)
{
    auto &settings = env->GetSettings();

    if (ImGui::CollapsingHeader("Global Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (readOnly)
            ImGui::BeginDisabled();

        ImGui::DragFloat3("Direction", &settings.LightDirection.x, 0.01f, -1.0f, 1.0f);

        float color[4] = {settings.LightColor.r / 255.0f, settings.LightColor.g / 255.0f,
                          settings.LightColor.b / 255.0f, settings.LightColor.a / 255.0f};
        if (ImGui::ColorEdit4("Color", color))
        {
            settings.LightColor.r = (unsigned char)(color[0] * 255.0f);
            settings.LightColor.g = (unsigned char)(color[1] * 255.0f);
            settings.LightColor.b = (unsigned char)(color[2] * 255.0f);
            settings.LightColor.a = (unsigned char)(color[3] * 255.0f);
        }

        ImGui::DragFloat("Ambient", &settings.AmbientIntensity, 0.005f, 0.0f, 2.0f);

        if (readOnly)
            ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (readOnly)
            ImGui::BeginDisabled();

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, settings.Skybox.TexturePath.c_str(), sizeof(buffer) - 1);

        if (ImGui::InputText("Texture", buffer, sizeof(buffer)))
        {
            settings.Skybox.TexturePath = buffer;
        }

        ImGui::SameLine();
        if (ImGui::Button("..."))
        {
            nfdu8char_t *outPath = NULL;
            nfdu8filteritem_t filterList[1] = {{"Textures/HDR", "png,jpg,hdr"}};
            nfdresult_t result = NFD_OpenDialog(&outPath, filterList, 1, NULL);
            if (result == NFD_OKAY)
            {
                std::filesystem::path p = outPath;
                if (Project::GetActive())
                    settings.Skybox.TexturePath =
                        std::filesystem::relative(p, Project::GetAssetDirectory()).string();
                else
                    settings.Skybox.TexturePath = p.filename().string();
                NFD_FreePath(outPath);
            }
        }

        ImGui::DragFloat("Exposure", &settings.Skybox.Exposure, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Brightness", &settings.Skybox.Brightness, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("Contrast", &settings.Skybox.Contrast, 0.01f, 0.0f, 5.0f);

        if (readOnly)
            ImGui::EndDisabled();
    }
}

} // namespace CHEngine
