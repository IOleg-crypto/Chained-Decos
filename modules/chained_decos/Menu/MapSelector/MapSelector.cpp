#include "MapSelector.h"
#include "MenuConstants.h"
#include "scene/resources/map/Core/MapLoader.h"
#include <raylib.h>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <imgui/imgui.h>
#include <cctype> // For std::tolower

// Use constants from namespace
using namespace MenuConstants;

MapSelector::MapSelector() {
    UpdatePagination();
    // Initialize placeholder thumbnail
    m_placeholderThumbnail = LoadTexture("../resources/map_previews/placeholder.jpg");
    if (m_placeholderThumbnail.id == 0) {
        // Create a simple colored texture as placeholder
        Image img = GenImageColor(128, 128, GRAY);
        m_placeholderThumbnail = LoadTextureFromImage(img);
        UnloadImage(img);
    }
}

MapSelector::~MapSelector() {
    // Clean up textures
    for (auto& pair : m_thumbnails) {
        UnloadTexture(pair.second);
    }
    UnloadTexture(m_placeholderThumbnail);
}

void MapSelector::UpdatePagination() {
    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;
    if (maps.empty()) {
        m_totalPages = 0;
        m_currentPage = 0;
        return;
    }

    m_totalPages = (static_cast<int>(maps.size()) - 1) / MAPS_PER_PAGE + 1;
    if (m_currentPage >= m_totalPages) {
        m_currentPage = std::max(0, m_totalPages - 1);
    }
}

void MapSelector::NextPageNav() {
    if (m_currentPage < m_totalPages - 1) {
        m_currentPage++;
    }
}

void MapSelector::PreviousPageNav() {
    if (m_currentPage > 0) {
        m_currentPage--;
    }
}

int MapSelector::GetStartMapIndex() const {
    return m_currentPage * MAPS_PER_PAGE;
}

int MapSelector::GetEndMapIndex() const {
    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;
    return std::min(GetStartMapIndex() + MAPS_PER_PAGE, static_cast<int>(maps.size()));
}

void MapSelector::InitializeMaps() {
    m_availableMaps.clear();
    m_selectedMap = 0;
    m_currentPage = 0;

    // First, scan for all available JSON maps automatically
    ScanForJsonMaps();

    // Model-based maps removed as per user request

    // If no JSON maps or models found, add a fallback built-in map
    if (m_availableMaps.empty()) {
        TraceLog(LOG_WARNING, "MapSelector::InitializeMaps() - No JSON maps or models found, adding fallback built-in map");
        MapInfo fallbackMap = {
            FALLBACK_MAP_NAME,
            FALLBACK_MAP_DISPLAY_NAME,
            FALLBACK_MAP_DESCRIPTION,
            "",
            YELLOW,
            true,
            false  // isModelBased = false for built-in maps
        };
        AddMap(fallbackMap);
    } else {
        TraceLog(LOG_INFO, "MapSelector::InitializeMaps() - Total maps available: %d (JSON: %d)",
                  m_availableMaps.size(), m_jsonMapsCount);
    }

    // Initialize pagination
    UpdatePagination();
    LoadThumbnails();
    TraceLog(LOG_INFO, "MapSelector::InitializeMaps() - Pagination initialized: %d pages for %d maps",
                m_totalPages, m_availableMaps.size());
}

void MapSelector::ClearMaps() {
    m_availableMaps.clear();
    m_filteredMaps.clear();
    m_selectedMap = 0;
    m_currentPage = 0;
    UpdatePagination();
    m_searchQuery.clear();
    m_currentFilter = MapFilter::JSON;
    // Clean up thumbnails
    for (auto& pair : m_thumbnails) {
        UnloadTexture(pair.second);
    }
    m_thumbnails.clear();
}

void MapSelector::SetSearchQuery(const std::string& query) {
    m_searchQuery = query;
    UpdateFilters();
}

void MapSelector::SetFilter(MapFilter filter) {
    m_currentFilter = filter;
    UpdateFilters();
}

