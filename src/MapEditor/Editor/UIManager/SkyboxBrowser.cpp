#include "SkyboxBrowser.h"
#include "../Editor.h"
#include <filesystem>
#include <nfd.h>
#include <raylib.h>
#include <rlImGui.h>
#include <imgui/imgui.h>

namespace fs = std::filesystem;

SkyboxBrowser::SkyboxBrowser(Editor* editor)
    : m_editor(editor), m_skyboxesScanned(false), m_selectedSkyboxIndex(0),
      m_skyboxPlaceholderTexture({0}), m_skyboxPlaceholderInitialized(false)
{
}

SkyboxBrowser::~SkyboxBrowser()
{
    // Cleanup placeholder texture
    if (m_skyboxPlaceholderInitialized && m_skyboxPlaceholderTexture.id != 0)
    {
        UnloadTexture(m_skyboxPlaceholderTexture);
        m_skyboxPlaceholderTexture = {0};
        m_skyboxPlaceholderInitialized = false;
    }
    
    // Cleanup preview textures
    for (auto& skybox : m_availableSkyboxes)
    {
        if (skybox.previewLoaded && skybox.previewTexture.id != 0)
        {
            UnloadTexture(skybox.previewTexture);
            skybox.previewTexture = {0};
            skybox.previewLoaded = false;
        }
    }
}

void SkyboxBrowser::RenderPanel(bool& isOpen)
{
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    ImVec2 windowSize(440, 540);
    ImVec2 desiredPos(static_cast<float>(screenWidth) - 460, 80);
    
    ImGui::SetNextWindowPos(desiredPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

    // Lazily load placeholder texture on first open of the panel
    if (!m_skyboxPlaceholderInitialized)
    {
        const char* placeholderPath = PROJECT_ROOT_DIR "/resources/map_previews/placeholder.jpg";
        Image placeholderImg = LoadImage(placeholderPath);
        if (placeholderImg.data != nullptr)
        {
            m_skyboxPlaceholderTexture = LoadTextureFromImage(placeholderImg);
            UnloadImage(placeholderImg);
            m_skyboxPlaceholderInitialized = (m_skyboxPlaceholderTexture.id != 0);
        }
    }

    if (ImGui::Begin("Set Skybox", &isOpen, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::TextWrapped("Current skybox: No skybox loaded");
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button("Load Skybox Image", ImVec2(200, 30)))
        {
            nfdfilteritem_t filterItem[1] = {{"Images", "png,jpg,jpeg,bmp,hdr,dds"}};
            nfdchar_t* outPath = nullptr;
            nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

            if (result == NFD_OKAY)
            {
                if (m_skyboxPlaceholderInitialized && m_skyboxPlaceholderTexture.id != 0)
                {
                    UnloadTexture(m_skyboxPlaceholderTexture);
                    m_skyboxPlaceholderTexture = {0};
                    m_skyboxPlaceholderInitialized = false;
                    m_skyboxPlaceholderPath.clear();
                }

                Image image = LoadImage(outPath);
                if (image.data != nullptr)
                {
                    m_skyboxPlaceholderTexture = LoadTextureFromImage(image);
                    UnloadImage(image);
                    if (m_skyboxPlaceholderTexture.id != 0)
                    {
                        m_skyboxPlaceholderInitialized = true;
                        m_skyboxPlaceholderPath = outPath;
                    }
                }
                NFD_FreePath(outPath);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Unload", ImVec2(100, 30)))
        {
            // Clear editor skybox via Editor API
            if (m_editor)
            {
                m_editor->SetSkyboxTexture(std::string());
            }

            if (m_skyboxPlaceholderInitialized && m_skyboxPlaceholderTexture.id != 0)
            {
                UnloadTexture(m_skyboxPlaceholderTexture);
                m_skyboxPlaceholderTexture = {0};
                m_skyboxPlaceholderInitialized = false;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (m_skyboxPlaceholderInitialized && m_skyboxPlaceholderTexture.id != 0)
        {
            ImGui::Text("Preview:");
            rlImGuiImageSize(&m_skyboxPlaceholderTexture, 64, 64);
        }
        else
        {
            ImGui::Text("Preview:");
            ImGui::Text("No preview available");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Load and apply skybox image
        if (m_skyboxPlaceholderInitialized && ImGui::Button("Apply to Scene", ImVec2(200, 30)))
        {
            if (m_editor && m_editor->GetSkybox())
            {
                // First apply texture
                m_editor->SetSkyboxTexture(m_skyboxPlaceholderPath);
                
                // Then apply shaders if they are set
                if (!m_skyboxPlaceholderPath.empty())
                {
                    m_editor->GetSkybox()->LoadMaterialShader(PROJECT_ROOT_DIR "/resources/shaders/skybox.vs", PROJECT_ROOT_DIR "/resources/shaders/skybox.fs");
                }
                
                TraceLog(LOG_INFO, "Applied skybox to editor scene: %s", m_skyboxPlaceholderPath.c_str());
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Shaders:");
        ImGui::Spacing();
    }
    ImGui::End();
}

void SkyboxBrowser::ScanDirectory()
{
    // Implementation for scanning skyboxes directory
    // This can be extended later if needed
    m_skyboxesScanned = true;
}

void SkyboxBrowser::LoadPreview(SkyboxInfo& skybox)
{
    // Implementation for loading skybox preview
    // This can be extended later if needed
    skybox.previewLoaded = true;
}

