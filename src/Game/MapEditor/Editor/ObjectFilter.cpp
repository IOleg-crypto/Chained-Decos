//
// Created by AI Assistant
//

#include "ObjectFilter.h"
#include "MapObject.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <fstream>
#include <map>

ObjectFilter::ObjectFilter() : m_filterActive(false), m_searchActive(false)
{
}

void ObjectFilter::SetFilterCriteria(const FilterCriteria& criteria)
{
    m_currentFilter = criteria;
    m_filterActive = true;
}

FilterCriteria ObjectFilter::GetFilterCriteria() const
{
    return m_currentFilter;
}

void ObjectFilter::ClearFilter()
{
    m_currentFilter = FilterCriteria();
    m_filteredIndices.clear();
    m_filterActive = false;
}

bool ObjectFilter::IsFilterActive() const
{
    return m_filterActive;
}

void ObjectFilter::SetSearchQuery(const std::string& query)
{
    m_searchQuery = query;
    m_searchActive = !query.empty();
}

std::string ObjectFilter::GetSearchQuery() const
{
    return m_searchQuery;
}

void ObjectFilter::ClearSearch()
{
    m_searchQuery.clear();
    m_searchResults.clear();
    m_searchActive = false;
}

bool ObjectFilter::IsSearchActive() const
{
    return m_searchActive;
}

std::vector<int> ObjectFilter::ApplyFilter(const std::vector<MapObject>& objects)
{
    m_filteredIndices.clear();
    
    if (!m_filterActive)
    {
        // Return all indices if no filter is active
        for (size_t i = 0; i < objects.size(); ++i)
        {
            m_filteredIndices.push_back(static_cast<int>(i));
        }
        return m_filteredIndices;
    }
    
    for (size_t i = 0; i < objects.size(); ++i)
    {
        if (MatchesFilter(objects[i], m_currentFilter))
        {
            m_filteredIndices.push_back(static_cast<int>(i));
        }
    }
    
    return m_filteredIndices;
}

std::vector<SearchResult> ObjectFilter::ApplySearch(const std::vector<MapObject>& objects)
{
    m_searchResults.clear();
    
    if (!m_searchActive || m_searchQuery.empty())
    {
        return m_searchResults;
    }
    
    for (size_t i = 0; i < objects.size(); ++i)
    {
        SearchResult result;
        if (MatchesSearch(objects[i], m_searchQuery, result))
        {
            result.objectIndex = static_cast<int>(i);
            m_searchResults.push_back(result);
        }
    }
    
    // Sort by relevance score (highest first)
    std::sort(m_searchResults.begin(), m_searchResults.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.relevanceScore > b.relevanceScore;
        });
    
    return m_searchResults;
}

std::vector<int> ObjectFilter::GetFilteredIndices() const
{
    return m_filteredIndices;
}

std::vector<SearchResult> ObjectFilter::GetSearchResults() const
{
    return m_searchResults;
}

bool ObjectFilter::MatchesFilter(const MapObject& object, const FilterCriteria& criteria) const
{
    // Name filter
    if (!criteria.name.empty() && !StringContains(object.GetName(), criteria.name))
        return false;
    
    // Type filter
    if (criteria.type >= 0 && object.GetType() != criteria.type)
        return false;
    
    // Model name filter
    if (!criteria.modelName.empty() && !StringContains(object.GetModelName(), criteria.modelName))
        return false;
    
    // Tags filter
    if (!criteria.tags.empty() && !StringContains(criteria.tags, criteria.tags))
        return false;
    
    // Position bounds filter
    Vector3 pos = object.GetPosition();
    if (pos.x < criteria.positionMin.x || pos.x > criteria.positionMax.x ||
        pos.y < criteria.positionMin.y || pos.y > criteria.positionMax.y ||
        pos.z < criteria.positionMin.z || pos.z > criteria.positionMax.z)
        return false;
    
    // Scale bounds filter
    Vector3 scale = object.GetScale();
    if (scale.x < criteria.scaleMin.x || scale.x > criteria.scaleMax.x ||
        scale.y < criteria.scaleMin.y || scale.y > criteria.scaleMax.y ||
        scale.z < criteria.scaleMin.z || scale.z > criteria.scaleMax.z)
        return false;
    
    // Custom filter
    if (m_customFilter && !m_customFilter(object, criteria))
        return false;
    
    return true;
}

