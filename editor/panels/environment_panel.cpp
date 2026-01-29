#include "environment_panel.h"
#include "editor/editor_layer.h"
#include "engine/graphics/render_types.h"
#include "engine/scene/project.h"
#include "filesystem"
#include "imgui.h"
#include "nfd.hpp"
#include "scene/scene.h"

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
    ImGui::PushID(this);

    if (!m_Context)
    {
        ImGui::Text("No active scene.");
        ImGui::PopID();
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Scene Background", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (readOnly)
            ImGui::BeginDisabled();

        const char *bgModes[] = {"Solid Color", "Texture", "3D Environment"};
        int currentMode = (int)m_Context->GetBackgroundMode();
        if (ImGui::Combo("Background Mode", &currentMode, bgModes, 3))
            m_Context->SetBackgroundMode((BackgroundMode)currentMode);

        if (m_Context->GetBackgroundMode() == BackgroundMode::Color)
        {
            Color color = m_Context->GetBackgroundColor();
            float c[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
            if (ImGui::ColorEdit4("Background Color", c))
            {
                m_Context->SetBackgroundColor(
                    {(unsigned char)(c[0] * 255), (unsigned char)(c[1] * 255),
                     (unsigned char)(c[2] * 255), (unsigned char)(c[3] * 255)});
            }
        }
        else if (m_Context->GetBackgroundMode() == BackgroundMode::Texture)
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, m_Context->GetBackgroundTexturePath().c_str(), sizeof(buffer) - 1);
            if (ImGui::InputText("Texture Path", buffer, sizeof(buffer)))
                m_Context->SetBackgroundTexturePath(buffer);

            ImGui::SameLine();
            if (ImGui::Button("..."))
            {
                nfdu8char_t *outPath = NULL;
                nfdu8filteritem_t filterList[1] = {{"Textures", "png,jpg,tga,bmp"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterList, 1, NULL);
                if (result == NFD_OKAY)
                {
                    std::filesystem::path p = outPath;
                    if (Project::GetActive())
                        m_Context->SetBackgroundTexturePath(
                            std::filesystem::relative(p, Project::GetAssetDirectory()).string());
                    else
                        m_Context->SetBackgroundTexturePath(p.filename().string());
                    NFD_FreePath(outPath);
                }
            }
        }

        if (readOnly)
            ImGui::EndDisabled();
    }

    ImGui::Separator();

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
                // m_Context->SetEnvironment(AssetManager::LoadEnvironment(outPath));
                NFD_FreePath(outPath);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("New Environment"))
        {
            NFD::UniquePath outPath;
            nfdfilteritem_t filterItem[1] = {{"Environment", "chenv"}};
            auto result = NFD::SaveDialog(outPath, filterItem, 1, nullptr, "Untitled.chenv");
            if (result == NFD_OKAY)
            {
                auto newEnv = std::make_shared<EnvironmentAsset>();
                newEnv->SetPath(outPath.get());
                m_Context->SetEnvironment(newEnv);
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
                env->Save(env->GetPath());
            }
        }

        DrawEnvironmentSettings(env, readOnly);
    }

    // Replaced m_DebugFlags with EditorLayer::Get().GetDebugRenderFlags()
    {
        bool is3D = m_Context->GetBackgroundMode() == BackgroundMode::Environment3D;
        ImGui::Separator();
        ImGui::Text("Debug Rendering");

        if (!is3D)
            ImGui::BeginDisabled();

        if (ImGui::CollapsingHeader("Debug Visualization", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &debugFlags = EditorLayer::Get().GetDebugRenderFlags();
            ImGui::Checkbox("Colliders", &debugFlags.DrawColliders);
            ImGui::Checkbox("Lights", &debugFlags.DrawLights);
            ImGui::Checkbox("Spawn Zones", &debugFlags.DrawSpawnZones);
            ImGui::Checkbox("Draw Grid", &debugFlags.DrawGrid);
        }

        if (!is3D)
        {
            ImGui::EndDisabled();
            ImGui::TextDisabled("(Hiding 3D Debug in UI Mode)");
        }
    }

    ImGui::PopID();
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
        if (ImGui::ColorEdit4("Light Color", color))
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
