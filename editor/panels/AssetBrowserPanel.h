#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace CHEngine
{
class AssetBrowserPanel
{
public:
    AssetBrowserPanel();

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
    float m_ThumbnailSize = 128.0f;
    float m_Padding = 16.0f;
    bool m_isVisible = true;
};
} // namespace CHEngine