void MapSelector::UpdateFilters() {
    m_filteredMaps.clear();
    for (const auto& map : m_availableMaps) {
        bool matchesFilter = true;
        bool matchesSearch = true;

        // Apply filter
        if (m_currentFilter == MapFilter::JSON && map.isModelBased) {
            matchesFilter = false;
        }

        // Apply search
        if (!m_searchQuery.empty()) {
            std::string lowerQuery = m_searchQuery;
            std::string lowerName = map.displayName;
            std::string lowerDesc = map.description;
            std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);
            if (lowerName.find(lowerQuery) == std::string::npos && lowerDesc.find(lowerQuery) == std::string::npos) {
                matchesSearch = false;
            }
        }

        if (matchesFilter && matchesSearch) {
            m_filteredMaps.push_back(map);
        }
    }
    m_selectedMap = 0;
    m_currentPage = 0;
    UpdatePagination();
    LoadThumbnails();
}

void MapSelector::LoadThumbnails() {
    for (const auto& map : m_availableMaps) {
        LoadThumbnailForMap(map);
    }
}

void MapSelector::LoadThumbnailForMap(const MapInfo& map) {
    if (map.previewImage.empty()) {
        return;
    }
    std::string path = "../" + map.previewImage;
    Texture2D tex = LoadTexture(path.c_str());
    if (tex.id != 0) {
        m_thumbnails[map.name] = tex;
    }
}

Texture2D MapSelector::GetThumbnailForMap(const std::string& mapName) const {
    auto it = m_thumbnails.find(mapName);
    if (it != m_thumbnails.end()) {
        return it->second;
    }
    return m_placeholderThumbnail;
}

void MapSelector::AddMap(const MapInfo& mapInfo) {
    m_availableMaps.push_back(mapInfo);
}

void MapSelector::SelectNextMap() {
    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;
    if (m_selectedMap < static_cast<int>(maps.size()) - 1) {
        m_selectedMap++;
        // Check if we need to change page
        int newPage = m_selectedMap / MAPS_PER_PAGE;
        if (newPage != m_currentPage) {
            m_currentPage = newPage;
        }
    }
}

void MapSelector::SelectPreviousMap() {
    if (m_selectedMap > 0) {
        m_selectedMap--;
        // Check if we need to change page
        int newPage = m_selectedMap / MAPS_PER_PAGE;
        if (newPage != m_currentPage) {
            m_currentPage = newPage;
        }
    }
}

void MapSelector::SelectMap(int index) {
    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;
    if (index >= 0 && index < static_cast<int>(maps.size())) {
        m_selectedMap = index;
        // Update current page based on selected map
        m_currentPage = m_selectedMap / MAPS_PER_PAGE;
    }
}

