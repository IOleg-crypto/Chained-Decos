#include "SkyboxBrowser.h"
#include "mapeditor/IEditor.h"
#include "nfd.h"
#include "scene/resources/map/Skybox/Skybox.h"
#include <filesystem>
#include <imgui.h>
#include <raylib.h>

#ifdef LoadImage
#undef LoadImage
#endif
#ifdef Rectangle
#undef Rectangle
#endif
#ifdef CloseWindow
#undef CloseWindow
#endif
#ifdef ShowCursor
#undef ShowCursor
#endif

#include <rlImGui.h>

namespace fs = std::filesystem;

SkyboxBrowser::SkyboxBrowser(IEditor *editor)
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
    for (auto &skybox : m_availableSkyboxes)
    {
        if (skybox.previewLoaded && skybox.previewTexture.id != 0)
        {
            UnloadTexture(skybox.previewTexture);
            skybox.previewTexture = {0};
            skybox.previewLoaded = false;
        }
    }
}

void SkyboxBrowser::RenderPanel(bool &isOpen)
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
        const std::string &currentSkyboxTexture =
            m_editor->GetGameMap().GetMapMetaData().skyboxTexture;

        // If there's a skybox in metadata, load it
        if (!currentSkyboxTexture.empty())
        {
            std::string skyboxPath = currentSkyboxTexture;
            // Ensure path starts with /
            if (skyboxPath[0] != '/' && skyboxPath[0] != '\\')
            {
                skyboxPath = "/" + skyboxPath;
            }
            std::string fullPathProject = std::string(PROJECT_ROOT_DIR) + skyboxPath;

            Image image = LoadImage(fullPathProject.c_str());
            if (image.data != nullptr)
            {
                m_skyboxPlaceholderTexture = LoadTextureFromImage(image);
                UnloadImage(image);
                if (m_skyboxPlaceholderTexture.id != 0)
                {
                    m_skyboxPlaceholderInitialized = true;
                    m_skyboxPlaceholderPath = fullPathProject;
                    m_lastLoadedMetadataSkybox = currentSkyboxTexture;
                }
            }
        }

        // If no skybox in metadata or failed to load, use placeholder
        if (!m_skyboxPlaceholderInitialized)
        {
            const char *placeholderPath =
                PROJECT_ROOT_DIR "/resources/map_previews/placeholder.jpg";
            Image placeholderImg = LoadImage(placeholderPath);
            if (placeholderImg.data != nullptr)
            {
                m_skyboxPlaceholderTexture = LoadTextureFromImage(placeholderImg);
                UnloadImage(placeholderImg);
                m_skyboxPlaceholderInitialized = (m_skyboxPlaceholderTexture.id != 0);
                m_lastLoadedMetadataSkybox = ""; // No skybox loaded
            }
        }
    }
    else
    {
        // Check if metadata skybox has changed
        const std::string &currentSkyboxTexture =
            m_editor->GetGameMap().GetMapMetaData().skyboxTexture;
        if (currentSkyboxTexture != m_lastLoadedMetadataSkybox)
        {
            // Unload current texture
            if (m_skyboxPlaceholderTexture.id != 0)
            {
                UnloadTexture(m_skyboxPlaceholderTexture);
                m_skyboxPlaceholderTexture = {0};
            }
            m_skyboxPlaceholderInitialized = false;
            m_skyboxPlaceholderPath.clear();

            // Reload with new skybox from metadata
            if (!currentSkyboxTexture.empty())
            {
                std::string skyboxPath = currentSkyboxTexture;
                // Ensure path starts with /
                if (skyboxPath[0] != '/' && skyboxPath[0] != '\\')
                {
                    skyboxPath = "/" + skyboxPath;
                }
                std::string fullPathProject = std::string(PROJECT_ROOT_DIR) + skyboxPath;

                Image image = LoadImage(fullPathProject.c_str());
                if (image.data != nullptr)
                {
                    m_skyboxPlaceholderTexture = LoadTextureFromImage(image);
                    UnloadImage(image);
                    if (m_skyboxPlaceholderTexture.id != 0)
                    {
                        m_skyboxPlaceholderInitialized = true;
                        m_skyboxPlaceholderPath = fullPathProject;
                        m_lastLoadedMetadataSkybox = currentSkyboxTexture;
                    }
                }
            }
            else
            {
                // Load placeholder
                const char *placeholderPath =
                    PROJECT_ROOT_DIR "/resources/map_previews/placeholder.jpg";
                Image placeholderImg = LoadImage(placeholderPath);
                if (placeholderImg.data != nullptr)
                {
                    m_skyboxPlaceholderTexture = LoadTextureFromImage(placeholderImg);
                    UnloadImage(placeholderImg);
                    m_skyboxPlaceholderInitialized = (m_skyboxPlaceholderTexture.id != 0);
                    m_lastLoadedMetadataSkybox = "";
                }
            }
        }
    }

    if (ImGui::Begin("Set Skybox", &isOpen, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("Current skybox: %s", m_skyboxPlaceholderPath.empty()
                                              ? "No skybox loaded"
                                              : m_skyboxPlaceholderPath.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Load Skybox Image", ImVec2(200, 30)))
        {
            nfdfilteritem_t filterItem[1] = {{"Images", "png,jpg,jpeg,bmp,hdr,dds"}};
            nfdchar_t *outPath = nullptr;
            nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

            if (result == NFD_OKAY)
            {
                if (m_skyboxPlaceholderInitialized && m_skyboxPlaceholderTexture.id != 0)
                {
                    UnloadTexture(m_skyboxPlaceholderTexture);
                    m_skyboxPlaceholderTexture = {0};
                    m_skyboxPlaceholderInitialized = false;
                    m_skyboxPlaceholderPath.clear();
                    m_lastLoadedMetadataSkybox.clear(); // Clear tracking
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
                        m_lastLoadedMetadataSkybox = ""; // User-loaded, not from metadata
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

            // Unload current texture
            if (m_skyboxPlaceholderInitialized && m_skyboxPlaceholderTexture.id != 0)
            {
                UnloadTexture(m_skyboxPlaceholderTexture);
                m_skyboxPlaceholderTexture = {0};
            }

            // Load placeholder image
            const char *placeholderPath =
                PROJECT_ROOT_DIR "/resources/map_previews/placeholder.jpg";
            Image placeholderImg = LoadImage(placeholderPath);
            if (placeholderImg.data != nullptr)
            {
                m_skyboxPlaceholderTexture = LoadTextureFromImage(placeholderImg);
                UnloadImage(placeholderImg);
                m_skyboxPlaceholderInitialized = (m_skyboxPlaceholderTexture.id != 0);
                m_skyboxPlaceholderPath.clear();
                m_lastLoadedMetadataSkybox.clear();
            }
            else
            {
                m_skyboxPlaceholderInitialized = false;
                m_skyboxPlaceholderPath.clear();
                m_lastLoadedMetadataSkybox.clear();
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
            if (m_editor)
            {
                // Apply texture (shaders are loaded automatically in SetSkyboxTexture)
                m_editor->SetSkyboxTexture(m_skyboxPlaceholderPath);
                if (m_editor->GetSkybox())
                {
                    m_editor->GetSkybox()->LoadMaterialShader(
                        PROJECT_ROOT_DIR "/resources/shaders/skybox.vs",
                        PROJECT_ROOT_DIR "/resources/shaders/skybox.fs");
                }

                TraceLog(LOG_INFO, "Applied skybox to editor scene: %s",
                         m_skyboxPlaceholderPath.c_str());
            }
        }

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

void SkyboxBrowser::LoadPreview(SkyboxInfo &skybox)
{
    // Implementation for loading skybox preview
    // This can be extended later if needed
    skybox.previewLoaded = true;
}
bool SkyboxBrowser::IsPlaceholderInitialized() const
{
    return m_skyboxPlaceholderInitialized;
}
Texture2D SkyboxBrowser::GetPlaceholderTexture() const
{
    return m_skyboxPlaceholderTexture;
}
void SkyboxBrowser::SetPlaceholderPath(const std::string &path)
{
    m_skyboxPlaceholderPath = path;
}
const std::string &SkyboxBrowser::GetPlaceholderPath() const
{
    return m_skyboxPlaceholderPath;
}
void SkyboxBrowser::SetSelectedSkyboxIndex(int index)
{
    m_selectedSkyboxIndex = index;
}
int SkyboxBrowser::GetSelectedSkyboxIndex() const
{
    return m_selectedSkyboxIndex;
}
const std::vector<SkyboxBrowser::SkyboxInfo> &SkyboxBrowser::GetAvailableSkyboxes() const
{
    return m_availableSkyboxes;
}




