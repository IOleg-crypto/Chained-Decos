#pragma once
#include "editor_panel.h"
#include "scene/resources/map/scene_metadata.h"
#include <filesystem>
#include <functional>
#include <raylib.h>
#include <string>
#include <vector>

namespace CHEngine
{
class EditorSceneActions;

enum class AssetType
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
    AssetType type;
    SceneType sceneType = SceneType::Game; // Only relevant for scenes
    Texture2D icon;
    bool isDirectory;
    uint64_t fileSize = 0;

    // For scene assets
    SceneMetadata metadata;
};

/**
 * @brief Modern Content Browser panel inspired by Hazel Engine
 * Displays assets in a grid view with thumbnails, supports scene types,
 * and provides drag-and-drop, search, and filtering
 */
class ContentBrowserPanel : public EditorPanel
{
public:
    ContentBrowserPanel(EditorSceneActions *sceneActions);
    virtual ~ContentBrowserPanel();

    virtual void OnImGuiRender() override;

    // Configuration
    void SetRootDirectory(const std::filesystem::path &path);
    void SetThumbnailSize(float size)
    {
        m_ThumbnailSize = size;
    }

    // Callbacks
    void SetOnSceneDoubleClick(std::function<void(const std::string &, SceneType)> callback);

private:
    // Rendering
    void RenderToolbar();
    void RenderBreadcrumbs();
    void RenderGridView();
    void RenderListView();
    void RenderAssetTile(AssetEntry &entry, int index);
    void RenderContextMenu(AssetEntry &entry);

    // Navigation
    void NavigateToDirectory(const std::filesystem::path &path);
    void NavigateBack();
    void NavigateForward();
    void RefreshDirectory();

    // Interaction
    void OnAssetDoubleClicked(AssetEntry &entry);
    void OnAssetRightClicked(AssetEntry &entry);

    // Asset Discovery
    void ScanCurrentDirectory();
    AssetType DetermineAssetType(const std::filesystem::path &path);

    // Icon Management
    void LoadDefaultIcons();
    Texture2D GetIconForAsset(const AssetEntry &entry);

private:
    // Current state
    std::filesystem::path m_RootDirectory;
    std::filesystem::path m_CurrentDirectory;
    std::vector<AssetEntry> m_CurrentAssets;

    // Navigation history
    std::vector<std::filesystem::path> m_BackHistory;
    std::vector<std::filesystem::path> m_ForwardHistory;

    // View settings
    bool m_GridView = true;
    float m_ThumbnailSize = 128.0f;
    float m_Padding = 16.0f;

    // Search & Filter
    char m_SearchBuffer[256] = "";
    bool m_FilterScenes = true;
    bool m_FilterModels = true;
    bool m_FilterTextures = true;
    bool m_FilterAudio = true;
    bool m_FilterScripts = true;
    bool m_FilterGameScenes = true;
    bool m_FilterUIScenes = true;

    // Icons
    Texture2D m_FolderIcon;
    Texture2D m_FileIcon;
    Texture2D m_SceneGameIcon;
    Texture2D m_SceneUIIcon;
    Texture2D m_ModelIcon;
    Texture2D m_TextureIcon;
    Texture2D m_AudioIcon;
    Texture2D m_ScriptIcon;

    // Callbacks
    std::function<void(const std::string &, SceneType)> m_OnSceneDoubleClick;

    // Context menu state
    int m_ContextMenuIndex = -1;

    EditorSceneActions *m_SceneActions = nullptr;
};

} // namespace CHEngine
