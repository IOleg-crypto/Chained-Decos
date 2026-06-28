#include "content_browser_panel.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "editor/editor_action_commands.h"
#include "editor/editor_layer.h"
#include "engine/core/log.h"
#include "engine/project/project.h"
#include "engine/scene/scene_events.h"
#include "imgui.h"

namespace Chained
{

ContentBrowserPanel::ContentBrowserPanel()
{
    m_Name = "Content Browser";
    m_Provider = std::make_unique<ContentBrowserProvider>();

    if (auto project = Project::GetActive())
    {
        m_Provider->SetRoot(project->GetAssetDirectoryForProject());
    }
    else
    {
        m_Provider->SetRoot(std::filesystem::current_path() / "assets");
    }
}

ContentBrowserPanel::~ContentBrowserPanel() = default;

void ContentBrowserPanel::OnImGuiRender(bool readOnly)
{
    if (!m_NextDirectory.empty())
    {
        m_Provider->Navigate(m_NextDirectory);
        m_NextDirectory.clear();
    }

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
        if (auto project = Project::GetActive())
        {
            m_Provider->SetRoot(project->GetAssetDirectoryForProject());
        }
        return false;
    });
}

void ContentBrowserPanel::RenderToolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

    if (m_Provider->GetCurrentDirectory() != m_Provider->GetRootDirectory())
    {
        if (ImGui::Button(ICON_FA_ARROW_LEFT))
        {
            m_Provider->GoUp();
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

    if (ImGui::InputTextWithHint("##Search", ICON_FA_MAGNIFYING_GLASS " Search...", m_FilterBuffer,
                                 sizeof(m_FilterBuffer)))
    {
        m_Provider->SetFilter(m_FilterBuffer, m_FilterType);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    const char* filterNames[] = {"All Types", "Scenes", "Prefabs", "Models", "Textures", "Scripts", "Audio"};
    if (ImGui::BeginCombo("##TypeFilter", filterNames[m_FilterType]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(filterNames); i++)
        {
            if (ImGui::Selectable(filterNames[i], m_FilterType == i))
            {
                m_FilterType = i;
                m_Provider->SetFilter(m_FilterBuffer, m_FilterType);
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Assets"))
    {
        m_Provider->GoToRoot();
    }

    // Breadcrumbs
    std::error_code ec;
    auto relPath = std::filesystem::relative(m_Provider->GetCurrentDirectory(), m_Provider->GetRootDirectory(), ec);
    if (!ec && !relPath.empty() && relPath != ".")
    {
        std::filesystem::path accumulated = m_Provider->GetRootDirectory();
        for (const auto& part : relPath)
        {
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
            accumulated /= part;
            if (ImGui::Button(part.string().c_str()))
            {
                m_Provider->Navigate(accumulated);
                break;
            }
        }
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - 160.0f);
    ImGui::SetNextItemWidth(150.0f);
    ImGui::SliderFloat("##IconScale", &m_IconScale, 0.5f, 2.0f, ICON_FA_IMAGE);

    ImGui::PopStyleVar(2);
}

void ContentBrowserPanel::RenderGridView()
{
    float cellSize = (m_ThumbnailSize * m_IconScale) + m_Padding;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = std::max(1, (int)(panelWidth / cellSize));

    ImGui::Columns(columnCount, nullptr, false);

    const auto& assets = m_Provider->GetAssets();
    if (assets.empty())
    {
        ImGui::TextDisabled("Empty directory or No assets found matching filters.");
        ImGui::Columns(1);
    }
    else
    {
        int i = 0;
        for (const auto& asset : assets)
        {
            ImGui::PushID(i++);
            const char* icon = asset.isDirectory ? ICON_FA_FOLDER : ICON_FA_FILE;
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
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            if (ImGui::Button(icon, {cellSize - m_Padding, m_ThumbnailSize * m_IconScale}))
            {
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

            // Popups
            if (ImGui::BeginPopupModal("RenameAsset", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Enter new name:");
                ImGui::InputText("##NewName", m_RenameBuffer, sizeof(m_RenameBuffer));
                if (ImGui::Button("OK", {120, 0}))
                {
                    EditorActionCommands::RenameAsset(m_RenamingPath, m_RenameBuffer);
                    m_Provider->Refresh();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", {120, 0}))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopupModal("DeleteAsset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Delete %s?", m_PathToDelete.filename().string().c_str());
                if (ImGui::Button("Delete", {120, 0}))
                {
                    EditorActionCommands::DeleteAsset(m_PathToDelete);
                    m_Provider->Refresh();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", {120, 0}))
                {
                    ImGui::CloseCurrentPopup();
                }
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
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
            ImGui::SetNextItemWidth(cellSize - m_Padding);
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cellSize - m_Padding);
            ImGui::TextUnformatted(asset.name.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleVar();
            ImGui::EndGroup();

            ImGui::NextColumn();
            ImGui::PopID();
        }
        ImGui::Columns(1);
    }

    if (ImGui::BeginPopupContextWindow(0, 1 | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::BeginMenu(ICON_FA_PLUS " Create"))
        {
            if (ImGui::MenuItem(ICON_FA_FOLDER " New Folder"))
            {
                EditorActionCommands::CreateFolder(m_Provider->GetCurrentDirectory());
                m_Provider->Refresh();
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
}

void ContentBrowserPanel::OnAssetDoubleClicked(const AssetEntry& entry)
{
    if (entry.isDirectory)
    {
        m_NextDirectory = entry.path;
    }
    else if (entry.type == EditorAssetType::Scene)
    {
        EditorLayer::Get().GetSceneManager().OpenScene(entry.path);
    }
}

void ContentBrowserPanel::RefreshDirectory() const
{
    m_Provider->Refresh();
}

void ContentBrowserPanel::SetRootDirectory(const std::filesystem::path& path) const
{
    m_Provider->SetRoot(path);
}

} // namespace Chained
