#ifndef CH_FLOW_NAVIGATOR_COMPONENT_H
#define CH_FLOW_NAVIGATOR_COMPONENT_H

#include "engine/core/reflection.h"
#include <string>

#include <unordered_map>

namespace CHEngine {

struct AnimationGraphComponent {
    std::string GraphPath = "assets/animations/default.chanim";
    bool Active = true;
    
    // Runtime State
    std::string CurrentStateName;
    bool Initialized = false;
    std::unordered_map<std::string, bool> Triggers;

    CH_REFLECT_BEGIN(AnimationGraphComponent)
        CH_FILE_NAMED(props, "Animation Graph", GraphPath, "chanim");
        CH_PROP(props, Active);
        PropertyMeta meta; meta.ReadOnly = true;
        CH_PROP_META_NAMED(props, "Current State", CurrentStateName, meta);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_FLOW_NAVIGATOR_COMPONENT_H
