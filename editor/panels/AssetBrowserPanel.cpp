//
// AssetBrowserPanel.cpp - Asset browser implementation
//

#include "AssetBrowserPanel.h"
#include "editor/IEditor.h"
#include <algorithm>
#include <imgui.h>

namespace fs = std::filesystem;

AssetBrowserPanel::AssetBrowserPanel(IEditor *editor) : m_editor(editor)
{
    // Set default root to resources folder
    m_rootPath = std::string(PROJECT_ROOT_DIR) + "/resources";
    m_currentPath = m_rootPath;
    RefreshCurrentDirectory();
}

void AssetBrowserPanel::SetRootPath(const std::string &path)
{
    m_rootPath = path;
    m_currentPath = path;
    RefreshCurrentDirectory();
}

void AssetBrowserPanel::RefreshCurrentDirectory()
{
    m_currentAssets.clear();

    if (!fs::exists(m_currentPath))
        return;

    // Add parent directory entry if not at root
    if (m_currentPath != m_rootPath)
    {
        AssetItem parent;
        parent.name = "..";
        parent.path = fs::path(m_currentPath).parent_path().string();
        parent.isDirectory = true;
        m_currentAssets.push_back(parent);
    }

    try
    {
        for (const auto &entry : fs::directory_iterator(m_currentPath))
        {
            AssetItem item;
            item.name = entry.path().filename().string();
            item.path = entry.path().string();
            item.isDirectory = entry.is_directory();

            if (!item.isDirectory)
            {
                item.extension = entry.path().extension().string();
            }

            // Filter by search if active
            if (!m_searchFilter.empty())
            {
                std::string lowerName = item.name;
                std::string lowerFilter = m_searchFilter;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(),
                               ::tolower);
                if (lowerName.find(lowerFilter) == std::string::npos)
                    continue;
            }

            m_currentAssets.push_back(item);
        }

        // Sort: directories first, then files alphabetically
        std::sort(m_currentAssets.begin(), m_currentAssets.end(),
                  [](const AssetItem &a, const AssetItem &b)
                  {
                      if (a.name == "..")
                          return true;
                      if (b.name == "..")
                          return false;
                      if (a.isDirectory != b.isDirectory)
                          return a.isDirectory;
                      return a.name < b.name;
                  });
    }
    catch (const std::exception &e)
    {
        // Handle permission errors etc.
    }
}

void AssetBrowserPanel::NavigateToDirectory(const std::string &path)
{
    m_currentPath = path;
    RefreshCurrentDirectory();
}

void AssetBrowserPanel::RenderBreadcrumbs()
{
    fs::path currentPath(m_currentPath);
    fs::path rootPath(m_rootPath);

    std::vector<std::pair<std::string, std::string>> parts;
    parts.push_back({"Resources", m_rootPath});

    if (m_currentPath != m_rootPath)
    {
        fs::path relative = fs::relative(currentPath, rootPath);
        fs::path accumulated = rootPath;
        for (const auto &part : relative)
        {
            accumulated /= part;
            parts.push_back({part.string(), accumulated.string()});
        }
    }

    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
            ImGui::SameLine();
        if (i > 0)
            ImGui::TextUnformatted("/");
        if (i > 0)
            ImGui::SameLine();

        if (i == parts.size() - 1)
        {
            ImGui::TextUnformatted(parts[i].first.c_str());
        }
        else
        {
            if (ImGui::SmallButton(parts[i].first.c_str()))
            {
                NavigateToDirectory(parts[i].second);
            }
        }
    }
}

void AssetBrowserPanel::HandleAssetDoubleClick(const AssetItem &asset)
{
    if (asset.isDirectory)
    {
        NavigateToDirectory(asset.path);
    }
    else
    {
        // Handle file opening based on extension
        if (asset.extension == ".json" && m_editor)
        {
            m_editor->GetSceneManager().LoadScene(asset.path);
        }
        else if ((asset.extension == ".obj" || asset.extension == ".gltf" ||
                  asset.extension == ".glb") &&
                 m_editor)
        {
            m_editor->GetSceneManager().LoadAndSpawnModel(asset.path);
        }
    }
}

