#ifndef CH_EDITOR_SETTINGS_H
#define CH_EDITOR_SETTINGS_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Chained
{
    struct EditorSettings
    {
        float CameraMoveSpeed = 10.0f;
        float CameraRotationSpeed = 0.1f;
        float CameraBoostMultiplier = 5.0f;
        bool ShowGrid = true;
        bool ShowGizmos = true;
        bool ShowSelectedWireframe = true;
    };

    struct EditorConfig
    {
        std::string LastProjectPath;
        std::string LastScenePath;
        bool LoadLastProjectOnStartup = false;
        bool AutoSaveEnabled = true;
        float AutoSaveInterval = 300.0f;
        std::vector<std::string> RecentProjects;
    };
}

#endif // CH_EDITOR_SETTINGS_H
