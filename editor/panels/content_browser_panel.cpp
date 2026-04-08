#include "content_browser_panel.h"

#include "engine/core/base.h"
#include "editor/editor_layer.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_events.h"
#include "IconsFontAwesome6.h"

#include "imgui.h"
#include <algorithm>
#include <fstream>
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

    ImGui::BeginDisabled(readOnly);
    RenderToolbar();
    ImGui::Separator();
    RenderGridView();

    ImGui::EndDisabled();
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
    
    if (ImGui::Button("Assets"))
    {
        m_CurrentDirectory = m_RootDirectory;
        RefreshDirectory();
    }

    if (!ec && !relPath.empty() && relPath != ".")
    {
        std::filesystem::path accumulated = m_RootDirectory;
        for (const auto& part : relPath)
        {
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
            
            accumulated /= part;
            if (ImGui::Button(part.string().c_str()))
            {
                m_CurrentDirectory = accumulated;
                RefreshDirectory();
                break;
            }
        }
    }

    // Icon Scale Slider (Right aligned)
    ImGui::SameLine(ImGui::GetWindowWidth() - 160.0f);
    ImGui::SetNextItemWidth(150.0f);
    ImGui::SliderFloat("##IconScale", &m_IconScale, 0.5f, 2.0f, ICON_FA_IMAGE);

    ImGui::PopStyleVar(2);
}

void ContentBrowserPanel::RenderGridView()
{
    float cellSize = (m_ThumbnailSize * m_IconScale) + m_Padding;
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

        ImGui::BeginGroup();
        
        // Thumbnail/Icon
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        float currentThumbnailSize = m_ThumbnailSize * m_IconScale;
        
        if (ImGui::Button(icon, {cellSize - m_Padding, currentThumbnailSize}))
        {
            // Clicked
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem(ICON_FA_PEN " Rename"))
            {
                m_RenamingPath = asset.path;
                strncpy(m_RenameBuffer, asset.name.c_str(), sizeof(m_RenameBuffer));
                ImGui::OpenPopup("RenameAsset");
            }
            if (ImGui::MenuItem(ICON_FA_TRASH " Delete"))
            {
                m_PathToDelete = asset.path;
                ImGui::OpenPopup("DeleteAsset?");
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("RenameAsset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter new name for %s:", m_RenamingPath.filename().string().c_str());
            ImGui::InputText("##NewName", m_RenameBuffer, sizeof(m_RenameBuffer));
            if (ImGui::Button("OK", {120, 0}))
            {
                std::filesystem::path newPath = m_RenamingPath.parent_path() / m_RenameBuffer;
                std::error_code ec;
                std::filesystem::rename(m_RenamingPath, newPath, ec);
                RefreshDirectory();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", {120, 0})) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("DeleteAsset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure you want to delete %s?\nThis operation cannot be undone!", m_PathToDelete.filename().string().c_str());
            if (ImGui::Button("Delete", {120, 0}))
            {
                std::error_code ec;
                std::filesystem::remove_all(m_PathToDelete, ec);
                RefreshDirectory();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", {120, 0})) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            OnAssetDoubleClicked(asset);
        }

        if (ImGui::BeginDragDropSource())
        {
            std::string pathStr = asset.path.string();
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
            ImGui::Text("%s %s", icon, asset.name.c_str());
            ImGui::EndDragDropSource();
        }

        ImGui::PopStyleColor();

        // Metadata/Label
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
        
        ImGui::SetNextItemWidth(cellSize - m_Padding);
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cellSize - m_Padding);
        
        float textWidth = ImGui::CalcTextSize(asset.name.c_str()).x;
        if (textWidth < cellSize - m_Padding)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (cellSize - m_Padding - textWidth) * 0.5f);
            
        ImGui::TextUnformatted(asset.name.c_str());
        ImGui::PopTextWrapPos();
        
        // Type Label
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        const char* typeLabel = asset.isDirectory ? "FOLDER" : "FILE";
        if (asset.type == EditorAssetType::Model) typeLabel = "MESH";
        else if (asset.type == EditorAssetType::Scene) typeLabel = "SCENE";
        else if (asset.type == EditorAssetType::Script) typeLabel = "SCRIPT";
        
        float typeLabelWidth = ImGui::CalcTextSize(typeLabel).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (cellSize - m_Padding - typeLabelWidth) * 0.5f);
        ImGui::TextDisabled("%s", typeLabel);
        ImGui::PopStyleColor();
        
        ImGui::PopStyleVar();
        ImGui::EndGroup();

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);

    // Empty space context menu
    if (ImGui::BeginPopupContextWindow(0, 1 | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::BeginMenu(ICON_FA_PLUS " Create"))
        {
            if (ImGui::MenuItem(ICON_FA_FOLDER " New Folder"))
            {
                std::filesystem::path newDir = m_CurrentDirectory / "New Folder";
                int i = 1;
                while (std::filesystem::exists(newDir))
                    newDir = m_CurrentDirectory / ("New Folder " + std::to_string(i++));
                std::filesystem::create_directory(newDir);
                RefreshDirectory();
            }
            if (ImGui::MenuItem(ICON_FA_FILE_CODE " New C# Script"))
            {
                std::filesystem::path newScript = m_CurrentDirectory / "NewScript.cs";
                int i = 1;
                while (std::filesystem::exists(newScript))
                    newScript = m_CurrentDirectory / ("NewScript" + std::to_string(i++) + ".cs");
                
                std::string className = newScript.stem().string();
                std::string templateContent = 
                    "using CHEngine;\n\n"
                    "namespace ChainedDecos.Scripts\n"
                    "{\n"
                    "    public class " + className + " : Script\n"
                    "    {\n"
                    "        public override void OnCreate()\n"
                    "        {\n"
                    "        }\n\n"
                    "        public override void OnUpdate(float deltaTime)\n"
                    "        {\n"
                    "        }\n"
                    "    }\n"
                    "}\n";
                
                std::ofstream ofs(newScript);
                ofs << templateContent;
                ofs.close();
                RefreshDirectory();
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
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
        EditorLayer::Get().OpenScene(entry.path);
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
