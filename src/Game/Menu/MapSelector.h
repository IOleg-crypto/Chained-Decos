/**
 * @file MapSelector.h
 * @brief Map selection and management system
 *
 * The MapSelector class handles the discovery, organization, and selection
 * of game maps from various sources including JSON files and 3D models.
 */

#ifndef MAP_SELECTOR_H
#define MAP_SELECTOR_H

#include <string>
#include <vector>
#include "MenuConstants.h"

namespace MenuConstants {
    using namespace MenuConstants;
}

/**
 * @struct MapInfo
 * @brief Information about a game map
 */
struct MapInfo {
    std::string name;
    std::string displayName;
    std::string description;
    std::string previewImage; // Path to preview image
    Color themeColor;
    bool isAvailable;
    bool isModelBased; // New field to distinguish model-based maps from JSON maps
};

/**
 * @class MapSelector
 * @brief Handles map discovery, selection, and pagination
 *
 * This class manages the collection of available game maps, including
 * automatic discovery of JSON map files and 3D model-based maps.
 * It provides pagination, selection, and rendering functionality.
 */
class MapSelector {
private:
    std::vector<MapInfo> m_availableMaps;
    int m_selectedMap = 0;
    int m_currentPage = 0;
    int m_totalPages = 0;
    int m_jsonMapsCount = 0; // Track number of JSON maps for logging

    // Pagination system
    void UpdatePagination();
    void NextPage();
    void PreviousPage();
    int GetStartMapIndex() const;
    int GetEndMapIndex() const;

public:
    /**
     * @brief Default constructor that initializes pagination
     */
    MapSelector();

    // Map management
    void InitializeMaps();
    void ScanForJsonMaps();
    void AddMap(const MapInfo& mapInfo);

    // Map selection
    void SelectNextMap();
    void SelectPreviousMap();
    void SelectMap(int index);

    // Navigation
    void NextPageNav();
    void PreviousPageNav();

    // Getters
    const MapInfo* GetSelectedMap() const;
    std::string GetSelectedMapName() const;
    const std::vector<MapInfo>& GetAvailableMaps() const { return m_availableMaps; }
    int GetSelectedMapIndex() const { return m_selectedMap; }
    int GetCurrentPage() const { return m_currentPage; }
    int GetTotalPages() const { return m_totalPages; }
    int GetJsonMapsCount() const { return m_jsonMapsCount; }

    // Rendering
    void RenderMapSelection() const;
    void RenderMapSelectionImGui() const;

    // Utility
    bool HasMaps() const { return !m_availableMaps.empty(); }
    void ClearMaps() { m_availableMaps.clear(); m_selectedMap = 0; m_currentPage = 0; UpdatePagination(); }

private:
    // Validation helpers
    bool IsValidMapIndex(int index) const { return index >= 0 && index < static_cast<int>(m_availableMaps.size()); }
    void ValidateSelection();

    // Internal scanning helpers
    void ScanDirectoryForMaps(const std::string& directory);
    MapInfo CreateMapInfoFromFile(const std::string& filePath, const std::string& rootDir);
};

#endif // MAP_SELECTOR_H