//
// AssetBrowserPanel.h - Asset browser panel
//

#ifndef ASSETBROWSERPANEL_H
#define ASSETBROWSERPANEL_H

#include "IEditorPanel.h"
#include <filesystem>
#include <string>
#include <vector>

class IEditor;

struct AssetItem
{
    std::string name;
    std::string path;
    std::string extension;
    bool isDirectory;
};

// Displays project assets (models, textures, maps)
class AssetBrowserPanel : public IEditorPanel
{
public:
    explicit AssetBrowserPanel(IEditor *editor);
    ~AssetBrowserPanel() override = default;

    // IEditorPanel interface
    void Render() override;
    const char *GetName() const override
    {
        return "AssetBrowser";
    }
    const char *GetDisplayName() const override
    {
        return "Asset Browser";
    }
    bool IsVisible() const override
    {
        return m_visible;
    }
    void SetVisible(bool visible) override
    {
        m_visible = visible;
    }

    // Browser-specific
    void SetRootPath(const std::string &path);
    void RefreshCurrentDirectory();

private:
    void NavigateToDirectory(const std::string &path);
    void RenderBreadcrumbs();
    void RenderFolderTree();
    void RenderFolderNode(const std::string &path);
    void RenderAssetGrid();
    void HandleAssetDoubleClick(const AssetItem &asset);

    IEditor *m_editor;
    bool m_visible = true;
    std::string m_rootPath;
    std::string m_currentPath;
    std::vector<AssetItem> m_currentAssets;
    float m_iconSize = 80.0f;
    std::string m_searchFilter;
};

#endif // ASSETBROWSERPANEL_H
