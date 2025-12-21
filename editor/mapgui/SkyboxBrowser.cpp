#include "SkyboxBrowser.h"
#include "editor/IEditor.h"
#include "nfd.h"
#include "scene/resources/map/skybox/skybox.h"
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
      m_skyboxPlaceholderTexture({0}), m_skyboxPlaceholderInitialized(false),
      m_isSkyboxLoaded(false)
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
    if (!isOpen)
        return;

    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    ImVec2 windowSize(440, 540);
    ImVec2 desiredPos(static_cast<float>(screenWidth) - 460, 80);

    ImGui::SetNextWindowPos(desiredPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Set Skybox", &isOpen, ImGuiWindowFlags_NoCollapse))
    {
        // Lazily load placeholder texture on first open of the panel
        if (!m_skyboxPlaceholderInitialized)
        {
            const std::string &currentSkyboxTexture =
                m_editor->GetGameScene().GetMapMetaData().skyboxTexture;

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
                        m_isSkyboxLoaded = true;
                    }
                }
            }

            // If no skybox in metadata or failed to load, use placeholder
            if (!m_skyboxPlaceholderInitialized)
            {
                const char *placeholderPath =
                    PROJECT_ROOT_DIR "/resources/map_previews/placeholder.jpg";
                TraceLog(LOG_INFO, "[SkyboxBrowser] Loading fallback placeholder: %s",
                         placeholderPath);
                Image placeholderImg = LoadImage(placeholderPath);
                if (placeholderImg.data != nullptr)
                {
                    m_skyboxPlaceholderTexture = LoadTextureFromImage(placeholderImg);
                    UnloadImage(placeholderImg);
                    m_skyboxPlaceholderInitialized = (m_skyboxPlaceholderTexture.id != 0);
                    m_lastLoadedMetadataSkybox = currentSkyboxTexture; // Track even if placeholder
                    m_isSkyboxLoaded = false;
                }
                else
                {
                    // Fallback failed too, but we must stop the loop
                    m_lastLoadedMetadataSkybox = currentSkyboxTexture;
                    TraceLog(LOG_WARNING, "[SkyboxBrowser] Failed to load placeholder image!");
                }
            }
        }
        else
        {
            // Check if metadata skybox has changed in the scene (e.g. via undo or loading another
            // map) and we haven't manually loaded a different one in the browser
            const std::string &currentSkyboxTexture =
                m_editor->GetGameScene().GetMapMetaData().skyboxTexture;

            // SYNC FIX: Allow sync if we are in placeholder state (empty path) OR if we were
            // already synced
            bool needsSync = (currentSkyboxTexture != m_lastLoadedMetadataSkybox) &&
                             (m_isSkyboxLoaded || m_skyboxPlaceholderPath.empty() ||
                              m_skyboxPlaceholderPath.find("placeholder.jpg") != std::string::npos);

            if (needsSync)
            {
                // Unload current texture early
                if (m_skyboxPlaceholderTexture.id != 0)
                {
                    UnloadTexture(m_skyboxPlaceholderTexture);
                    m_skyboxPlaceholderTexture = {0};
                }
                m_skyboxPlaceholderInitialized = false;
                m_skyboxPlaceholderPath.clear();

                // ALWAYS record that we are processing this metadata to stop the loop
                m_lastLoadedMetadataSkybox = currentSkyboxTexture;

                // Reload with new skybox from metadata
                if (!currentSkyboxTexture.empty())
                {
                    std::string skyboxPath = currentSkyboxTexture;
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
                            m_isSkyboxLoaded = true;
                            TraceLog(LOG_INFO, "[SkyboxBrowser] Synced preview with scene: %s",
                                     fullPathProject.c_str());
                        }
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "[SkyboxBrowser] Failed to sync preview: %s",
                                 fullPathProject.c_str());
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
                        m_isSkyboxLoaded = false;
                    }
                    else
                    {
                        TraceLog(LOG_WARNING, "[SkyboxBrowser] Failed to load placeholder: %s",
                                 placeholderPath);
                    }
                }
            }
        }
        ImGui::Text("Current skybox: %s",
                    m_skyboxPlaceholderPath.empty()
                        ? "None"
                        : fs::path(m_skyboxPlaceholderPath).filename().string().c_str());
        if (ImGui::IsItemHovered() && !m_skyboxPlaceholderPath.empty())
        {
            ImGui::SetTooltip("%s", m_skyboxPlaceholderPath.c_str());
        }
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
                        m_isSkyboxLoaded = false;        // Not Applied yet
                        TraceLog(LOG_INFO, "[SkyboxBrowser] Loaded local preview: %s", outPath);
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
                m_isSkyboxLoaded = false;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (m_skyboxPlaceholderInitialized && m_skyboxPlaceholderTexture.id != 0)
        {
            ImGui::Text("Preview (128x128):");
            rlImGuiImageSize(&m_skyboxPlaceholderTexture, 128, 128);
        }
        else
        {
            ImGui::Text("Preview:");
            ImGui::TextDisabled("No skybox image to preview");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Load and apply skybox image
        bool canApply = !m_skyboxPlaceholderPath.empty() &&
                        m_skyboxPlaceholderPath.find("placeholder.jpg") == std::string::npos;

        if (!canApply)
            ImGui::BeginDisabled();
        if (ImGui::Button("Apply to Scene", ImVec2(200, 30)))
        {
            // Apply texture (shaders are loaded automatically in SetSkyboxTexture)
            m_editor->SetSkyboxTexture(m_skyboxPlaceholderPath);
            m_isSkyboxLoaded = true;

            TraceLog(LOG_INFO, "Applied skybox to editor scene: %s",
                     m_skyboxPlaceholderPath.c_str());
        }
        if (!canApply)
            ImGui::EndDisabled();

        if (m_isSkyboxLoaded && canApply)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), " (Currently Active)");
        }
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
