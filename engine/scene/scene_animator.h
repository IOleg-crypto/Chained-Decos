#ifndef CH_SCENE_ANIMATOR_H
#define CH_SCENE_ANIMATOR_H

#include "engine/core/base.h"

namespace CHEngine
{
class Scene;

class SceneAnimator
{
public:
    SceneAnimator(Scene *scene) : m_Scene(scene)
    {
    }

    void OnUpdate(float deltaTime);

private:
    Scene *m_Scene;
};
} // namespace CHEngine

#endif // CH_SCENE_ANIMATOR_H
