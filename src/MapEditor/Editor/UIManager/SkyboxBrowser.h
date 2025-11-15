#ifndef SKYBOX_BROWSER_H
#define SKYBOX_BROWSER_H

#include <string>
#include <vector>
#include <raylib.h>
#include <imgui/imgui.h>

class Editor;

// SkyboxBrowser - handles skybox browsing and preview
class SkyboxBrowser
{
public:
    struct SkyboxInfo
    {
        std::string filename;
        std::string fullPath;
        Texture2D previewTexture;
        bool previewLoaded;
    };

    SkyboxBrowser(Editor* editor);
    ~SkyboxBrowser();

    // Render skybox panel UI
    void RenderPanel(bool& isOpen);

    // Scan skyboxes directory
    void ScanDirectory();

    // Load preview for a skybox
    void LoadPreview(SkyboxInfo& skybox);

    // Get available skyboxes
    const std::vector<SkyboxInfo>& GetAvailableSkyboxes() const { return m_availableSkyboxes; }

    // Get selected skybox index
    int GetSelectedSkyboxIndex() const { return m_selectedSkyboxIndex; }
    void SetSelectedSkyboxIndex(int index) { m_selectedSkyboxIndex = index; }

    // Get placeholder path
    const std::string& GetPlaceholderPath() const { return m_skyboxPlaceholderPath; }
    void SetPlaceholderPath(const std::string& path) { m_skyboxPlaceholderPath = path; }

    // Check if placeholder is initialized
    bool IsPlaceholderInitialized() const { return m_skyboxPlaceholderInitialized; }
    Texture2D GetPlaceholderTexture() const { return m_skyboxPlaceholderTexture; }

private:
    Editor* m_editor;
    std::vector<SkyboxInfo> m_availableSkyboxes;
    bool m_skyboxesScanned;
    int m_selectedSkyboxIndex;

    // Placeholder texture for skybox panel
    Texture2D m_skyboxPlaceholderTexture;
    bool m_skyboxPlaceholderInitialized;
    std::string m_skyboxPlaceholderPath;

    // Track last loaded metadata skybox to avoid reloading every frame
    std::string m_lastLoadedMetadataSkybox;
};

#endif // SKYBOX_BROWSER_H

