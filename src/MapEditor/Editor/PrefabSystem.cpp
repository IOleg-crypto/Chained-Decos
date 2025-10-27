#include "PrefabSystem.h"
#include <raylib.h>
#include <raymath.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

PrefabSystem::PrefabSystem()
    : m_selectedPrefab(-1)
    , m_previewLoaded(false)
    , m_currentCategory("All")
{
}

PrefabSystem::~PrefabSystem()
{
    Cleanup();
}

bool PrefabSystem::Initialize(const std::string& prefabDir)
{
    m_prefabDirectory = prefabDir;

    // Create directory if it doesn't exist
    if (!fs::exists(prefabDir))
    {
        fs::create_directories(prefabDir);
    }

    // Set up thumbnail directory
    m_thumbnailDirectory = prefabDir + "/thumbnails/";
    if (!fs::exists(m_thumbnailDirectory))
    {
        fs::create_directories(m_thumbnailDirectory);
    }

    // Refresh the prefab library
    RefreshPrefabLibrary();

    return true;
}

void PrefabSystem::Cleanup()
{
    if (m_previewLoaded)
    {
        UnloadModel(m_previewModel);
        m_previewLoaded = false;
    }

    for (auto& prefab : m_prefabs)
    {
        if (prefab.thumbnailLoaded)
        {
            UnloadTexture(prefab.thumbnail);
        }
    }
    m_prefabs.clear();
    m_prefabObjects.clear();
    m_placedInstances.clear();
    m_selectedPrefab = -1;
}

bool PrefabSystem::CreatePrefab(const std::string& name, const std::vector<MapObject>& objects)
{
    if (!ValidatePrefabName(name) || objects.empty())
        return false;

    PrefabProperties prefab;
    prefab.name = name;

    // Calculate bounding box
    if (!objects.empty())
    {
        prefab.boundingBoxMin = objects[0].GetPosition();
        prefab.boundingBoxMax = objects[0].GetPosition();

        for (const auto& obj : objects)
        {
            Vector3 pos = obj.GetPosition();
            prefab.boundingBoxMin.x = std::min(prefab.boundingBoxMin.x, pos.x);
            prefab.boundingBoxMin.y = std::min(prefab.boundingBoxMin.y, pos.y);
            prefab.boundingBoxMin.z = std::min(prefab.boundingBoxMin.z, pos.z);
            prefab.boundingBoxMax.x = std::max(prefab.boundingBoxMax.x, pos.x);
            prefab.boundingBoxMax.y = std::max(prefab.boundingBoxMax.y, pos.y);
            prefab.boundingBoxMax.z = std::max(prefab.boundingBoxMax.z, pos.z);
        }
    }

    // Store objects
    m_prefabObjects[name] = objects;
    m_prefabs.push_back(prefab);

    // Generate thumbnail
    GenerateDefaultThumbnail(prefab);

    return true;
}

bool PrefabSystem::LoadPrefab(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to load prefab: " << filePath << std::endl;
        return false;
    }

    // Read prefab data from file
    PrefabProperties prefab;
    prefab.sourceFile = filePath;

    // Parse file content (simplified for now)
    std::string line;
    while (std::getline(file, line))
    {
        if (line.substr(0, 5) == "Name:")
        {
            prefab.name = line.substr(5);
        }
        else if (line.substr(0, 11) == "Category:")
        {
            prefab.category = line.substr(11);
        }
        else if (line.substr(0, 6) == "Tags:")
        {
            // Parse tags
            std::string tagsStr = line.substr(5);
            size_t pos = 0;
            std::string tag;
            while ((pos = tagsStr.find(',')) != std::string::npos)
            {
                tag = tagsStr.substr(0, pos);
                prefab.tags.push_back(tag);
                tagsStr.erase(0, pos + 1);
            }
            if (!tagsStr.empty())
            {
                prefab.tags.push_back(tagsStr);
            }
        }
    }

    // Load associated objects if available
    std::vector<MapObject> objects;
    m_prefabObjects[prefab.name] = objects;

    m_prefabs.push_back(prefab);

    // Try to load thumbnail
    LoadThumbnail(prefab);

    return true;
}

bool PrefabSystem::SavePrefab(const std::string& name, const std::string& filePath)
{
    auto it = std::find_if(m_prefabs.begin(), m_prefabs.end(),
        [&name](const PrefabProperties& p) { return p.name == name; });

    if (it == m_prefabs.end())
        return false;

    return SavePrefabToFile(*it, filePath);
}

