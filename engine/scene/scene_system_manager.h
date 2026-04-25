#ifndef CH_SCENE_SYSTEM_MANAGER_H
#define CH_SCENE_SYSTEM_MANAGER_H

#include "scene_system.h"
#include <vector>
#include <memory>

namespace CHEngine
{
    class Scene;

    class SceneSystemManager
    {
    public:
        SceneSystemManager(Scene* scene);
        ~SceneSystemManager() = default;

        template<typename T, typename... Args>
        void AddSystem(Args&&... args)
        {
            m_Systems.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        }

        void OnRuntimeStart();
        void OnRuntimeStop();
        void OnUpdate(Timestep ts);
        void OnUpdateEditor(Timestep ts);

    private:
        Scene* m_Scene;
        std::vector<std::unique_ptr<ISceneSystem>> m_Systems;
    };
}

#endif // CH_SCENE_SYSTEM_MANAGER_H
