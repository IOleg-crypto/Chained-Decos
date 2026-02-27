#ifndef CH_CONSTANTS_H
#define CH_CONSTANTS_H

#include <string_view>

namespace CHEngine::Constants
{
    namespace Paths
    {
        constexpr std::string_view EnginePrefix = "engine/";
        constexpr size_t EnginePrefixSize = EnginePrefix.size();
        
        constexpr std::string_view DefaultAssetDir = "assets";
    }

    namespace Limits
    {
        constexpr int MaxLights = 16;
    }

    namespace Assets
    {
        constexpr std::string_view ProjectExtension = ".chproject";
        constexpr std::string_view SceneExtension = ".chscene";
        constexpr std::string_view PrefabExtension = ".chprefab";
    }
}

#endif // CH_CONSTANTS_H
