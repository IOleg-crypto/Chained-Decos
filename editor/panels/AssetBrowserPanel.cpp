#include "AssetBrowserPanel.h"
#include "editor/utils/IconsFontAwesome5.h"
#include "rlImGui.h"
#include <algorithm>
#include <imgui.h>

namespace CHEngine
{
AssetBrowserPanel::AssetBrowserPanel()
    : m_RootPath(PROJECT_ROOT_DIR "/resources"), m_CurrentDirectory(m_RootPath)
{
    RefreshAssets();
}

AssetBrowserPanel::~AssetBrowserPanel()
{
    for (auto &pair : m_ThumbnailCache)
    {
        ::UnloadTexture(pair.second);
    }
    m_ThumbnailCache.clear();
}

void AssetBrowserPanel::OnImGuiRender()
{
    ImGui::Begin("File manager");

    if (m_CurrentDirectory != m_RootPath)
    {
        if (ImGui::Button(ICON_FA_ARROW_LEFT " Back"))
        {
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
            RefreshAssets();
        }
        ImGui::Separator();
    }

    static float padding = 16.0f;
    static float thumbnailSize = 64.0f;
    float cellSize = thumbnailSize + padding;

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1)
        columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    for (auto &assetItem : m_Assets)
    {
        ImGui::PushID(assetItem.Name.c_str());

        // Icon Selection
        const char *icon = ICON_FA_FILE;
        Texture2D *thumbnail = nullptr;

        if (assetItem.IsDirectory)
        {
            icon = ICON_FA_FOLDER;
        }
        else
        {
            std::string ext = assetItem.Path.extension().string();
            // Convert to lowercase for comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".glb" || ext == ".obj")
                icon = ICON_FA_CUBE;
            else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
            {
                icon = ICON_FA_IMAGE;
                std::string pathStr = assetItem.Path.string();

                if (m_ThumbnailCache.find(pathStr) == m_ThumbnailCache.end())
                {
                    Texture2D tex = ::LoadTexture(pathStr.c_str());
                    if (tex.id != 0)
                        m_ThumbnailCache[pathStr] = tex;
                }

                if (m_ThumbnailCache.find(pathStr) != m_ThumbnailCache.end())
                    thumbnail = &m_ThumbnailCache[pathStr];
            }
            else if (ext == ".lua" || ext == ".cpp" || ext == ".h")
                icon = ICON_FA_FILE_CODE;
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        bool clicked = false;
        if (thumbnail)
        {
            clicked = rlImGuiImageButtonSize(assetItem.Name.c_str(), thumbnail,
                                             {thumbnailSize, thumbnailSize});
        }
        else
        {
            clicked = ImGui::Button(icon, {thumbnailSize, thumbnailSize});
        }

        if (ImGui::BeginDragDropSource())
        {
            std::string itemPath = assetItem.Path.string();
            const char *itemPathStr = itemPath.c_str();
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPathStr,
                                      (strlen(itemPathStr) + 1) * sizeof(char));
            ImGui::Text("%s", assetItem.Name.c_str());
            ImGui::EndDragDropSource();
        }

        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (assetItem.IsDirectory)
                {
                    m_CurrentDirectory /= assetItem.Path.filename();
                    RefreshAssets();
                }
            }
            ImGui::SetTooltip("%s", assetItem.Name.c_str());
        }

        ImGui::TextWrapped("%s", assetItem.Name.c_str());

        ImGui::NextColumn();

        ImGui::PopID();
    }

    ImGui::Columns(1);

    // ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
    // ImGui::SliderFloat("Padding", &padding, 0, 32);

    ImGui::End();
}

void AssetBrowserPanel::RefreshAssets()
{
    m_Assets.clear();

    for (auto &directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
    {
        const auto &path = directoryEntry.path();
        auto relativePath = std::filesystem::relative(path, m_RootPath);
        std::string filenameString = relativePath.filename().string();

        m_Assets.push_back({filenameString, path, directoryEntry.is_directory()});
    }

    // Sort: folders first, then names
    std::ranges::sort(m_Assets,
                      [](const AssetItem &a, const AssetItem &b)
                      {
                          if (a.IsDirectory != b.IsDirectory)
                              return a.IsDirectory;
                          return a.Name < b.Name;
                      });
}
} // namespace CHEngine
