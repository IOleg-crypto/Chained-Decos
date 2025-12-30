#ifndef ASSETBROWSERPANEL_H
#define ASSETBROWSERPANEL_H

#include <filesystem>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
class AssetBrowserPanel
{
public:
    AssetBrowserPanel();
    ~AssetBrowserPanel();

    // --- Panel Lifecycle ---
public:
    void OnImGuiRender();

    // --- Configuration ---
public:
    bool IsVisible() const;
    void SetVisible(bool visible);
    void SetRootDirectory(const std::filesystem::path &path);

    // --- Internal Logic ---
private:
    void RefreshAssets();

    // --- Member Variables ---
private:
    std::filesystem::path m_RootPath;
    std::filesystem::path m_CurrentDirectory;

    struct AssetItem
    {
        std::string Name;
        std::filesystem::path Path;
        bool IsDirectory;
    };

    std::vector<AssetItem> m_Assets;
    std::unordered_map<std::string, Texture2D> m_ThumbnailCache;
    float m_ThumbnailSize = 128.0f;
    float m_Padding = 16.0f;
    bool m_isVisible = true;
};
} // namespace CHEngine
#endif