void MapSelector::HandleKeyboardNavigation() {
    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;
    if (maps.empty()) return;

    int currentPageStart = GetStartMapIndex();
    int currentPageEnd = GetEndMapIndex();
    
    if (m_usePanelView) {
        // Panel view: simple vertical navigation (UP/DOWN only)
        if (IsKeyPressed(KEY_UP)) {
            if (m_selectedMap > currentPageStart) {
                m_selectedMap--;
            } else if (m_currentPage > 0) {
                // Go to previous page and select last map
                PreviousPageNav();
                m_selectedMap = GetEndMapIndex() - 1;
            } else if (m_selectedMap > 0) {
                // Wrap around to last map
                m_selectedMap = static_cast<int>(maps.size()) - 1;
            }
            // Update page if needed
            UpdatePagination();
            int newPageStart = GetStartMapIndex();
            if (m_selectedMap < newPageStart) {
                m_currentPage = m_selectedMap / MAPS_PER_PAGE;
                UpdatePagination();
            }
        } else if (IsKeyPressed(KEY_DOWN)) {
            if (m_selectedMap < currentPageEnd - 1) {
                m_selectedMap++;
            } else if (m_currentPage < m_totalPages - 1) {
                // Go to next page and select first map
                NextPageNav();
                m_selectedMap = GetStartMapIndex();
            } else if (m_selectedMap < static_cast<int>(maps.size()) - 1) {
                // Wrap around to first map
                m_selectedMap = 0;
                m_currentPage = 0;
                UpdatePagination();
            }
            // Update page if needed
            UpdatePagination();
            int newPageEnd = GetEndMapIndex();
            if (m_selectedMap >= newPageEnd) {
                m_currentPage = m_selectedMap / MAPS_PER_PAGE;
                UpdatePagination();
            }
        }
    } else {
        // Grid view: 4-column navigation (LEFT/RIGHT/UP/DOWN)
        int mapsPerRow = 4;
        int pageSize = currentPageEnd - currentPageStart;
        
        int row = (m_selectedMap - currentPageStart) / mapsPerRow;
        int col = (m_selectedMap - currentPageStart) % mapsPerRow;

        if (IsKeyPressed(KEY_LEFT)) {
            if (col > 0) {
                m_selectedMap--;
            } else if (row > 0) {
                // Wrap to end of previous row
                m_selectedMap = currentPageStart + (row - 1) * mapsPerRow + (mapsPerRow - 1);
                if (m_selectedMap >= currentPageEnd) {
                    m_selectedMap = currentPageEnd - 1;
                }
            }
        } else if (IsKeyPressed(KEY_RIGHT)) {
            int maxCol = (currentPageEnd - currentPageStart - row * mapsPerRow - 1) % mapsPerRow;
            if (col < mapsPerRow - 1 && m_selectedMap + 1 < currentPageEnd) {
                m_selectedMap++;
            } else if (row < (pageSize - 1) / mapsPerRow) {
                // Wrap to start of next row
                m_selectedMap = currentPageStart + (row + 1) * mapsPerRow;
                if (m_selectedMap >= currentPageEnd) {
                    m_selectedMap = currentPageEnd - 1;
                }
            }
        } else if (IsKeyPressed(KEY_UP)) {
            if (row > 0) {
                m_selectedMap -= mapsPerRow;
            } else if (m_currentPage > 0) {
                // Go to previous page
                PreviousPageNav();
                int newPageEnd = GetEndMapIndex();
                int newPageStart = GetStartMapIndex();
                int lastRow = (newPageEnd - newPageStart - 1) / mapsPerRow;
                m_selectedMap = newPageStart + lastRow * mapsPerRow + col;
                if (m_selectedMap >= newPageEnd) {
                    m_selectedMap = newPageEnd - 1;
                }
            }
        } else if (IsKeyPressed(KEY_DOWN)) {
            int maxRow = (pageSize - 1) / mapsPerRow;
            if (row < maxRow && m_selectedMap + mapsPerRow < currentPageEnd) {
                m_selectedMap += mapsPerRow;
            } else if (m_currentPage < m_totalPages - 1) {
                // Go to next page
                NextPageNav();
                int newPageStart = GetStartMapIndex();
                m_selectedMap = newPageStart + col;
                int newPageEnd = GetEndMapIndex();
                if (m_selectedMap >= newPageEnd) {
                    m_selectedMap = newPageEnd - 1;
                }
            }
        }
    }
    
    // Ensure selection is valid
    if (m_selectedMap < 0) m_selectedMap = 0;
    if (m_selectedMap >= static_cast<int>(maps.size())) {
        m_selectedMap = static_cast<int>(maps.size()) - 1;
    }
    
    // Update pagination if selection moved to different page
    int pageOfSelection = m_selectedMap / MAPS_PER_PAGE;
    if (pageOfSelection != m_currentPage) {
        m_currentPage = pageOfSelection;
        UpdatePagination();
    }
}

const MapInfo* MapSelector::GetSelectedMap() const {
    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;
    if (m_selectedMap >= 0 && m_selectedMap < static_cast<int>(maps.size())) {
        return &maps[m_selectedMap];
    }
    return nullptr;
}

std::string MapSelector::GetSelectedMapName() const {
    const MapInfo* selectedMapInfo = GetSelectedMap();
    if (selectedMapInfo) {
        // Check if this is a JSON map (indicated by file path starting with "maps/")
        if (selectedMapInfo->name.find("maps/") == 0 || selectedMapInfo->name.find(".json") != std::string::npos) {
            // Return the full path for JSON maps
            return "../" + selectedMapInfo->name;
        }
        else {
            // Return the map name for built-in maps
            return selectedMapInfo->name;
        }
    }
    return ""; // Default fallback
}