bool PrefabSystem::DeletePrefab(const std::string& name)
{
    auto it = std::find_if(m_prefabs.begin(), m_prefabs.end(),
        [&name](const PrefabProperties& p) { return p.name == name; });

    if (it == m_prefabs.end())
        return false;

    // Unload thumbnail
    if (it->thumbnailLoaded)
    {
        UnloadTexture(it->thumbnail);
    }

    // Remove from collections
    m_prefabs.erase(it);
    m_prefabObjects.erase(name);

    if (m_selectedPrefab >= static_cast<int>(m_prefabs.size()))
    {
        m_selectedPrefab = static_cast<int>(m_prefabs.size()) - 1;
    }

    return true;
}

void PrefabSystem::RefreshPrefabLibrary()
{
    ClearPrefabLibrary();

    if (!fs::exists(m_prefabDirectory))
        return;

    // Scan directory for prefab files
    for (const auto& entry : fs::directory_iterator(m_prefabDirectory))
    {
        if (entry.path().extension() == ".prefab")
        {
            LoadPrefab(entry.path().string());
        }
    }

    UpdateCategoryList();
}

void PrefabSystem::ClearPrefabLibrary()
{
    for (auto& prefab : m_prefabs)
    {
        if (prefab.thumbnailLoaded)
        {
            UnloadTexture(prefab.thumbnail);
        }
    }
    m_prefabs.clear();
    m_prefabObjects.clear();
    m_selectedPrefab = -1;
}

std::vector<std::string> PrefabSystem::GetPrefabNames() const
{
    std::vector<std::string> names;
    for (const auto& prefab : m_prefabs)
    {
        names.push_back(prefab.name);
    }
    return names;
}

std::vector<std::string> PrefabSystem::GetCategories() const
{
    return m_categories;
}

PrefabInstance* PrefabSystem::PlacePrefab(const std::string& prefabName, const Vector3& position)
{
    auto it = std::find_if(m_prefabs.begin(), m_prefabs.end(),
        [&prefabName](const PrefabProperties& p) { return p.name == prefabName; });

    if (it == m_prefabs.end())
        return nullptr;

    PrefabInstance instance;
    instance.prefabName = prefabName;
    instance.position = position;
    instance.instanceName = prefabName + "_" + std::to_string(m_placedInstances.size());

    m_placedInstances.push_back(instance);
    return &m_placedInstances.back();
}

bool PrefabSystem::RemovePrefabInstance(const std::string& instanceName)
{
    auto it = std::find_if(m_placedInstances.begin(), m_placedInstances.end(),
        [&instanceName](const PrefabInstance& inst) { return inst.instanceName == instanceName; });

    if (it == m_placedInstances.end())
        return false;

    m_placedInstances.erase(it);
    return true;
}

bool PrefabSystem::UpdatePrefabInstance(const std::string& instanceName, const PrefabInstance& instance)
{
    auto it = std::find_if(m_placedInstances.begin(), m_placedInstances.end(),
        [&instanceName](const PrefabInstance& inst) { return inst.instanceName == instanceName; });

    if (it == m_placedInstances.end())
        return false;

    *it = instance;
    return true;
}

void PrefabSystem::SelectPrefab(int index)
{
    if (index >= -1 && index < static_cast<int>(m_prefabs.size()))
    {
        m_selectedPrefab = index;
    }
}

void PrefabSystem::SelectPrefabByName(const std::string& name)
{
    for (size_t i = 0; i < m_prefabs.size(); ++i)
    {
        if (m_prefabs[i].name == name)
        {
            m_selectedPrefab = static_cast<int>(i);
            return;
        }
    }
    m_selectedPrefab = -1;
}

PrefabProperties* PrefabSystem::GetSelectedPrefabProperties()
{
    if (m_selectedPrefab >= 0 && m_selectedPrefab < static_cast<int>(m_prefabs.size()))
    {
        return &m_prefabs[m_selectedPrefab];
    }
    return nullptr;
}

PrefabInstance* PrefabSystem::GetInstance(const std::string& instanceName)
{
    auto it = std::find_if(m_placedInstances.begin(), m_placedInstances.end(),
        [&instanceName](const PrefabInstance& inst) { return inst.instanceName == instanceName; });

    if (it != m_placedInstances.end())
    {
        return &(*it);
    }
    return nullptr;
}

void PrefabSystem::ClearAllInstances()
{
    m_placedInstances.clear();
}

void PrefabSystem::SetCategoryFilter(const std::string& category)
{
    m_currentCategory = category;
}

void PrefabSystem::SetSearchQuery(const std::string& query)
{
    m_searchQuery = query;
}

void PrefabSystem::AddTagFilter(const std::string& tag)
{
    m_selectedTags.push_back(tag);
}

void PrefabSystem::RemoveTagFilter(const std::string& tag)
{
    m_selectedTags.erase(std::remove(m_selectedTags.begin(), m_selectedTags.end(), tag), m_selectedTags.end());
}

