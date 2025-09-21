//
// Created by AI Assistant
//

#ifndef OBJECTFILTER_H
#define OBJECTFILTER_H

#include <vector>
#include <string>
#include <functional>
#include <raylib.h>
#include <memory>

// Forward declaration
class MapObject;

// Filter criteria for object filtering
struct FilterCriteria
{
    std::string name;           // Name contains text
    int type;                   // Object type (-1 for any)
    std::string layer;          // Layer name (empty for any)
    bool visible;               // Visibility state (-1 for any)
    bool locked;                // Lock state (-1 for any)
    Vector3 positionMin;        // Minimum position bounds
    Vector3 positionMax;        // Maximum position bounds
    Vector3 scaleMin;           // Minimum scale bounds
    Vector3 scaleMax;           // Maximum scale bounds
    std::string modelName;      // Model name contains text
    std::string tags;           // Tags contain text
    
    FilterCriteria() 
        : type(-1), visible(-1), locked(-1),
          positionMin{-1000, -1000, -1000}, positionMax{1000, 1000, 1000},
          scaleMin{0, 0, 0}, scaleMax{100, 100, 100} {}
};

// Search result structure
struct SearchResult
{
    int objectIndex;
    float relevanceScore;
    std::string matchedProperty;
    std::string matchedText;
    
    SearchResult() : objectIndex(-1), relevanceScore(0.0f) {}
    SearchResult(int index, float score, const std::string& property, const std::string& text)
        : objectIndex(index), relevanceScore(score), matchedProperty(property), matchedText(text) {}
};

// Object filter and search manager
class ObjectFilter
{
private:
    FilterCriteria m_currentFilter;
    std::string m_searchQuery;
    std::vector<int> m_filteredIndices;
    std::vector<SearchResult> m_searchResults;
    bool m_filterActive;
    bool m_searchActive;
    
    // Filter functions
    std::function<bool(const MapObject&, const FilterCriteria&)> m_customFilter;
    
public:
    ObjectFilter();
    ~ObjectFilter() = default;
    
    // Filtering operations
    void SetFilterCriteria(const FilterCriteria& criteria);
    FilterCriteria GetFilterCriteria() const;
    void ClearFilter();
    bool IsFilterActive() const;
    
    // Search operations
    void SetSearchQuery(const std::string& query);
    std::string GetSearchQuery() const;
    void ClearSearch();
    bool IsSearchActive() const;
    
    // Apply filters and search
    std::vector<int> ApplyFilter(const std::vector<MapObject>& objects);
    std::vector<SearchResult> ApplySearch(const std::vector<MapObject>& objects);
    std::vector<int> GetFilteredIndices() const;
    std::vector<SearchResult> GetSearchResults() const;
    
    // Individual filter checks
    bool MatchesFilter(const MapObject& object, const FilterCriteria& criteria) const;
    bool MatchesSearch(const MapObject& object, const std::string& query, SearchResult& result) const;
    
    // Custom filter support
    void SetCustomFilter(std::function<bool(const MapObject&, const FilterCriteria&)> filter);
    void ClearCustomFilter();
    
    // Quick filters
    void FilterByType(const std::vector<MapObject>& objects, int type);
    void FilterByLayer(const std::vector<MapObject>& objects, const std::string& layerName);
    void FilterByVisibility(const std::vector<MapObject>& objects, bool visible);
    void FilterByLockState(const std::vector<MapObject>& objects, bool locked);
    void FilterByBounds(const std::vector<MapObject>& objects, Vector3 min, Vector3 max);
    
    // Advanced search
    std::vector<SearchResult> SearchByName(const std::vector<MapObject>& objects, const std::string& query);
    std::vector<SearchResult> SearchByTags(const std::vector<MapObject>& objects, const std::string& query);
    std::vector<SearchResult> SearchByModel(const std::vector<MapObject>& objects, const std::string& query);
    std::vector<SearchResult> SearchAll(const std::vector<MapObject>& objects, const std::string& query);
    
    // Filter statistics
    size_t GetFilteredCount() const;
    size_t GetSearchResultCount() const;
    std::string GetFilterSummary() const;
    
    // Filter presets
    void SaveFilterPreset(const std::string& name, const FilterCriteria& criteria);
    bool LoadFilterPreset(const std::string& name, FilterCriteria& criteria);
    std::vector<std::string> GetAvailablePresets() const;
    bool DeleteFilterPreset(const std::string& name);
    
    // Utility functions
    static bool StringContains(const std::string& text, const std::string& query, bool caseSensitive = false);
    static float CalculateRelevanceScore(const std::string& text, const std::string& query);
    static std::vector<std::string> SplitTags(const std::string& tags);
    static bool HasTag(const std::string& tags, const std::string& tag);
};

#endif // OBJECTFILTER_H
