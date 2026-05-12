#ifndef CH_ANIMATION_GRAPH_DATA_H
#define CH_ANIMATION_GRAPH_DATA_H

#include <string>
#include <vector>
#include "imgui_internal.h"

namespace CHEngine {

struct AnimationState {
    std::string Name;
    std::string AnimationPath;
    ImRect Rect;
    bool Selected = false;
    bool IsLooping = true;
    
    // Output slots (triggers or conditions)
    std::vector<std::string> Transitions;
};

struct AnimationTransition {
    size_t FromState;
    size_t FromSlot;
    size_t ToState;
    size_t ToSlot;
};

struct AnimationGraphData {
    std::vector<AnimationState> Nodes;
    std::vector<AnimationTransition> Links;
};

} // namespace CHEngine

#endif // CH_ANIMATION_GRAPH_DATA_H
