/**
 * @file MapSelector.h
 * The MapSelector class handles the discovery, organization, and selection
 * of game maps from various sources including JSON files and 3D models.
 */

#ifndef MAP_SELECTOR_H
#define MAP_SELECTOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include "MenuConstants.h"
#include <raylib.h> // For Texture2D


/**
 * @struct MapInfo
 * Information about a game map
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
 * Handles map discovery, selection, and pagination.
 * This class manages the collection of available game maps, including
 * automatic discovery of JSON map files and 3D model-based maps.
 * It provides pagination, selection, and rendering functionality.
 */
class MapSelector {
private:
    std::vector<MapInfo> m_availableMaps;
    std::vector<MapInfo> m_filteredMaps; // For search and filtering
    int m_selectedMap = 0;
    int m_currentPage = 0;
    int m_totalPages = 0;
    int m_jsonMapsCount = 0; // Track number of JSON maps for logging

    // New UI variables
    std::string m_searchQuery;
    enum class MapFilter { JSON };
    MapFilter m_currentFilter = MapFilter::JSON;
    std::unordered_map<std::string, Texture2D> m_thumbnails; // Map name to texture
    Texture2D m_placeholderThumbnail; // Placeholder for missing thumbnails

    // Pagination system
    void UpdatePagination();
    void NextPage();
    void PreviousPage();
    int GetStartMapIndex() const;
    int GetEndMapIndex() const;

    // New filtering and thumbnail methods
    void ApplyFilters();
    void LoadThumbnails();
    void LoadThumbnailForMap(const MapInfo& map);
    Texture2D GetThumbnailForMap(const std::string& mapName) const;

public:
    /**
     * Default constructor that initializes pagination
     */
    MapSelector();

    /**
     * Destructor that cleans up resources
     */
    ~MapSelector();

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
    void RenderMapSelectionImGui();
    void RenderMapSelectionWindow(); // New window-style interface

    // New UI methods
    void SetSearchQuery(const std::string& query);
    void SetFilter(MapFilter filter);
    void UpdateFilters();
    void HandleKeyboardNavigation();

    // Utility
    bool HasMaps() const { return !m_availableMaps.empty(); }
    void ClearMaps();

private:
    // Validation helpers
    bool IsValidMapIndex(int index) const { return index >= 0 && index < static_cast<int>(m_availableMaps.size()); }
    void ValidateSelection();

    // Internal scanning helpers
    void ScanDirectoryForMaps(const std::string& directory);
    MapInfo CreateMapInfoFromFile(const std::string& filePath, const std::string& rootDir);
};

#endif // MAP_SELECTOR_H