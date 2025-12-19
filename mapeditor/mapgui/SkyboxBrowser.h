#ifndef SKYBOX_BROWSER_H
#define SKYBOX_BROWSER_H

#include <imgui.h>
#include <raylib.h>
#include <string>
#include <vector>

#include "IEditor.h"

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

    SkyboxBrowser(IEditor *editor);
    ~SkyboxBrowser();

    // Render skybox panel UI
    void RenderPanel(bool &isOpen);

    // Scan skyboxes directory
    void ScanDirectory();

    // Load preview for a skybox
    void LoadPreview(SkyboxInfo &skybox);

    // Get available skyboxes
    const std::vector<SkyboxInfo> &GetAvailableSkyboxes() const;

    // Get selected skybox index
    int GetSelectedSkyboxIndex() const;
    void SetSelectedSkyboxIndex(int index);

    // Get placeholder path
    const std::string &GetPlaceholderPath() const;
    void SetPlaceholderPath(const std::string &path);

    // Check if placeholder is initialized
    bool IsPlaceholderInitialized() const;
    Texture2D GetPlaceholderTexture() const;

private:
    IEditor *m_editor;
    std::vector<SkyboxInfo> m_availableSkyboxes;
    bool m_skyboxesScanned;
    int m_selectedSkyboxIndex;
    bool m_isSkyboxLoaded;

    // Placeholder texture for skybox panel
    Texture2D m_skyboxPlaceholderTexture;
    bool m_skyboxPlaceholderInitialized;
    std::string m_skyboxPlaceholderPath;

    // Track last loaded metadata skybox to avoid reloading every frame
    std::string m_lastLoadedMetadataSkybox;
};

#endif // SKYBOX_BROWSER_H
