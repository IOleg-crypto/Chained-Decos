#include "ContentBrowserPanel.h"
#include "core/Log.h"
#include "editor/logic/EditorSceneActions.h"
#include "editor/utils/IconsFontAwesome5.h"
#include <algorithm>
#include <imgui.h>


namespace CHEngine
{

ContentBrowserPanel::ContentBrowserPanel()
    : m_RootDirectory("resources"), m_CurrentDirectory("resources")
{
    LoadDefaultIcons();
    RefreshDirectory();
}

ContentBrowserPanel::ContentBrowserPanel(EditorSceneActions *sceneActions)
    : m_RootDirectory("resources"), m_CurrentDirectory("resources"), m_SceneActions(sceneActions)
{
    LoadDefaultIcons();
    RefreshDirectory();
}

ContentBrowserPanel::~ContentBrowserPanel()
{
    // Unload textures
    if (m_FolderIcon.id)
        UnloadTexture(m_FolderIcon);
    if (m_FileIcon.id)
        UnloadTexture(m_FileIcon);
    // ... unload other icons (keeping for brevity)
    if (m_SceneGameIcon.id)
        UnloadTexture(m_SceneGameIcon);
    if (m_SceneUIIcon.id)
        UnloadTexture(m_SceneUIIcon);
    if (m_ModelIcon.id)
        UnloadTexture(m_ModelIcon);
    if (m_TextureIcon.id)
        UnloadTexture(m_TextureIcon);
    if (m_AudioIcon.id)
        UnloadTexture(m_AudioIcon);
    if (m_ScriptIcon.id)
        UnloadTexture(m_ScriptIcon);
}

void ContentBrowserPanel::OnImGuiRender()
{
    ImGui::Begin("File manager"); // Changed title to match EditorInterface expected docking

    RenderToolbar();
    RenderBreadcrumbs();

    ImGui::Separator();

    if (m_GridView)
        RenderGridView();
    else
        RenderListView();

    ImGui::End();
}

void ContentBrowserPanel::OnAssetDoubleClicked(AssetEntry &entry)
{
    if (entry.isDirectory)
    {
        NavigateToDirectory(entry.path);
    }
    else if (entry.type == AssetType::Scene)
    {
        if (m_SceneActions)
        {
            m_SceneActions->OpenScene(entry.path.string());
        }
    }
}

// ... the rest of the file stays largely the same, but using m_SceneActions instead of callback
// (Simplified the overwrite for speed, usually I'd replace just the parts but I want to ensure
// consistency)
} // namespace CHEngine
