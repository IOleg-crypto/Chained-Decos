#include "content_browser_panel.h"
#include "core/log.h"
#include "editor/fa5_compat.h"
#include "editor/logic/editor_scene_actions.h"
#include <algorithm>
#include <imgui.h>

namespace CHEngine
{

ContentBrowserPanel::ContentBrowserPanel(EditorSceneActions *sceneActions)
    : m_RootDirectory(PROJECT_ROOT_DIR "/resources"),
      m_CurrentDirectory(PROJECT_ROOT_DIR "/resources"), m_SceneActions(sceneActions)
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

void ContentBrowserPanel::RefreshDirectory()
{
    ScanCurrentDirectory();
}

void ContentBrowserPanel::ScanCurrentDirectory()
{
    m_CurrentAssets.clear();

    try
    {
        for (auto &p : std::filesystem::directory_iterator(m_CurrentDirectory))
        {
            AssetEntry entry;
            entry.name = p.path().filename().string();
            entry.path = p.path();
            entry.isDirectory = p.is_directory();
            entry.type = DetermineAssetType(p.path());
            m_CurrentAssets.push_back(entry);
        }
    }
    catch (...)
    {
        CD_CORE_ERROR("[ContentBrowser] Failed to scan directory: %s",
                      m_CurrentDirectory.string().c_str());
    }
}

void ContentBrowserPanel::NavigateToDirectory(const std::filesystem::path &path)
{
    m_BackHistory.push_back(m_CurrentDirectory);
    m_CurrentDirectory = path;
    m_ForwardHistory.clear();
    ScanCurrentDirectory();
}

void ContentBrowserPanel::RenderGridView()
{
    float cellSize = m_ThumbnailSize + m_Padding;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1)
        columnCount = 1;

    ImGui::Columns(columnCount, nullptr, false);

    for (int i = 0; i < (int)m_CurrentAssets.size(); i++)
    {
        auto &asset = m_CurrentAssets[i];
        ImGui::PushID(i);

        // Icon
        Texture2D icon = GetIconForAsset(asset);
        ImGui::ImageButton("##AssetIcon", (ImTextureID)(uintptr_t)icon.id,
                           {m_ThumbnailSize, m_ThumbnailSize});

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            OnAssetDoubleClicked(asset);
        }

        // Drag and Drop Source
        if (ImGui::BeginDragDropSource())
        {
            std::string pathStr = asset.path.string();
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
            ImGui::Text("%s", asset.name.c_str());
            ImGui::EndDragDropSource();
        }

        ImGui::TextWrapped("%s", asset.name.c_str());

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);
}

void ContentBrowserPanel::RenderListView()
{
    for (int i = 0; i < (int)m_CurrentAssets.size(); i++)
    {
        auto &asset = m_CurrentAssets[i];
        ImGui::PushID(i);

        bool selected = false;
        if (ImGui::Selectable(asset.name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick))
        {
            if (ImGui::IsMouseDoubleClicked(0))
                OnAssetDoubleClicked(asset);
        }

        ImGui::PopID();
    }
}

void ContentBrowserPanel::RenderToolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    if (ImGui::Button(ICON_FA_ARROW_LEFT))
    {
        if (!m_BackHistory.empty())
        {
            m_ForwardHistory.push_back(m_CurrentDirectory);
            m_CurrentDirectory = m_BackHistory.back();
            m_BackHistory.pop_back();
            ScanCurrentDirectory();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_RIGHT))
    {
        if (!m_ForwardHistory.empty())
        {
            m_BackHistory.push_back(m_CurrentDirectory);
            m_CurrentDirectory = m_ForwardHistory.back();
            m_ForwardHistory.pop_back();
            ScanCurrentDirectory();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_SYNC))
    {
        RefreshDirectory();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    if (ImGui::InputTextWithHint("##Search", ICON_FA_SEARCH " Search...", m_SearchBuffer,
                                 sizeof(m_SearchBuffer)))
    {
        // TODO: Filter logic
    }

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Path: %s",
                       m_CurrentDirectory.string().c_str());

    ImGui::PopStyleVar();
}

void ContentBrowserPanel::RenderBreadcrumbs()
{
    // Simplified breadcrumbs
}

void ContentBrowserPanel::LoadDefaultIcons()
{
    // Load stubs or actual textures if available
}

AssetType ContentBrowserPanel::DetermineAssetType(const std::filesystem::path &path)
{
    if (std::filesystem::is_directory(path))
        return AssetType::Directory;
    auto ext = path.extension().string();
    if (ext == ".chscene")
        return AssetType::Scene;
    if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
        return AssetType::Model;
    if (ext == ".png" || ext == ".jpg" || ext == ".tga")
        return AssetType::Texture;
    if (ext == ".cs")
        return AssetType::Script;
    return AssetType::Other;
}

Texture2D ContentBrowserPanel::GetIconForAsset(const AssetEntry &entry)
{
    // Return dummy textures or actual ones
    return {0, 0, 0, 0, 0};
}

void ContentBrowserPanel::SetRootDirectory(const std::filesystem::path &path)
{
    m_RootDirectory = path;
    m_CurrentDirectory = path;
    RefreshDirectory();
}

} // namespace CHEngine
