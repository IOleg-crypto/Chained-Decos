#ifndef CH_ANIMATION_GRAPH_SYSTEM_H
#define CH_ANIMATION_GRAPH_SYSTEM_H

#include "engine/core/timestep.h"

namespace CHEngine {

class Scene;

class AnimationGraphSystem {
public:
    static void Update(Scene* scene, Timestep ts);
};

} // namespace CHEngine

#endif // CH_ANIMATION_GRAPH_SYSTEM_H
