#ifndef CH_CONTENT_BROWSER_PANEL_H
#define CH_CONTENT_BROWSER_PANEL_H

#include "panel.h"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace CHEngine
{
enum class EditorAssetType
{
    Directory,
    Scene,
    Script,
    Model,
    Texture,
    Audio,
    Prefab,
    Other
};

struct AssetEntry
{
    std::string name;
    std::filesystem::path path;
    EditorAssetType type;
    uint32_t icon;
    bool isDirectory;
};

class ContentBrowserPanel : public Panel
{
public:
    ContentBrowserPanel();
    ~ContentBrowserPanel();

    virtual void OnImGuiRender(bool readOnly = false) override;
    virtual void OnEvent(Event& e) override;
    void SetRootDirectory(const std::filesystem::path& path);

private:
    void RenderToolbar();
    void RenderGridView();
    void RefreshDirectory();
    void ScanCurrentDirectory();

private:
    EditorAssetType DetermineAssetType(const std::filesystem::path& path);
    void LoadDefaultIcons();
    uint32_t GetIconForAsset(const AssetEntry& entry);
    void OnAssetDoubleClicked(AssetEntry& entry);

private:
    std::filesystem::path m_RootDirectory;
    std::filesystem::path m_CurrentDirectory;
    std::vector<AssetEntry> m_CurrentAssets;

    float m_ThumbnailSize = 96.0f;
    float m_Padding = 16.0f;

    // Filtering
    char m_FilterBuffer[128] = "";
    int m_FilterType = 0; // 0 = All, or specific type

    uint32_t m_FolderIcon = 0;
    uint32_t m_FileIcon = 0;
    float m_IconScale = 1.0f;

    // Asset Management
    std::filesystem::path m_RenamingPath;
    char m_RenameBuffer[256] = "";
    std::filesystem::path m_PathToDelete;
};
} // namespace CHEngine

#endif // CH_CONTENT_BROWSER_PANEL_H