void MapSelector::ScanForJsonMaps() {
    m_jsonMapsCount = 0;

    try {
        namespace fs = std::filesystem;
        std::string rootDir = PROJECT_ROOT_DIR "/resources/maps";

        TraceLog(LOG_INFO, "MapSelector::ScanForJsonMaps() - Scanning for JSON map files...");
        TraceLog(LOG_INFO, "MapSelector::ScanForJsonMaps() - Project root directory: %s", rootDir.c_str());

        for (const std::string& dir : MAP_SEARCH_DIRECTORIES) {
            std::string fullDir = rootDir + dir;
            TraceLog(LOG_DEBUG, "MapSelector::ScanForJsonMaps() - Checking directory: %s", fullDir.c_str());

            if (!fs::exists(fullDir)) {
                TraceLog(LOG_DEBUG, "MapSelector::ScanForJsonMaps() - Directory does not exist: %s", fullDir.c_str());
                continue;
            }

            if (!fs::is_directory(fullDir)) {
                TraceLog(LOG_DEBUG, "MapSelector::ScanForJsonMaps() - Path is not a directory: %s", fullDir.c_str());
                continue;
            }

            TraceLog(LOG_INFO, "MapSelector::ScanForJsonMaps() - Scanning directory: %s", fullDir.c_str());

            int filesInDirectory = 0;
            for (const auto& entry : fs::directory_iterator(fullDir)) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                filesInDirectory++;
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();

                bool isJson = (extension == ".json" && filename != "game.cfg" && filename != "config.json");
                TraceLog(LOG_DEBUG, "MapSelector::ScanForJsonMaps() - File: %s, Extension: %s, Is JSON: %s", filename.c_str(), extension.c_str(), isJson ? "Yes" : "No");

                // Look for .json files (excluding system files)
                if (isJson) {
                    // Compute a relative path from the maps root directory and normalize separators
                    // Use lexical relative (no filesystem access) to avoid exceptions and overhead
                    std::filesystem::path relPath = std::filesystem::path(entry.path()).lexically_relative(rootDir);
                    std::string mapPath = relPath.generic_string(); // use forward slashes
                    // Remove any leading ./ or leading path separators
                    while (!mapPath.empty() && (mapPath.front() == '/' || mapPath.front() == '\\' || (mapPath.size() >= 2 && mapPath.substr(0,2) == "./"))) {
                        if (mapPath.size() >= 2 && mapPath.substr(0,2) == "./") mapPath = mapPath.substr(2);
                        else mapPath = mapPath.substr(1);
                    }

                    // Extract display name from filename (remove .json extension)
                    std::string displayName = filename.substr(0, filename.length() - 5); // Remove .json

                    // Capitalize first letter and improve formatting
                    if (!displayName.empty()) {
                        displayName[0] = toupper(displayName[0]);
                    }

                    // Replace underscores with spaces for better readability
                    std::replace(displayName.begin(), displayName.end(), '_', ' ');

                    // Create a more descriptive name
                    std::string fullDisplayName = displayName + " (Map)";

                    // Generate description based on file location or name
                    std::string description = "Custom map";
                    if (mapPath.find("parkour") != std::string::npos) {
                        description = "Parkour challenge map";
                    } else if (mapPath.find("exported") != std::string::npos) {
                        description = "Exported custom map";
                    }

                    // Assign color based on map type or name
                    Color mapColor = Color{255, 200, 100, 255}; // Default orange
                    if (displayName.find("Parkour") != std::string::npos) {
                        mapColor = SKYBLUE; // Blue for parkour maps
                    } else if (displayName.find("Exported") != std::string::npos) {
                        mapColor = LIME; // Green for exported maps
                    }

                    MapInfo mapInfo = {
                        mapPath,  // Store the relative path
                        fullDisplayName,
                        description,
                        "/resources/map_previews/custom_map.png",
                        mapColor,
                        true,
                        false  // isModelBased = false for JSON maps
                    };

                    AddMap(mapInfo);
                    m_jsonMapsCount++;
                    TraceLog(LOG_INFO, "MapSelector::ScanForJsonMaps() - Added map: %s (%s)", fullDisplayName.c_str(), mapPath.c_str());
                }
            }

            if (filesInDirectory == 0) {
                TraceLog(LOG_DEBUG, "MapSelector::ScanForJsonMaps() - No files found in directory: %s", fullDir.c_str());
            }
            TraceLog(LOG_INFO, "MapSelector::ScanForJsonMaps() - Directory %s contains %d files", fullDir.c_str(), filesInDirectory);
        }

        TraceLog(LOG_INFO, "MapSelector::ScanForJsonMaps() - Scan completed, found %d maps total", m_availableMaps.size());

        // List all found maps for debugging
        for (size_t i = 0; i < m_availableMaps.size(); ++i) {
            const auto& map = m_availableMaps[i];
            TraceLog(LOG_INFO, "MapSelector::ScanForJsonMaps() - Map %d: %s -> %s", i, map.displayName.c_str(), map.name.c_str());
        }
    } 
    catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "MapSelector::ScanForJsonMaps() - Exception while scanning for JSON maps: %s", e.what());
        TraceLog(LOG_INFO, "MapSelector::ScanForJsonMaps() - Continuing with any maps found so far");
    } 
}

