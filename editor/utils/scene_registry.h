#ifndef CH_SCENE_REGISTRY_H
#define CH_SCENE_REGISTRY_H

#include <string>
#include <vector>
#include <filesystem>
#include "engine/scene/project.h"

namespace CHEngine
{
    class SceneRegistry
    {
    public:
        static std::vector<std::string> GetAvailableScenes()
        {
            std::vector<std::string> scenes;
            auto project = Project::GetActive();
            if (!project)
            {
                return scenes;
            }

            std::filesystem::path assetDir = project->GetAssetDirectory();
            std::filesystem::path scenesDir = assetDir / "scenes";

            if (std::filesystem::exists(scenesDir))
            {
                for (auto &entry : std::filesystem::recursive_directory_iterator(scenesDir))
                {
                    if (entry.path().extension() == ".chscene")
                    {
                        // Return path relative to assets
                        std::string relPath = std::filesystem::relative(entry.path(), assetDir).string();
                        scenes.push_back(relPath);
                    }
                }
            }
            return scenes;
        }
    };
}

#endif // CH_SCENE_REGISTRY_H
