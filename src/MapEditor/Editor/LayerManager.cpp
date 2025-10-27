//
// Created by AI Assistant
//

#include "LayerManager.h"
#include <algorithm>
#include <sstream>

LayerManager::LayerManager() : m_defaultLayerName("Default"), m_nextLayerOrder(0)
{
    // Create default layer
    CreateLayer("Default", "#FFFFFF");
}

bool LayerManager::CreateLayer(const std::string& name, const std::string& color)
{
    if (LayerExists(name))
        return false;
    
    Layer newLayer(name);
    newLayer.color = color;
    newLayer.order = m_nextLayerOrder++;
    
    m_layers.push_back(newLayer);
    return true;
}

bool LayerManager::DeleteLayer(const std::string& name)
{
    if (name == m_defaultLayerName)
        return false; // Can't delete default layer
    
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [&name](const Layer& layer) { return layer.name == name; });
    
    if (it != m_layers.end())
    {
        // Move all objects from deleted layer to default layer
        for (int objectId : it->objectIds)
        {
            AddObjectToLayer(m_defaultLayerName, objectId);
        }
        
        m_layers.erase(it);
        return true;
    }
    
    return false;
}

bool LayerManager::RenameLayer(const std::string& oldName, const std::string& newName)
{
    if (oldName == m_defaultLayerName || LayerExists(newName))
        return false;
    
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [&oldName](const Layer& layer) { return layer.name == oldName; });
    
    if (it != m_layers.end())
    {
        it->name = newName;
        return true;
    }
    
    return false;
}

bool LayerManager::MoveLayerUp(const std::string& name)
{
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [&name](const Layer& layer) { return layer.name == name; });
    
    if (it != m_layers.end() && it != m_layers.begin())
    {
        auto prevIt = it - 1;
        std::swap(it->order, prevIt->order);
        return true;
    }
    
    return false;
}

bool LayerManager::MoveLayerDown(const std::string& name)
{
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [&name](const Layer& layer) { return layer.name == name; });
    
    if (it != m_layers.end() && it != m_layers.end() - 1)
    {
        auto nextIt = it + 1;
        std::swap(it->order, nextIt->order);
        return true;
    }
    
    return false;
}

bool LayerManager::SetLayerVisibility(const std::string& name, bool visible)
{
    Layer* layer = GetLayer(name);
    if (layer)
    {
        layer->visible = visible;
        return true;
    }
    return false;
}

bool LayerManager::SetLayerLock(const std::string& name, bool locked)
{
    Layer* layer = GetLayer(name);
    if (layer)
    {
        layer->locked = locked;
        return true;
    }
    return false;
}

bool LayerManager::SetLayerColor(const std::string& name, const std::string& color)
{
    Layer* layer = GetLayer(name);
    if (layer)
    {
        layer->color = color;
        return true;
    }
    return false;
}

bool LayerManager::AddObjectToLayer(const std::string& layerName, int objectId)
{
    // First remove object from any existing layer
    RemoveObjectFromLayer(objectId);
    
    Layer* layer = GetLayer(layerName);
    if (layer)
    {
        layer->objectIds.push_back(objectId);
        return true;
    }
    return false;
}

bool LayerManager::RemoveObjectFromLayer(int objectId)
{
    for (auto& layer : m_layers)
    {
        auto it = std::find(layer.objectIds.begin(), layer.objectIds.end(), objectId);
        if (it != layer.objectIds.end())
        {
            layer.objectIds.erase(it);
            return true;
        }
    }
    return false;
}

bool LayerManager::MoveObjectToLayer(int objectId, const std::string& newLayerName)
{
    return AddObjectToLayer(newLayerName, objectId);
}

std::string LayerManager::GetObjectLayer(int objectId) const
{
    for (const auto& layer : m_layers)
    {
        auto it = std::find(layer.objectIds.begin(), layer.objectIds.end(), objectId);
        if (it != layer.objectIds.end())
        {
            return layer.name;
        }
    }
    return m_defaultLayerName;
}

std::vector<int> LayerManager::GetObjectsInLayer(const std::string& layerName) const
{
    const Layer* layer = GetLayer(layerName);
    if (layer)
    {
        return layer->objectIds;
    }
    return {};
}

std::vector<std::string> LayerManager::GetLayerNames() const
{
    std::vector<std::string> names;
    for (const auto& layer : m_layers)
    {
        names.push_back(layer.name);
    }
    return names;
}