void PrefabSystem::ClearFilters()
{
    m_currentCategory = "All";
    m_searchQuery.clear();
    m_selectedTags.clear();
}

void PrefabSystem::GeneratePreview(const std::string& prefabName)
{
    if (m_previewLoaded)
    {
        UnloadModel(m_previewModel);
        m_previewLoaded = false;
    }

    LoadPreviewModel(prefabName);
}

void PrefabSystem::RenderPreview(const Vector3& position, float scale)
{
    if (m_previewLoaded)
    {
        Matrix transform = MatrixMultiply(MatrixScale(scale, scale, scale),
                                        MatrixMultiply(MatrixRotateXYZ({0.0f, 0.0f, 0.0f}), MatrixTranslate(position.x, position.y, position.z)));
        DrawModel(m_previewModel, position, scale, WHITE);
    }
}

void PrefabSystem::UpdatePreviewTransform(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    m_previewTransform = MatrixMultiply(MatrixScale(scale.x, scale.y, scale.z),
                                      MatrixMultiply(MatrixRotateXYZ(rotation), MatrixTranslate(position.x, position.y, position.z)));
}

Vector3 PrefabSystem::GetPrefabBounds(const std::string& prefabName) const
{
    auto it = std::find_if(m_prefabs.begin(), m_prefabs.end(),
        [&prefabName](const PrefabProperties& p) { return p.name == prefabName; });

    if (it != m_prefabs.end())
    {
        Vector3 size = {
            it->boundingBoxMax.x - it->boundingBoxMin.x,
            it->boundingBoxMax.y - it->boundingBoxMin.y,
            it->boundingBoxMax.z - it->boundingBoxMin.z
        };
        return size;
    }

    return {1.0f, 1.0f, 1.0f};
}

float PrefabSystem::GetPrefabPolygonCount(const std::string& prefabName) const
{
    auto it = std::find_if(m_prefabs.begin(), m_prefabs.end(),
        [&prefabName](const PrefabProperties& p) { return p.name == prefabName; });

    if (it != m_prefabs.end())
    {
        return static_cast<float>(it->polygonCount);
    }

    return 0.0f;
}

std::vector<std::string> PrefabSystem::GetPrefabTags(const std::string& prefabName) const
{
    auto it = std::find_if(m_prefabs.begin(), m_prefabs.end(),
        [&prefabName](const PrefabProperties& p) { return p.name == prefabName; });

    if (it != m_prefabs.end())
    {
        return it->tags;
    }

    return {};
}

bool PrefabSystem::ExportPrefab(const std::string& prefabName, const std::string& filePath)
{
    auto it = std::find_if(m_prefabs.begin(), m_prefabs.end(),
        [&prefabName](const PrefabProperties& p) { return p.name == prefabName; });

    if (it == m_prefabs.end())
        return false;

    return SavePrefabToFile(*it, filePath);
}

bool PrefabSystem::ImportPrefab(const std::string& filePath)
{
    return LoadPrefab(filePath);
}

std::string PrefabSystem::SerializePrefabLibrary() const
{
    // Serialize entire prefab library to string
    std::string data = "PrefabLibrary\n";
    data += "Count: " + std::to_string(m_prefabs.size()) + "\n";

    for (const auto& prefab : m_prefabs)
    {
        data += "Prefab: " + prefab.name + "\n";
        data += "Category: " + prefab.category + "\n";
        data += "Description: " + prefab.description + "\n";
        // Add more serialization as needed
    }

    return data;
}

bool PrefabSystem::DeserializePrefabLibrary(const std::string& data)
{
    // Deserialize prefab library from string
    // Implementation would parse the data and recreate prefabs
    return true;
}

void PrefabSystem::Render()
{
    // Main rendering function for prefab system UI
}

void PrefabSystem::RenderPrefabBrowser()
{
    // Render prefab browser interface
}

void PrefabSystem::RenderInstanceList()
{
    // Render list of placed prefab instances
}

void PrefabSystem::RenderPreviewWindow()
{
    // Render preview window for selected prefab
}

void PrefabSystem::LoadThumbnail(PrefabProperties& prefab)
{
    std::string thumbnailPath = m_thumbnailDirectory + prefab.name + ".png";

    if (fs::exists(thumbnailPath))
    {
        prefab.thumbnail = LoadTexture(thumbnailPath.c_str());
        prefab.thumbnailLoaded = (prefab.thumbnail.id != 0);
        prefab.thumbnailPath = thumbnailPath;
    }
    else
    {
        GenerateDefaultThumbnail(prefab);
    }
}

void PrefabSystem::UnloadThumbnail(PrefabProperties& prefab)
{
    if (prefab.thumbnailLoaded)
    {
        UnloadTexture(prefab.thumbnail);
        prefab.thumbnailLoaded = false;
    }
}

