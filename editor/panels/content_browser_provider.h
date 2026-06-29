#ifndef CH_CONTENT_BROWSER_PROVIDER_H
#define CH_CONTENT_BROWSER_PROVIDER_H

#include "editor/asset_types.h"
#include <filesystem>
#include <string>
#include <vector>

namespace Chained
{

class ContentBrowserProvider
{
public:
    ContentBrowserProvider();

    void SetRoot(const std::filesystem::path& path);
    void SetFilter(const std::string& query, int typeFilter);
    void Refresh();

    void Navigate(const std::filesystem::path& path);
    void GoUp();
    void GoToRoot();

    const std::vector<AssetEntry>& GetAssets() const
    {
        return m_CurrentAssets;
    }
    const std::filesystem::path& GetCurrentDirectory() const
    {
        return m_CurrentDirectory;
    }
    const std::filesystem::path& GetRootDirectory() const
    {
        return m_RootDirectory;
    }

private:
    void Scan();
    EditorAssetType DetermineAssetType(const std::filesystem::path& path);

    std::filesystem::path m_RootDirectory;
    std::filesystem::path m_CurrentDirectory;
    std::vector<AssetEntry> m_CurrentAssets;

    std::string m_FilterQuery;
    int m_FilterType = 0;
};

} // namespace Chained

#endif // CH_CONTENT_BROWSER_PROVIDER_H
