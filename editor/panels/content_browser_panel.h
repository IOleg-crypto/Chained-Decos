#ifndef CH_CONTENT_BROWSER_PANEL_H
#define CH_CONTENT_BROWSER_PANEL_H

#include "panel.h"
#include <filesystem>
#include <functional>
#include <raylib.h>
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
    Other
};

struct AssetEntry
{
    std::string name;
    std::filesystem::path path;
    EditorAssetType type;
    Texture2D icon;
    bool isDirectory;
};

class ContentBrowserPanel : public Panel
{
public:
    ContentBrowserPanel();
    ~ContentBrowserPanel();

    virtual void OnImGuiRender(bool readOnly = false) override;
    void SetRootDirectory(const std::filesystem::path &path);

    using SceneOpenCallback = std::function<void(const std::filesystem::path &)>;
    void SetSceneOpenCallback(SceneOpenCallback callback)
    {
        m_OnSceneOpenCallback = callback;
    }

private:
    void RenderToolbar();
    void RenderGridView();
    void RefreshDirectory();
    void ScanCurrentDirectory();
    EditorAssetType DetermineAssetType(const std::filesystem::path &path);
    void LoadDefaultIcons();
    Texture2D GetIconForAsset(const AssetEntry &entry);
    void OnAssetDoubleClicked(AssetEntry &entry);

private:
    std::filesystem::path m_RootDirectory;
    std::filesystem::path m_CurrentDirectory;
    std::vector<AssetEntry> m_CurrentAssets;

    float m_ThumbnailSize = 96.0f;
    float m_Padding = 16.0f;

    Texture2D m_FolderIcon;
    Texture2D m_FileIcon;
    SceneOpenCallback m_OnSceneOpenCallback;
};
} // namespace CHEngine

#endif // CH_CONTENT_BROWSER_PANEL_H
