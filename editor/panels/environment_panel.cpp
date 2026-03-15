#include "environment_panel.h"
#include "editor/editor_layer.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/environment_importer.h"
#include "engine/scene/project.h"
#include "imgui.h"
#include "nfd.hpp"
#include "scene/scene.h"
#include <filesystem>

namespace CHEngine
{

EnvironmentPanel::EnvironmentPanel()
{
    m_Name = "Environment";
}

void EnvironmentPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }

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
            if (ImGui::Button("..."))
            {
                nfdu8char_t* outPath = NULL;
                nfdu8filteritem_t filterList[1] = {{"Textures", "png,jpg,tga,bmp"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterList, 1, NULL);
                if (result == NFD_OKAY)
                {
                    std::filesystem::path p = outPath;
                    if (Project::GetActive())
                    {
                        m_Context->GetSettings().BackgroundTexturePath =
                            std::filesystem::relative(p, Project::GetAssetDirectory()).string();
                    }
                    else
                    {
                        m_Context->GetSettings().BackgroundTexturePath = p.filename().string();
                    }
                    NFD_FreePath(outPath);
                }
            }
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
            nfdu8char_t* outPath = NULL;
            nfdu8filteritem_t filterList[1] = {{"Environment", "chenv"}};
            nfdresult_t result = NFD_OpenDialog(&outPath, filterList, 1, NULL);
            if (result == NFD_OKAY)
            {
                if (auto project = Project::GetActive())
                {
                    m_Context->GetSettings().Environment = AssetManager::Get().Get<EnvironmentAsset>(outPath);
                }
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

    // Replaced m_DebugFlags with EditorLayer::Get().GetDebugRenderFlags()
    {
        bool is3D = m_Context->GetSettings().Mode == BackgroundMode::Environment3D;
        ImGui::Separator();
        ImGui::Text("Viewport Tools");

        const char* diagnosticModes[] = {"Normal Render", "Normals visualization", "Lighting only", "Albedo only"};
        int currentDiag = (int)m_Context->GetSettings().DiagnosticMode;
        if (ImGui::Combo("Diagnostic Mode", &currentDiag, diagnosticModes, 4))
        {
            m_Context->GetSettings().DiagnosticMode = (float)currentDiag;
            Renderer::Get().SetDiagnosticMode((float)currentDiag);
        }

        ImGui::Separator();
        ImGui::Text("Debug Rendering");

        if (!is3D)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::CollapsingHeader("Debug Visualization", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& debugFlags = m_Context->GetSettings().DebugFlags;
            ImGui::Checkbox("Colliders", &debugFlags.DrawColliders);
            ImGui::Checkbox("Mesh Hierarchy", &debugFlags.DrawCollisionModelBox);
            ImGui::Checkbox("Lights", &debugFlags.DrawLights);
            ImGui::Checkbox("Spawn Zones", &debugFlags.DrawSpawnZones);
            ImGui::Checkbox("Draw Grid", &debugFlags.DrawGrid);

            if (debugFlags.DrawGrid)
            {
                auto& grid = m_Context->GetSettings().Grid;
                ImGui::Indent(12.0f);
                ImGui::DragInt  ("Slices",  &grid.Slices,  1,    4, 200);
                ImGui::DragFloat("Spacing", &grid.Spacing, 0.1f, 0.1f, 50.0f);
                ImGui::Unindent(12.0f);
            }
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
        if (ImGui::Button("..."))
        {
            nfdu8char_t* outPath = NULL;
            nfdu8filteritem_t filterList[1] = {{"Textures/HDR", "png,jpg,hdr"}};
            nfdresult_t result = NFD_OpenDialog(&outPath, filterList, 1, NULL);
            if (result == NFD_OKAY)
            {
                std::filesystem::path p = outPath;
                if (Project::GetActive())
                {
                    settings.Skybox.TexturePath = std::filesystem::relative(p, Project::GetAssetDirectory()).string();
                }
                else
                {
                    settings.Skybox.TexturePath = p.filename().string();
                }
                NFD_FreePath(outPath);
            }
        }

        ImGui::SliderInt("Mapping Mode", &settings.Skybox.Mode, 0, 2);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("0: Equirectangular (Sphere)\n1: Horizontal Cross (Cube)\n2: Cubemap (GPU Generated)");
        }
        ImGui::TextDisabled("0: Sphere, 1: Cross, 2: Cubemap");

        ImGui::DragFloat("Exposure", &settings.Skybox.Exposure, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Brightness", &settings.Skybox.Brightness, 0.01f, -2.0f, 2.0f);
        ImGui::DragFloat("Contrast", &settings.Skybox.Contrast, 0.01f, 0.0f, 5.0f);

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
    }

    if (ImGui::CollapsingHeader("Fog Visualization", ImGuiTreeNodeFlags_DefaultOpen))
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

        ImGui::DragFloat("Density", &fog.Density, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Start", &fog.Start, 0.1f, 0.0f, 1000.0f);
        ImGui::DragFloat("End", &fog.End, 0.1f, 0.0f, 1000.0f);

        if (readOnly)
        {
            ImGui::EndDisabled();
        }
    }
}

} // namespace CHEngine