void MapSelector::RenderMapSelection() const {
    // Constants for layout
    constexpr int MAP_BOX_WIDTH = 280;
    constexpr int MAP_BOX_HEIGHT = 160;
    constexpr int MARGIN = 20;
    constexpr int TOP_MARGIN = 150;

    // Calculate starting Y position for centering
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int totalContentHeight = (MAP_BOX_HEIGHT * 3) + MARGIN;
    int startY = (screenHeight - totalContentHeight) / 2;

    // Title
    const char* title = "SELECT MAP";
    int titleFontSize = TITLE_FONT_SIZE;
    int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title, (screenWidth - titleWidth) / 2, startY - 80, titleFontSize, WHITE);

    // Calculate which maps to show on current page
    int startIndex = GetStartMapIndex();
    int endIndex = GetEndMapIndex();

    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;

    // Render map selection boxes
    for (int i = startIndex; i < endIndex; ++i) {
        const auto& map = maps[i];
        int row = (i - startIndex) / 4;
        int col = (i - startIndex) % 4;

        int x = (screenWidth - (MAP_BOX_WIDTH * 4 + MARGIN * 3)) / 2 + col * (MAP_BOX_WIDTH + MARGIN);
        int y = startY + row * (MAP_BOX_HEIGHT + MARGIN);

        // Determine if this map is selected
        bool isSelected = (i == m_selectedMap);

        // Draw selection box
        Color boxColor = isSelected ? map.themeColor : Fade(map.themeColor, 0.3f);
        Color borderColor = isSelected ? WHITE : Fade(WHITE, 0.5f);

        // Main box
        DrawRectangle(x, y, MAP_BOX_WIDTH, MAP_BOX_HEIGHT, boxColor);
        DrawRectangleLines(x, y, MAP_BOX_WIDTH, MAP_BOX_HEIGHT, borderColor);

        // Map name
        int nameFontSize = NAME_FONT_SIZE;
        DrawText(map.displayName.c_str(), x + 10, y + 10, nameFontSize, BLACK);

        // Map description
        int descFontSize = DESCRIPTION_FONT_SIZE;
        int descriptionY = y + 40;
        DrawText(map.description.c_str(), x + 10, descriptionY, descFontSize, Fade(BLACK, 0.7f));

        // Map type indicator
        std::string typeText = map.isModelBased ? "Model-based" : "JSON Map";
        Color typeColor = map.isModelBased ? BLUE : GREEN;
        DrawText(typeText.c_str(), x + 10, y + MAP_BOX_HEIGHT - 30, 14, typeColor);

        // Selection indicator
        if (isSelected) {
            DrawRectangleLines(x - 2, y - 2, MAP_BOX_WIDTH + 4, MAP_BOX_HEIGHT + 4, YELLOW);
        }
    }

    // Render pagination info
    std::string pageInfo = TextFormat("Page %d of %d", m_currentPage + 1, m_totalPages);
    int pageInfoFontSize = PAGE_INFO_FONT_SIZE;
    int pageInfoWidth = MeasureText(pageInfo.c_str(), pageInfoFontSize);
    DrawText(pageInfo.c_str(), (screenWidth - pageInfoWidth) / 2, startY + MAP_BOX_HEIGHT * 2 + MARGIN + 20, pageInfoFontSize, WHITE);

    // Instructions
    const char* instructions = "[Arrow Keys: Navigate Maps] [Enter: Select Map] [Esc: Back to Menu]";
    int instrFontSize = INSTRUCTIONS_FONT_SIZE;
    int instrWidth = MeasureText(instructions, instrFontSize);
    DrawText(instructions, (screenWidth - instrWidth) / 2, screenHeight - 40, instrFontSize, Fade(WHITE, 0.7f));
}

