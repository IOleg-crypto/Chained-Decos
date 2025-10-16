#include "Menu.h"
#include "../Map/MapLoader.h"
#include <raylib.h>
#include <iostream>
#include <filesystem>
#include <algorithm>


// Map selection methods
void Menu::InitializeMaps()
{
    m_availableMaps.clear();
    m_selectedMap = 0;
    m_currentPage = 0;

    // First, scan for all available JSON maps automatically
    ScanForJsonMaps();

    // Then, scan for models in the resources directory and create model-based maps
    std::string resourcesDir = PROJECT_ROOT_DIR "/resources";
    MapLoader loader;
    auto models = loader.LoadModelsFromDirectory(resourcesDir);

    if (!models.empty())
    {
        TraceLog(LOG_INFO, "Menu::InitializeMaps() - Found %d models in resources directory", models.size());

        // Create map entries for each model
        for (const auto& model : models)
        {
            std::string mapName = "model_" + model.name;
            std::string displayName = model.name + " (Model)";
            std::string description = "Model-based map using " + model.name;

            m_availableMaps.push_back({
                mapName,
                displayName,
                description,
                model.path,
                PURPLE,  // Color for model-based maps
                true,
                true     // isModelBased = true for model-based maps
            });

            TraceLog(LOG_INFO, "Menu::InitializeMaps() - Added model-based map: %s", displayName.c_str());
        }
    }

    // If no JSON maps or models found, add a fallback built-in map
    if (m_availableMaps.empty())
    {
        TraceLog(LOG_WARNING, "Menu::InitializeMaps() - No JSON maps or models found, adding fallback built-in map");
        m_availableMaps.push_back({
            "parkour_test",
            "Built-in Parkour",
            "Default parkour level with basic platforming",
            NULL,
            YELLOW,
            true,
            false  // isModelBased = false for built-in maps
        });
    }
    else
    {
        TraceLog(LOG_INFO, "Menu::InitializeMaps() - Total maps available: %d (JSON: %d, Models: %d)",
                 m_availableMaps.size(), m_jsonMapsCount, models.size());
    }

    // Initialize pagination
    UpdatePagination();
    TraceLog(LOG_INFO, "Menu::InitializeMaps() - Pagination initialized: %d pages for %d maps",
             m_totalPages, m_availableMaps.size());
}

const MapInfo* Menu::GetSelectedMap() const
{
    if (m_selectedMap >= 0 && m_selectedMap < static_cast<int>(m_availableMaps.size()))
    {
        return &m_availableMaps[m_selectedMap];
    }
    return nullptr;
}

std::string Menu::GetSelectedMapName() const
{
    const MapInfo* selectedMap = GetSelectedMap();
    if (selectedMap)
    {
        // Check if this is a JSON map (indicated by file path starting with "maps/")
        if (selectedMap->name.find("maps/") == 0 || selectedMap->name.find(".json") != std::string::npos)
        {
            // Return the full path for JSON maps
            return PROJECT_ROOT_DIR + selectedMap->name;
        }
        else if (selectedMap->name.find("model_") == 0)
        {
            // This is a model-based map - return the model path
            return selectedMap->previewImage; // previewImage field contains the model path for model-based maps
        }
        else
        {
            // Return the map name for built-in maps
            return selectedMap->name;
        }
    }
    return ""; // Default fallback
}