bool LayerManager::LayerExists(const std::string& name) const
{
    return GetLayer(name) != nullptr;
}

Layer* LayerManager::GetLayer(const std::string& name)
{
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [&name](const Layer& layer) { return layer.name == name; });
    
    return (it != m_layers.end()) ? &(*it) : nullptr;
}

const Layer* LayerManager::GetLayer(const std::string& name) const
{
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [&name](const Layer& layer) { return layer.name == name; });
    
    return (it != m_layers.end()) ? &(*it) : nullptr;
}

bool LayerManager::IsLayerVisible(const std::string& name) const
{
    const Layer* layer = GetLayer(name);
    return layer ? layer->visible : true;
}

bool LayerManager::IsLayerLocked(const std::string& name) const
{
    const Layer* layer = GetLayer(name);
    return layer ? layer->locked : false;
}

bool LayerManager::IsObjectVisible(int objectId) const
{
    std::string layerName = GetObjectLayer(objectId);
    return IsLayerVisible(layerName);
}

bool LayerManager::IsObjectLocked(int objectId) const
{
    std::string layerName = GetObjectLayer(objectId);
    return IsLayerLocked(layerName);
}

void LayerManager::ShowAllLayers()
{
    for (auto& layer : m_layers)
    {
        layer.visible = true;
    }
}

void LayerManager::HideAllLayers()
{
    for (auto& layer : m_layers)
    {
        layer.visible = false;
    }
}

void LayerManager::UnlockAllLayers()
{
    for (auto& layer : m_layers)
    {
        layer.locked = false;
    }
}

void LayerManager::LockAllLayers()
{
    for (auto& layer : m_layers)
    {
        layer.locked = false;
    }
}

void LayerManager::SetDefaultLayer(const std::string& name)
{
    if (LayerExists(name))
    {
        m_defaultLayerName = name;
    }
}

std::string LayerManager::GetDefaultLayer() const
{
    return m_defaultLayerName;
}

void LayerManager::SetLayerOrder(const std::string& name, int order)
{
    Layer* layer = GetLayer(name);
    if (layer)
    {
        layer->order = order;
    }
}

int LayerManager::GetLayerOrder(const std::string& name) const
{
    const Layer* layer = GetLayer(name);
    return layer ? layer->order : -1;
}

std::vector<Layer> LayerManager::GetLayersByOrder() const
{
    std::vector<Layer> orderedLayers = m_layers;
    std::sort(orderedLayers.begin(), orderedLayers.end(),
        [](const Layer& a, const Layer& b) { return a.order < b.order; });
    return orderedLayers;
}

void LayerManager::Clear()
{
    m_layers.clear();
    m_defaultLayerName = "Default";
    m_nextLayerOrder = 0;
    CreateLayer("Default", "#FFFFFF");
}

size_t LayerManager::GetLayerCount() const
{
    return m_layers.size();
}

std::string LayerManager::GenerateUniqueLayerName(const std::string& baseName) const
{
    std::string name = baseName;
    int counter = 1;
    
    while (LayerExists(name))
    {
        name = baseName + " " + std::to_string(counter);
        counter++;
    }
    
    return name;
}

std::string LayerManager::SerializeToJson() const
{
    std::ostringstream json;
    json << "{\n";
    json << "  \"layers\": [\n";
    
    for (size_t i = 0; i < m_layers.size(); ++i)
    {
        const auto& layer = m_layers[i];
        
        json << "    {\n";
        json << "      \"name\": \"" << layer.name << "\",\n";
        json << "      \"color\": \"" << layer.color << "\",\n";
        json << "      \"visible\": " << (layer.visible ? "true" : "false") << ",\n";
        json << "      \"locked\": " << (layer.locked ? "true" : "false") << ",\n";
        json << "      \"order\": " << layer.order << ",\n";
        json << "      \"objectIds\": [";
        
        for (size_t j = 0; j < layer.objectIds.size(); ++j)
        {
            json << layer.objectIds[j];
            if (j < layer.objectIds.size() - 1)
                json << ", ";
        }
        
        json << "]\n";
        
        if (i < m_layers.size() - 1)
            json << "    },\n";
        else
            json << "    }\n";
    }
    
    json << "  ],\n";
    json << "  \"defaultLayer\": \"" << m_defaultLayerName << "\"\n";
    json << "}\n";
    
    return json.str();
}

bool LayerManager::DeserializeFromJson(const std::string& json)
{
    // Simplified JSON parsing - in production, use proper JSON library
    // This is a placeholder implementation
    return true;
}