bool ObjectFilter::MatchesSearch(const MapObject& object, const std::string& query, SearchResult& result) const
{
    float bestScore = 0.0f;
    std::string bestMatch;
    std::string bestProperty;
    
    // Search in name
    if (StringContains(object.GetName(), query))
    {
        float score = CalculateRelevanceScore(object.GetName(), query);
        if (score > bestScore)
        {
            bestScore = score;
            bestMatch = object.GetName();
            bestProperty = "Name";
        }
    }
    
    // Search in model name
    if (StringContains(object.GetModelName(), query))
    {
        float score = CalculateRelevanceScore(object.GetModelName(), query);
        if (score > bestScore)
        {
            bestScore = score;
            bestMatch = object.GetModelName();
            bestProperty = "Model";
        }
    }
    
    // Search in tags (if available)
    std::string tags = ""; // Would need to be added to MapObject
    if (StringContains(tags, query))
    {
        float score = CalculateRelevanceScore(tags, query);
        if (score > bestScore)
        {
            bestScore = score;
            bestMatch = tags;
            bestProperty = "Tags";
        }
    }
    
    if (bestScore > 0.0f)
    {
        result.relevanceScore = bestScore;
        result.matchedProperty = bestProperty;
        result.matchedText = bestMatch;
        return true;
    }
    
    return false;
}

void ObjectFilter::SetCustomFilter(std::function<bool(const MapObject&, const FilterCriteria&)> filter)
{
    m_customFilter = filter;
}

void ObjectFilter::ClearCustomFilter()
{
    m_customFilter = nullptr;
}

void ObjectFilter::FilterByType(const std::vector<MapObject>& objects, int type)
{
    FilterCriteria criteria;
    criteria.type = type;
    SetFilterCriteria(criteria);
    ApplyFilter(objects);
}

void ObjectFilter::FilterByLayer(const std::vector<MapObject>& objects, const std::string& layerName)
{
    FilterCriteria criteria;
    criteria.layer = layerName;
    SetFilterCriteria(criteria);
    ApplyFilter(objects);
}

void ObjectFilter::FilterByVisibility(const std::vector<MapObject>& objects, bool visible)
{
    FilterCriteria criteria;
    criteria.visible = visible;
    SetFilterCriteria(criteria);
    ApplyFilter(objects);
}

void ObjectFilter::FilterByLockState(const std::vector<MapObject>& objects, bool locked)
{
    FilterCriteria criteria;
    criteria.locked = locked;
    SetFilterCriteria(criteria);
    ApplyFilter(objects);
}

void ObjectFilter::FilterByBounds(const std::vector<MapObject>& objects, Vector3 min, Vector3 max)
{
    FilterCriteria criteria;
    criteria.positionMin = min;
    criteria.positionMax = max;
    SetFilterCriteria(criteria);
    ApplyFilter(objects);
}

std::vector<SearchResult> ObjectFilter::SearchByName(const std::vector<MapObject>& objects, const std::string& query)
{
    std::vector<SearchResult> results;
    
    for (size_t i = 0; i < objects.size(); ++i)
    {
        if (StringContains(objects[i].GetName(), query))
        {
            float score = CalculateRelevanceScore(objects[i].GetName(), query);
            results.emplace_back(static_cast<int>(i), score, "Name", objects[i].GetName());
        }
    }
    
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.relevanceScore > b.relevanceScore;
        });
    
    return results;
}

std::vector<SearchResult> ObjectFilter::SearchByTags(const std::vector<MapObject>& objects, const std::string& query)
{
    std::vector<SearchResult> results;
    
    for (size_t i = 0; i < objects.size(); ++i)
    {
        std::string tags = ""; // Would need to be added to MapObject
        if (StringContains(tags, query))
        {
            float score = CalculateRelevanceScore(tags, query);
            results.emplace_back(static_cast<int>(i), score, "Tags", tags);
        }
    }
    
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.relevanceScore > b.relevanceScore;
        });
    
    return results;
}

std::vector<SearchResult> ObjectFilter::SearchByModel(const std::vector<MapObject>& objects, const std::string& query)
{
    std::vector<SearchResult> results;
    
    for (size_t i = 0; i < objects.size(); ++i)
    {
        if (StringContains(objects[i].GetModelName(), query))
        {
            float score = CalculateRelevanceScore(objects[i].GetModelName(), query);
            results.emplace_back(static_cast<int>(i), score, "Model", objects[i].GetModelName());
        }
    }
    
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.relevanceScore > b.relevanceScore;
        });
    
    return results;
}

