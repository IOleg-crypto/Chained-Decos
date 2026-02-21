#include "content_browser_panel.h"
#include "editor/actions/scene_actions.h"
#include "engine/core/base.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_events.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"
#include <algorithm>
#include <unordered_map>

namespace CHEngine
{
ContentBrowserPanel::ContentBrowserPanel()
{
    m_Name = "Content Browser";

    auto project = Project::GetActive();
    if (project)
    {
        m_RootDirectory = Project::GetAssetDirectory();
    }
    else
    {
        // Fallback: use project root if project not loaded yet
        m_RootDirectory = std::filesystem::current_path() / "assets";
    }

    m_CurrentDirectory = m_RootDirectory;
    RefreshDirectory();
}

ContentBrowserPanel::~ContentBrowserPanel()
{
    // Unload textures if they were loaded
}

void ContentBrowserPanel::OnImGuiRender(bool readOnly)
{
    if (!m_IsOpen)
    {
        return;
    }
    ImGui::Begin(m_Name.c_str(), &m_IsOpen);
    ImGui::PushID(this);

    ImGui::BeginDisabled(readOnly);
    RenderToolbar();
    ImGui::Separator();
    RenderGridView();

    ImGui::EndDisabled();
    ImGui::PopID();
    ImGui::End();
}

void ContentBrowserPanel::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<ProjectOpenedEvent>([this](ProjectOpenedEvent& e) {
        SetRootDirectory(Project::GetAssetDirectory());
        return false;
    });
}

void ContentBrowserPanel::RenderToolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

    // Navigation Buttons
    if (m_CurrentDirectory != m_RootDirectory)
    {
        if (ImGui::Button(ICON_FA_ARROW_LEFT))
        {
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
            RefreshDirectory();
        }
    }
    else
    {
        ImGui::BeginDisabled(true);
        ImGui::Button(ICON_FA_ARROW_LEFT);
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);

    // Search Filter
    if (ImGui::InputTextWithHint("##Search", ICON_FA_MAGNIFYING_GLASS " Search...", m_FilterBuffer,
                                 sizeof(m_FilterBuffer)))
    {
        RefreshDirectory();
    }

    ImGui::SameLine();

    // Type Filter Dropdown
    ImGui::SetNextItemWidth(150);
    const char* filterNames[] = {"All Types", "Scenes", "Prefabs", "Models", "Textures", "Scripts", "Audio"};
    if (ImGui::BeginCombo("##TypeFilter", filterNames[m_FilterType]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(filterNames); i++)
        {
            bool isSelected = (m_FilterType == i);
            if (ImGui::Selectable(filterNames[i], isSelected))
            {
                m_FilterType = i;
                RefreshDirectory();
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();

    // Breadcrumbs
    std::error_code ec;
    auto relPath = std::filesystem::relative(m_CurrentDirectory, m_RootDirectory, ec);
    std::string rel = (ec || relPath.empty() || relPath == ".") ? "Assets" : relPath.generic_string();

    ImGui::Text("  %s", rel.c_str());

    ImGui::PopStyleVar(2);
}

void ContentBrowserPanel::RenderGridView()
{
    float cellSize = m_ThumbnailSize + m_Padding;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1)
    {
        columnCount = 1;
    }

    ImGui::Columns(columnCount, nullptr, false);

    int i = 0;
    for (auto& asset : m_CurrentAssets)
    {
        ImGui::PushID(i++);

        const char* icon = asset.isDirectory ? ICON_FA_FOLDER : ICON_FA_FILE;

        // Custom Icons per type
        if (!asset.isDirectory)
        {
            switch (asset.type)
            {
            case EditorAssetType::Scene:
                icon = ICON_FA_CUBES;
                break;
            case EditorAssetType::Prefab:
                icon = ICON_FA_CUBE;
                break;
            case EditorAssetType::Model:
                icon = ICON_FA_SHAPES;
                break;
            case EditorAssetType::Texture:
                icon = ICON_FA_IMAGE;
                break;
            case EditorAssetType::Script:
                icon = ICON_FA_FILE_CODE;
                break;
            case EditorAssetType::Audio:
                icon = ICON_FA_MUSIC;
                break;
            default:
                icon = ICON_FA_FILE;
                break;
            }
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        // Center the icon button
        float availX = ImGui::GetContentRegionAvail().x;
        float offsetX = (availX - m_ThumbnailSize) * 0.5f;
        if (offsetX > 0.0f)
        {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        }

        if (ImGui::Button(icon, {m_ThumbnailSize, m_ThumbnailSize}))
        {
            // Single click selection logic could go here
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

            // Fancy drag preview
            ImGui::Text("%s %s", icon, asset.name.c_str());

            ImGui::EndDragDropSource();
        }

        ImGui::TextWrapped("%s", asset.name.c_str());

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);
}

void ContentBrowserPanel::OnAssetDoubleClicked(AssetEntry& entry)
{
    if (entry.isDirectory)
    {
        m_CurrentDirectory = entry.path;
        RefreshDirectory();
    }
    else if (entry.type == EditorAssetType::Scene)
    {
        SceneActions::Open(entry.path);
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
    {
        return;
    }

    std::string searchFilter = m_FilterBuffer;
    // Case-insensitive search
    std::transform(searchFilter.begin(), searchFilter.end(), searchFilter.begin(), ::tolower);

    for (auto& p : std::filesystem::directory_iterator(m_CurrentDirectory, ec))
    {
        AssetEntry entry;
        entry.name = p.path().filename().string();
        entry.path = p.path();
        entry.isDirectory = p.is_directory();
        entry.type = DetermineAssetType(p.path());

        // 1. Name Filter
        std::string nameLower = entry.name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        if (!searchFilter.empty() && nameLower.find(searchFilter) == std::string::npos)
        {
            continue;
        }

        // 2. Type Filter (Directories always shown)
        if (!entry.isDirectory && m_FilterType > 0)
        {
            // "All Types", "Scenes", "Prefabs", "Models", "Textures", "Scripts", "Audio"
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

EditorAssetType ContentBrowserPanel::DetermineAssetType(const std::filesystem::path& path)
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
    // ToLower extension
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto it = s_ExtensionMap.find(ext);
    if (it != s_ExtensionMap.end())
    {
        return it->second;
    }

    return EditorAssetType::Other;
}

void ContentBrowserPanel::SetRootDirectory(const std::filesystem::path& path)
{
    m_RootDirectory = path;
    m_CurrentDirectory = path;
    RefreshDirectory();
}
} // namespace CHEngine
