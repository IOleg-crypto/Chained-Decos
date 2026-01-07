#include "environment_panel.h"
#include "engine/scene/components.h"
#include "engine/scene/project.h"
#include <filesystem>
#include <imgui.h>
#include <nfd.h>

namespace CH
{
void EnvironmentPanel::OnImGuiRender(Scene *scene, bool readOnly)
{
    if (!m_IsOpen)
        return;

    ImGui::Begin("Skybox", &m_IsOpen);
    ImGui::BeginDisabled(readOnly);
    if (scene)
    {
        auto &skybox = scene->GetSkybox();
        if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, skybox.TexturePath.c_str());
            if (ImGui::InputText("Texture Path", buffer, sizeof(buffer)))
            {
                skybox.TexturePath = std::string(buffer);
            }
            ImGui::SameLine();
            if (ImGui::Button("...##Skybox"))
            {
                nfdchar_t *outPath = NULL;
                nfdu8filteritem_t filterItem[1] = {{"Image Files", "hdr,png,jpg,tga,bmp"}};
                nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
                if (result == NFD_OKAY)
                {
                    std::filesystem::path fullPath = outPath;
                    std::filesystem::path assetDir = Project::GetAssetDirectory();
                    std::error_code ec;
                    auto relativePath = std::filesystem::relative(fullPath, assetDir, ec);
                    if (!ec)
                        skybox.TexturePath = relativePath.string();
                    else
                        skybox.TexturePath = fullPath.string();
                    NFD_FreePath(outPath);
                }
            }

            ImGui::DragFloat("Exposure##Skybox", &skybox.Exposure, 0.01f, 0.0f, 10.0f);
            ImGui::DragFloat("Brightness##Skybox", &skybox.Brightness, 0.01f, -1.0f, 1.0f);
            ImGui::DragFloat("Contrast##Skybox", &skybox.Contrast, 0.01f, 0.0f, 2.0f);
        }
    }
    else
    {
        ImGui::TextDisabled("No active scene context.");
    }
    ImGui::EndDisabled();
    ImGui::End();
}
} // namespace CH
