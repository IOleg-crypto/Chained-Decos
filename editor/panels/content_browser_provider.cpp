#include "content_browser_provider.h"
#include <algorithm>
#include <unordered_map>

namespace Chained
{

ContentBrowserProvider::ContentBrowserProvider()
{
}

void ContentBrowserProvider::SetRoot(const std::filesystem::path& path)
{
    m_RootDirectory = path;
    m_CurrentDirectory = path;
    Scan();
}

void ContentBrowserProvider::SetFilter(const std::string& query, int typeFilter)
{
    m_FilterQuery = query;
    std::transform(m_FilterQuery.begin(), m_FilterQuery.end(), m_FilterQuery.begin(), ::tolower);
    m_FilterType = typeFilter;
    Scan();
}

void ContentBrowserProvider::Refresh()
{
    Scan();
}

void ContentBrowserProvider::Navigate(const std::filesystem::path& path)
{
    m_CurrentDirectory = path;
    Scan();
}

void ContentBrowserProvider::GoUp()
{
    if (m_CurrentDirectory != m_RootDirectory)
    {
        m_CurrentDirectory = m_CurrentDirectory.parent_path();
        Scan();
    }
}

void ContentBrowserProvider::GoToRoot()
{
    m_CurrentDirectory = m_RootDirectory;
    Scan();
}

void ContentBrowserProvider::Scan()
{
    m_CurrentAssets.clear();
    std::error_code ec;

    if (!std::filesystem::exists(m_CurrentDirectory, ec))
    {
        return;
    }

    for (auto& p : std::filesystem::directory_iterator(m_CurrentDirectory, ec))
    {
        AssetEntry entry;
        entry.name = p.path().filename().string();
        entry.path = p.path();
        entry.isDirectory = p.is_directory();
        entry.type = DetermineAssetType(p.path());

        // 1. Name Filter
        if (!m_FilterQuery.empty())
        {
            std::string nameLower = entry.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            if (nameLower.find(m_FilterQuery) == std::string::npos)
            {
                continue;
            }
        }

        // 2. Type Filter (Directories always shown)
        if (!entry.isDirectory && m_FilterType > 0)
        {
            bool match = false;
            switch (m_FilterType)
            {
            case 1:
                match = (entry.type == EditorAssetType::Scene);
                break;
            case 2:
                match = (entry.type == EditorAssetType::Prefab);
                break;
            case 3:
                match = (entry.type == EditorAssetType::Model);
                break;
            case 4:
                match = (entry.type == EditorAssetType::Texture);
                break;
            case 5:
                match = (entry.type == EditorAssetType::Script);
                break;
            case 6:
                match = (entry.type == EditorAssetType::Audio);
                break;
            }
            if (!match)
            {
                continue;
            }
        }

        m_CurrentAssets.push_back(entry);
    }

    // Sort: Directories first, then alphabetical
    std::sort(m_CurrentAssets.begin(), m_CurrentAssets.end(), [](const AssetEntry& a, const AssetEntry& b) {
        if (a.isDirectory != b.isDirectory)
        {
            return a.isDirectory > b.isDirectory;
        }
        return a.name < b.name;
    });
}

EditorAssetType ContentBrowserProvider::DetermineAssetType(const std::filesystem::path& path)
{
    if (std::filesystem::is_directory(path))
    {
        return EditorAssetType::Directory;
    }

    static const std::unordered_map<std::string, EditorAssetType> s_ExtensionMap = {
        {".chscene", EditorAssetType::Scene},   {".chmap", EditorAssetType::Scene},
        {".chprefab", EditorAssetType::Prefab}, {".h", EditorAssetType::Script},
        {".cpp", EditorAssetType::Script},      {".obj", EditorAssetType::Model},
        {".gltf", EditorAssetType::Model},      {".glb", EditorAssetType::Model},
        {".png", EditorAssetType::Texture},     {".jpg", EditorAssetType::Texture},
        {".tga", EditorAssetType::Texture},     {".wav", EditorAssetType::Audio},
        {".ogg", EditorAssetType::Audio},       {".mp3", EditorAssetType::Audio}};

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto it = s_ExtensionMap.find(ext);
    return (it != s_ExtensionMap.end()) ? it->second : EditorAssetType::Other;
}

} // namespace Chained
