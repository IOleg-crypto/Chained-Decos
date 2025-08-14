#ifndef MODELCONFIG_H
#define MODELCONFIG_H
#include <raylib.h>
#include <string>
#include <vector>

// Model configuration structures (better data organization)
struct ModelInstanceConfig
{
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 rotation = {0.0f, 0.0f, 0.0f}; // New feature
    float scale = 1.0f;
    Color color = WHITE;
    bool spawn = true;
    std::string tag = ""; // For filtering and search
};

struct ModelFileConfig
{
    std::string name;
    std::string path;
    std::string category = "default"; // Model categorization
    bool spawn = true;
    bool hasCollision = false;  // Enable collision detection for this model
    float lodDistance = 100.0f; // Level of Detail distance
    std::vector<ModelInstanceConfig> instances;

    // Metadata
    bool hasAnimations = false;
    bool preload = true;
    int priority = 0; // Loading priority
};

// Loading statistics
struct LoadingStats
{
    int totalModels = 0;
    int loadedModels = 0;
    int failedModels = 0;
    int totalInstances = 0;
    float loadingTime = 0.0f;

    float GetSuccessRate() const
    {
        return totalModels > 0 ? (float)loadedModels / totalModels : 0.0f;
    }
};

#endif /* MODELINSTANCE_H */