// ImGui-based map selection rendering
void MapSelector::RenderMapSelectionImGui() {
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "MAP SELECTION");

    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;
    if (maps.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No maps available");
    } else {
        // Calculate which maps to show on current page
        int startIndex = GetStartMapIndex();
        int endIndex = GetEndMapIndex();

        // Render maps for current page
        for (int i = startIndex; i < endIndex; ++i) {
            const auto& map = maps[i];
            bool isSelected = (i == m_selectedMap);

            // Map button with styling
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 1.0f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }

            std::string buttonLabel = map.displayName + "##" + std::to_string(i);
            if (ImGui::Button(buttonLabel.c_str(), ImVec2(300, 50))) {
                SelectMap(i);
            }

            if (isSelected) {
                ImGui::PopStyleColor(2);
            }

            // Map details
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), map.description.c_str());

            // Map type indicator
            ImGui::SameLine(500);
            std::string typeText = map.isModelBased ? "Model-based" : "JSON Map";
            ImVec4 typeColor = map.isModelBased ?
                ImVec4(0.4f, 0.6f, 1.0f, 1.0f) : ImVec4(0.4f, 1.0f, 0.6f, 1.0f);
            ImGui::TextColored(typeColor, typeText.c_str());
        }

        // Pagination controls
        if (m_totalPages > 1) {
            ImGui::Separator();
            ImGui::Text("Page %d of %d", m_currentPage + 1, m_totalPages);

            ImGui::SameLine();
            if (m_currentPage > 0 && ImGui::Button("Previous Page")) {
                PreviousPageNav();
            }

            ImGui::SameLine();
            if (m_currentPage < m_totalPages - 1 && ImGui::Button("Next Page")) {
                NextPageNav();
            }
        }
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                       "Use Arrow Keys to navigate, ENTER to select, ESC for back");
}

// Panel-style map selection interface (Half-Life style with large horizontal panels)
void MapSelector::RenderMapSelectionPanels() {
    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;
    
    if (maps.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No maps available");
        return;
    }

    // Render maps as horizontal panels (scrollable)
    int startIndex = GetStartMapIndex();
    int endIndex = GetEndMapIndex();
    
    for (int i = startIndex; i < endIndex; ++i) {
        const auto& map = maps[i];
        bool isSelected = (i == m_selectedMap);

        // Create a panel for each map
        ImGui::PushID(i);
        
        // Panel frame with selection highlight
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.3f, 0.4f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);
        } else {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.25f, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.5f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
        }
        
        // Begin child window for panel (clickable)
        float panelWidth = ImGui::GetContentRegionAvail().x - 20;
        ImGui::BeginChild(("MapPanel##" + std::to_string(i)).c_str(), 
                         ImVec2(panelWidth, 180), true, 
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        
        // Make the entire panel clickable
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            SelectMap(i);
        }
        
        // Panel content: Image on left, text on right
        ImGui::Columns(2, ("MapPanelCols##" + std::to_string(i)).c_str(), false);
        ImGui::SetColumnWidth(0, 260); // Fixed width for image
        
        // Left column: Thumbnail (larger size)
        Texture2D thumb = GetThumbnailForMap(map.name);
        ImGui::Image((ImTextureID)(uintptr_t)thumb.id, ImVec2(240, 160));
        
        ImGui::NextColumn();
        
        // Right column: Map information
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        
        // Map name (larger, bold)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::SetWindowFontScale(1.2f);
        ImGui::Text("%s", map.displayName.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        
        // Description
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "%s", map.description.c_str());
        
        ImGui::Spacing();
        
        // Map type indicator
        std::string typeText = map.isModelBased ? "Model-based map" : "JSON Map";
        ImVec4 typeColor = map.isModelBased ?
            ImVec4(0.4f, 0.6f, 1.0f, 1.0f) : ImVec4(0.4f, 1.0f, 0.6f, 1.0f);
        ImGui::TextColored(typeColor, "%s", typeText.c_str());
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        // Select button (bottom right)
        float buttonWidth = 120.0f;
        ImGui::SetCursorPosX(ImGui::GetColumnWidth(1) - buttonWidth);
        std::string buttonLabel = "Select##" + std::to_string(i);
        if (ImGui::Button(buttonLabel.c_str(), ImVec2(buttonWidth, 35))) {
            SelectMap(i);
        }
        
        ImGui::Columns(1);
        ImGui::EndChild();
        
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        
        ImGui::PopID();
        
        ImGui::Spacing();
    }
}

