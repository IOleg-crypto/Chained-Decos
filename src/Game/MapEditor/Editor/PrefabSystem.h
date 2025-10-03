#ifndef PREFABSYSTEM_H
#define PREFABSYSTEM_H

#include <raylib.h>
#include <vector>
#include <string>
#include <memory>
#include <map>

#include "MapObject.h"

// Prefab types
enum class PrefabType
{
    STATIC_MESH = 0,
    ANIMATED_MESH = 1,
    PARTICLE_SYSTEM = 2,
    LIGHT_SETUP = 3,
    TERRAIN_PIECE = 4,
    ARCHITECTURAL = 5
};

// Prefab properties
struct PrefabProperties
{
    std::string name;
    std::string description;
    PrefabType type;
    std::string thumbnailPath;
    Texture thumbnail;
    bool thumbnailLoaded;

    // Source information
    std::string sourceFile;
    std::string category;
    std::vector<std::string> tags;

    // Metadata
    std::string author;
    std::string version;
    std::string license;

    // Technical details
    Vector3 boundingBoxMin;
    Vector3 boundingBoxMax;
    float fileSize; // in bytes
    int polygonCount;

    PrefabProperties() :
        name("New Prefab"),
        type(PrefabType::STATIC_MESH),
        thumbnailLoaded(false),
        boundingBoxMin({0.0f, 0.0f, 0.0f}),
        boundingBoxMax({1.0f, 1.0f, 1.0f}),
        fileSize(0.0f),
        polygonCount(0)
    {}
};

// Prefab instance for placement in scenes
struct PrefabInstance
{
    std::string prefabName;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    std::string instanceName;
    bool visible;
    int layer;

    PrefabInstance() :
        position({0.0f, 0.0f, 0.0f}),
        rotation({0.0f, 0.0f, 0.0f}),
        scale({1.0f, 1.0f, 1.0f}),
        visible(true),
        layer(0)
    {}
};

// Prefab system for managing reusable object templates
class PrefabSystem
{
private:
    std::vector<PrefabProperties> m_prefabs;
    std::map<std::string, std::vector<MapObject>> m_prefabObjects;
    std::vector<PrefabInstance> m_placedInstances;

    int m_selectedPrefab;
    std::string m_prefabDirectory;
    std::string m_thumbnailDirectory;

    // Categories and filtering
    std::vector<std::string> m_categories;
    std::string m_currentCategory;
    std::string m_searchQuery;
    std::vector<std::string> m_selectedTags;

    // Preview system
    Model m_previewModel;
    bool m_previewLoaded;
    Matrix m_previewTransform;

public:
    PrefabSystem();
    ~PrefabSystem();

    // Initialization
    bool Initialize(const std::string& prefabDir = "prefabs/");
    void Cleanup();

    // Prefab management
    bool CreatePrefab(const std::string& name, const std::vector<MapObject>& objects);
    bool LoadPrefab(const std::string& filePath);
    bool SavePrefab(const std::string& name, const std::string& filePath);
    bool DeletePrefab(const std::string& name);

    // Prefab library operations
    void RefreshPrefabLibrary();
    void ClearPrefabLibrary();
    std::vector<std::string> GetPrefabNames() const;
    std::vector<std::string> GetCategories() const;

    // Prefab placement
    PrefabInstance* PlacePrefab(const std::string& prefabName, const Vector3& position);
    bool RemovePrefabInstance(const std::string& instanceName);
    bool UpdatePrefabInstance(const std::string& instanceName, const PrefabInstance& instance);

    // Selection and preview
    void SelectPrefab(int index);
    void SelectPrefabByName(const std::string& name);
    int GetSelectedPrefab() const { return m_selectedPrefab; }
    PrefabProperties* GetSelectedPrefabProperties();

    // Instance management
    std::vector<PrefabInstance> GetPlacedInstances() const { return m_placedInstances; }
    PrefabInstance* GetInstance(const std::string& instanceName);
    void ClearAllInstances();

    // Filtering and search
    void SetCategoryFilter(const std::string& category);
    void SetSearchQuery(const std::string& query);
    void AddTagFilter(const std::string& tag);
    void RemoveTagFilter(const std::string& tag);
    void ClearFilters();

    // Preview system
    void GeneratePreview(const std::string& prefabName);
    void RenderPreview(const Vector3& position, float scale = 1.0f);
    void UpdatePreviewTransform(const Vector3& position, const Vector3& rotation, const Vector3& scale);

    // Utility functions
    Vector3 GetPrefabBounds(const std::string& prefabName) const;
    float GetPrefabPolygonCount(const std::string& prefabName) const;
    std::vector<std::string> GetPrefabTags(const std::string& prefabName) const;

    // File operations
    bool ExportPrefab(const std::string& prefabName, const std::string& filePath);
    bool ImportPrefab(const std::string& filePath);

    // Serialization
    std::string SerializePrefabLibrary() const;
    bool DeserializePrefabLibrary(const std::string& data);

    // Rendering
    void Render();
    void RenderPrefabBrowser();
    void RenderInstanceList();
    void RenderPreviewWindow();

private:
    // Helper functions
    void LoadThumbnail(PrefabProperties& prefab);
    void UnloadThumbnail(PrefabProperties& prefab);
    void GenerateDefaultThumbnail(PrefabProperties& prefab);

    bool ValidatePrefabName(const std::string& name) const;
    std::string GenerateUniquePrefabName(const std::string& baseName) const;

    void UpdateCategoryList();
    void UpdateFilteredPrefabs();

    // File I/O
    bool SavePrefabToFile(const PrefabProperties& prefab, const std::string& filePath);
    bool LoadPrefabFromFile(PrefabProperties& prefab, const std::string& filePath);

    // Preview management
    void LoadPreviewModel(const std::string& prefabName);
    void UnloadPreviewModel();
};

#endif // PREFABSYSTEM_H