void PrefabSystem::GenerateDefaultThumbnail(PrefabProperties& prefab)
{
    // Generate a simple colored thumbnail as fallback
    const int thumbSize = 128;
    Image img = GenImageColor(thumbSize, thumbSize, SKYBLUE);

    // Draw a simple pattern or text
    for (int y = 0; y < thumbSize; y += 16)
    {
        for (int x = 0; x < thumbSize; x += 16)
        {
            Color pixelColor = ((x + y) % 32 == 0) ? DARKBLUE : BLUE;
            ImageDrawPixel(&img, x, y, pixelColor);
        }
    }

    prefab.thumbnail = LoadTextureFromImage(img);
    prefab.thumbnailLoaded = (prefab.thumbnail.id != 0);
    UnloadImage(img);
}

bool PrefabSystem::ValidatePrefabName(const std::string& name) const
{
    if (name.empty() || name.length() > 50)
        return false;

    // Check for invalid characters
    for (char c : name)
    {
        if (!isalnum(c) && c != ' ' && c != '_' && c != '-')
            return false;
    }

    return true;
}

std::string PrefabSystem::GenerateUniquePrefabName(const std::string& baseName) const
{
    std::string name = baseName;
    int counter = 1;

    while (std::any_of(m_prefabs.begin(), m_prefabs.end(),
        [&name](const PrefabProperties& p) { return p.name == name; }))
    {
        name = baseName + "_" + std::to_string(counter);
        counter++;
    }

    return name;
}

void PrefabSystem::UpdateCategoryList()
{
    m_categories.clear();
    m_categories.push_back("All");

    for (const auto& prefab : m_prefabs)
    {
        if (std::find(m_categories.begin(), m_categories.end(), prefab.category) == m_categories.end())
        {
            m_categories.push_back(prefab.category);
        }
    }
}

void PrefabSystem::UpdateFilteredPrefabs()
{
    // Update filtered list based on current filters
    // This would be used for displaying filtered prefabs in the browser
}

bool PrefabSystem::SavePrefabToFile(const PrefabProperties& prefab, const std::string& filePath)
{
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to save prefab: " << filePath << std::endl;
        return false;
    }

    file << "Prefab File\n";
    file << "Name: " << prefab.name << "\n";
    file << "Type: " << static_cast<int>(prefab.type) << "\n";
    file << "Category: " << prefab.category << "\n";
    file << "Description: " << prefab.description << "\n";
    file << "Author: " << prefab.author << "\n";
    file << "Version: " << prefab.version << "\n";

    // Write tags
    file << "Tags: ";
    for (size_t i = 0; i < prefab.tags.size(); ++i)
    {
        if (i > 0) file << ",";
        file << prefab.tags[i];
    }
    file << "\n";

    // Write bounds
    file << "BoundsMin: " << prefab.boundingBoxMin.x << "," << prefab.boundingBoxMin.y << "," << prefab.boundingBoxMin.z << "\n";
    file << "BoundsMax: " << prefab.boundingBoxMax.x << "," << prefab.boundingBoxMax.y << "," << prefab.boundingBoxMax.z << "\n";

    return true;
}

bool PrefabSystem::LoadPrefabFromFile(PrefabProperties& prefab, const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to load prefab file: " << filePath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.substr(0, 5) == "Name:")
        {
            prefab.name = line.substr(5);
        }
        else if (line.substr(0, 9) == "Category:")
        {
            prefab.category = line.substr(9);
        }
        else if (line.substr(0, 12) == "Description:")
        {
            prefab.description = line.substr(12);
        }
        else if (line.substr(0, 7) == "Author:")
        {
            prefab.author = line.substr(7);
        }
        else if (line.substr(0, 8) == "Version:")
        {
            prefab.version = line.substr(8);
        }
        else if (line.substr(0, 5) == "Tags:")
        {
            std::string tagsStr = line.substr(5);
            size_t pos = 0;
            std::string tag;
            while ((pos = tagsStr.find(',')) != std::string::npos)
            {
                tag = tagsStr.substr(0, pos);
                prefab.tags.push_back(tag);
                tagsStr.erase(0, pos + 1);
            }
            if (!tagsStr.empty())
            {
                prefab.tags.push_back(tagsStr);
            }
        }
    }

    return true;
}

void PrefabSystem::LoadPreviewModel(const std::string& prefabName)
{
    // Load a preview model for the selected prefab
    // For now, just create a simple cube as placeholder
    if (m_previewLoaded)
    {
        UnloadModel(m_previewModel);
    }

    m_previewModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    m_previewLoaded = (m_previewModel.meshCount > 0);
}

void PrefabSystem::UnloadPreviewModel()
{
    if (m_previewLoaded)
    {
        UnloadModel(m_previewModel);
        m_previewLoaded = false;
    }
}