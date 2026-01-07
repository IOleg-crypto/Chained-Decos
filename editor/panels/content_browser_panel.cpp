#include "content_browser_panel.h"
#include "engine/core/base.h"
#include "engine/core/log.h"
#include <algorithm>
#include <extras/IconsFontAwesome6.h>
#include <imgui.h>

namespace CH
{
ContentBrowserPanel::ContentBrowserPanel()
{
    // These should ideally be project-relative or from a global engine resource folder
    m_RootDirectory = std::filesystem::current_path() / "assets";
    m_CurrentDirectory = m_RootDirectory;

    // LoadDefaultIcons(); // TODO
    RefreshDirectory();
}

ContentBrowserPanel::~ContentBrowserPanel()
{
    // Unload textures if they were loaded
}

void ContentBrowserPanel::OnImGuiRender(bool *p_open, bool readOnly)
{
    ImGui::Begin("Content Browser", p_open);

    ImGui::BeginDisabled(readOnly);
    RenderToolbar();
    ImGui::Separator();
    RenderGridView();

    ImGui::EndDisabled();
    ImGui::End();
}

void ContentBrowserPanel::RenderToolbar()
{
    if (m_CurrentDirectory != m_RootDirectory)
    {
        if (ImGui::Button(ICON_FA_ARROW_LEFT " Back"))
        {
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
            RefreshDirectory();
        }
    }

    ImGui::SameLine();
    ImGui::Text("Path: %s",
                std::filesystem::relative(m_CurrentDirectory, m_RootDirectory).string().c_str());
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

        const char *icon = asset.isDirectory ? ICON_FA_FOLDER : ICON_FA_FILE;

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        if (ImGui::Button(icon, {m_ThumbnailSize, m_ThumbnailSize}))
        {
            // Single click - select?
        }
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            OnAssetDoubleClicked(asset);
        }

        // Drag and Drop
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

void ContentBrowserPanel::OnAssetDoubleClicked(AssetEntry &entry)
{
    if (entry.isDirectory)
    {
        m_CurrentDirectory = entry.path;
        RefreshDirectory();
    }
    else if (entry.type == AssetType::Scene && m_OnSceneOpenCallback)
    {
        m_OnSceneOpenCallback(entry.path);
    }
}

void ContentBrowserPanel::RefreshDirectory()
{
    ScanCurrentDirectory();
}

void ContentBrowserPanel::ScanCurrentDirectory()
{
    m_CurrentAssets.clear();
    std::error_code ec;

    if (!std::filesystem::exists(m_CurrentDirectory, ec))
        return;

    for (auto &p : std::filesystem::directory_iterator(m_CurrentDirectory, ec))
    {
        AssetEntry entry;
        entry.name = p.path().filename().string();
        entry.path = p.path();
        entry.isDirectory = p.is_directory();
        entry.type = DetermineAssetType(p.path());
        m_CurrentAssets.push_back(entry);
    }
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
    return AssetType::Other;
}

void ContentBrowserPanel::SetRootDirectory(const std::filesystem::path &path)
{
    m_RootDirectory = path;
    m_CurrentDirectory = path;
    RefreshDirectory();
}
} // namespace CH
