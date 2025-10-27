//
// Created by AI Assistant
//

#ifndef LAYERMANAGER_H
#define LAYERMANAGER_H

#include <vector>
#include <string>
#include <memory>
#include <map>

// Layer structure for organizing objects
struct Layer
{
    std::string name;
    std::string color;          // Hex color for layer visualization
    bool visible;               // Layer visibility
    bool locked;                // Layer lock state
    int order;                  // Display order
    std::vector<int> objectIds; // IDs of objects in this layer
    
    Layer() : visible(true), locked(false), order(0) {}
    Layer(const std::string& layerName) 
        : name(layerName), visible(true), locked(false), order(0) {}
};

// Layer manager for organizing map objects
class LayerManager
{
private:
    std::vector<Layer> m_layers;
    std::string m_defaultLayerName;
    int m_nextLayerOrder;
    
public:
    LayerManager();
    ~LayerManager() = default;
    
    // Layer management
    bool CreateLayer(const std::string& name, const std::string& color = "#FFFFFF");
    bool DeleteLayer(const std::string& name);
    bool RenameLayer(const std::string& oldName, const std::string& newName);
    bool MoveLayerUp(const std::string& name);
    bool MoveLayerDown(const std::string& name);
    
    // Layer properties
    bool SetLayerVisibility(const std::string& name, bool visible);
    bool SetLayerLock(const std::string& name, bool locked);
    bool SetLayerColor(const std::string& name, const std::string& color);
    
    // Object layer operations
    bool AddObjectToLayer(const std::string& layerName, int objectId);
    bool RemoveObjectFromLayer(int objectId);
    bool MoveObjectToLayer(int objectId, const std::string& newLayerName);
    std::string GetObjectLayer(int objectId) const;
    
    // Query operations
    std::vector<int> GetObjectsInLayer(const std::string& layerName) const;
    std::vector<std::string> GetLayerNames() const;
    bool LayerExists(const std::string& name) const;
    Layer* GetLayer(const std::string& name);
    const Layer* GetLayer(const std::string& name) const;
    
    // Visibility and lock queries
    bool IsLayerVisible(const std::string& name) const;
    bool IsLayerLocked(const std::string& name) const;
    bool IsObjectVisible(int objectId) const;
    bool IsObjectLocked(int objectId) const;
    
    // Bulk operations
    void ShowAllLayers();
    void HideAllLayers();
    void UnlockAllLayers();
    void LockAllLayers();
    
    // Default layer operations
    void SetDefaultLayer(const std::string& name);
    std::string GetDefaultLayer() const;
    
    // Layer ordering
    void SetLayerOrder(const std::string& name, int order);
    int GetLayerOrder(const std::string& name) const;
    std::vector<Layer> GetLayersByOrder() const;
    
    // Utility functions
    void Clear();
    size_t GetLayerCount() const;
    std::string GenerateUniqueLayerName(const std::string& baseName = "Layer") const;
    
    // Serialization
    std::string SerializeToJson() const;
    bool DeserializeFromJson(const std::string& json);
};

#endif // LAYERMANAGER_H
