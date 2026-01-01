#ifndef SCENE_METADATA_H
#define SCENE_METADATA_H

#include "GameScene.h"
#include <chrono>
#include <raylib.h>
#include <string>

namespace CHEngine
{

/**
 * @brief Extended metadata for scenes used by Content Browser
 */
struct SceneMetadata
{
    std::string name;                               // Scene display name
    std::string path;                               // Full file path
    SceneType type = SceneType::Game;               // Scene type (Game/UI)
    std::string author;                             // Creator name
    std::chrono::system_clock::time_point created;  // Creation timestamp
    std::chrono::system_clock::time_point modified; // Last modified
    int entityCount = 0;                            // Number of entities/objects
    std::string thumbnailPath;                      // Path to preview image
    uint64_t fileSize = 0;                          // File size in bytes

    // Computed properties
    std::string GetDisplayName() const
    {
        return name.empty() ? "Untitled" : name;
    }

    std::string GetTypeString() const
    {
        return type == SceneType::Game ? "Game" : "UI";
    }

    std::string GetRelativeTimestamp() const; // "2 hours ago", "5 mins ago", etc.
};

/**
 * @brief Helper to load scene metadata from .chscene file
 */
class SceneMetadataReader
{
public:
    static SceneMetadata LoadFromFile(const std::string &scenePath);
    static bool SaveToFile(const std::string &scenePath, const SceneMetadata &metadata);
    static std::string
    CalculateRelativeTime(const std::chrono::system_clock::time_point &timestamp);

private:
};

} // namespace CHEngine

#endif // SCENE_METADATA_H
