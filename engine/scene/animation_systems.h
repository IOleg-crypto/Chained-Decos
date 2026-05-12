#ifndef CH_ANIMATION_SYSTEMS_H
#define CH_ANIMATION_SYSTEMS_H
#include "engine/core/timestep.h"

namespace CHEngine {
class Scene;
namespace AnimationSystems {
    void UpdatePlayback(Scene* scene, Timestep ts);
    void UpdateGraphs(Scene* scene, Timestep ts);
}
}


#endif // CH_ANIMATION_SYSTEMS_H
