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

    void OnImGuiRender();

    bool IsVisible() const
    {
        return m_isVisible;
    }
    void SetVisible(bool visible)
    {
        m_isVisible = visible;
    }

private:
    void RefreshAssets();

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