void Menu::ScanForJsonMaps()
{
    m_jsonMapsCount = 0;

    try
    {
        namespace fs = std::filesystem;
        std::string rootDir = PROJECT_ROOT_DIR;

        // Scan multiple potential map directories
        std::vector<std::string> searchDirectories = {
            rootDir + "/src/Game/Resource",
            rootDir + "/src/Game/Resource/maps",
            rootDir + "/resources/maps",
            rootDir + "/maps"
        };

        TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Scanning for JSON map files...");
        TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Project root directory: %s", rootDir.c_str());

        for (const std::string& dir : searchDirectories)
        {
            TraceLog(LOG_DEBUG, "Menu::ScanForJsonMaps() - Checking directory: %s", dir.c_str());

            if (!fs::exists(dir))
            {
                TraceLog(LOG_DEBUG, "Menu::ScanForJsonMaps() - Directory does not exist: %s", dir.c_str());
                continue;
            }

            if (!fs::is_directory(dir))
            {
                TraceLog(LOG_DEBUG, "Menu::ScanForJsonMaps() - Path is not a directory: %s", dir.c_str());
                continue;
            }

            TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Scanning directory: %s", dir.c_str());

            int filesInDirectory = 0;
            for (const auto& entry : fs::directory_iterator(dir))
            {
                if (!entry.is_regular_file())
                    continue;

                filesInDirectory++;
                std::string filename = entry.path().filename().string();
                std::string extension = entry.path().extension().string();

                TraceLog(LOG_DEBUG, "Menu::ScanForJsonMaps() - Found file: %s", filename.c_str());

                // Look for .json files (excluding system files)
                if (extension == ".json" &&
                    filename != "game.cfg" &&
                    filename != "config.json")
                {
                    std::string mapPath = entry.path().string();
                    // Convert to relative path from project root
                    if (mapPath.find(rootDir) == 0)
                    {
                        mapPath = mapPath.substr(rootDir.length());
                    }

                    // Extract display name from filename (remove .json extension)
                    std::string displayName = filename.substr(0, filename.length() - 5); // Remove .json

                    // Capitalize first letter and improve formatting
                    if (!displayName.empty())
                    {
                        displayName[0] = toupper(displayName[0]);
                    }

                    // Replace underscores with spaces for better readability
                    std::replace(displayName.begin(), displayName.end(), '_', ' ');

                    // Create a more descriptive name
                    std::string fullDisplayName = displayName + " (Map)";

                    // Generate description based on file location or name
                    std::string description = "Custom map";
                    if (mapPath.find("test") != std::string::npos)
                    {
                        description = "Test map for development";
                    }
                    else if (mapPath.find("parkour") != std::string::npos)
                    {
                        description = "Parkour challenge map";
                    }

                    // Assign color based on map type or name
                    Color mapColor = Color{255, 200, 100, 255}; // Default orange
                    if (displayName.find("Test") != std::string::npos)
                    {
                        mapColor = LIME; // Green for test maps
                    }
                    else if (displayName.find("Parkour") != std::string::npos)
                    {
                        mapColor = SKYBLUE; // Blue for parkour maps
                    }

                    m_availableMaps.push_back({
                        mapPath,  // Store the relative path
                        fullDisplayName,
                        description,
                        "/resources/map_previews/custom_map.png",
                        mapColor,
                        true,
                        false  // isModelBased = false for JSON maps
                    });

                    m_jsonMapsCount++;
                    TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Added map: %s (%s)", fullDisplayName.c_str(), mapPath.c_str());
                }
            }

            TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Directory %s contains %d files", dir.c_str(), filesInDirectory);
        }

        TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Scan completed, found %d maps total", m_availableMaps.size());

        // List all found maps for debugging
        for (size_t i = 0; i < m_availableMaps.size(); ++i)
        {
            const auto& map = m_availableMaps[i];
            TraceLog(LOG_INFO, "Menu::ScanForJsonMaps() - Map %d: %s -> %s", i, map.displayName.c_str(), map.name.c_str());
        }
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Menu::ScanForJsonMaps() - Error scanning for JSON maps: %s", e.what());
    }
}

void Menu::RenderMapSelection() const
{
    // Constants for layout
    constexpr int MAPS_PER_PAGE = 6; // 2 rows × 3 columns
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
    int titleFontSize = 48;
    int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title, (screenWidth - titleWidth) / 2, startY - 80, titleFontSize, WHITE);

    // Calculate which maps to show on current page
    int startIndex = m_currentPage * MAPS_PER_PAGE;
    int endIndex = std::min(startIndex + MAPS_PER_PAGE, static_cast<int>(m_availableMaps.size()));

    // Render map selection boxes
    for (int i = startIndex; i < endIndex; ++i)
    {
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
        int nameFontSize = 24;
        DrawText(map.displayName.c_str(), x + 10, y + 10, nameFontSize, BLACK);

        // Map description
        int descFontSize = 16;
        int descriptionY = y + 40;
        DrawText(map.description.c_str(), x + 10, descriptionY, descFontSize, Fade(BLACK, 0.7f));

        // Map type indicator
        std::string typeText = map.isModelBased ? "Model-based" : "JSON Map";
        Color typeColor = map.isModelBased ? BLUE : GREEN;
        DrawText(typeText.c_str(), x + 10, y + MAP_BOX_HEIGHT - 30, 14, typeColor);

        // Selection indicator
        if (isSelected)
        {
            DrawRectangleLines(x - 2, y - 2, MAP_BOX_WIDTH + 4, MAP_BOX_HEIGHT + 4, YELLOW);
        }
    }

    // Render pagination info
    std::string pageInfo = TextFormat("Page %d of %d", m_currentPage + 1, m_totalPages);
    int pageInfoFontSize = 20;
    int pageInfoWidth = MeasureText(pageInfo.c_str(), pageInfoFontSize);
    DrawText(pageInfo.c_str(), (screenWidth - pageInfoWidth) / 2, startY + MAP_BOX_HEIGHT * 2 + MARGIN + 20, pageInfoFontSize, WHITE);

    // Instructions
    const char* instructions = "[Arrow Keys/Arrow Right: Navigate Maps] [Enter: Select Map] [Esc: Back to Menu]";
    int instrFontSize = 16;
    int instrWidth = MeasureText(instructions, instrFontSize);
    DrawText(instructions, (screenWidth - instrWidth) / 2, screenHeight - 40, instrFontSize, Fade(WHITE, 0.7f));
}