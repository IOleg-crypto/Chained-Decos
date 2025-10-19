#include "MapSelector.h"
#include "../Map/MapLoader.h"
#include <raylib.h>
#include <iostream>
#include <filesystem>
#include <algorithm>

MapSelector::MapSelector() {
    UpdatePagination();
}

void MapSelector::UpdatePagination() {
    if (m_availableMaps.empty()) {
        m_totalPages = 0;
        m_currentPage = 0;
        return;
    }

    m_totalPages = (static_cast<int>(m_availableMaps.size()) - 1) / MAPS_PER_PAGE + 1;
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
    return m_currentPage;
}

int MapSelector::GetEndMapIndex() const {
    return std::min(GetStartMapIndex() + MAPS_PER_PAGE, static_cast<int>(m_availableMaps.size()));
}

void MapSelector::InitializeMaps() {
    m_availableMaps.clear();
    m_selectedMap = 0;
    m_currentPage = 0;

    // First, scan for all available JSON maps automatically
    ScanForJsonMaps();

    // Then, scan for models in the resources directory and create model-based maps
    std::string resourcesDir = PROJECT_ROOT_DIR "/resources";
    MapLoader loader;
    auto models = loader.LoadModelsFromDirectory(resourcesDir);

    if (!models.empty()) {
        TraceLog(LOG_INFO, "MapSelector::InitializeMaps() - Found %d models in resources directory", models.size());

        // Create map entries for each model
        for (const auto& model : models) {
            std::string mapName = "model_" + model.name;
            std::string displayName = model.name + " (Model)";
            std::string description = "Model-based map using " + model.name;

            MapInfo mapInfo = {
                mapName,
                displayName,
                description,
                model.path,
                PURPLE,  // Color for model-based maps
                true,
                true     // isModelBased = true for model-based maps
            };

            AddMap(mapInfo);
            TraceLog(LOG_INFO, "MapSelector::InitializeMaps() - Added model-based map: %s", displayName.c_str());
        }
    }

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
        TraceLog(LOG_INFO, "MapSelector::InitializeMaps() - Total maps available: %d (JSON: %d, Models: %d)",
                 m_availableMaps.size(), m_jsonMapsCount, models.size());
    }

    // Initialize pagination
    UpdatePagination();
    TraceLog(LOG_INFO, "MapSelector::InitializeMaps() - Pagination initialized: %d pages for %d maps",
              totalPages, availableMaps.size());
}

void MapSelector::AddMap(const MapInfo& mapInfo) {
    m_availableMaps.push_back(mapInfo);
}

void MapSelector::SelectNextMap() {
    if (m_selectedMap < static_cast<int>(m_availableMaps.size()) - 1) {
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
    if (index >= 0 && index < static_cast<int>(m_availableMaps.size())) {
        m_selectedMap = index;
        // Update current page based on selected map
        m_currentPage = m_selectedMap / MAPS_PER_PAGE;
    }
}

const MapInfo* MapSelector::GetSelectedMap() const {
    if (m_selectedMap >= 0 && m_selectedMap < static_cast<int>(m_availableMaps.size())) {
        return &m_availableMaps[m_selectedMap];
    }
    return nullptr;
}

std::string MapSelector::GetSelectedMapName() const {
    const MapInfo* selectedMapInfo = GetSelectedMap();
    if (selectedMapInfo) {
        // Check if this is a JSON map (indicated by file path starting with "maps/")
        if (selectedMapInfo->name.find("maps/") == 0 || selectedMapInfo->name.find(".json") != std::string::npos) {
            // Return the full path for JSON maps
            return PROJECT_ROOT_DIR + selectedMapInfo->name;
        } 
        else {
            // Return the map name for built-in maps
            return selectedMapInfo->name;
        }
    }
    return ""; // Default fallback
}

void MapSelector::ScanForJsonMaps() {
    jsonMapsCount = 0;

    try {
        namespace fs = std::filesystem;
        std::string rootDir = PROJECT_ROOT_DIR;

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

                TraceLog(LOG_DEBUG, "MapSelector::ScanForJsonMaps() - Found file: %s", filename.c_str());

                // Look for .json files (excluding system files)
                if (extension == ".json" &&
                    filename != "game.cfg" &&
                    filename != "config.json") {
                    std::string mapPath = entry.path().string();
                    // Convert to relative path from project root
                    if (mapPath.find(rootDir) == 0) {
                        mapPath = mapPath.substr(rootDir.length());
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
                    if (mapPath.find("test") != std::string::npos) {
                        description = "Test map for development";
                    } else if (mapPath.find("parkour") != std::string::npos) {
                        description = "Parkour challenge map";
                    }

                    // Assign color based on map type or name
                    Color mapColor = Color{255, 200, 100, 255}; // Default orange
                    if (displayName.find("Test") != std::string::npos) {
                        mapColor = LIME; // Green for test maps
                    } else if (displayName.find("Parkour") != std::string::npos) {
                        mapColor = SKYBLUE; // Blue for parkour maps
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
                    jsonMapsCount++;
                    TraceLog(LOG_INFO, "MapSelector::ScanForJsonMaps() - Added map: %s (%s)", fullDisplayName.c_str(), mapPath.c_str());
                }
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
    int totalContentHeight = (MAP_BOX_HEIGHT * 2) + MARGIN;
    int startY = (screenHeight - totalContentHeight) / 2;

    // Title
    const char* title = "SELECT MAP";
    int titleFontSize = TITLE_FONT_SIZE;
    int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title, (screenWidth - titleWidth) / 2, startY - 80, titleFontSize, WHITE);

    // Calculate which maps to show on current page
    int startIndex = GetStartMapIndex();
    int endIndex = GetEndMapIndex();

    // Render map selection boxes
    for (int i = startIndex; i < endIndex; ++i) {
        const auto& map = m_availableMaps[i];
        int row = (i - startIndex) / 3;
        int col = (i - startIndex) % 3;

        int x = (screenWidth - (MAP_BOX_WIDTH * 3 + MARGIN * 2)) / 2 + col * (MAP_BOX_WIDTH + MARGIN);
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
    std::string pageInfo = TextFormat("Page %d of %d", currentPage + 1, totalPages);
    int pageInfoFontSize = PAGE_INFO_FONT_SIZE;
    int pageInfoWidth = MeasureText(pageInfo.c_str(), pageInfoFontSize);
    DrawText(pageInfo.c_str(), (screenWidth - pageInfoWidth) / 2, startY + MAP_BOX_HEIGHT * 2 + MARGIN + 20, pageInfoFontSize, WHITE);

    // Instructions
    const char* instructions = "[Arrow Keys: Navigate Maps] [Enter: Select Map] [Esc: Back to Menu]";
    int instrFontSize = INSTRUCTIONS_FONT_SIZE;
    int instrWidth = MeasureText(instructions, instrFontSize);
    DrawText(instructions, (screenWidth - instrWidth) / 2, screenHeight - 40, instrFontSize, Fade(WHITE, 0.7f));
}