// New window-style map selection interface
void MapSelector::RenderMapSelectionWindow() {
    ImVec2 windowSize = ImGui::GetWindowSize();

    // Begin a scrollable window, leaving space for buttons at the bottom
    ImGui::BeginChild("MapSelectionWindow", ImVec2(0, -100), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    // Title
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "MAP SELECTION");
    ImGui::Separator();

    const std::vector<MapInfo>& maps = m_filteredMaps.empty() ? m_availableMaps : m_filteredMaps;
    if (maps.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No maps available");
    } else {
        // Search and filter controls
        static char searchBuffer[256] = "";
        if (m_searchQuery.empty()) {
            strcpy(searchBuffer, "");
        } else {
            strcpy(searchBuffer, m_searchQuery.c_str());
        }
        if (ImGui::InputText("Search", searchBuffer, IM_ARRAYSIZE(searchBuffer))) {
            m_searchQuery = searchBuffer;
            UpdateFilters();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            m_searchQuery.clear();
            strcpy(searchBuffer, "");
            UpdateFilters();
        }

        ImGui::SameLine();
        
        // Checkbox to toggle between grid and panel view
        ImGui::Text("View:");
        ImGui::SameLine();
        if (ImGui::Checkbox("Panel View", &m_usePanelView)) {
            // View mode changed
        }
        
        ImGui::Separator();
        
        // Render based on view mode
        if (m_usePanelView) {
            // Use panel view
            RenderMapSelectionPanels();
            
            // Pagination for panel view
            if (m_totalPages > 1) {
                ImGui::Separator();
                ImGui::Text("Page %d of %d", m_currentPage + 1, m_totalPages);
                ImGui::SameLine();
                if (m_currentPage > 0 && ImGui::Button("Previous")) {
                    PreviousPageNav();
                }
                ImGui::SameLine();
                if (m_currentPage < m_totalPages - 1 && ImGui::Button("Next")) {
                    NextPageNav();
                }
            }
        } else {
            // Original grid view
            ImGui::Text("Maps:");
            ImGui::Columns(4, "MapGrid", false);
            int startIndex = GetStartMapIndex();
            int endIndex = GetEndMapIndex();
            for (int i = startIndex; i < endIndex; ++i) {
                const auto& map = maps[i];
                bool isSelected = (i == m_selectedMap);

                // Thumbnail
                Texture2D thumb = GetThumbnailForMap(map.name);
                ImGui::Image((ImTextureID)(uintptr_t)thumb.id, ImVec2(100, 100));
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                }

                // Map name
                ImGui::TextWrapped(map.displayName.c_str());
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), map.description.c_str());

                // Map type indicator
                std::string typeText = map.isModelBased ? "Model-based" : "JSON Map";
                ImVec4 typeColor = map.isModelBased ?
                    ImVec4(0.4f, 0.6f, 1.0f, 1.0f) : ImVec4(0.4f, 1.0f, 0.6f, 1.0f);
                ImGui::TextColored(typeColor, typeText.c_str());

                // Select button
                std::string buttonLabel = "Select##" + std::to_string(i);
                if (ImGui::Button(buttonLabel.c_str(), ImVec2(-1, 0))) {
                    SelectMap(i);
                }

                if (isSelected) {
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                }

                ImGui::NextColumn();
            }
            ImGui::Columns(1);

            // Pagination for grid view
            if (m_totalPages > 1) {
                ImGui::Separator();
                ImGui::Text("Page %d of %d", m_currentPage + 1, m_totalPages);
                ImGui::SameLine();
                if (m_currentPage > 0 && ImGui::Button("Previous")) {
                    PreviousPageNav();
                }
                ImGui::SameLine();
                if (m_currentPage < m_totalPages - 1 && ImGui::Button("Next")) {
                    NextPageNav();
                }
            }
        }
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                        "Use Arrow Keys to navigate, ENTER to select, ESC for back");

    // Log final cursor position and scroll info
    ImVec2 cursorPos = ImGui::GetCursorPos();
    float currentScrollY = ImGui::GetScrollY();
    //TraceLog(LOG_INFO, "MapSelector::RenderMapSelectionWindow() - Final cursor Y: %.0f, Current scroll Y: %.0f", cursorPos.y, currentScrollY);

    ImGui::EndChild();

    // // Log if scrolling is possible
    // float maxScrollY = ImGui::GetScrollMaxY();
    // if (maxScrollY > 0) {
    //     TraceLog(LOG_INFO, "MapSelector::RenderMapSelectionWindow() - Scrolling is possible, ScrollMaxY: %.0f", maxScrollY);
    // } else {
    //     TraceLog(LOG_INFO, "MapSelector::RenderMapSelectionWindow() - No scrolling needed, content fits");
    // }
}