std::vector<SearchResult> ObjectFilter::SearchAll(const std::vector<MapObject>& objects, const std::string& query)
{
    std::vector<SearchResult> allResults;
    
    auto nameResults = SearchByName(objects, query);
    auto modelResults = SearchByModel(objects, query);
    auto tagResults = SearchByTags(objects, query);
    
    allResults.insert(allResults.end(), nameResults.begin(), nameResults.end());
    allResults.insert(allResults.end(), modelResults.begin(), modelResults.end());
    allResults.insert(allResults.end(), tagResults.begin(), tagResults.end());
    
    // Remove duplicates and sort by relevance
    std::sort(allResults.begin(), allResults.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.relevanceScore > b.relevanceScore;
        });
    
    auto it = std::unique(allResults.begin(), allResults.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.objectIndex == b.objectIndex;
        });
    allResults.erase(it, allResults.end());
    
    return allResults;
}

size_t ObjectFilter::GetFilteredCount() const
{
    return m_filteredIndices.size();
}

size_t ObjectFilter::GetSearchResultCount() const
{
    return m_searchResults.size();
}

std::string ObjectFilter::GetFilterSummary() const
{
    if (!m_filterActive)
        return "No filter active";
    
    std::ostringstream oss;
    oss << "Filtered: " << m_filteredIndices.size() << " objects";
    
    if (!m_currentFilter.name.empty())
        oss << " | Name: " << m_currentFilter.name;
    if (m_currentFilter.type >= 0)
        oss << " | Type: " << m_currentFilter.type;
    if (!m_currentFilter.layer.empty())
        oss << " | Layer: " << m_currentFilter.layer;
    
    return oss.str();
}

void ObjectFilter::SaveFilterPreset(const std::string& name, const FilterCriteria& criteria)
{
    // Simplified preset saving - in production, use proper serialization
    std::cout << "Saving filter preset: " << name << std::endl;
}

bool ObjectFilter::LoadFilterPreset(const std::string& name, FilterCriteria& criteria)
{
    // Simplified preset loading - in production, use proper deserialization
    std::cout << "Loading filter preset: " << name << std::endl;
    return true;
}

std::vector<std::string> ObjectFilter::GetAvailablePresets() const
{
    // Simplified preset listing - in production, read from file system
    return {"Default", "Cubes Only", "Models Only", "Large Objects"};
}

bool ObjectFilter::DeleteFilterPreset(const std::string& name)
{
    // Simplified preset deletion - in production, remove from file system
    std::cout << "Deleting filter preset: " << name << std::endl;
    return true;
}

// Static utility functions
bool ObjectFilter::StringContains(const std::string& text, const std::string& query, bool caseSensitive)
{
    if (query.empty())
        return true;
    
    std::string textToSearch = text;
    std::string queryToSearch = query;
    
    if (!caseSensitive)
    {
        std::transform(textToSearch.begin(), textToSearch.end(), textToSearch.begin(), ::tolower);
        std::transform(queryToSearch.begin(), queryToSearch.end(), queryToSearch.begin(), ::tolower);
    }
    
    return textToSearch.find(queryToSearch) != std::string::npos;
}

float ObjectFilter::CalculateRelevanceScore(const std::string& text, const std::string& query)
{
    if (query.empty())
        return 0.0f;
    
    std::string lowerText = text;
    std::string lowerQuery = query;
    
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    size_t pos = lowerText.find(lowerQuery);
    if (pos == std::string::npos)
        return 0.0f;
    
    // Higher score for exact matches and matches at the beginning
    float score = 1.0f;
    
    if (pos == 0)
        score += 0.5f; // Bonus for starting with query
    
    if (text == query)
        score += 1.0f; // Bonus for exact match
    
    // Penalty for longer text (prefer shorter, more relevant matches)
    score -= static_cast<float>(text.length() - query.length()) * 0.01f;
    
    return std::max(0.0f, score);
}

std::vector<std::string> ObjectFilter::SplitTags(const std::string& tags)
{
    std::vector<std::string> result;
    std::stringstream ss(tags);
    std::string tag;
    
    while (std::getline(ss, tag, ','))
    {
        // Trim whitespace
        tag.erase(0, tag.find_first_not_of(" \t"));
        tag.erase(tag.find_last_not_of(" \t") + 1);
        
        if (!tag.empty())
        {
            result.push_back(tag);
        }
    }
    
    return result;
}

bool ObjectFilter::HasTag(const std::string& tags, const std::string& tag)
{
    auto tagList = SplitTags(tags);
    return std::find(tagList.begin(), tagList.end(), tag) != tagList.end();
}
