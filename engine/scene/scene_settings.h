#ifndef CH_SCENE_SETTINGS_H
#define CH_SCENE_SETTINGS_H

#include "engine/graphics/environment.h"
#include "engine/scene/components/control_component.h"
#include "raylib.h"
#include <string>
#include <memory>

namespace CHEngine
{
    enum class BackgroundMode
    {
        Color = 0,
        Texture = 1,
        Environment3D = 2
    };

    struct SceneSettings
    {
        std::string Name = "Untitled Scene";
        std::string ScenePath;
        std::shared_ptr<EnvironmentAsset> Environment;
        
        BackgroundMode Mode = BackgroundMode::Environment3D;
        Color BackgroundColor = {30, 30, 30, 255};
        std::string BackgroundTexturePath;

        CanvasSettings Canvas;
    };

} // namespace CHEngine

#endif // CH_SCENE_SETTINGS_H
