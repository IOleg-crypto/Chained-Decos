#include "content_browser_panel.h"
#include "algorithm"
#include "editor/actions/scene_actions.h"
#include "engine/core/base.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "extras/IconsFontAwesome6.h"
#include "imgui.h"

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
            return;
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
        std::error_code ec;
        auto relPath = std::filesystem::relative(m_CurrentDirectory, m_RootDirectory, ec);
        std::string rel = (ec || relPath.empty() || relPath == ".") ? "Assets" : relPath.generic_string();

        ImGui::Text("Path: %s", rel.c_str());
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

    EditorAssetType ContentBrowserPanel::DetermineAssetType(const std::filesystem::path &path)
    {
        if (std::filesystem::is_directory(path))
            return EditorAssetType::Directory;

        std::string ext = path.extension().string();
        if (ext == ".chscene" || ext == ".chmap")
            return EditorAssetType::Scene;
        if (ext == ".h" || ext == ".cpp")
            return EditorAssetType::Script;
        if (ext == ".obj" || ext == ".gltf" || ext == ".glb")
            return EditorAssetType::Model;
        if (ext == ".png" || ext == ".jpg" || ext == ".tga")
            return EditorAssetType::Texture;
        if (ext == ".wav" || ext == ".ogg" || ext == ".mp3")
            return EditorAssetType::Audio;

        return EditorAssetType::Other;
    }

    void ContentBrowserPanel::SetRootDirectory(const std::filesystem::path &path)
    {
        m_RootDirectory = path;
        m_CurrentDirectory = path;
        RefreshDirectory();
    }
} // namespace CHEngine
