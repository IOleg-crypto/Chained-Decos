#ifndef CH_SCENE_CONTEXT_H
#define CH_SCENE_CONTEXT_H

#include "entt/entt.hpp"

namespace Chained
{
    class ScriptEngine;

    /**
     * @brief Context containing all resources specific to a single scene.
     * Passed to systems to avoid global registry access where possible.
     */
    struct SceneContext
    {
        entt::registry* Registry = nullptr;
        
        // Scene-specific systems
        ScriptEngine* Scripting = nullptr;

        // Add more scene-local resources here (e.g. NavMesh, etc.)
    };
}

#endif // CH_SCENE_CONTEXT_H