void AssetBrowserPanel::RenderAssetGrid()
{
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columns = static_cast<int>(panelWidth / (m_iconSize + 20.0f));
    if (columns < 1)
        columns = 1;

    if (ImGui::BeginTable("AssetGrid", columns))
    {
        for (const auto &asset : m_currentAssets)
        {
            ImGui::TableNextColumn();

            ImGui::PushID(asset.path.c_str());

            // Icon/button
            const char *icon = asset.isDirectory ? "[D]" : "[F]";
            if (asset.extension == ".obj" || asset.extension == ".gltf" ||
                asset.extension == ".glb")
                icon = "[M]"; // Model
            else if (asset.extension == ".png" || asset.extension == ".jpg")
                icon = "[T]"; // Texture
            else if (asset.extension == ".json")
                icon = "[J]"; // JSON/Map

            ImVec2 buttonSize(m_iconSize, m_iconSize);
            bool isFolder = asset.isDirectory;

            if (isFolder)
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      ImVec4(1.0f, 0.9f, 0.4f, 1.0f)); // Yellowish for folders

            if (ImGui::Button(icon, buttonSize))
            {
                // Single click: select
            }

            if (isFolder)
                ImGui::PopStyleColor();

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                HandleAssetDoubleClick(asset);
            }

            // Tooltip
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", asset.path.c_str());
            }

            // Label
            ImGui::TextWrapped("%s", asset.name.c_str());

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

void AssetBrowserPanel::Render()
{
    if (!m_visible)
        return;

    if (ImGui::Begin("Asset Browser", &m_visible))
    {
        // Toolbar
        if (ImGui::Button("Refresh"))
        {
            RefreshCurrentDirectory();
        }
        ImGui::SameLine();

        // Search
        ImGui::SetNextItemWidth(200);
        char searchBuf[256];
        strncpy(searchBuf, m_searchFilter.c_str(), sizeof(searchBuf) - 1);
        if (ImGui::InputTextWithHint("##search", "Search...", searchBuf, sizeof(searchBuf)))
        {
            m_searchFilter = searchBuf;
            RefreshCurrentDirectory();
        }

        ImGui::SameLine();
        ImGui::SliderFloat("##iconSize", &m_iconSize, 40.0f, 120.0f, "%.0f");

        ImGui::Separator();

        // Breadcrumbs
        RenderBreadcrumbs();
        ImGui::Separator();

        // Asset Browser Body (Two Columns)
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float treeWidth = 200.0f; // Initial width of the folder tree

        ImGui::BeginChild("AssetBrowserContent", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

        // Use a simple split or columns
        if (ImGui::BeginTable("AssetBrowserLayout", 2,
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
        {
            ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, treeWidth);
            ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // Column 1: Folder Tree
            ImGui::TableNextColumn();
            ImGui::BeginChild("FolderTreeArea");
            RenderFolderTree();
            ImGui::EndChild();

            // Column 2: File Grid
            ImGui::TableNextColumn();
            ImGui::BeginChild("AssetGridArea");
            RenderAssetGrid();
            ImGui::EndChild();

            ImGui::EndTable();
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

void AssetBrowserPanel::RenderFolderTree()
{
    RenderFolderNode(m_rootPath);
}

void AssetBrowserPanel::RenderFolderNode(const std::string &path)
{
    namespace fs = std::filesystem;
    fs::path p(path);
    std::string name = p.filename().string();
    if (name.empty())
        name = "Resources";

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_SpanAvailWidth;

    // Check if it should be selected
    if (m_currentPath == path)
        flags |= ImGuiTreeNodeFlags_Selected;

    bool isLeaf = true;
    try
    {
        for (const auto &entry : fs::directory_iterator(path))
        {
            if (entry.is_directory())
            {
                isLeaf = false;
                break;
            }
        }
    }
    catch (...)
    {
    }

    if (isLeaf)
        flags |= ImGuiTreeNodeFlags_Leaf;

    // Folder colors for tree
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));
    bool open = ImGui::TreeNodeEx(name.c_str(), flags);
    ImGui::PopStyleColor();

    if (ImGui::IsItemClicked())
    {
        NavigateToDirectory(path);
    }

    if (open)
    {
        try
        {
            for (const auto &entry : fs::directory_iterator(path))
            {
                if (entry.is_directory())
                {
                    RenderFolderNode(entry.path().string());
                }
            }
        }
        catch (...)
        {
        }
        ImGui::TreePop();
    }
}
