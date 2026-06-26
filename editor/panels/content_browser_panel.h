#ifndef CH_CONTENT_BROWSER_PANEL_H
#define CH_CONTENT_BROWSER_PANEL_H

#include "content_browser_provider.h"
#include "panel.h"
#include <memory>

namespace Chained
{

class ContentBrowserPanel : public Panel
{
public:
    ContentBrowserPanel();
    ~ContentBrowserPanel() override;

    void OnImGuiRender(bool readOnly = false) override;
    void OnEvent(Event& e) override;
    void SetRootDirectory(const std::filesystem::path& path) const;

private:
    void RenderToolbar();
    void RenderGridView();
    void RefreshDirectory() const;

    void OnAssetDoubleClicked(const AssetEntry& entry);

private:
    std::unique_ptr<ContentBrowserProvider> m_Provider;

    float m_ThumbnailSize = 96.0f;
    float m_Padding = 16.0f;
    float m_IconScale = 1.0f;

    // UI State
    char m_FilterBuffer[128] = "";
    int m_FilterType = 0;

    // Mutation State
    std::filesystem::path m_RenamingPath;
    char m_RenameBuffer[256] = "";
    std::filesystem::path m_PathToDelete;
    std::filesystem::path m_NextDirectory;
};

} // namespace Chained

#endif // CH_CONTENT_BROWSER_PANEL_H
